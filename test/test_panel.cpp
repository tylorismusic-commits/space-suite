// Space Suite — panel interaction tests (no X server needed).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cairo.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "params.hpp"
#include "presets.hpp"
#include "user_presets.hpp"
#include "ui/panel.hpp"
#include "value_text.hpp"

using namespace space;

namespace {
int gFailures = 0;
void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++gFailures;
}
} // namespace

int main() {
  const std::filesystem::path config =
      "/tmp/opencode/space-suite-test-config";
  std::filesystem::remove_all(config);
  std::filesystem::create_directories(config / "home");
  setenv("XDG_CONFIG_HOME", config.string().c_str(), 1);
  setenv("HOME", (config / "home").string().c_str(), 1);

  cairo_surface_t* surf =
      cairo_image_surface_create(CAIRO_FORMAT_RGB24, kPanelWidth, kPanelHeight);
  cairo_t* cr = cairo_create(surf);

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    check(state.page == 0, "starts on the Reverb page");

    // Header page tab: Delay.
    panel.pointerDown(shared, state, 803, 31, false);
    panel.pointerUp();
    check(state.page == 1, "Delay page tab selects the Delay page");

    // Mode tab: Tape (index 1).
    panel.draw(cr, shared, state);
    panel.pointerDown(shared, state, 400, 90, false);
    panel.pointerUp();
    check(shared.get(P_DELAY_MODE) == 1.0, "Tape mode tab selects Tape");

    // Bypass pill.
    panel.draw(cr, shared, state);
    panel.pointerDown(shared, state, 780, 540, false);
    panel.pointerUp();
    check(shared.get(P_BYPASS) == 1.0, "bypass pill toggles on");
  }

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    // Next preset arrow.
    panel.pointerDown(shared, state, 72 + 330 - 18, 32, false);
    panel.pointerUp();
    check(state.presetIndex == 0, "preset next arrow applies a preset");
    check(state.page == kPresets[0].page, "preset switches to its page");
  }

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    // Apply the Chorus preset directly and check the page and on flag.
    panel.applyPreset(shared, state, 10);
    check(state.page == 2, "Chorus preset switches to the Mod page");
    check(shared.get(P_MOD_ON) == 1.0, "Chorus preset turns modulation on");
  }

  {
    Shared shared;
    double v = -1.0;
    check(parseParamValue(shared, P_DELAY_TIME, "250 ms", &v) &&
              std::fabs(v - 250.0) < 1e-9,
          "typed delay time accepts 250 ms");
    check(parseParamValue(shared, P_REVERB_DECAY, "4.5 s", &v) &&
              std::fabs(v - 4.5) < 1e-9,
          "typed reverb decay accepts 4.5 s");
    check(parseParamValue(shared, P_DELAY_MODE, "Tape", &v) &&
              std::fabs(v - 1.0) < 1e-9,
          "typed delay mode accepts Tape");
    check(parseParamValue(shared, P_REVERB_MODE, "Hall", &v) &&
              std::fabs(v - 2.0) < 1e-9,
          "typed reverb mode accepts Hall");
  }

  // Every control must format to text and parse back without drift.
  {
    Shared shared;
    bool ok = true;
    for (uint32_t id = 0; id < P_COUNT; ++id) {
      const ParamSpec& spec = kParamSpecs[id];
      for (double v : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const double value = spec.min + (spec.max - spec.min) * v;
        const std::string text = formatParamValue(shared, id, value);
        double out = 0.0;
        if (!parseParamValue(shared, id, text, &out)) {
          ok = false;
          std::printf("parse failed: %s -> '%s'\n", spec.name, text.c_str());
          continue;
        }
        const double tolerance =
            spec.stepped ? 0.51 : std::max(1e-9, (spec.max - spec.min) * 0.02);
        if (std::fabs(out - value) > tolerance) {
          ok = false;
          std::printf("round-trip drift: %s %.4f -> '%s' -> %.4f\n",
                      spec.name, value, text.c_str(), out);
        }
      }
    }
    check(ok, "every typed control value round-trips");
  }

  // Preset browser: clicking a row loads it, clicking a category header or
  // the empty gap does not load a hidden preset.
  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    check(state.presetOpen != 0, "preset bar opens the browser");
    const int before = state.presetIndex;
    panel.pointerDown(shared, state, 200, 56.0 + 12.0, false);
    panel.pointerUp();
    check(state.presetIndex == before,
          "clicking a category header does not select a preset");
  }

  // User presets: save, duplicate, recall, dirty tracking, conflict
  // handling, rename, export, import, rescan and delete.
  {
    Shared shared;
    Panel panel;
    PanelState state;
    auto indexOf = [&](const char* name) {
      for (int i = 0; i < panel.presetCount(); ++i) {
        if (std::string(panel.presetName(i)) == name) return i;
      }
      return -1;
    };
    state.page = 1;
    shared.set(P_MIX, 42.0);
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 117, 439, false);  // SAVE
    panel.pointerUp();
    check(state.savePreset, "SAVE opens the preset name field");
    for (const char* c = "Test Space"; *c; ++c) panel.textKey(shared, state, 0, *c);
    panel.textKey(shared, state, Panel::kTextEnter, 0);
    check(panel.presetCount() == kPresetCount + 1 &&
              std::string(panel.presetName(kPresetCount)) == "Test Space",
          "saving adds a named user preset");
    check(std::filesystem::exists(userPresetDir() / "Test Space.space-suite"),
          "saved preset file exists");

    // Duplicate the selected user preset.
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 208, 439, false);  // DUPLICATE
    panel.pointerUp();
    check(panel.presetCount() == kPresetCount + 2 &&
              indexOf("Test Space copy") >= 0,
          "DUPLICATE creates a copy");

    // Delete the duplicate.
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 198, 469, false);  // DELETE
    panel.pointerUp();
    check(state.deletePresetConfirm, "DELETE asks for confirmation");
    panel.pointerDown(shared, state, 198, 469, false);  // CONFIRM
    panel.pointerUp();
    check(panel.presetCount() == kPresetCount + 1 &&
              indexOf("Test Space copy") < 0,
          "DELETE removes the duplicate");

    // Recall the user preset, including its editor page.
    panel.applyPreset(shared, state, indexOf("Test Space"));
    check(std::fabs(shared.get(P_MIX) - 42.0) < 1e-6 && state.page == 1,
          "user preset recalls values and page");
    shared.set(P_MIX, 7.0);
    panel.tick(shared, state);
    check(state.presetDirty, "host change marks the preset dirty");
    shared.set(P_MIX, 42.0);
    panel.tick(shared, state);
    check(!state.presetDirty, "restoring the value clears dirty");

    // Rename conflict, then a free rename.
    state.page = 2;
    shared.set(P_MIX, 11.0);
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 117, 439, false);  // SAVE another
    panel.pointerUp();
    for (const char* c = "Other Space"; *c; ++c) panel.textKey(shared, state, 0, *c);
    panel.textKey(shared, state, Panel::kTextEnter, 0);
    const int other = indexOf("Other Space");
    check(other >= 0, "second user preset saved");
    panel.applyPreset(shared, state, other);
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 118, 469, false);  // RENAME
    panel.pointerUp();
    check(state.renamePreset, "RENAME opens the name field");
    for (const char* c = "Test Space"; *c; ++c) panel.textKey(shared, state, 0, *c);
    panel.textKey(shared, state, Panel::kTextEnter, 0);
    check(!state.presetError.empty() && indexOf("Other Space") >= 0,
          "rename conflict is reported");
    state.presetText = "Renamed Space";
    panel.textKey(shared, state, Panel::kTextEnter, 0);
    check(indexOf("Renamed Space") >= 0 && indexOf("Other Space") < 0,
          "rename moves the preset");

    // Export the selected preset.
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 286, 469, false);  // EXPORT
    panel.pointerUp();
    check(std::filesystem::exists(userPresetExportDir() /
                                  "Renamed Space.space-suite"),
          "EXPORT writes the preset file");

    // Drag-and-drop import with a collision suffix.
    const std::filesystem::path external = config / "external";
    std::filesystem::create_directories(external);
    std::vector<double> imported;
    for (uint32_t i = 0; i < P_COUNT; ++i) imported.push_back(shared.get(i));
    imported[P_MIX] = 55.0;
    check(writeUserPreset(external, "Test Space", imported, 2),
          "import fixture written");
    check(panel.importDroppedFile(
              shared, state, (external / "Test Space.space-suite").string()),
          "dropped preset imports");
    check(indexOf("Test Space (2)") >= 0 &&
              std::fabs(shared.get(P_MIX) - 55.0) < 1e-6 && state.page == 2,
          "imported preset loads its values and page");

    // A preset written by another instance appears when the list opens.
    const int beforeRescan = panel.presetCount();
    saveUserPreset(userPresetDir(), "External Space", shared, state.page);
    state.presetOpen = 0;
    panel.pointerDown(shared, state, 200, 32, false);
    panel.pointerUp();
    check(panel.presetCount() == beforeRescan + 1 &&
              indexOf("External Space") >= 0,
          "opening the list rescans presets");
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
