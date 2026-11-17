// Space Suite — DSP sanity tests
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "dsp/engine.hpp"
#include "params.hpp"

using namespace space;

namespace {
int gFailures = 0;
void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++gFailures;
}

constexpr float kSr = 48000.0f;

void processBlocks(Engine& engine, std::vector<float>& l,
                   std::vector<float>& r) {
  Meters meters;
  for (size_t start = 0; start < l.size(); start += 256) {
    const int n = static_cast<int>(std::min<size_t>(256, l.size() - start));
    engine.process(l.data() + start, r.data() + start, n, meters);
  }
}

double rms(const std::vector<float>& x, size_t from, size_t to) {
  double sum = 0.0;
  for (size_t i = from; i < to && i < x.size(); ++i) {
    sum += static_cast<double>(x[i]) * x[i];
  }
  return std::sqrt(sum / std::max<size_t>(1, to - from));
}

bool finite(const std::vector<float>& x) {
  for (float v : x) {
    if (!std::isfinite(v)) return false;
  }
  return true;
}
} // namespace

int main() {
  {
    // Bypass is exactly the input.
    Engine engine;
    engine.prepare(kSr);
    Controls c;
    c.bypass = true;
    c.inputDb = 0.0f;
    c.outputDb = 0.0f;
    engine.setControls(c, 0.0f, false);
    engine.reset();
    const int n = 512;
    std::vector<float> l(n), r(n), inL(n), inR(n);
    for (int i = 0; i < n; ++i) {
      const float s = 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * i / kSr);
      l[i] = r[i] = inL[i] = inR[i] = s;
    }
    processBlocks(engine, l, r);
    float maxErr = 0.0f;
    for (int i = 0; i < n; ++i) {
      maxErr = std::max(maxErr, std::fabs(l[i] - inL[i]));
      maxErr = std::max(maxErr, std::fabs(r[i] - inR[i]));
    }
    check(maxErr < 1e-6f, "bypass returns the exact input");
  }

  {
    // A reverb tail continues after the input stops.
    Engine engine;
    engine.prepare(kSr);
    Controls c;
    c.reverbOn = true;
    c.reverbMode = 2;
    c.reverbDecay = 4.0f;
    c.reverbMix = 60.0f;
    c.delayOn = false;
    c.modOn = false;
    c.mix = 1.0f;
    engine.setControls(c, 0.0f, false);
    engine.reset();
    const int active = static_cast<int>(kSr * 0.25f);
    const int tail = static_cast<int>(kSr * 1.0f);
    std::vector<float> l(active + tail), r(active + tail);
    for (int i = 0; i < active; ++i) {
      const float s = 0.4f * std::sin(2.0f * 3.14159265f * 1000.0f * i / kSr);
      l[i] = r[i] = s;
    }
    processBlocks(engine, l, r);
    const double t = rms(l, active + static_cast<int>(kSr * 0.15f),
                         active + static_cast<int>(kSr * 0.35f));
    check(t > 1e-4 && t < 0.5, "reverb leaves a finite tail after input stops");
    check(finite(l) && finite(r), "reverb output is finite");
  }

  {
    // A digital delay repeats at the requested time.
    Engine engine;
    engine.prepare(kSr);
    Controls c;
    c.delayOn = true;
    c.delayMode = 0;
    c.delaySync = false;
    c.delayTimeMs = 100.0f;
    c.delayFeedback = 0.0f;
    c.delayMix = 100.0f;
    c.reverbOn = false;
    c.modOn = false;
    c.mix = 1.0f;
    engine.setControls(c, 0.0f, false);
    engine.reset();
    const int n = static_cast<int>(kSr * 0.3f);
    std::vector<float> l(n, 0.0f), r(n, 0.0f);
    l[0] = r[0] = 0.5f;
    processBlocks(engine, l, r);
    const int centre = static_cast<int>(kSr * 0.1f);
    int peak = 0;
    for (int i = centre - 200; i <= centre + 200; ++i) {
      if (i >= 0 && i < n && std::fabs(l[i]) > std::fabs(l[peak])) peak = i;
    }
    check(peak >= centre - 48 && peak <= centre + 48,
          "digital delay repeats at 100 ms");
    check(std::fabs(l[peak]) > 0.05f, "digital delay repeat is audible");
  }

  {
    // Chorus changes the signal but stays finite.
    Engine engine;
    engine.prepare(kSr);
    Controls c;
    c.reverbOn = false;
    c.delayOn = false;
    c.modOn = true;
    c.modMode = 0;
    c.modRate = 0.4f;
    c.modDepth = 70.0f;
    c.modMix = 60.0f;
    c.mix = 1.0f;
    engine.setControls(c, 0.0f, false);
    engine.reset();
    const int n = static_cast<int>(kSr * 0.3f);
    std::vector<float> l(n), r(n), inL(n);
    for (int i = 0; i < n; ++i) {
      const float s = 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * i / kSr);
      l[i] = r[i] = inL[i] = s;
    }
    processBlocks(engine, l, r);
    double diff = 0.0;
    for (int i = 0; i < n; ++i) diff += std::fabs(l[i] - inL[i]);
    check(diff > 1.0, "chorus changes the signal");
    check(finite(l) && finite(r), "modulation output is finite");
  }

  {
    // Extreme settings do not produce NaN or a runaway.
    Engine engine;
    engine.prepare(kSr);
    Controls c;
    c.reverbOn = true;
    c.reverbMode = 0;
    c.reverbDecay = 12.0f;
    c.reverbMix = 100.0f;
    c.delayOn = true;
    c.delayMode = 1;
    c.delayFeedback = 95.0f;
    c.delayMix = 100.0f;
    c.modOn = true;
    c.modMode = 1;
    c.modFeedback = 95.0f;
    c.modMix = 100.0f;
    c.mix = 1.0f;
    engine.setControls(c, 0.0f, false);
    engine.reset();
    const int n = static_cast<int>(kSr * 1.0f);
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i) {
      l[i] = r[i] = 0.5f * std::sin(2.0f * 3.14159265f * 220.0f * i / kSr);
    }
    processBlocks(engine, l, r);
    float peak = 0.0f;
    for (float v : l) peak = std::max(peak, std::fabs(v));
    check(finite(l) && finite(r), "extreme settings stay finite");
    check(peak < 8.0f, "extreme settings do not run away");
  }

  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
