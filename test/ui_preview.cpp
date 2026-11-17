// Space Suite — renders each page to a PNG for visual checks.
// Usage: space_ui_preview <font-dir> <output-dir> [scale]
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cairo.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "params.hpp"
#include "presets.hpp"
#include "ui/geometry.hpp"
#include "ui/panel.hpp"

using namespace space;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s <font-dir> <output-dir> [scale]\n", argv[0]);
    return 2;
  }
  const std::string fontDir = argv[1];
  const std::string outDir = argv[2];
  const double scale = argc > 3 ? std::atof(argv[3]) : 1.0;
  const UiSize size = sizeForScale(scale);
  const double drawScale = static_cast<double>(size.width) / kPanelWidth;

  static const char* kNames[3] = {"reverb", "delay", "mod"};

  Panel panel;
  panel.loadFont(fontDir);

  for (int page = 0; page < 3; ++page) {
    Shared shared;
    shared.set(P_REVERB_ON, 1);
    shared.set(P_DELAY_ON, page >= 1 ? 1 : 0);
    shared.set(P_MOD_ON, page >= 2 ? 1 : 0);
    shared.set(P_REVERB_MIX, 35);
    shared.set(P_DELAY_MIX, 25);
    shared.set(P_MOD_MIX, 40);
    shared.inPeakLDb.store(-9.0f);
    shared.inPeakRDb.store(-11.0f);
    shared.outPeakLDb.store(-7.0f);
    shared.outPeakRDb.store(-8.0f);

    PanelState state;
    state.page = page;
    for (int i = 0; i < 20; ++i) panel.tick(shared, state);

    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width, size.height);
    cairo_t* cr = cairo_create(surface);
    cairo_scale(cr, drawScale, drawScale);
    panel.draw(cr, shared, state);
    cairo_destroy(cr);

    const std::string path = outDir + "/" + kNames[page] + ".png";
    const cairo_status_t st = cairo_surface_write_to_png(surface, path.c_str());
    if (st != CAIRO_STATUS_SUCCESS) {
      std::printf("write failed: %s\n", cairo_status_to_string(st));
    }
    cairo_surface_destroy(surface);
    std::printf("wrote %s\n", path.c_str());
  }

  // Space Ghost — the mandatory preset.
  for (int i = 0; i < kPresetCount; ++i) {
    if (std::string(kPresets[i].name) != "Space Ghost") continue;
    Shared shared;
    PanelState state;
    panel.applyPreset(shared, state, i);
    shared.inPeakLDb.store(-9.0f);
    shared.inPeakRDb.store(-11.0f);
    shared.outPeakLDb.store(-7.0f);
    shared.outPeakRDb.store(-8.0f);
    for (int t = 0; t < 20; ++t) panel.tick(shared, state);
    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width, size.height);
    cairo_t* cr = cairo_create(surface);
    cairo_scale(cr, drawScale, drawScale);
    panel.draw(cr, shared, state);
    cairo_destroy(cr);
    const std::string path = outDir + "/space-ghost.png";
    cairo_surface_write_to_png(surface, path.c_str());
    cairo_surface_destroy(surface);
    std::printf("wrote %s\n", path.c_str());
    break;
  }
  return 0;
}
