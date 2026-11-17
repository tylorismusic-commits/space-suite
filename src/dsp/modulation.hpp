// Space Suite — chorus / flanger / phaser / dimension
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <hdsp/smoother.hpp>
#include <hdsp/util.hpp>

#include "controls.hpp"

namespace space {

class Modulation {
public:
  void prepare(float sampleRate) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    maxSamples_ = std::max(64, static_cast<int>(sr_ * 0.060f) + 4);
    bufferL_.assign(static_cast<size_t>(maxSamples_), 0.0f);
    bufferR_.assign(static_cast<size_t>(maxSamples_), 0.0f);
    write_ = 0;
    rate_.setTime(40.0f, sr_);
    depth_.setTime(40.0f, sr_);
    feedback_.setTime(30.0f, sr_);
    width_.setTime(30.0f, sr_);
    mix_.setTime(25.0f, sr_);
    rate_.reset(0.35f);
    depth_.reset(0.5f);
    feedback_.reset(0.2f);
    width_.reset(0.7f);
    mix_.reset(0.4f);
    phase_ = 0.0f;
    reset();
  }

  void setControls(const Controls& c) noexcept {
    on_ = c.modOn;
    mode_ = std::clamp(c.modMode, 0, 3);
    rate_.setTarget(hdsp::clampf(c.modRate, 0.05f, 10.0f));
    depth_.setTarget(hdsp::clampf(c.modDepth / 100.0f, 0.0f, 1.0f));
    feedback_.setTarget(hdsp::clampf(c.modFeedback / 100.0f, 0.0f, 0.90f));
    width_.setTarget(hdsp::clampf(c.modWidth / 100.0f, 0.0f, 1.0f));
    mix_.setTarget(hdsp::clampf(c.modMix / 100.0f, 0.0f, 1.0f));
  }

  void process(float& l, float& r) noexcept {
    const float dryL = l;
    const float dryR = r;
    const float rate = rate_.next();
    const float depth = depth_.next();
    const float feedback = feedback_.next();
    const float width = width_.next();
    const float mix = on_ ? mix_.next() : 0.0f;

    phase_ += rate / sr_;
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;
    }
    const float lfoL = std::sin(2.0f * hdsp::kPi * phase_);
    const float lfoR = std::sin(2.0f * hdsp::kPi * (phase_ + 0.25f));

    if (mode_ == 0) {
      // Chorus: two modulated delay lines, 12 ms centre.
      const float base = 0.012f * sr_;
      const float swing = 0.006f * sr_ * depth;
      const float dL = hdsp::clampf(base + swing * lfoL, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      const float dR = hdsp::clampf(base + swing * lfoR, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      float wetL = read(bufferL_, dL);
      float wetR = read(bufferR_, dR);
      bufferL_[write_] = dryL;
      bufferR_[write_] = dryR;
      write_ = (write_ + 1) % maxSamples_;
      const float mid = 0.5f * (wetL + wetR);
      const float side = 0.5f * (wetL - wetR) * width;
      wetL = mid + side;
      wetR = mid - side;
      l = dryL * (1.0f - mix) + wetL * mix;
      r = dryR * (1.0f - mix) + wetR * mix;
    } else if (mode_ == 1) {
      // Flanger: short modulated delay with feedback.
      const float base = 0.0025f * sr_;
      const float swing = 0.0020f * sr_ * depth;
      const float dL = hdsp::clampf(base + swing * lfoL, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      const float dR = hdsp::clampf(base + swing * lfoR, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      float wetL = read(bufferL_, dL);
      float wetR = read(bufferR_, dR);
      bufferL_[write_] = dryL + wetL * feedback;
      bufferR_[write_] = dryR + wetR * feedback;
      write_ = (write_ + 1) % maxSamples_;
      const float mid = 0.5f * (wetL + wetR);
      const float side = 0.5f * (wetL - wetR) * width;
      wetL = mid + side;
      wetR = mid - side;
      l = dryL * (1.0f - mix) + wetL * mix;
      r = dryR * (1.0f - mix) + wetR * mix;
    } else if (mode_ == 2) {
      // Phaser: four modulated first-order allpass stages per channel.
      const float centre = 700.0f * std::pow(6.0f, depth * 0.5f);
      const float sweep = 3.0f;
      const float fL = centre * std::pow(sweep, lfoL);
      const float fR = centre * std::pow(sweep, lfoR);
      float xL = dryL + phaserLastL_ * feedback;
      float xR = dryR + phaserLastR_ * feedback;
      for (int i = 0; i < 4; ++i) {
        const float aL = allpassCoeff(fL * (1.0f + 0.25f * i));
        const float aR = allpassCoeff(fR * (1.0f + 0.25f * i));
        xL = apL_[i].process(xL, aL);
        xR = apR_[i].process(xR, aR);
      }
      phaserLastL_ = xL;
      phaserLastR_ = xR;
      const float mid = 0.5f * (xL + xR);
      const float side = 0.5f * (xL - xR) * width;
      l = dryL * (1.0f - mix) + (mid + side) * mix;
      r = dryR * (1.0f - mix) + (mid - side) * mix;
    } else {
      // Dimension: two short delays, one LFO, no feedback.
      const float base = 0.008f * sr_;
      const float swing = 0.004f * sr_ * depth;
      const float dL = hdsp::clampf(base - swing * lfoL, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      const float dR = hdsp::clampf(base + swing * lfoL, 1.0f,
                                    static_cast<float>(maxSamples_ - 2));
      float wetL = read(bufferL_, dL);
      float wetR = read(bufferR_, dR);
      bufferL_[write_] = dryL;
      bufferR_[write_] = dryR;
      write_ = (write_ + 1) % maxSamples_;
      const float mid = 0.5f * (wetL + wetR);
      const float side = 0.5f * (wetL - wetR) * width;
      wetL = mid + side;
      wetR = mid - side;
      l = dryL * (1.0f - mix) + wetL * mix;
      r = dryR * (1.0f - mix) + wetR * mix;
    }
  }

  void reset() noexcept {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    write_ = 0;
    phase_ = 0.0f;
    phaserLastL_ = 0.0f;
    phaserLastR_ = 0.0f;
    for (int i = 0; i < 4; ++i) {
      apL_[i].reset();
      apR_[i].reset();
    }
    rate_.snap();
    depth_.snap();
    feedback_.snap();
    width_.snap();
    mix_.snap();
  }

private:
  struct Allpass1 {
    float x1 = 0.0f;
    float y1 = 0.0f;

    float process(float x, float a) noexcept {
      const float y = a * x + x1 - a * y1;
      x1 = x;
      y1 = y;
      return y;
    }

    void reset() noexcept {
      x1 = 0.0f;
      y1 = 0.0f;
    }
  };

  float allpassCoeff(float hz) const noexcept {
    const float f = hdsp::clampf(hz, 80.0f, sr_ * 0.45f);
    const float t = std::tan(hdsp::kPi * f / sr_);
    return (t - 1.0f) / (t + 1.0f);
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
  int maxSamples_ = 2880;
  int write_ = 0;
  std::vector<float> bufferL_;
  std::vector<float> bufferR_;
  hdsp::Smoothed rate_;
  hdsp::Smoothed depth_;
  hdsp::Smoothed feedback_;
  hdsp::Smoothed width_;
  hdsp::Smoothed mix_;
  float phase_ = 0.0f;
  float phaserLastL_ = 0.0f;
  float phaserLastR_ = 0.0f;
  Allpass1 apL_[4];
  Allpass1 apR_[4];
  bool on_ = false;
  int mode_ = 0;
};

} // namespace space
