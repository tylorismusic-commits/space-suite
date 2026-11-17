// Space Suite — stereo delay with Digital, Tape, Analog and Ping-Pong modes
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <hdsp/envelope.hpp>
#include <hdsp/smoother.hpp>
#include <hdsp/util.hpp>

#include "controls.hpp"

namespace space {

inline float delayDivisionBeats(int index) noexcept {
  static const float kBeats[9] = {4.0f,  2.0f,  1.0f,  0.5f, 0.333f,
                                  0.25f, 0.166f, 0.125f, 0.75f};
  return kBeats[std::clamp(index, 0, 8)];
}

class Delay {
public:
  void prepare(float sampleRate, float maxMs = 2000.0f) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    maxSamples_ = std::max(64, static_cast<int>(sr_ * maxMs * 0.001f) + 4);
    bufferL_.assign(static_cast<size_t>(maxSamples_), 0.0f);
    bufferR_.assign(static_cast<size_t>(maxSamples_), 0.0f);
    write_ = 0;
    time_.setTime(100.0f, sr_);
    feedback_.setTime(20.0f, sr_);
    mix_.setTime(20.0f, sr_);
    width_.setTime(20.0f, sr_);
    duck_.setTime(20.0f, sr_);
    time_.reset(0.25f * sr_);
    feedback_.reset(0.3f);
    mix_.reset(0.0f);
    width_.reset(0.5f);
    duck_.reset(0.3f);
    toneL_.set(4000.0f, sr_);
    toneR_.set(4000.0f, sr_);
    analogL_.set(5000.0f, sr_);
    analogR_.set(5000.0f, sr_);
    modPhase_ = 0.0f;
    modInc_ = 2.0f * hdsp::kPi * 0.3f / sr_;
    wowPhase_ = 0.0f;
    flutterPhase_ = 0.0f;
    duckEnv_ = 0.0f;
    reset();
    setControls(Controls{}, 120.0f);
  }

  void setControls(const Controls& c, float bpm) noexcept {
    on_ = c.delayOn;
    mode_ = std::clamp(c.delayMode, 0, 3);
    bpm_ = (std::isfinite(bpm) && bpm > 1.0f) ? bpm : c.bpm;
    if (!(std::isfinite(bpm_) && bpm_ > 1.0f)) {
      bpm_ = 120.0f;
    }

    const float beats = delayDivisionBeats(c.delayDiv);
    float ms = c.delaySync ? (60000.0f / bpm_ * beats) : c.delayTimeMs;
    ms = hdsp::clampf(ms, 1.0f,
                      static_cast<float>(maxSamples_ - 3) * 1000.0f / sr_);
    time_.setTarget(ms * 0.001f * sr_);

    const float feedback = hdsp::clampf(c.delayFeedback / 100.0f, 0.0f, 0.95f);
    feedback_.setTarget(feedback);
    mix_.setTarget(hdsp::clampf(c.delayMix / 100.0f, 0.0f, 1.0f));
    width_.setTarget(hdsp::clampf(c.delayWidth / 100.0f, 0.0f, 1.0f));
    duck_.setTarget(hdsp::clampf(c.delayDuck / 100.0f, 0.0f, 1.0f));

    float toneHz = 200.0f * std::pow(40.0f, hdsp::clampf(c.delayTone / 100.0f, 0.0f, 1.0f));
    if (mode_ == 1) {
      toneHz *= 0.55f;  // Tape playback bandwidth.
    } else if (mode_ == 2) {
      toneHz *= 0.35f;  // BBD dark repeat.
    }
    toneL_.set(toneHz, sr_);
    toneR_.set(toneHz, sr_);
    analogL_.set(2500.0f, sr_);
    analogR_.set(2500.0f, sr_);

    const float mod = hdsp::clampf(c.delayMod / 100.0f, 0.0f, 1.0f);
    modDepth_ = mod * 0.002f * sr_;  // up to 2 ms
    wowDepthMs_ = (0.10f + 0.55f * mod);
    flutterDepthMs_ = (0.015f + 0.08f * mod);
  }

