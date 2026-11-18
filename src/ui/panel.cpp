// Space Suite — panel.cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/panel.hpp"

#include "presets.hpp"
#include "value_text.hpp"

#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace space {

namespace {

struct Rgb {
  double r, g, b;
};

// One skin per page, in the Comp Suite chrome.  The faceplate and header keep
// the same material; only the accent, tab and indicator colours change.
struct Skin {
  const char* caption;
  Rgb bodyTop, bodyBottom;
  Rgb legend, legendDim;
  Rgb headerBg;
  Rgb tabOff, tabOn, tabTextOff, tabTextOn;
  Rgb meterFrame, meterFace, meterNeedle, meterScale;
  Rgb meterBlock;
  bool hasBlueBlock;
  Rgb knobCap, knobCapEdge, knobSkirt, knobIndicator;
  bool knurled;
  Rgb btnOff, btnOn, btnText, btnTextOn;
  Rgb screw;
};

Skin skinFor(int page) {
  switch (page) {
    case 1:  // Delay — warm amber
      return {"DELAY  -  digital / tape / analog / ping-pong",
              {0.32, 0.27, 0.20}, {0.20, 0.16, 0.11},
              {0.95, 0.91, 0.82}, {0.68, 0.62, 0.52},
              {0.16, 0.13, 0.09},
              {0.36, 0.30, 0.22}, {0.58, 0.38, 0.16},
              {0.85, 0.78, 0.66}, {1.0, 0.85, 0.55},
              {0.12, 0.09, 0.06}, {0.80, 0.76, 0.66},
              {0.12, 0.09, 0.06}, {0.25, 0.20, 0.15},
              {0.12, 0.09, 0.06}, false,
              {0.82, 0.76, 0.64}, {0.50, 0.42, 0.30},
              {0.20, 0.16, 0.11}, {0.95, 0.65, 0.20},
              true,
              {0.30, 0.24, 0.17}, {0.88, 0.58, 0.20},
              {0.90, 0.85, 0.74}, {1.0, 0.95, 0.80},
              {0.55, 0.48, 0.36}};
    case 2:  // Mod — violet
      return {"MOD  -  chorus / flanger / phaser / dimension",
              {0.24, 0.20, 0.32}, {0.15, 0.12, 0.22},
              {0.93, 0.89, 0.98}, {0.66, 0.60, 0.75},
              {0.12, 0.09, 0.18},
              {0.30, 0.25, 0.40}, {0.40, 0.25, 0.60},
              {0.82, 0.78, 0.90}, {0.90, 0.75, 1.0},
              {0.10, 0.08, 0.15}, {0.78, 0.75, 0.82},
              {0.12, 0.10, 0.16}, {0.25, 0.22, 0.30},
              {0.10, 0.08, 0.15}, false,
              {0.80, 0.76, 0.86}, {0.46, 0.42, 0.55},
              {0.16, 0.13, 0.22}, {0.80, 0.55, 1.0},
              false,
              {0.26, 0.22, 0.34}, {0.60, 0.40, 0.90},
              {0.88, 0.85, 0.94}, {1.0, 0.97, 1.0},
              {0.55, 0.52, 0.62}};
    default:  // Reverb — blue
      return {"REVERB  -  plate / room / hall / spring",
              {0.19, 0.25, 0.35}, {0.12, 0.16, 0.24},
              {0.90, 0.91, 0.89}, {0.62, 0.66, 0.70},
              {0.11, 0.14, 0.20},
              {0.26, 0.31, 0.40}, {0.16, 0.24, 0.36},
              {0.72, 0.78, 0.86}, {0.45, 0.75, 1.0},
              {0.10, 0.13, 0.18}, {0.73, 0.73, 0.70},
              {0.12, 0.12, 0.12}, {0.25, 0.25, 0.25},
              {0.10, 0.13, 0.18}, false,
              {0.76, 0.78, 0.81}, {0.45, 0.48, 0.53},
              {0.14, 0.17, 0.22}, {0.08, 0.09, 0.11},
              false,
              {0.22, 0.27, 0.34}, {0.32, 0.58, 0.88},
              {0.86, 0.89, 0.93}, {0.98, 0.99, 1.0},
              {0.55, 0.58, 0.62}};
  }
}

template <size_t N>
std::vector<const char*> sc(const char* const (&labels)[N]) {
  return std::vector<const char*>(labels, labels + N);
}

const char* const kScale18[] = {"-18", "0", "+18"};
const char* const kScaleMix[] = {"DRY", "", "", "", "WET"};
constexpr double kPi = 3.14159265358979323846;

// Header geometry.
constexpr double kPresetX = 72.0;
constexpr double kPresetY = 14.0;
constexpr double kPresetW = 330.0;
constexpr double kPresetH = 36.0;
constexpr double kListY = 56.0;
constexpr double kListRow = 22.0;
constexpr double kListVisible = 17.0;
constexpr double kListFooterH = 68.0;

double nowMs() {
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void roundRect(cairo_t* cr, double x, double y, double w, double h, double r) {
  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -kPi / 2, 0);
  cairo_arc(cr, x + w - r, y + h - r, r, 0, kPi / 2);
  cairo_arc(cr, x + r, y + h - r, r, kPi / 2, kPi);
  cairo_arc(cr, x + r, y + r, r, kPi, 3 * kPi / 2);
  cairo_close_path(cr);
}

void setRgb(cairo_t* cr, Rgb c, double a = 1.0) {
  cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

Rgb mix(Rgb a, Rgb b, double t) {
  return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
          a.b + (b.b - a.b) * t};
}

double knobAngle(double f) {
  return (135.0 + 270.0 * f) * kPi / 180.0;
}

double clamp01(double v) {
  return std::min(std::max(v, 0.0), 1.0);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction / typography
// ---------------------------------------------------------------------------

Panel::Panel() {
  grain_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
  unsigned char* data = cairo_image_surface_get_data(grain_);
  const int stride = cairo_image_surface_get_stride(grain_);
  unsigned int seed = 22222u;
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      seed = seed * 1103515245u + 12345u;
      const unsigned v = (seed >> 16) & 0xff;
      const unsigned a = (v < 128) ? 16u : 0u;
      const unsigned c = (v < 128) ? 0u : 255u;
      unsigned char* px = data + y * stride + x * 4;
      px[0] = static_cast<unsigned char>(c & 0xff);
      px[1] = static_cast<unsigned char>(c & 0xff);
      px[2] = static_cast<unsigned char>(c & 0xff);
      px[3] = static_cast<unsigned char>(a);
    }
  }
  cairo_surface_mark_dirty(grain_);
  fallbackFont_ = cairo_toy_font_face_create(
      "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  font_ = fallbackFont_;
  cairo_font_face_reference(font_);
  reloadUserPresets();
}

Panel::~Panel() {
  if (grain_) cairo_surface_destroy(grain_);
  if (font_ && font_ != fallbackFont_) cairo_font_face_destroy(font_);
  if (fallbackFont_) cairo_font_face_destroy(fallbackFont_);
  if (ftFace_) FT_Done_Face(static_cast<FT_Face>(ftFace_));
  if (ft_) FT_Done_FreeType(static_cast<FT_Library>(ft_));
}

void Panel::loadFont(const std::string& dir) {
  if (ft_) return;
  std::string path = dir;
  if (!path.empty() && path.back() != '/') path += '/';
  path += "Jost-ExtraBold.ttf";
  FT_Library lib = nullptr;
  if (FT_Init_FreeType(&lib) != 0) return;
  FT_Face face = nullptr;
  if (FT_New_Face(lib, path.c_str(), 0, &face) != 0) {
    FT_Done_FreeType(lib);
    return;
  }
  cairo_font_face_t* font = cairo_ft_font_face_create_for_ft_face(face, 0);
  if (cairo_font_face_status(font) != CAIRO_STATUS_SUCCESS) {
    cairo_font_face_destroy(font);
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
    return;
  }
  if (font_ && font_ != fallbackFont_) cairo_font_face_destroy(font_);
  ft_ = lib;
  ftFace_ = face;
  font_ = font;
}

void Panel::text(cairo_t* cr, const char* s, double x, double y, double size,
                 int align, double r, double g, double b) const {
  cairo_set_font_face(cr, font_);
  cairo_set_font_size(cr, size);
  cairo_set_source_rgb(cr, r, g, b);
  cairo_text_extents_t e;
  cairo_text_extents(cr, s, &e);
  double tx = x;
  if (align == 1) tx = x - e.width * 0.5 - e.x_bearing;
  if (align == 2) tx = x - e.width - e.x_bearing;
  cairo_move_to(cr, tx, y);
  cairo_show_text(cr, s);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void Panel::layout(const Shared& shared, const PanelState& state) {
  widgets_.clear();

  // Power.
  {
    Widget p{};
    p.kind = Kind::Power;
    p.param = P_POWER;
    p.x = 34;
    p.y = 30;
    p.r = 14;
    widgets_.push_back(p);
  }

  // Top page tabs, in signal-path order: MOD -> DELAY -> REVERB.
  // The page values are still 0 = Reverb, 1 = Delay, 2 = Mod, so the default
  // page remains Reverb even though it is the rightmost tab.
  static const char* kPageNames[3] = {"MOD", "DELAY", "REVERB"};
  static const int kPageOrder[3] = {2, 1, 0};
  for (int i = 0; i < 3; ++i) {
    Widget t{};
    t.kind = Kind::PageTab;
    t.value = kPageOrder[i];
    t.x = 600 + i * 138;
    t.y = 13;
    t.w = 130;
    t.h = 36;
    t.label = kPageNames[i];
    widgets_.push_back(t);
  }

  // Mode tabs below the header, one row per page.
  const int page = std::clamp(state.page, 0, 2);
  uint32_t modeParam = page == 0 ? P_REVERB_MODE : page == 1 ? P_DELAY_MODE
                                                             : P_MOD_MODE;
  static const char* kRevModes[4] = {"Plate", "Room", "Hall", "Spring"};
  static const char* kDelayModes[4] = {"Digital", "Tape", "Analog", "Ping-Pong"};
  static const char* kModModes[4] = {"Chorus", "Flanger", "Phaser", "Dimension"};
  const char* const* modeNames =
      page == 0 ? kRevModes : page == 1 ? kDelayModes : kModModes;
  const double modeW = (980.0 - 3.0 * 8.0) / 4.0;
  for (int i = 0; i < 4; ++i) {
    Widget t{};
    t.kind = Kind::ModelTab;
    t.param = modeParam;
    t.value = i;
    t.x = 30 + i * (modeW + 8.0);
    t.y = 70;
    t.w = modeW;
    t.h = 40;
    t.label = modeNames[i];
    widgets_.push_back(t);
  }

  auto knob = [&](uint32_t param, double x, double y, double r, const char* label,
                  std::vector<const char*> scale = {},
                  bool below = false, int steps = 0,
                  double scaleRadius = 1.44) {
    Widget k{};
    k.kind = Kind::Knob;
    k.param = param;
    k.x = x;
    k.y = y;
    k.r = r;
    k.steps = steps;
    k.scaleRadius = scaleRadius;
    k.label = label;
    k.scale = std::move(scale);
    k.labelBelow = below;
    widgets_.push_back(k);
  };

  auto pill = [&](uint32_t param, double x, double y, double w, double h,
                  const char* label) {
    Widget b{};
    b.kind = Kind::Pill;
    b.param = param;
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.value = 1.0;
    b.label = label;
    widgets_.push_back(b);
  };

  // Chain strip, at the top of the body inset.
  pill(P_MOD_ON, 700, 128, 80, 22, "MOD");
  pill(P_DELAY_ON, 795, 128, 80, 22, "DELAY");
  pill(P_REVERB_ON, 890, 128, 80, 22, "REVERB");

  // Global bottom row.
  knob(P_INPUT, 78, 540, 22, "INPUT", sc(kScale18), true);
  knob(P_OUTPUT, 930, 540, 22, "OUTPUT", sc(kScale18), true);
  knob(P_MIX, 850, 540, 22, "MIX", sc(kScaleMix), true);
  pill(P_BYPASS, 730, 528, 90, 24, "BYPASS");

  // Page-specific controls.  Deliberate rows: four large knobs on top, three
  // secondary knobs on the bottom, global controls in their own row.
  switch (page) {
    case 0:
      knob(P_REVERB_SIZE, 180, 270, 38, "SIZE");
      knob(P_REVERB_DECAY, 380, 270, 38, "DECAY");
      knob(P_REVERB_PREDELAY, 580, 270, 38, "PRE-DELAY");
      knob(P_REVERB_TONE, 780, 270, 38, "TONE");
      knob(P_REVERB_WIDTH, 280, 450, 34, "WIDTH");
      knob(P_REVERB_DUCK, 480, 450, 34, "DUCK");
      knob(P_REVERB_MIX, 680, 450, 34, "MIX");
      pill(P_REVERB_FREEZE, 820, 438, 100, 28, "FREEZE");
      break;
    case 1:
      pill(P_DELAY_SYNC, 150, 190, 90, 24, "SYNC");
      if (!(state.hostTempo && shared.get(P_DELAY_SYNC) > 0.5)) {
        knob(P_BPM, 150, 280, 22, "BPM");
      }
      if (shared.get(P_DELAY_SYNC) > 0.5) {
        knob(P_DELAY_DIV, 280, 280, 38, "DIVISION", {}, false, 9);
      } else {
        knob(P_DELAY_TIME, 280, 280, 38, "TIME");
      }
      knob(P_DELAY_FEEDBACK, 470, 280, 38, "FEEDBACK");
      knob(P_DELAY_TONE, 660, 280, 38, "TONE");
      knob(P_DELAY_MOD, 850, 280, 38, "MOD");
      knob(P_DELAY_WIDTH, 280, 450, 34, "WIDTH");
      knob(P_DELAY_DUCK, 480, 450, 34, "DUCK");
      knob(P_DELAY_MIX, 680, 450, 34, "MIX");
      break;
    case 2:
      knob(P_MOD_RATE, 220, 300, 40, "RATE");
      knob(P_MOD_DEPTH, 400, 300, 40, "DEPTH");
      knob(P_MOD_FEEDBACK, 580, 300, 40, "FEEDBACK");
      knob(P_MOD_WIDTH, 760, 300, 40, "WIDTH");
      knob(P_MOD_MIX, 480, 505, 36, "MIX");
      break;
  }
}

// ---------------------------------------------------------------------------
// Hit testing / interaction
// ---------------------------------------------------------------------------

bool Panel::hitWidget(const Widget& w, double x, double y) const {
  switch (w.kind) {
    case Kind::Knob: {
      const double dx = x - w.x;
      const double dy = y - w.y;
      return dx * dx + dy * dy <= w.r * w.r * 1.35;
    }
    case Kind::Power: {
      const double dx = x - w.x;
      const double dy = y - w.y;
      return dx * dx + dy * dy <= w.r * w.r * 1.4;
    }
    case Kind::Toggle:
      return x >= w.x - w.w * 0.5 && x <= w.x + w.w * 0.5 && y >= w.y &&
             y <= w.y + w.h;
    default:
      return x >= w.x && x <= w.x + w.w && y >= w.y && y <= w.y + w.h;
  }
}

double Panel::presetResetValue(const PanelState& state,
                               uint32_t param) const {
  double value = kParamSpecs[param].def;
  if (state.presetIndex >= kPresetCount) {
    const int user = state.presetIndex - kPresetCount;
    if (user >= 0 && user < static_cast<int>(userPresets_.size()) &&
        param < userPresets_[static_cast<size_t>(user)].values.size()) {
      value = userPresets_[static_cast<size_t>(user)].values[param];
    }
    return value;
  }
  if (state.presetIndex >= 0 && state.presetIndex < kPresetCount) {
    for (int i = 0; i < kPresets[state.presetIndex].count; ++i) {
      const PresetValue& v = kPresets[state.presetIndex].values[i];
      if (v.param == static_cast<int>(param)) {
        value = v.value;
        break;
      }
    }
  }
  return value;
}

int Panel::presetTotalRows() const {
  int rows = 0;
  const char* last = nullptr;
  for (int i : presetOrder()) {
    const char* category = presetCategory(i);
    if (!last || std::strcmp(last, category) != 0) {
      last = category;
      ++rows;
    }
    ++rows;
  }
  return rows;
}

int Panel::presetRow(int index) const {
  int row = 0;
  const char* last = nullptr;
  for (int i : presetOrder()) {
    const char* category = presetCategory(i);
    if (!last || std::strcmp(last, category) != 0) {
      last = category;
      ++row;
    }
    if (i == index) return row;
    ++row;
  }
  return 0;
}

int Panel::presetCount() const {
  return kPresetCount + static_cast<int>(userPresets_.size());
}

const char* Panel::presetName(int index) const {
  if (index >= kPresetCount) {
    const int user = index - kPresetCount;
    if (user >= 0 && user < static_cast<int>(userPresets_.size())) {
      return userPresets_[static_cast<size_t>(user)].name.c_str();
    }
    return "";
  }
  if (index >= 0 && index < kPresetCount) return kPresets[index].name;
  return "";
}

const char* Panel::presetCategory(int index) const {
  if (index >= kPresetCount) return "User";
  if (index >= 0 && index < kPresetCount) return kPresets[index].category;
  return "";
}

int Panel::presetPage(int index) const {
  if (index >= kPresetCount) {
    const int user = index - kPresetCount;
    if (user >= 0 && user < static_cast<int>(userPresets_.size())) {
      return userPresets_[static_cast<size_t>(user)].page;
    }
    return -1;
  }
  if (index >= 0 && index < kPresetCount) return kPresets[index].page;
  return -1;
}

std::vector<int> Panel::presetOrder() const {
  std::vector<int> v;
  for (int i = 0; i < kPresetCount; ++i) v.push_back(i);
  std::stable_sort(v.begin(), v.end(), [](int a, int b) {
    const int c = std::strcmp(kPresets[a].category, kPresets[b].category);
    if (c != 0) return c < 0;
    return std::strcmp(kPresets[a].name, kPresets[b].name) < 0;
  });
  for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
    v.push_back(kPresetCount + i);
  }
  return v;
}

bool Panel::pointerDown(Shared& shared, PanelState& state, double x, double y,
                        bool fine) {
  (void)fine;
  if (handlePresetClick(shared, state, x, y)) return true;
  for (const Widget& w : widgets_) {
    if (!hitWidget(w, x, y)) continue;
    switch (w.kind) {
      case Kind::Knob: {
        const double now = nowMs();
        const bool doubleClick = lastClickParam_ == static_cast<int>(w.param) &&
                                 now - lastClickTime_ <= kDoubleClickMs;
        lastClickParam_ = static_cast<int>(w.param);
        lastClickTime_ = now;
        if (doubleClick) {
          shared.set(w.param, presetResetValue(state, w.param));
          state.presetDirty = true;
          dragParam_ = -1;
          return true;
        }
        dragParam_ = static_cast<int>(w.param);
        dragSteps_ = w.steps;
        dragStartY_ = y;
        dragStartValue_ = shared.get(w.param);
        if (dragSteps_ > 1) {
          dragStartValue_ = std::min(dragStartValue_, dragSteps_ - 1.0);
        }
        return false;
      }
      case Kind::Power:
        shared.set(P_POWER, shared.get(P_POWER) > 0.5 ? 0.0 : 1.0);
        state.presetDirty = true;
        return true;
      case Kind::PageTab:
        state.page = static_cast<int>(w.value);
        return true;
      case Kind::ModelTab:
      case Kind::Button:
        shared.set(w.param, w.value);
        state.presetDirty = true;
        return true;
      case Kind::Pill:
        shared.set(w.param, shared.get(w.param) > 0.5 ? 0.0 : 1.0);
        state.presetDirty = true;
        return true;
      case Kind::Toggle: {
        const double f = (y - w.y) / w.h;
        shared.set(w.param, f < 0.5 ? 1.0 : 0.0);
        state.presetDirty = true;
        return true;
      }
      case Kind::Cycle: {
        const int v = static_cast<int>(shared.get(w.param) + 0.5);
        shared.set(w.param, static_cast<double>((v + 1) % 3));
        state.presetDirty = true;
        return true;
      }
    }
  }
  return false;
}

bool Panel::pointerMove(Shared& shared, PanelState& state, double x, double y,
                        bool fine) {
  (void)x;
  if (dragParam_ < 0) return false;
  const uint32_t param = static_cast<uint32_t>(dragParam_);
  const ParamSpec& spec = kParamSpecs[param];
  const double perPixel = spec.stepped
                              ? 1.0 / 18.0
                              : (spec.max - spec.min) / (fine ? 800.0 : 200.0);
  double v = dragStartValue_ + (dragStartY_ - y) * perPixel;
  if (dragSteps_ > 1) v = std::min(v, dragSteps_ - 1.0);
  shared.set(param, v);
  state.presetDirty = true;
  return true;
}

void Panel::pointerUp() { dragParam_ = -1; }

bool Panel::wheel(Shared& shared, PanelState& state, double x, double y,
                  double dy) {
  if (state.presetOpen && x >= kPresetX && x <= kPresetX + kPresetW &&
      y >= kListY &&
      y <= kListY + kListVisible * kListRow + kListFooterH + 10) {
    const int maxScroll = std::max(
        0, presetTotalRows() - static_cast<int>(kListVisible));
    state.presetScroll = std::min(maxScroll,
                                  std::max(0, state.presetScroll - (int)dy));
    return true;
  }
  for (const Widget& w : widgets_) {
    if (w.kind == Kind::Knob && hitWidget(w, x, y)) {
      const ParamSpec& spec = kParamSpecs[w.param];
      if (spec.stepped) {
        shared.set(w.param, shared.get(w.param) - (dy > 0 ? 1.0 : -1.0));
      } else {
        shared.set(w.param,
                   shared.get(w.param) - dy * (spec.max - spec.min) * 0.01);
      }
      state.presetDirty = true;
      return true;
    }
  }
  return false;
}

void Panel::tick(const Shared& shared, PanelState& state) {
  state.inLevelL = shared.inPeakLDb.load(std::memory_order_relaxed);
  state.inLevelR = shared.inPeakRDb.load(std::memory_order_relaxed);
  state.outLevelL = shared.outPeakLDb.load(std::memory_order_relaxed);
  state.outLevelR = shared.outPeakRDb.load(std::memory_order_relaxed);
  state.hostBpm = shared.hostBpm.load(std::memory_order_relaxed);
  state.hostTempo = shared.hostTempo.load(std::memory_order_relaxed);

  if (state.presetIndex >= 0 &&
      state.presetSnapshot.size() == static_cast<size_t>(P_COUNT)) {
    bool dirty = false;
    for (uint32_t i = 0; i < P_COUNT; ++i) {
      if (std::fabs(shared.get(i) - state.presetSnapshot[i]) > 1e-9) {
        dirty = true;
        break;
      }
    }
    state.presetDirty = dirty;
  }
  if (state.presetOpen && !state.savePreset && !state.renamePreset) {
    if (--state.presetScanTicks <= 0) {
      state.presetScanTicks = 120;
      rescanUserPresets(state);
    }
  } else {
    state.presetScanTicks = 60;
  }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void Panel::draw(cairo_t* cr, Shared& shared, const PanelState& state) {
  layout(shared, state);
  const int page = std::clamp(state.page, 0, 2);

  cairo_save(cr);
  cairo_set_source_rgb(cr, 0.07, 0.07, 0.07);
  cairo_paint(cr);

  drawHeader(cr, shared, state);
  drawBody(cr, page);

  drawLevelMeter(cr, 20, 150, 16, 352, state.inLevelL, state.inLevelR, -60.0,
                 0.0, true, page);
  drawLevelMeter(cr, 986, 150, 16, 352, state.outLevelL, state.outLevelR, -60.0,
                 3.0, false, page);

  if (page == 1) {
    const Skin skin = skinFor(page);
    char readout[32];
    if (shared.get(P_DELAY_SYNC) > 0.5) {
      const int div = std::clamp(
          static_cast<int>(shared.get(P_DELAY_DIV) + 0.5), 0, 8);
      std::snprintf(readout, sizeof(readout), "%s", kDivLabels[div]);
    } else {
      std::snprintf(readout, sizeof(readout), "%.0f ms",
                    shared.get(P_DELAY_TIME));
    }
    text(cr, readout, 280, 345, 12, 1, skin.legend.r, skin.legend.g,
         skin.legend.b);
    char bpm[48];
    if (state.hostTempo && shared.get(P_DELAY_SYNC) > 0.5) {
      std::snprintf(bpm, sizeof(bpm), "HOST %.1f BPM", state.hostBpm);
      text(cr, bpm, 150, 280, 10, 1, skin.legend.r, skin.legend.g,
           skin.legend.b);
      text(cr, "SYNCED TO HOST", 150, 296, 8.5, 1, skin.legendDim.r,
           skin.legendDim.g, skin.legendDim.b);
    } else {
      std::snprintf(bpm, sizeof(bpm), "%.0f BPM", shared.get(P_BPM));
      text(cr, bpm, 150, 320, 10, 1, skin.legendDim.r, skin.legendDim.g,
           skin.legendDim.b);
    }
  }

  if (page == 2) {
    // A small animated LFO display fills the Mod page and ties Rate, Depth and
    // Width together without adding another control.
    const Skin skin = skinFor(page);
    const double x = 220.0, y = 366.0, w = 600.0, h = 58.0;
    roundRect(cr, x, y, w, h, 4);
    cairo_set_source_rgba(cr, 0.025, 0.027, 0.030, 0.96);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1.0);
    setRgb(cr, skin.tabOn, 0.65);
    roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 4);
    cairo_stroke(cr);

    const double rate = std::clamp(shared.get(P_MOD_RATE), 0.05, 10.0);
    const double depth = std::clamp(shared.get(P_MOD_DEPTH) / 100.0, 0.0, 1.0);
    const double width = std::clamp(shared.get(P_MOD_WIDTH) / 100.0, 0.0, 1.0);
    const double phase = nowMs() * 0.001 * rate;
    auto lfo = [&](double phaseOffset, Rgb colour, double alpha) {
      cairo_set_line_width(cr, 1.6);
      setRgb(cr, colour, alpha);
      for (int i = 0; i < static_cast<int>(w); ++i) {
        const double u = i / (w - 1.0);
        const double v = std::sin(2.0 * kPi * (u * 2.0 + phase + phaseOffset));
        const double py = y + h * 0.5 - v * h * 0.34 * depth;
        if (i == 0) cairo_move_to(cr, x + i, py);
        else cairo_line_to(cr, x + i, py);
      }
      cairo_stroke(cr);
    };
    lfo(0.0, skin.knobIndicator, 0.95);
    lfo(0.08 * width, skin.knobIndicator, 0.40);

    cairo_set_line_width(cr, 1.0);
    setRgb(cr, skin.legendDim, 0.35);
    cairo_move_to(cr, x + 10, y + h * 0.5);
    cairo_line_to(cr, x + w - 10, y + h * 0.5);
    cairo_stroke(cr);
    text(cr, "LFO", x + 12, y + 15, 9, 0, skin.legendDim.r, skin.legendDim.g,
         skin.legendDim.b);
    char rateText[32];
    std::snprintf(rateText, sizeof(rateText), "%.2f Hz", rate);
    text(cr, rateText, x + w - 12, y + 15, 9, 2, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  }

  for (const Widget& w : widgets_) {
    drawWidget(cr, w, shared, state, page);
  }

  if (dragParam_ >= 0) {
    const uint32_t p = static_cast<uint32_t>(dragParam_);
    const std::string value =
        std::string(kParamSpecs[p].name) + "   " +
        formatParamValue(shared, p, shared.get(p));
    const Skin skin = skinFor(page);
    text(cr, value.c_str(), kPanelWidth * 0.5, 629, 15, 1, skin.btnOn.r,
         skin.btnOn.g, skin.btnOn.b);
  }

  if (editing_) drawTextEntry(cr, shared);
  if (state.presetOpen) drawPresetList(cr, shared, state);

  cairo_restore(cr);
}

void Panel::drawHeader(cairo_t* cr, const Shared& shared,
                       const PanelState& state) const {
  (void)shared;
  const Skin skin = skinFor(state.page);
  cairo_pattern_t* bar = cairo_pattern_create_linear(0, 0, 0, 62);
  cairo_pattern_add_color_stop_rgb(bar, 0.0, skin.headerBg.r * 1.35,
                                   skin.headerBg.g * 1.35,
                                   skin.headerBg.b * 1.35);
  cairo_pattern_add_color_stop_rgb(bar, 1.0, skin.headerBg.r * 0.75,
                                   skin.headerBg.g * 0.75,
                                   skin.headerBg.b * 0.75);
  cairo_rectangle(cr, 0, 0, kPanelWidth, 62);
  cairo_set_source(cr, bar);
  cairo_fill(cr);
  cairo_pattern_destroy(bar);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, 0.5);
  cairo_line_to(cr, kPanelWidth, 0.5);
  cairo_stroke(cr);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_move_to(cr, 0, 61.5);
  cairo_line_to(cr, kPanelWidth, 61.5);
  cairo_stroke(cr);

