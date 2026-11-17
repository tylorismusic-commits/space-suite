// Space Suite — plate / room / hall / spring reverb
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Eight-line feedback delay network with input diffusion, damping and slow
// modulation.  Plate, Room, Hall and Spring change the line lengths,
// damping, diffusion and decay.  Based on Vocal Suite's Space engine.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <hdsp/envelope.hpp>
#include <hdsp/smoother.hpp>
#include <hdsp/util.hpp>

#include "controls.hpp"

namespace space {

class Reverb {
public:
  void prepare(float sampleRate) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    maxSamples_ = std::max(64, static_cast<int>(sr_ * 0.30f) + 4);
    for (int i = 0; i < 8; ++i) {
      lineL_[i].buf.assign(static_cast<size_t>(maxSamples_), 0.0f);
      lineR_[i].buf.assign(static_cast<size_t>(maxSamples_), 0.0f);
      lineL_[i].write = 0;
      lineR_[i].write = 0;
    }
    preSize_ = std::max(16, static_cast<int>(sr_ * 0.21f) + 2);
    preL_.assign(static_cast<size_t>(preSize_), 0.0f);
    preR_.assign(static_cast<size_t>(preSize_), 0.0f);
    preWrite_ = 0;
    const float apMs[4] = {5.1f, 7.3f, 9.7f, 12.1f};
    for (int i = 0; i < 4; ++i) {
      apL_[i].prepare(static_cast<int>(sr_ * apMs[i] * 0.001f) + 1);
      apR_[i].prepare(static_cast<int>(sr_ * apMs[i] * 0.001f) + 1);
    }
    static const float kSpringApMs[6] = {1.7f, 2.4f, 3.3f, 4.6f,
                                         6.1f, 8.2f};
    for (int i = 0; i < 6; ++i) {
      springApL_[i].prepare(
          static_cast<int>(sr_ * kSpringApMs[i] * 0.001f) + 1);
      springApR_[i].prepare(
          static_cast<int>(sr_ * kSpringApMs[i] * 0.001f) + 1);
    }
    mix_.setTime(25.0f, sr_);
    widthS_.setTime(30.0f, sr_);
    duckS_.setTime(30.0f, sr_);
    preS_.setTime(30.0f, sr_);
    mix_.reset(0.0f);
    widthS_.reset(0.7f);
    duckS_.reset(0.3f);
    preS_.reset(0.0f);
    modPhase_ = 0.0f;
    modInc_ = 2.0f * hdsp::kPi * 0.13f / sr_;
    duckEnv_ = 0.0f;
    reset();
    setControls(Controls{});
  }

  void setControls(const Controls& c) noexcept {
    on_ = c.reverbOn;
    mode_ = std::clamp(c.reverbMode, 0, 3);
    freeze_ = c.reverbFreeze;
    const float size = hdsp::clampf(0.6f + 0.01f * c.reverbSize, 0.6f, 1.6f);
    const float rt60 = hdsp::clampf(c.reverbDecay, 0.3f, 12.0f);

    static const float kPlate[8] = {29.7f, 37.1f, 41.1f, 43.7f,
                                    47.9f, 53.3f, 59.3f, 67.1f};
    static const float kRoom[8] = {17.3f, 19.7f, 23.1f, 27.7f,
                                   31.3f, 37.9f, 43.1f, 47.3f};
    static const float kHall[8] = {41.1f, 47.9f, 53.3f, 59.3f,
                                   67.1f, 73.7f, 83.9f, 97.1f};
    static const float kSpring[8] = {13.3f, 16.1f, 18.7f, 21.7f,
                                     25.3f, 29.9f, 34.7f, 39.1f};
    const float* base = mode_ == 1 ? kRoom
                         : mode_ == 2 ? kHall
                         : mode_ == 3 ? kSpring
                                      : kPlate;
    for (int i = 0; i < 8; ++i) {
      const float offset = (i & 1) ? 1.7f : 0.0f;
      const float ms = base[i] * size + offset;
      len_[i] = hdsp::clampf(ms * 0.001f * sr_, 16.0f,
                             static_cast<float>(maxSamples_ - 3));
      if (lenCur_[i] <= 0.0f) {
        lenCur_[i] = len_[i];
      }
      const float lenSec = len_[i] / sr_;
      float g = std::pow(10.0f, -3.0f * lenSec / rt60);
      g = hdsp::clampf(g, 0.15f, freeze_ ? 0.9975f : 0.995f);
      if (mode_ == 1) {
        g *= 0.97f;  // Room decays a little faster.
      }
      if (mode_ == 3) {
        g *= 0.92f;  // Spring is shorter, darker and more damped.
      }
      fb_[i] = g;
    }

    const float tone = hdsp::clampf(c.reverbTone / 100.0f, 0.0f, 1.0f);
    float dampHz = 2000.0f * std::pow(8.0f, tone);
    if (mode_ == 3) {
      dampHz *= 0.55f;
    }
    dampG_ = 1.0f - std::exp(-2.0f * hdsp::kPi *
                             hdsp::clampf(dampHz, 300.0f, sr_ * 0.45f) / sr_);
    preS_.setTarget(hdsp::clampf(c.reverbPreDelay * 0.001f * sr_, 0.0f,
                                 static_cast<float>(preSize_ - 2)));
    widthS_.setTarget(hdsp::clampf(c.reverbWidth / 100.0f, 0.0f, 1.0f));
    duckS_.setTarget(hdsp::clampf(c.reverbDuck / 100.0f, 0.0f, 1.0f));
    mix_.setTarget(hdsp::clampf(c.reverbMix / 100.0f, 0.0f, 1.0f));
    diffusion_ = mode_ == 1 ? 0.35f : (mode_ == 2 ? 0.6f : (mode_ == 3 ? 0.25f : 0.5f));
    inputGain_ = freeze_ ? 0.0f : 0.22f;
  }

