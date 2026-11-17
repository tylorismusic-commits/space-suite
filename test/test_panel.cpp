// Space Suite — panel interaction tests (no X server needed).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cairo.h>

#include <cmath>
#include <cstdio>

#include "params.hpp"
#include "presets.hpp"
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

  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