  text(cr, "SPACE SUITE", 500, 38, 19, 1, 0.84, 0.86, 0.88);
  drawPresetBar(cr, shared, state);
}

void Panel::drawBody(cairo_t* cr, int page) const {
  const Skin skin = skinFor(page);

  // Faceplate.
  cairo_pattern_t* pat = cairo_pattern_create_linear(0, 122, 0, 604);
  cairo_pattern_add_color_stop_rgb(pat, 0.0, skin.bodyTop.r, skin.bodyTop.g,
                                   skin.bodyTop.b);
  cairo_pattern_add_color_stop_rgb(pat, 1.0, skin.bodyBottom.r,
                                   skin.bodyBottom.g, skin.bodyBottom.b);
  roundRect(cr, 16, 122, 1008, 482, 6);
  cairo_set_source(cr, pat);
  cairo_fill(cr);
  cairo_pattern_destroy(pat);

  if (grain_) {
    cairo_pattern_t* grain = cairo_pattern_create_for_surface(grain_);
    cairo_pattern_set_extend(grain, CAIRO_EXTEND_REPEAT);
    cairo_save(cr);
    cairo_rectangle(cr, 17, 123, 1006, 480);
    cairo_clip(cr);
    cairo_set_source(cr, grain);
    cairo_paint_with_alpha(cr, 0.018);
    cairo_restore(cr);
    cairo_pattern_destroy(grain);
  }

  // Inner shadow under the top edge.
  cairo_pattern_t* inner = cairo_pattern_create_linear(0, 123, 0, 175);
  cairo_pattern_add_color_stop_rgba(inner, 0.0, 0.0, 0.0, 0.0, 0.30);
  cairo_pattern_add_color_stop_rgba(inner, 1.0, 0.0, 0.0, 0.0, 0.0);
  cairo_rectangle(cr, 17, 123, 1006, 52);
  cairo_set_source(cr, inner);
  cairo_fill(cr);
  cairo_pattern_destroy(inner);

  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {1.0, 1.0, 1.0}, 0.16);
  cairo_move_to(cr, 18, 123.5);
  cairo_line_to(cr, 1022, 123.5);
  cairo_stroke(cr);
  setRgb(cr, {0.0, 0.0, 0.0}, 0.22);
  cairo_move_to(cr, 18, 602.5);
  cairo_line_to(cr, 1022, 602.5);
  cairo_stroke(cr);

  for (double sx : {32.0, 1008.0}) {
    for (double sy : {138.0, 588.0}) {
      cairo_arc(cr, sx, sy, 4.5, 0, 2 * kPi);
      setRgb(cr, skin.screw, 0.9);
      cairo_fill(cr);
    }
  }

  // Chain divider under the mode tabs.
  setRgb(cr, {0.0, 0.0, 0.0}, 0.24);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 30, 116.5);
  cairo_line_to(cr, 1010, 116.5);
  cairo_stroke(cr);
  setRgb(cr, {1.0, 1.0, 1.0}, 0.08);
  cairo_move_to(cr, 30, 117.5);
  cairo_line_to(cr, 1010, 117.5);
  cairo_stroke(cr);

  // Signal-flow legend between the chain pills.
  text(cr, "SIGNAL FLOW", 632, 144, 9, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);
  text(cr, "\u203a", 786, 144, 16, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);
  text(cr, "\u203a", 881, 144, 16, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);

  // Footer band.
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_rectangle(cr, 0, 604, kPanelWidth, 36);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, 604.5);
  cairo_line_to(cr, kPanelWidth, 604.5);
  cairo_stroke(cr);
  text(cr, skin.caption, kPanelWidth * 0.5, 629, 15, 1, 0.86, 0.88, 0.90);
}