  void process(float& l, float& r) noexcept {
    if (!on_) {
      return;
    }
    const float dryL = l;
    const float dryR = r;
    const float in = 0.5f * (std::fabs(dryL) + std::fabs(dryR));
    const float atk = 1.0f - hdsp::timeCoeff(5.0f, sr_);
    const float rel = 1.0f - hdsp::timeCoeff(180.0f, sr_);
    duckEnv_ += (in - duckEnv_) * (in > duckEnv_ ? atk : rel);

    preL_[preWrite_] = dryL;
    preR_[preWrite_] = dryR;
    const float preSamples = preS_.next();
    int read = preWrite_ - static_cast<int>(std::lround(preSamples));
    if (read < 0) {
      read += preSize_;
    }
    float inL = preL_[read];
    float inR = preR_[read];
    preWrite_ = (preWrite_ + 1) % preSize_;

    for (int i = 0; i < 4; ++i) {
      inL = apL_[i].process(inL, diffusion_);
      inR = apR_[i].process(inR, diffusion_);
    }

    float dL[8], dR[8];
    for (int i = 0; i < 8; ++i) {
      lenCur_[i] += (len_[i] - lenCur_[i]) * 0.0008f;
      const float modDepth = mode_ == 3 ? 0.0012f : 0.0007f;
      const float mod =
          modDepth * sr_ * std::sin(modPhase_ + static_cast<float>(i) * 0.7f);
      const float len = hdsp::clampf(lenCur_[i] + mod, 4.0f,
                                     static_cast<float>(maxSamples_ - 3));
      dL[i] = lineL_[i].read(len);
      dR[i] = lineR_[i].read(len);
      dampL_[i] += (dL[i] - dampL_[i]) * dampG_;
      dampR_[i] += (dR[i] - dampR_[i]) * dampG_;
      dL[i] = dampL_[i];
      dR[i] = dampR_[i];
    }
    modPhase_ += modInc_;
    if (modPhase_ > 2.0f * hdsp::kPi) {
      modPhase_ -= 2.0f * hdsp::kPi;
    }

    hadamard(dL);
    hadamard(dR);
    constexpr float kNorm = 0.35355339f;  // 1/sqrt(8)
    for (int i = 0; i < 8; ++i) {
      const float sign = (i & 1) ? -1.0f : 1.0f;
      const float injL = inL * inputGain_ * sign;
      const float injR = inR * inputGain_ * ((i & 2) ? -1.0f : 1.0f);
      lineL_[i].push(injL + dL[i] * kNorm * fb_[i]);
      lineR_[i].push(injR + dR[i] * kNorm * fb_[i]);
    }

    float wetL = 0.0f;
    float wetR = 0.0f;
    for (int i = 0; i < 8; ++i) {
      const float tap = (i & 1) ? -0.18f : 0.18f;
      wetL += dL[i] * tap;
      wetR += dR[i] * ((i & 2) ? -0.18f : 0.18f);
    }
    const float width = widthS_.next();
    const float mid = 0.5f * (wetL + wetR);
    const float side = 0.5f * (wetL - wetR) * width;
    wetL = mid + side;
    wetR = mid - side;
    if (mode_ == 3) {
      // Short dispersive allpass chain: the chirpy, springy tail that the
      // FDN alone does not produce.
      for (int i = 0; i < 6; ++i) {
        wetL = springApL_[i].process(wetL, 0.68f);
        wetR = springApR_[i].process(wetR, 0.68f);
      }
    }

    const float duck = duckS_.next();
    const float duckGain = 1.0f - duck * hdsp::clampf(duckEnv_ * 8.0f, 0.0f, 1.0f);
    const float mix = mix_.next() * duckGain;
    l = dryL * (1.0f - mix) + wetL * mix;
    r = dryR * (1.0f - mix) + wetR * mix;
  }