  void process(float& l, float& r) noexcept {
    const float dryL = l;
    const float dryR = r;
    const float in = 0.5f * (std::fabs(dryL) + std::fabs(dryR));
    const float atk = 1.0f - hdsp::timeCoeff(5.0f, sr_);
    const float rel = 1.0f - hdsp::timeCoeff(150.0f, sr_);
    duckEnv_ += (in - duckEnv_) * (in > duckEnv_ ? atk : rel);

    float d = time_.next();
    const float mod = modDepth_ * std::sin(modPhase_);
    modPhase_ += modInc_;
    if (modPhase_ > 2.0f * hdsp::kPi) {
      modPhase_ -= 2.0f * hdsp::kPi;
    }
    float tapeMod = 0.0f;
    if (mode_ == 1) {
      wowPhase_ += 2.0f * hdsp::kPi * 0.55f / sr_;
      flutterPhase_ += 2.0f * hdsp::kPi * 6.5f / sr_;
      if (wowPhase_ > 2.0f * hdsp::kPi) wowPhase_ -= 2.0f * hdsp::kPi;
      if (flutterPhase_ > 2.0f * hdsp::kPi) flutterPhase_ -= 2.0f * hdsp::kPi;
      tapeMod = (wowDepthMs_ * std::sin(wowPhase_) +
                 flutterDepthMs_ * std::sin(flutterPhase_)) *
                0.001f * sr_;
    } else if (mode_ == 2) {
      tapeMod = 0.00025f * sr_ * std::sin(modPhase_ * 0.7f);
    }
    d += mod + tapeMod;
    d = hdsp::clampf(d, 1.0f, static_cast<float>(maxSamples_ - 2));

    float wetL = toneL_.process(read(bufferL_, d));
    float wetR = toneR_.process(read(bufferR_, d));

    const float fb = feedback_.next();
    float fbL = wetL * fb;
    float fbR = wetR * fb;
    if (mode_ == 1) {
      fbL = softClip(fbL * 1.25f) * 0.8f;
      fbR = softClip(fbR * 1.25f) * 0.8f;
    } else if (mode_ == 2) {
      fbL = analogL_.process(softClip(fbL * 1.15f));
      fbR = analogR_.process(softClip(fbR * 1.15f));
      fbL *= 0.9f;
      fbR *= 0.9f;
    }
    if (mode_ == 3) {
      std::swap(fbL, fbR);
    }

    bufferL_[write_] = dryL + fbL;
    bufferR_[write_] = dryR + fbR;
    write_ = (write_ + 1) % maxSamples_;

    const float width = width_.next();
    const float mid = 0.5f * (wetL + wetR);
    const float side = 0.5f * (wetL - wetR) * width;
    wetL = mid + side;
    wetR = mid - side;

    const float duck = duck_.next();
    const float duckGain = 1.0f - duck * hdsp::clampf(duckEnv_ * 8.0f, 0.0f, 1.0f);
    const float mix = on_ ? mix_.next() * duckGain : 0.0f;
    l = dryL * (1.0f - mix) + wetL * mix;
    r = dryR * (1.0f - mix) + wetR * mix;
  }

  void reset() noexcept {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    write_ = 0;
    modPhase_ = 0.0f;
    wowPhase_ = 0.0f;
    flutterPhase_ = 0.0f;
    duckEnv_ = 0.0f;
    toneL_.reset();
    toneR_.reset();
    analogL_.reset();
    analogR_.reset();
    time_.snap();
    feedback_.snap();
    mix_.snap();
    width_.snap();
    duck_.snap();
  }

private:
  static float softClip(float x) noexcept {
    return std::tanh(x);
  }

  float read(const std::vector<float>& buf, float delaySamples) const noexcept {
    float pos = static_cast<float>(write_) - delaySamples;
    while (pos < 0.0f) {
      pos += static_cast<float>(maxSamples_);
    }
    const int i0 = static_cast<int>(pos) % maxSamples_;
    const int i1 = (i0 + 1) % maxSamples_;
    const float frac = pos - std::floor(pos);
    return buf[static_cast<size_t>(i0)] * (1.0f - frac) +
           buf[static_cast<size_t>(i1)] * frac;
  }

  float sr_ = 48000.0f;
  int maxSamples_ = 96000;
  int write_ = 0;
  float bpm_ = 120.0f;
  std::vector<float> bufferL_;
  std::vector<float> bufferR_;
  hdsp::Smoothed time_;
  hdsp::Smoothed feedback_;
  hdsp::Smoothed mix_;
  hdsp::Smoothed width_;
  hdsp::Smoothed duck_;
  hdsp::OnePoleLP toneL_, toneR_;
  hdsp::OnePoleLP analogL_, analogR_;
  float modPhase_ = 0.0f;
  float modInc_ = 0.0f;
  float modDepth_ = 0.0f;
  float wowPhase_ = 0.0f;
  float flutterPhase_ = 0.0f;
  float wowDepthMs_ = 0.1f;
  float flutterDepthMs_ = 0.02f;
  float duckEnv_ = 0.0f;
  bool on_ = false;
  int mode_ = 0;
};

} // namespace space