void Panel::drawLevelMeter(cairo_t* cr, double x, double y, double w, double h,
                           double dbL, double dbR, double minDb, double maxDb,
                           bool labelsRight, int page) const {
  const Skin skin = skinFor(page);
  const double gap = 4.0;
  const double pairWidth = w * 2.0 + gap;
  const double outer = labelsRight ? x + pairWidth : x;

  for (int ch = 0; ch < 2; ++ch) {
    const double bx = x + ch * (w + gap);
    cairo_rectangle(cr, bx, y, w, h);
    cairo_set_source_rgb(cr, 0.055, 0.055, 0.06);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.65);
    cairo_rectangle(cr, bx + 0.5, y + 0.5, w - 1, h - 1);
    cairo_stroke(cr);
  }

  static const double kMarks[] = {3, 0, -6, -12, -24, -36, -48};
  for (double m : kMarks) {
    const double f = (m - minDb) / (maxDb - minDb);
    const double ty = y + h - h * f;
    if (ty < y - 1 || ty > y + h + 1) continue;
    setRgb(cr, skin.legend, 0.55);
    cairo_set_line_width(cr, 1.0);
    const double tx0 = labelsRight ? outer : outer - 4;
    const double tx1 = labelsRight ? outer + 4 : outer;
    cairo_move_to(cr, tx0, ty);
    cairo_line_to(cr, tx1, ty);
    cairo_stroke(cr);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%+.0f", m);
    text(cr, buf, labelsRight ? outer + 8 : outer - 8, ty + 3, 9,
         labelsRight ? 0 : 2, skin.legend.r, skin.legend.g, skin.legend.b);
  }

  const double levels[2] = {dbL, dbR};
  for (int ch = 0; ch < 2; ++ch) {
    const double f = clamp01((levels[ch] - minDb) / (maxDb - minDb));
    const double fh = h * f;
    if (fh <= 0.5) continue;
    const double bx = x + ch * (w + gap);
    cairo_pattern_t* pat = cairo_pattern_create_linear(0, y + h, 0, y);
    const Rgb accent = skin.btnOn;
    cairo_pattern_add_color_stop_rgb(pat, 0.0, accent.r * 0.42,
                                     accent.g * 0.42, accent.b * 0.42);
    cairo_pattern_add_color_stop_rgb(pat, 0.82, accent.r * 0.86,
                                     accent.g * 0.86, accent.b * 0.86);
    cairo_pattern_add_color_stop_rgb(pat, 0.96, 0.93, 0.86, 0.55);
    cairo_pattern_add_color_stop_rgb(pat, 1.0, 0.95, 0.35, 0.28);
    cairo_rectangle(cr, bx, y + h - fh, w, fh);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
  }
}

