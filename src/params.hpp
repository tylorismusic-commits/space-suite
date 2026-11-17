// Space Suite — params.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

#include "dsp/controls.hpp"

namespace space {

// Parameter ids are the CLAP parameter ids, the index into Shared::values and
// the ordering of the LV2 control ports (port = 4 + id).  Append only.
enum ParamId : uint32_t {
  P_POWER = 0,
  P_BYPASS,
  P_INPUT,
  P_OUTPUT,
  P_MIX,

  P_REVERB_ON,
  P_REVERB_MODE,
  P_REVERB_SIZE,
  P_REVERB_DECAY,
  P_REVERB_PREDELAY,
  P_REVERB_TONE,
  P_REVERB_WIDTH,
  P_REVERB_DUCK,
  P_REVERB_MIX,
  P_REVERB_FREEZE,

  P_DELAY_ON,
  P_DELAY_MODE,
  P_DELAY_SYNC,
  P_DELAY_DIV,
  P_DELAY_TIME,
  P_DELAY_FEEDBACK,
  P_DELAY_TONE,
  P_DELAY_MOD,
  P_DELAY_WIDTH,
  P_DELAY_DUCK,
  P_DELAY_MIX,

  P_MOD_ON,
  P_MOD_MODE,
  P_MOD_RATE,
  P_MOD_DEPTH,
  P_MOD_FEEDBACK,
  P_MOD_WIDTH,
  P_MOD_MIX,

  P_BPM,
  P_COUNT
};

struct Labels {
  const char* const* names = nullptr;
  int count = 0;
};

template <size_t N>
constexpr Labels labels(const char* const (&arr)[N]) noexcept {
  return Labels{arr, static_cast<int>(N)};
}

// One table drives the DSP, CLAP parameters, the editor and LV2 metadata.
struct ParamSpec {
  const char* symbol;
  const char* name;
  const char* module;
  double min;
  double max;
  double def;
  bool stepped;
  bool hidden;
  bool toggled;
  const char* unit;
  Labels labels;
};

inline constexpr const char* kReverbModeLabels[] = {"Plate", "Room", "Hall",
                                                    "Spring"};
inline constexpr const char* kDelayModeLabels[] = {"Digital", "Tape", "Analog",
                                                   "Ping-Pong"};
inline constexpr const char* kModModeLabels[] = {"Chorus", "Flanger", "Phaser",
                                                 "Dimension"};
inline constexpr const char* kDivLabels[] = {"1/1", "1/2", "1/4", "1/8",
                                            "1/8T", "1/16", "1/16T", "1/32",
                                            "1/8."};

inline constexpr ParamSpec kParamSpecs[P_COUNT] = {
    {"power", "Power", "Global", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"bypass", "Bypass", "Global", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"input", "Input", "Global", -18.0, 18.0, 0.0, false, false, false, "dB", {}},
    {"output", "Output", "Global", -18.0, 18.0, 0.0, false, false, false, "dB", {}},
    {"mix", "Mix", "Global", 0.0, 100.0, 100.0, false, false, false, "%", {}},

    {"reverb_on", "Reverb On", "Reverb", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"reverb_mode", "Reverb Mode", "Reverb", 0.0, 3.0, 0.0, true, false, false, nullptr,
     labels(kReverbModeLabels)},
    {"reverb_size", "Size", "Reverb", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"reverb_decay", "Decay", "Reverb", 0.3, 12.0, 2.4, false, false, false, "s", {}},
    {"reverb_predelay", "Pre-Delay", "Reverb", 0.0, 200.0, 25.0, false, false, false,
     "ms", {}},
    {"reverb_tone", "Tone", "Reverb", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"reverb_width", "Width", "Reverb", 0.0, 100.0, 70.0, false, false, false, "%", {}},
    {"reverb_duck", "Duck", "Reverb", 0.0, 100.0, 30.0, false, false, false, "%", {}},
    {"reverb_mix", "Reverb Mix", "Reverb", 0.0, 100.0, 30.0, false, false, false, "%", {}},
    {"reverb_freeze", "Freeze", "Reverb", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},

    {"delay_on", "Delay On", "Delay", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"delay_mode", "Delay Mode", "Delay", 0.0, 3.0, 0.0, true, false, false, nullptr,
     labels(kDelayModeLabels)},
    {"delay_sync", "Sync", "Delay", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"delay_div", "Division", "Delay", 0.0, 8.0, 3.0, true, false, false, nullptr,
     labels(kDivLabels)},
    {"delay_time", "Time", "Delay", 20.0, 2000.0, 375.0, false, false, false, "ms", {}},
    {"delay_feedback", "Feedback", "Delay", 0.0, 95.0, 35.0, false, false, false, "%", {}},
    {"delay_tone", "Tone", "Delay", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"delay_mod", "Mod", "Delay", 0.0, 100.0, 15.0, false, false, false, "%", {}},
    {"delay_width", "Width", "Delay", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"delay_duck", "Duck", "Delay", 0.0, 100.0, 30.0, false, false, false, "%", {}},
    {"delay_mix", "Delay Mix", "Delay", 0.0, 100.0, 25.0, false, false, false, "%", {}},

    {"mod_on", "Mod On", "Modulation", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"mod_mode", "Mod Mode", "Modulation", 0.0, 3.0, 0.0, true, false, false, nullptr,
     labels(kModModeLabels)},
    {"mod_rate", "Rate", "Modulation", 0.05, 10.0, 0.35, false, false, false, "Hz", {}},
    {"mod_depth", "Depth", "Modulation", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"mod_feedback", "Feedback", "Modulation", 0.0, 95.0, 20.0, false, false, false, "%", {}},
    {"mod_width", "Width", "Modulation", 0.0, 100.0, 70.0, false, false, false, "%", {}},
    {"mod_mix", "Mod Mix", "Modulation", 0.0, 100.0, 40.0, false, false, false, "%", {}},

    {"bpm", "BPM", "Global", 40.0, 240.0, 120.0, false, true, false, "BPM", {}},
};

static_assert(sizeof(kParamSpecs) / sizeof(kParamSpecs[0]) == P_COUNT,
              "kParamSpecs must cover every parameter id");

class Shared {
public:
  Shared() {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      values_[p].store(kParamSpecs[p].def, std::memory_order_relaxed);
      dirty_[p].store(false, std::memory_order_relaxed);
    }
    inPeakDb.store(-120.0f);
    outPeakDb.store(-120.0f);
    inPeakLDb.store(-120.0f);
    inPeakRDb.store(-120.0f);
    outPeakLDb.store(-120.0f);
    outPeakRDb.store(-120.0f);
    hostBpm.store(120.0f);
    hostTempo.store(false);
  }

