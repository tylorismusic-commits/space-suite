// Space Suite — editor geometry rules.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdio>
#include <cstdlib>

#include "ui/geometry.hpp"

using namespace space;

namespace {

int gFailures = 0;

void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) {
    ++gFailures;
  }
}

bool onAspect(UiSize s) {
  return static_cast<uint64_t>(s.width) * kPanelHeight ==
         static_cast<uint64_t>(s.height) * kPanelWidth;
}

} // namespace

int main() {
  bool allAspect = true;
  bool idempotent = true;
  bool bounded = true;
  std::srand(7);
  for (int i = 0; i < 2000; ++i) {
    const uint32_t w = 1 + std::rand() % 6000;
    const uint32_t h = 1 + std::rand() % 6000;
    const UiSize s = snapSize(w, h);
    allAspect = allAspect && onAspect(s);
    idempotent = idempotent && snapSize(s.width, s.height).width == s.width;
    const double scale = static_cast<double>(s.width) / kPanelWidth;
    bounded = bounded && scale >= kMinUiScale - 1e-9 && scale <= kMaxUiScale + 1e-9;
  }
  check(allAspect, "any requested size snaps to exactly 1040:640");
  check(idempotent, "snapping a valid size changes nothing");
  check(bounded, "snapped scale stays within the allowed range");

  check(snapSize(100, 100).width == 520 && snapSize(100, 100).height == 320,
        "tiny request clamps to the minimum (0.5x)");
  check(snapSize(9000, 9000).width == 2600 && snapSize(9000, 9000).height == 1600,
        "huge request clamps to the maximum (2.5x)");
  check(snapSize(1560, 900).width == 1560 && snapSize(1560, 900).height == 960,
        "width drag wins over a mismatched height");
  check(snapSize(1040, 640).width == 1040 && snapSize(1040, 640).height == 640,
        "nominal size is unchanged");

  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