void Panel::drawWidget(cairo_t* cr, const Widget& w, const Shared& shared,
                       const PanelState& state, int page) const {
  const Skin skin = skinFor(page);
  switch (w.kind) {
    case Kind::Power: {
      const bool on = shared.get(P_POWER) > 0.5;
      cairo_arc(cr, w.x, w.y, w.r, 0, 2 * kPi);
      setRgb(cr, {0.12, 0.12, 0.12});
      cairo_fill(cr);
      cairo_arc(cr, w.x, w.y, w.r - 1.5, 0, 2 * kPi);
      setRgb(cr, on ? Rgb{0.36, 0.86, 0.46} : Rgb{0.42, 0.42, 0.42});
      cairo_set_line_width(cr, 2.2);
      cairo_new_path(cr);
      cairo_arc(cr, w.x, w.y, 7, -kPi / 2 + 0.6, -kPi / 2 + 2 * kPi - 0.6);
      cairo_stroke(cr);
      cairo_move_to(cr, w.x, w.y - 9);
      cairo_line_to(cr, w.x, w.y - 1);
      cairo_stroke(cr);
      break;
    }
    case Kind::PageTab: {
      const bool selected = static_cast<int>(w.value) == state.page;
      roundRect(cr, w.x, w.y, w.w, w.h, 4);
      setRgb(cr, selected ? skin.tabOn : skin.tabOff);
      cairo_fill(cr);
      const Rgb tc = selected ? skin.tabTextOn : skin.tabTextOff;
      text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 5, 14, 1, tc.r,
           tc.g, tc.b);
      break;
    }
    case Kind::ModelTab: {
      const bool selected = std::fabs(shared.get(w.param) - w.value) < 0.5;
      roundRect(cr, w.x, w.y, w.w, w.h, 4);
      setRgb(cr, selected ? skin.tabOn : skin.tabOff);
      cairo_fill(cr);
      const Rgb tc = selected ? skin.tabTextOn : skin.tabTextOff;
      text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 5, 15, 1, tc.r,
           tc.g, tc.b);
      break;
    }
    case Kind::Knob:
      drawKnob(cr, w, shared.get(w.param), page);
      break;
    case Kind::Button:
      drawButton(cr, w, std::fabs(shared.get(w.param) - w.value) < 1e-3, page);
      break;
    case Kind::Toggle:
      drawToggle(cr, w, shared.get(w.param) > 0.5, page);
      break;
    case Kind::Pill:
      drawPill(cr, w, shared.get(w.param) > 0.5, page);
      break;
    case Kind::Cycle:
      drawPill(cr, w, shared.get(w.param) > 0.5, page);
      break;
  }
}