  double get(uint32_t p) const noexcept {
    return values_[p].load(std::memory_order_relaxed);
  }

  void set(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) {
      return;
    }
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) {
      v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    }
    values_[p].store(v, std::memory_order_relaxed);
    dirty_[p].store(true, std::memory_order_release);
  }

  void setFromHost(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) {
      return;
    }
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) {
      v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    }
    values_[p].store(v, std::memory_order_relaxed);
  }

  bool takeDirty(uint32_t p) noexcept {
    return dirty_[p].exchange(false, std::memory_order_acquire);
  }

  bool isDirty(uint32_t p) const noexcept {
    return dirty_[p].load(std::memory_order_acquire);
  }

  bool anyDirty() const noexcept {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      if (dirty_[p].load(std::memory_order_acquire)) {
        return true;
      }
    }
    return false;
  }

  std::atomic<float> inPeakDb;
  std::atomic<float> outPeakDb;
  std::atomic<float> inPeakLDb;
  std::atomic<float> inPeakRDb;
  std::atomic<float> outPeakLDb;
  std::atomic<float> outPeakRDb;
  std::atomic<float> hostBpm;
  std::atomic<bool> hostTempo;

private:
  std::atomic<double> values_[P_COUNT];
  std::atomic<bool> dirty_[P_COUNT];
};

inline Controls controlsFrom(const Shared& s) noexcept {
  Controls c;
  c.power = s.get(P_POWER) > 0.5;
  c.bypass = s.get(P_BYPASS) > 0.5;
  c.inputDb = static_cast<float>(s.get(P_INPUT));
  c.outputDb = static_cast<float>(s.get(P_OUTPUT));
  c.mix = static_cast<float>(s.get(P_MIX) / 100.0);

  c.reverbOn = s.get(P_REVERB_ON) > 0.5;
  c.reverbMode = static_cast<int>(s.get(P_REVERB_MODE) + 0.5);
  c.reverbSize = static_cast<float>(s.get(P_REVERB_SIZE));
  c.reverbDecay = static_cast<float>(s.get(P_REVERB_DECAY));
  c.reverbPreDelay = static_cast<float>(s.get(P_REVERB_PREDELAY));
  c.reverbTone = static_cast<float>(s.get(P_REVERB_TONE));
  c.reverbWidth = static_cast<float>(s.get(P_REVERB_WIDTH));
  c.reverbDuck = static_cast<float>(s.get(P_REVERB_DUCK));
  c.reverbMix = static_cast<float>(s.get(P_REVERB_MIX));
  c.reverbFreeze = s.get(P_REVERB_FREEZE) > 0.5;

  c.delayOn = s.get(P_DELAY_ON) > 0.5;
  c.delayMode = static_cast<int>(s.get(P_DELAY_MODE) + 0.5);
  c.delaySync = s.get(P_DELAY_SYNC) > 0.5;
  c.delayDiv = static_cast<int>(s.get(P_DELAY_DIV) + 0.5);
  c.delayTimeMs = static_cast<float>(s.get(P_DELAY_TIME));
  c.delayFeedback = static_cast<float>(s.get(P_DELAY_FEEDBACK));
  c.delayTone = static_cast<float>(s.get(P_DELAY_TONE));
  c.delayMod = static_cast<float>(s.get(P_DELAY_MOD));
  c.delayWidth = static_cast<float>(s.get(P_DELAY_WIDTH));
  c.delayDuck = static_cast<float>(s.get(P_DELAY_DUCK));
  c.delayMix = static_cast<float>(s.get(P_DELAY_MIX));

  c.modOn = s.get(P_MOD_ON) > 0.5;
  c.modMode = static_cast<int>(s.get(P_MOD_MODE) + 0.5);
  c.modRate = static_cast<float>(s.get(P_MOD_RATE));
  c.modDepth = static_cast<float>(s.get(P_MOD_DEPTH));
  c.modFeedback = static_cast<float>(s.get(P_MOD_FEEDBACK));
  c.modWidth = static_cast<float>(s.get(P_MOD_WIDTH));
  c.modMix = static_cast<float>(s.get(P_MOD_MIX));

  c.bpm = static_cast<float>(s.get(P_BPM));
  return c;
}

} // namespace space