  void reset() noexcept {
    for (int i = 0; i < 8; ++i) {
      std::fill(lineL_[i].buf.begin(), lineL_[i].buf.end(), 0.0f);
      std::fill(lineR_[i].buf.begin(), lineR_[i].buf.end(), 0.0f);
      lineL_[i].write = 0;
      lineR_[i].write = 0;
      dampL_[i] = 0.0f;
      dampR_[i] = 0.0f;
      lenCur_[i] = 0.0f;
    }
    std::fill(preL_.begin(), preL_.end(), 0.0f);
    std::fill(preR_.begin(), preR_.end(), 0.0f);
    preWrite_ = 0;
    for (int i = 0; i < 4; ++i) {
      apL_[i].reset();
      apR_[i].reset();
    }
    for (int i = 0; i < 6; ++i) {
      springApL_[i].reset();
      springApR_[i].reset();
    }
    duckEnv_ = 0.0f;
    modPhase_ = 0.0f;
    mix_.snap();
    widthS_.snap();
    duckS_.snap();
    preS_.snap();
  }

private:
  struct Line {
    std::vector<float> buf;
    int write = 0;

    float read(float delaySamples) const noexcept {
      const int size = static_cast<int>(buf.size());
      float pos = static_cast<float>(write) - delaySamples;
      while (pos < 0.0f) {
        pos += static_cast<float>(size);
      }
      const int i0 = static_cast<int>(pos) % size;
      const int i1 = (i0 + 1) % size;
      const float frac = pos - std::floor(pos);
      return buf[static_cast<size_t>(i0)] * (1.0f - frac) +
             buf[static_cast<size_t>(i1)] * frac;
    }

    void push(float x) noexcept {
      buf[static_cast<size_t>(write)] = x;
      write = (write + 1) % static_cast<int>(buf.size());
    }
  };

  struct Allpass {
    std::vector<float> buf;
    int write = 0;

    void prepare(int size) noexcept {
      buf.assign(static_cast<size_t>(std::max(2, size)), 0.0f);
      write = 0;
    }

    float process(float x, float g) noexcept {
      const int size = static_cast<int>(buf.size());
      const float delayed = buf[static_cast<size_t>(write)];
      const float y = -g * x + delayed;
      buf[static_cast<size_t>(write)] = x + g * y;
      write = (write + 1) % size;
      return y;
    }

    void reset() noexcept {
      std::fill(buf.begin(), buf.end(), 0.0f);
      write = 0;
    }
  };

  static void hadamard(float* d) noexcept {
    for (int len = 1; len < 8; len <<= 1) {
      for (int i = 0; i < 8; i += len << 1) {
        for (int j = 0; j < len; ++j) {
          const float a = d[i + j];
          const float b = d[i + j + len];
          d[i + j] = a + b;
          d[i + j + len] = a - b;
        }
      }
    }
  }

  float sr_ = 48000.0f;
  int maxSamples_ = 14400;
  int preSize_ = 0;
  int preWrite_ = 0;
  Line lineL_[8], lineR_[8];
  Allpass apL_[4], apR_[4];
  Allpass springApL_[6], springApR_[6];
  std::vector<float> preL_, preR_;
  float len_[8] = {};
  float lenCur_[8] = {};
  float fb_[8] = {};
  float dampL_[8] = {};
  float dampR_[8] = {};
  float dampG_ = 0.3f;
  float diffusion_ = 0.5f;
  float modPhase_ = 0.0f;
  float modInc_ = 0.0f;
  float duckEnv_ = 0.0f;
  float inputGain_ = 0.22f;
  bool freeze_ = false;
  hdsp::Smoothed mix_;
  hdsp::Smoothed widthS_;
  hdsp::Smoothed duckS_;
  hdsp::Smoothed preS_;
  bool on_ = false;
  int mode_ = 0;
};

} // namespace space