void Panel::drawPill(cairo_t* cr, const Widget& w, bool on, int page) const {
  const Skin skin = skinFor(page);
  roundRect(cr, w.x, w.y, w.w, w.h, 5);
  setRgb(cr, on ? skin.btnOn : skin.btnOff);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {0.0, 0.0, 0.0}, 0.35);
  roundRect(cr, w.x + 0.5, w.y + 0.5, w.w - 1.0, w.h - 1.0, 5);
  cairo_stroke(cr);
  const Rgb tc = on ? skin.btnTextOn : skin.btnText;
  text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 3.5, 10, 1, tc.r,
       tc.g, tc.b);
}

void Panel::drawKnob(cairo_t* cr, const Widget& w, double value,
                     int page) const {
  const Skin skin = skinFor(page);
  const double cx = w.x;
  const double cy = w.y;
  const double r = w.r;

  setRgb(cr, skin.legendDim, 0.55);
  cairo_set_line_width(cr, 1.0);
  for (int i = 0; i <= 10; ++i) {
    const double a = knobAngle(i / 10.0);
    cairo_move_to(cr, cx + std::cos(a) * r * 1.10,
                  cy + std::sin(a) * r * 1.10);
    cairo_line_to(cr, cx + std::cos(a) * r * 1.19,
                  cy + std::sin(a) * r * 1.19);
    cairo_stroke(cr);
  }

  const ParamSpec& paramSpec = kParamSpecs[w.param];
  if (w.scale.empty() && !paramSpec.stepped) {
    const double a0 = knobAngle(0.0);
    const double a1 = knobAngle(1.0);
    text(cr, "\u2212", cx + std::cos(a0) * (r * 1.44),
         cy + std::sin(a0) * (r * 1.44) + 4, 12, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
    text(cr, "+", cx + std::cos(a1) * (r * 1.44),
         cy + std::sin(a1) * (r * 1.44) + 4, 12, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  }

  const size_t n = w.scale.size();
  for (size_t i = 0; i < n; ++i) {
    const double f = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
    const double a = knobAngle(f);
    const double lx = cx + std::cos(a) * (r * w.scaleRadius);
    const double ly = cy + std::sin(a) * (r * w.scaleRadius);
    text(cr, w.scale[i], lx, ly + 4, 10.5, 1, skin.legend.r, skin.legend.g,
         skin.legend.b);
  }

  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.18);
  cairo_arc(cr, cx + 0.5, cy + 1.5, r * 1.03, 0, 2 * kPi);
  cairo_fill(cr);

  cairo_pattern_t* body = cairo_pattern_create_linear(0, cy - r, 0, cy + r);
  const Rgb top = mix(skin.knobSkirt, {1, 1, 1}, 0.16);
  const Rgb bot = mix(skin.knobSkirt, {0, 0, 0}, 0.30);
  cairo_pattern_add_color_stop_rgb(body, 0.0, top.r, top.g, top.b);
  cairo_pattern_add_color_stop_rgb(body, 1.0, bot.r, bot.g, bot.b);
  cairo_arc(cr, cx, cy, r, 0, 2 * kPi);
  cairo_set_source(cr, body);
  cairo_fill(cr);
  cairo_pattern_destroy(body);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.45);
  cairo_arc(cr, cx, cy, r - 0.5, 0, 2 * kPi);
  cairo_stroke(cr);

  cairo_pattern_t* cap = cairo_pattern_create_radial(
      cx - r * 0.35, cy - r * 0.40, r * 0.05, cx, cy, r * 0.98);
  const Rgb hi = mix(skin.knobCap, {1, 1, 1}, 0.24);
  cairo_pattern_add_color_stop_rgb(cap, 0.0, hi.r, hi.g, hi.b);
  cairo_pattern_add_color_stop_rgb(cap, 0.75, skin.knobCap.r, skin.knobCap.g,
                                   skin.knobCap.b);
  cairo_pattern_add_color_stop_rgb(cap, 1.0, skin.knobCapEdge.r,
                                   skin.knobCapEdge.g, skin.knobCapEdge.b);
  cairo_arc(cr, cx, cy, r * 0.82, 0, 2 * kPi);
  cairo_set_source(cr, cap);
  cairo_fill(cr);
  cairo_pattern_destroy(cap);

  if (skin.knurled) {
    cairo_set_line_width(cr, 0.8);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.22);
    for (int k = 0; k < 60; ++k) {
      const double a = k * 2 * kPi / 60.0;
      cairo_move_to(cr, cx + std::cos(a) * r * 0.86,
                    cy + std::sin(a) * r * 0.86);
      cairo_line_to(cr, cx + std::cos(a) * r * 0.99,
                    cy + std::sin(a) * r * 0.99);
      cairo_stroke(cr);
    }
  }

  const ParamSpec& spec = kParamSpecs[w.param];
  const double vmax = w.steps > 1 ? w.steps - 1 : spec.max;
  const double span = vmax - spec.min;
  const double frac =
      span > 0.0 ? (std::min(value, vmax) - spec.min) / span : 0.0;
  const double a = knobAngle(clamp01(frac));
  cairo_set_line_width(cr, 2.0);
  setRgb(cr, skin.knobIndicator);
  cairo_move_to(cr, cx + std::cos(a) * r * 0.26,
                cy + std::sin(a) * r * 0.26);
  cairo_line_to(cr, cx + std::cos(a) * r * 0.78,
                cy + std::sin(a) * r * 0.78);
  cairo_stroke(cr);

  const double labelY = w.labelBelow ? cy + r * 1.44 + 20 : cy - r * 1.44 - 18;
  text(cr, w.label, cx, labelY, 11, 1, skin.legend.r, skin.legend.g,
       skin.legend.b);
}

