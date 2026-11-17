// Space Suite — engine.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>

#include <hdsp/db.hpp>
#include <hdsp/smoother.hpp>
#include <hdsp/util.hpp>

#include "controls.hpp"
#include "delay.hpp"
#include "modulation.hpp"
#include "reverb.hpp"

namespace space {

// Fixed chain: input → modulation → delay → reverb → output.
// Each section keeps its own mix; the global mix is applied last.
class Engine {
public:
  void prepare(float sampleRate, int /*maxBlock*/ = 0) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    reverb_.prepare(sr_);
    delay_.prepare(sr_);
    mod_.prepare(sr_);
    input_.setTime(15.0f, sr_);
    output_.setTime(15.0f, sr_);
    mix_.setTime(20.0f, sr_);
    power_.setTime(20.0f, sr_);
    bypass_.setTime(20.0f, sr_);
    input_.reset(1.0f);
    output_.reset(1.0f);
    mix_.reset(1.0f);
    power_.reset(1.0f);
    bypass_.reset(0.0f);
    reset();
  }

  void setControls(const Controls& c, float hostBpm = 0.0f,
                   bool hostTempo = false) noexcept {
    controls_ = c;
    const float bpm = (hostTempo && std::isfinite(hostBpm) && hostBpm > 1.0f)
                          ? hostBpm
                          : c.bpm;
    reverb_.setControls(c);
    delay_.setControls(c, bpm);
    mod_.setControls(c);
    input_.setTarget(hdsp::dbToGain(c.inputDb));
    output_.setTarget(hdsp::dbToGain(c.outputDb));
    mix_.setTarget(hdsp::clampf(c.mix, 0.0f, 1.0f));
    power_.setTarget(c.power ? 1.0f : 0.0f);
    bypass_.setTarget(c.bypass ? 1.0f : 0.0f);
    lastBpm_ = bpm;
    hostTempo_ = hostTempo;
  }

  void reset() noexcept {
    reverb_.reset();
    delay_.reset();
    mod_.reset();
    input_.snap();
    output_.snap();
    mix_.snap();
    power_.snap();
    bypass_.snap();
  }

  void process(float* left, float* right, int frames,
               Meters& meters) noexcept {
    float inPeakL = 0.0f;
    float inPeakR = 0.0f;
    float outPeakL = 0.0f;
    float outPeakR = 0.0f;

    for (int i = 0; i < frames; ++i) {
      const float rawL = std::isfinite(left[i]) ? left[i] : 0.0f;
      const float rawR = std::isfinite(right[i]) ? right[i] : 0.0f;
      const float inL = hdsp::clampf(rawL, -32.0f, 32.0f);
      const float inR = hdsp::clampf(rawR, -32.0f, 32.0f);

      const float gainIn = input_.next();
      float dryL = inL * gainIn;
      float dryR = inR * gainIn;
      float wetL = dryL;
      float wetR = dryR;

      mod_.process(wetL, wetR);
      delay_.process(wetL, wetR);
      reverb_.process(wetL, wetR);

      const float m = mix_.next();
      float outL = dryL + m * (wetL - dryL);
      float outR = dryR + m * (wetR - dryR);
      const float gainOut = output_.next();
      outL *= gainOut;
      outR *= gainOut;

      // Power and bypass both return the untouched input; the crossfade is
      // smoothed so toggling never clicks.
      const float active = power_.next() * (1.0f - bypass_.next());
      outL = inL + active * (outL - inL);
      outR = inR + active * (outR - inR);

      left[i] = std::isfinite(outL) ? outL : 0.0f;
      right[i] = std::isfinite(outR) ? outR : 0.0f;

      inPeakL = std::max(inPeakL, std::fabs(inL));
      inPeakR = std::max(inPeakR, std::fabs(inR));
      outPeakL = std::max(outPeakL, std::fabs(outL));
      outPeakR = std::max(outPeakR, std::fabs(outR));
    }

    meters.inPeakLDb = hdsp::gainToDb(inPeakL);
    meters.inPeakRDb = hdsp::gainToDb(inPeakR);
    meters.inPeakDb = std::max(meters.inPeakLDb, meters.inPeakRDb);
    meters.outPeakLDb = hdsp::gainToDb(outPeakL);
    meters.outPeakRDb = hdsp::gainToDb(outPeakR);
    meters.outPeakDb = std::max(meters.outPeakLDb, meters.outPeakRDb);
    meters.bpm = lastBpm_;
    meters.hostTempo = hostTempo_;
  }

  int latencySamples() const noexcept { return 0; }
  float bpm() const noexcept { return lastBpm_; }
  bool hostTempo() const noexcept { return hostTempo_; }

private:
  float sr_ = 48000.0f;
  float lastBpm_ = 120.0f;
  bool hostTempo_ = false;
  Controls controls_;
  Reverb reverb_;
  Delay delay_;
  Modulation mod_;
  hdsp::Smoothed input_;
  hdsp::Smoothed output_;
  hdsp::Smoothed mix_;
  hdsp::Smoothed power_;
  hdsp::Smoothed bypass_;
};

} // namespace space