void Panel::drawButton(cairo_t* cr, const Widget& w, bool lit,
                       int page) const {
  const Skin skin = skinFor(page);
  const Rgb base = lit ? skin.btnOn : skin.btnOff;
  cairo_pattern_t* pat = cairo_pattern_create_linear(0, w.y, 0, w.y + w.h);
  const Rgb hi = mix(base, {1, 1, 1}, lit ? 0.10 : 0.07);
  const Rgb lo = mix(base, {0, 0, 0}, 0.10);
  cairo_pattern_add_color_stop_rgb(pat, 0.0, hi.r, hi.g, hi.b);
  cairo_pattern_add_color_stop_rgb(pat, 1.0, lo.r, lo.g, lo.b);
  roundRect(cr, w.x, w.y, w.w, w.h, 3);
  cairo_set_source(cr, pat);
  cairo_fill(cr);
  cairo_pattern_destroy(pat);
  setRgb(cr, mix(base, lit ? Rgb{1, 1, 1} : Rgb{0, 0, 0}, 0.35), 0.9);
  cairo_set_line_width(cr, 1.0);
  roundRect(cr, w.x + 0.5, w.y + 0.5, w.w - 1, w.h - 1, 3);
  cairo_stroke(cr);
  const Rgb tc = lit ? skin.btnTextOn : skin.btnText;
  text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 4, 11.5, 1, tc.r,
       tc.g, tc.b);
}

void Panel::drawToggle(cairo_t* cr, const Widget& w, bool on,
                       int page) const {
  const Skin skin = skinFor(page);
  roundRect(cr, w.x - w.w * 0.5, w.y, w.w, w.h, 4);
  setRgb(cr, mix(skin.knobSkirt, {0, 0, 0}, 0.3));
  cairo_fill(cr);
  const double leverY = on ? w.y + 12 : w.y + w.h - 12;
  cairo_arc(cr, w.x, leverY, 8, 0, 2 * kPi);
  setRgb(cr, skin.knobCap);
  cairo_fill(cr);
  text(cr, w.buttonText[0], w.x, w.y - 8, 12, 1, skin.legend.r, skin.legend.g,
       skin.legend.b);
  text(cr, w.buttonText[1], w.x, w.y + w.h + 18, 12, 1, skin.legend.r,
       skin.legend.g, skin.legend.b);
}

// ---------------------------------------------------------------------------
// Preset browser
// ---------------------------------------------------------------------------

void Panel::drawPresetBar(cairo_t* cr, const Shared& shared,
                          const PanelState& state) const {
  (void)shared;
  roundRect(cr, kPresetX, kPresetY, kPresetW, kPresetH, 4);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.35);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
  roundRect(cr, kPresetX + 0.5, kPresetY + 0.5, kPresetW - 1, kPresetH - 1, 4);
  cairo_stroke(cr);

  text(cr, "\u2039", kPresetX + 18, kPresetY + 25, 20, 1, 0.72, 0.75, 0.78);
  text(cr, "\u203a", kPresetX + kPresetW - 18, kPresetY + 25, 20, 1, 0.72,
       0.75, 0.78);

  char label[160];
  const bool notice = !state.presetNotice.empty() &&
                      nowMs() < state.presetNoticeUntil;
  if (notice) {
    std::snprintf(label, sizeof(label), "%s", state.presetNotice.c_str());
  } else if (state.presetIndex >= 0 &&
             state.presetIndex < presetCount()) {
    std::snprintf(label, sizeof(label), "%s%s", presetName(state.presetIndex),
                  state.presetDirty ? " *" : "");
  } else {
    std::snprintf(label, sizeof(label), "Init%s", state.presetDirty ? " *" : "");
  }
  text(cr, label, kPresetX + kPresetW * 0.5, kPresetY + 24, 14, 1,
       notice ? 1.0 : 0.90, notice ? 0.75 : 0.92, notice ? 0.25 : 0.94);
}

void Panel::drawPresetList(cairo_t* cr, const Shared& shared,
                           const PanelState& state) const {
  (void)shared;
  const Skin skin = skinFor(state.page);
  const int count = presetCount();
  const double h = kListVisible * kListRow + kListFooterH + 10;
  roundRect(cr, kPresetX, kListY, kPresetW, h, 5);
  cairo_set_source_rgba(cr, 0.07, 0.08, 0.09, 0.97);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  setRgb(cr, skin.tabOn, 0.7);
  roundRect(cr, kPresetX + 0.5, kListY + 0.5, kPresetW - 1, h - 1, 5);
  cairo_stroke(cr);

  cairo_save(cr);
  roundRect(cr, kPresetX + 1, kListY + 1, kPresetW - 2,
            kListVisible * kListRow + 8, 4);
  cairo_clip(cr);
  static const char* kPageShort[3] = {"REV", "DLY", "MOD"};
  const char* lastCategory = nullptr;
  double y = kListY + 6 - state.presetScroll * kListRow;
  for (int i : presetOrder()) {
    const char* category = presetCategory(i);
    if (!lastCategory || std::strcmp(lastCategory, category) != 0) {
      lastCategory = category;
      text(cr, category, kPresetX + 14, y + 15, 11, 0, 0.52, 0.66, 0.80);
      y += kListRow;
      if (y > kListY + kListVisible * kListRow + 6) break;
    }
    if (y + kListRow > kListY && y < kListY + kListVisible * kListRow + 6) {
      if (i == state.presetIndex) {
        roundRect(cr, kPresetX + 6, y + 1, kPresetW - 12, kListRow - 2, 3);
        setRgb(cr, skin.tabOn, 0.85);
        cairo_fill(cr);
      }
      text(cr, presetName(i), kPresetX + 26, y + 15, 12.5, 0,
           i == state.presetIndex ? 1.0 : 0.86,
           i == state.presetIndex ? 1.0 : 0.88,
           i == state.presetIndex ? 1.0 : 0.90);
      const int page = presetPage(i);
      text(cr, page >= 0 ? kPageShort[std::clamp(page, 0, 2)] : "USR",
           kPresetX + kPresetW - 16, y + 15, 10.5, 2, 0.56, 0.60, 0.66);
    }
    y += kListRow;
  }
  cairo_restore(cr);

  const double footerY = kListY + kListVisible * kListRow + 9;
  const double row2 = footerY + 30;
  const bool userCurrent = state.presetIndex >= kPresetCount &&
                           state.presetIndex < count;
  const bool anyCurrent = state.presetIndex >= 0 && state.presetIndex < count;

  if (state.savePreset || state.renamePreset) {
    const double bx = kPresetX + 10;
    const double bw = kPresetW - 20;
    roundRect(cr, bx, footerY, bw, 24, 4);
    cairo_set_source_rgba(cr, 0.025, 0.03, 0.04, 0.98);
    cairo_fill(cr);
    setRgb(cr, state.presetError.empty() ? skin.tabOn : Rgb{0.82, 0.20, 0.18},
           0.9);
    cairo_set_line_width(cr, 1.2);
    roundRect(cr, bx + 0.5, footerY + 0.5, bw - 1, 23, 4);
    cairo_stroke(cr);
    text(cr, state.renamePreset ? "RENAME" : "SAVE AS", bx + 8, footerY + 16,
         8.0, 0, 0.55, 0.62, 0.70);
    const std::string shown = state.presetText + "|";
    text(cr, shown.c_str(), bx + 68, footerY + 16, 10.5, 0, 0.98, 0.80, 0.32);
    text(cr, "Enter saves · Esc cancels", bx + bw - 8, footerY + 16, 7.5, 2,
         0.55, 0.60, 0.66);
    const std::string hint = state.presetError.empty()
                                 ? "Saved in the user preset bank"
                                 : state.presetError;
    text(cr, hint.c_str(), bx + 2, footerY + 40, 8.0, 0,
         state.presetError.empty() ? 0.55 : 0.82,
         state.presetError.empty() ? 0.60 : 0.20,
         state.presetError.empty() ? 0.66 : 0.18);
    return;
  }

  auto button = [&](double x, double yy, double w, const char* label,
                    bool accent) {
    roundRect(cr, x, yy, w, 24, 4);
    setRgb(cr, accent ? Rgb{0.82, 0.20, 0.18} : Rgb{0.16, 0.19, 0.23});
    cairo_fill(cr);
    text(cr, label, x + w * 0.5, yy + 16, 7.5, 1, accent ? 0.06 : 0.80,
         accent ? 0.08 : 0.84, accent ? 0.10 : 0.88);
  };
  button(kPresetX + 10, footerY, 70, "SAVE", false);
  if (anyCurrent) button(kPresetX + 90, footerY, 92, "DUPLICATE", false);
  button(kPresetX + kPresetW - 78, footerY, 68, "CLOSE", false);
  if (userCurrent) {
    button(kPresetX + 10, row2, 72, "RENAME", false);
    button(kPresetX + 90, row2, 72,
           state.deletePresetConfirm ? "CONFIRM" : "DELETE",
           state.deletePresetConfirm);
    button(kPresetX + 170, row2, 88, "EXPORT", false);
  } else if (anyCurrent) {
    button(kPresetX + 10, row2, 88, "EXPORT", false);
    text(cr, "Factory preset · read-only", kPresetX + 110, row2 + 16, 7.5, 0,
         0.55, 0.60, 0.66);
  } else {
    text(cr, "Save or drag a preset file here", kPresetX + 12, row2 + 16, 8,
         0, 0.55, 0.60, 0.66);
  }
}

void Panel::beginTextEntry(Shared& shared, PanelState& state, double x,
                           double y) {
  (void)shared;
  state.presetOpen = 0;
  state.savePreset = false;
  state.renamePreset = false;
  state.deletePresetConfirm = false;
  state.presetText.clear();
  state.presetTextFresh = false;
  state.presetError.clear();
  state.presetOverwriteConfirm = false;
  for (const Widget& w : widgets_) {
    if (!hitWidget(w, x, y)) continue;
    if (w.kind != Kind::Knob) return;
    editing_ = true;
    editParam_ = w.param;
    editText_.clear();
    editBoxX_ = std::clamp(x + 16.0, 4.0, kPanelWidth - kEditW - 4.0);
    editBoxY_ = y - kEditH - 14.0;
    if (editBoxY_ < 4.0) {
      editBoxY_ = std::min(y + 20.0, kPanelHeight - kEditH - 4.0);
    }
    return;
  }
}

bool Panel::hitTextEntry(double x, double y) const {
  return editing_ && x >= editBoxX_ && x <= editBoxX_ + kEditW &&
         y >= editBoxY_ && y <= editBoxY_ + kEditH;
}

bool Panel::textKey(Shared& shared, PanelState& state, int key, char ch) {
  if (state.savePreset || state.renamePreset) {
    if (key == kTextEnter) return commitPresetName(shared, state);
    if (key == kTextEscape) {
      state.savePreset = false;
      state.renamePreset = false;
      state.presetText.clear();
      state.presetTextFresh = false;
      state.presetError.clear();
      state.presetOverwriteConfirm = false;
      state.presetOpen = 0;
      return true;
    }
    if (key == kTextBackspace) {
      state.presetError.clear();
      state.presetOverwriteConfirm = false;
      if (state.presetTextFresh) {
        state.presetText.clear();
        state.presetTextFresh = false;
      } else if (!state.presetText.empty()) {
        state.presetText.pop_back();
      }
      return true;
    }
    if (ch >= 32 && ch < 127) {
      state.presetError.clear();
      state.presetOverwriteConfirm = false;
      if (state.presetTextFresh) {
        state.presetText.clear();
        state.presetTextFresh = false;
      }
      if (state.presetText.size() < 60) state.presetText.push_back(ch);
    }
    return true;
  }
  if (!editing_) return false;
  if (key == kTextEnter) return commitTextEntry(shared, state);
  if (key == kTextEscape) {
    editing_ = false;
    editText_.clear();
    return true;
  }
  if (key == kTextBackspace) {
    if (!editText_.empty()) editText_.pop_back();
    return true;
  }
  if (ch >= 32 && ch < 127 && editText_.size() < 24) {
    editText_.push_back(ch);
  }
  return true;
}

bool Panel::commitTextEntry(Shared& shared, PanelState& state) {
  if (!editing_) return false;
  if (editText_.empty()) {
    editing_ = false;
    return false;
  }
  double value = 0.0;
  if (!parseParamValue(shared, editParam_, editText_, &value)) {
    return false;
  }
  shared.set(editParam_, value);
  state.presetDirty = true;
  editing_ = false;
  editText_.clear();
  return true;
}

void Panel::drawTextEntry(cairo_t* cr, const Shared& shared) const {
  if (!editing_) return;
  const Skin skin = skinFor(0);
  const ParamSpec& spec = kParamSpecs[editParam_];
  const std::string current =
      formatEditValue(shared, editParam_, shared.get(editParam_));
  const std::string shown = (editText_.empty() ? current : editText_) + "|";
  roundRect(cr, editBoxX_, editBoxY_, kEditW, kEditH, 4);
  cairo_set_source_rgba(cr, 0.035, 0.04, 0.05, 0.98);
  cairo_fill(cr);
  setRgb(cr, skin.tabOn, 0.9);
  cairo_set_line_width(cr, 1.2);
  roundRect(cr, editBoxX_ + 0.5, editBoxY_ + 0.5, kEditW - 1, kEditH - 1, 4);
  cairo_stroke(cr);
  text(cr, spec.name, editBoxX_ + 8, editBoxY_ + 12, 8.0, 0,
       skin.legendDim.r, skin.legendDim.g, skin.legendDim.b);
  const Rgb gold = {0.98, 0.80, 0.32};
  text(cr, shown.c_str(), editBoxX_ + 8, editBoxY_ + 26, 10.5, 0, gold.r,
       gold.g, gold.b);
}

void Panel::applyPreset(Shared& shared, PanelState& state, int index) const {
  if (index < 0 || index >= presetCount()) return;
  if (index >= kPresetCount) {
    const int user = index - kPresetCount;
    if (user < 0 || user >= static_cast<int>(userPresets_.size())) return;
    const UserPreset& p = userPresets_[static_cast<size_t>(user)];
    for (uint32_t i = 0; i < P_COUNT && i < p.values.size(); ++i) {
      shared.set(i, p.values[i]);
    }
    if (p.page >= 0 && p.page <= 2) state.page = p.page;
  } else {
    const Preset& p = kPresets[index];
    state.page = std::clamp(p.page, 0, 2);
    for (int i = 0; i < p.count; ++i) {
      const PresetValue& v = p.values[i];
      if (v.param >= 0 && v.param < static_cast<int>(P_COUNT)) {
        shared.set(static_cast<uint32_t>(v.param), v.value);
      }
    }
  }
  state.presetIndex = index;
  state.presetDirty = false;
  state.presetSnapshot.resize(P_COUNT);
  for (uint32_t i = 0; i < P_COUNT; ++i) {
    state.presetSnapshot[i] = shared.get(i);
  }
  state.presetError.clear();
  state.presetOverwriteConfirm = false;
}

bool Panel::handlePresetClick(Shared& shared, PanelState& state, double x,
                              double y) {
  if (state.savePreset || state.renamePreset) return true;
  if (y >= kPresetY && y <= kPresetY + kPresetH) {
    if (x >= kPresetX && x <= kPresetX + 36) {
      rescanUserPresets(state);
      const int next = state.presetIndex <= 0 ? 0 : state.presetIndex - 1;
      applyPreset(shared, state, next);
      return true;
    }
    if (x >= kPresetX + kPresetW - 36 && x <= kPresetX + kPresetW) {
      rescanUserPresets(state);
      const int next = state.presetIndex < 0 ? 0 : state.presetIndex + 1;
      applyPreset(shared, state, std::min(presetCount() - 1, next));
      return true;
    }
    if (x > kPresetX + 36 && x < kPresetX + kPresetW - 36) {
      state.presetOpen = state.presetOpen ? 0 : 1;
      state.deletePresetConfirm = false;
      state.presetError.clear();
      state.presetOverwriteConfirm = false;
      state.presetScanTicks = 120;
      if (state.presetOpen) {
        rescanUserPresets(state);
        const int maxScroll = std::max(
            0, presetTotalRows() - static_cast<int>(kListVisible));
        state.presetScroll =
            state.presetIndex >= 0
                ? std::clamp(presetRow(state.presetIndex) -
                                 static_cast<int>(kListVisible) / 2,
                             0, maxScroll)
                : 0;
      }
      return true;
    }
  }
  if (!state.presetOpen) return false;
  const double h = kListVisible * kListRow + kListFooterH + 10;
  if (x < kPresetX || x > kPresetX + kPresetW || y < kListY ||
      y > kListY + h) {
    state.presetOpen = 0;
    return true;
  }

  double rowY = kListY + 6 - state.presetScroll * kListRow;
  const double rowsBottom = kListY + kListVisible * kListRow + 6;
  const char* lastCategory = nullptr;
  for (int i : presetOrder()) {
    const char* category = presetCategory(i);
    if (!lastCategory || std::strcmp(lastCategory, category) != 0) {
      lastCategory = category;
      rowY += kListRow;
      if (rowY >= rowsBottom) break;
    }
    if (rowY < kListY) {
      rowY += kListRow;
      continue;
    }
    if (y >= rowY && y < rowY + kListRow) {
      applyPreset(shared, state, i);
      return true;
    }
    rowY += kListRow;
    if (rowY >= rowsBottom) break;
  }

  const double footerY = kListY + kListVisible * kListRow + 9;
  const double row2 = footerY + 30;
  const bool userCurrent = state.presetIndex >= kPresetCount &&
                           state.presetIndex < presetCount();
  const bool anyCurrent = state.presetIndex >= 0 &&
                          state.presetIndex < presetCount();
  const bool inRow1 = y >= footerY && y <= footerY + 24;
  const bool inRow2 = y >= row2 && y <= row2 + 24;
  if (inRow1 && x >= kPresetX + 10 && x <= kPresetX + 80) {
    state.savePreset = true;
    state.renamePreset = false;
    state.deletePresetConfirm = false;
    state.presetError.clear();
    state.presetOverwriteConfirm = false;
    state.presetText = userCurrent ? presetName(state.presetIndex) : "My Preset";
    state.presetTextFresh = true;
    return true;
  }
  if (anyCurrent && inRow1 && x >= kPresetX + 90 && x <= kPresetX + 182) {
    duplicatePreset(shared, state);
    state.presetOpen = 0;
    return true;
  }
  if (inRow1 && x >= kPresetX + kPresetW - 78 &&
      x <= kPresetX + kPresetW - 10) {
    state.presetOpen = 0;
    return true;
  }
  if (userCurrent && inRow2 && x >= kPresetX + 10 && x <= kPresetX + 82) {
    state.renamePreset = true;
    state.savePreset = false;
    state.deletePresetConfirm = false;
    state.presetError.clear();
    state.presetOverwriteConfirm = false;
    state.presetText = presetName(state.presetIndex);
    state.presetTextFresh = true;
    return true;
  }
  if (userCurrent && inRow2 && x >= kPresetX + 90 && x <= kPresetX + 162) {
    if (!state.deletePresetConfirm) {
      state.deletePresetConfirm = true;
      return true;
    }
    const std::string name = presetName(state.presetIndex);
    if (deleteUserPreset(userPresetDir(), name)) {
      reloadUserPresets();
      state.presetIndex = -1;
      state.presetDirty = true;
      state.presetSnapshot.clear();
      setPresetNotice(state, "Deleted · " + name);
    }
    state.deletePresetConfirm = false;
    state.presetOpen = 0;
    return true;
  }
  state.deletePresetConfirm = false;
  if (anyCurrent && ((userCurrent && inRow2 && x >= kPresetX + 170 &&
                      x <= kPresetX + 258) ||
                     (!userCurrent && inRow2 && x >= kPresetX + 10 &&
                      x <= kPresetX + 98))) {
    exportPreset(shared, state);
    state.presetOpen = 0;
    return true;
  }
  return true;
}

void Panel::reloadUserPresets() {
  userPresets_ = loadUserPresets(userPresetDir());
}

void Panel::rescanUserPresets(PanelState& state) {
  std::string selected;
  if (state.presetIndex >= kPresetCount) {
    selected = presetName(state.presetIndex);
  }
  reloadUserPresets();
  if (state.presetIndex < kPresetCount) return;
  int found = -1;
  for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
    if (userPresets_[static_cast<size_t>(i)].name == selected) {
      found = kPresetCount + i;
      break;
    }
  }
  state.presetIndex = found;
  if (found < 0) {
    state.presetSnapshot.clear();
    state.presetDirty = false;
  }
}

void Panel::setPresetNotice(PanelState& state, const std::string& text) const {
  state.presetNotice = text;
  state.presetNoticeUntil = nowMs() + 3500.0;
}

void Panel::duplicatePreset(Shared& shared, PanelState& state) {
  const int index = state.presetIndex;
  if (index < 0 || index >= presetCount()) return;
  std::vector<double> values;
  std::string base;
  int page = state.page;
  if (index >= kPresetCount) {
    const int user = index - kPresetCount;
    if (user < 0 || user >= static_cast<int>(userPresets_.size())) return;
    values = userPresets_[static_cast<size_t>(user)].values;
    base = userPresets_[static_cast<size_t>(user)].name;
    page = userPresets_[static_cast<size_t>(user)].page;
  } else {
    values.reserve(P_COUNT);
    for (uint32_t i = 0; i < P_COUNT; ++i) values.push_back(shared.get(i));
    base = kPresets[index].name;
  }
  const std::string name =
      uniquePresetName(userPresetDir(), base + " copy");
  if (!writeUserPreset(userPresetDir(), name, values, page)) {
    setPresetNotice(state, "Duplicate failed");
    return;
  }
  reloadUserPresets();
  int found = -1;
  for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
    if (userPresets_[static_cast<size_t>(i)].name == name) {
      found = kPresetCount + i;
      break;
    }
  }
  state.presetIndex = found;
  state.presetDirty = false;
  state.presetSnapshot = values;
  setPresetNotice(state, "Duplicated · " + name);
}

void Panel::exportPreset(const Shared& shared, PanelState& state) const {
  const int index = state.presetIndex;
  if (index < 0 || index >= presetCount()) return;
  std::vector<double> values;
  values.reserve(P_COUNT);
  for (uint32_t i = 0; i < P_COUNT; ++i) values.push_back(shared.get(i));
  const std::string name = presetName(index);
  if (!writeUserPreset(userPresetExportDir(), name, values, state.page)) {
    setPresetNotice(state, "Export failed");
    return;
  }
  setPresetNotice(state, "Exported · " + name);
}

bool Panel::importPresetFile(Shared& shared, PanelState& state,
                             const std::string& path) {
  UserPreset preset;
  if (!readUserPresetFile(path, preset)) {
    setPresetNotice(state, "Not a Space Suite preset");
    return false;
  }
  const std::string name =
      uniquePresetName(userPresetDir(), preset.name);
  if (!writeUserPreset(userPresetDir(), name, preset.values, preset.page)) {
    setPresetNotice(state, "Import failed");
    return false;
  }
  reloadUserPresets();
  int found = -1;
  for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
    if (userPresets_[static_cast<size_t>(i)].name == name) {
      found = kPresetCount + i;
      break;
    }
  }
  if (found >= 0) applyPreset(shared, state, found);
  state.presetError.clear();
  setPresetNotice(state, "Imported · " + name);
  return true;
}

bool Panel::commitPresetName(Shared& shared, PanelState& state) {
  const std::string name = sanitizePresetName(state.presetText);
  if (name.empty()) return false;
  const std::filesystem::path target =
      userPresetDir() / (name + kUserPresetExt);
  std::error_code ec;
  const bool exists = std::filesystem::exists(target, ec) && !ec;
  if (state.savePreset) {
    const std::string current =
        state.presetIndex >= kPresetCount ? presetName(state.presetIndex) : "";
    if (exists && name != current && !state.presetOverwriteConfirm) {
      state.presetError = "Name in use · Enter again to overwrite";
      state.presetOverwriteConfirm = true;
      return false;
    }
    if (!saveUserPreset(userPresetDir(), name, shared, state.page)) {
      state.presetError = "Could not write the preset file";
      return false;
    }
    reloadUserPresets();
    int index = -1;
    for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
      if (userPresets_[static_cast<size_t>(i)].name == name) {
        index = kPresetCount + i;
        break;
      }
    }
    state.presetIndex = index;
    state.presetDirty = false;
    state.presetSnapshot.resize(P_COUNT);
    for (uint32_t i = 0; i < P_COUNT; ++i) {
      state.presetSnapshot[i] = shared.get(i);
    }
    setPresetNotice(state, "Saved · " + name);
  } else if (state.renamePreset) {
    const int user = state.presetIndex - kPresetCount;
    if (user < 0 || user >= static_cast<int>(userPresets_.size())) return false;
    const std::string oldName = userPresets_[static_cast<size_t>(user)].name;
    if (exists && name != oldName) {
      state.presetError = "Name already in use";
      return false;
    }
    const std::vector<double> values =
        userPresets_[static_cast<size_t>(user)].values;
    if (!renameUserPreset(userPresetDir(), oldName, name, values,
                          userPresets_[static_cast<size_t>(user)].page)) {
      state.presetError = "Could not rename the preset file";
      return false;
    }
    reloadUserPresets();
    int index = -1;
    for (int i = 0; i < static_cast<int>(userPresets_.size()); ++i) {
      if (userPresets_[static_cast<size_t>(i)].name == name) {
        index = kPresetCount + i;
        break;
      }
    }
    state.presetIndex = index;
    setPresetNotice(state, "Renamed · " + name);
  }
  state.savePreset = false;
  state.renamePreset = false;
  state.deletePresetConfirm = false;
  state.presetOpen = 0;
  state.presetText.clear();
  state.presetTextFresh = false;
  state.presetError.clear();
  state.presetOverwriteConfirm = false;
  return true;
}

bool Panel::importDroppedFile(Shared& shared, PanelState& state,
                              const std::string& path) {
  std::string ext = std::filesystem::path(path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (ext == kUserPresetExt) return importPresetFile(shared, state, path);
  return false;
}

std::vector<Panel::WidgetRect> Panel::widgetRects() const {
  std::vector<WidgetRect> out;
  out.reserve(widgets_.size());
  for (const Widget& w : widgets_) {
    WidgetRect r;
    r.param = w.param;
    r.label = w.label;
    r.knob = w.kind == Kind::Knob;
    r.toggle = w.kind == Kind::Toggle;
    r.r = w.r;
    if (w.kind == Kind::Knob) {
      r.x = w.x - w.r;
      r.y = w.y - w.r;
      r.w = w.r * 2.0;
      r.h = w.r * 2.0;
    } else if (w.kind == Kind::Toggle) {
      r.x = w.x - w.w * 0.5;
      r.y = w.y;
      r.w = w.w;
      r.h = w.h;
    } else {
      r.x = w.x;
      r.y = w.y;
      r.w = w.w;
      r.h = w.h;
    }
    out.push_back(r);
  }
  return out;
}

} // namespace space
