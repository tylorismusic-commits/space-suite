// Space Suite — LV2 plugin
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Control ports mirror the CLAP parameter ids: port = kPortFirstParam + ParamId.

#include <lv2/core/lv2.h>

#include <cmath>
#include <cstring>
#include <memory>

#include "dsp/engine.hpp"
#include "params.hpp"

namespace {

constexpr const char* kPluginUri = "https://github.com/tylorismusic-commits/space-suite";

enum Port : uint32_t {
  PORT_AUDIO_IN_L = 0,
  PORT_AUDIO_IN_R = 1,
  PORT_AUDIO_OUT_L = 2,
  PORT_AUDIO_OUT_R = 3,
  PORT_FIRST_PARAM = 4,
  PORT_IN_PEAK_L = PORT_FIRST_PARAM + space::P_COUNT,
  PORT_IN_PEAK_R,
  PORT_OUT_PEAK_L,
  PORT_OUT_PEAK_R,
  PORT_IN_PEAK,
  PORT_OUT_PEAK,
  PORT_LATENCY,
  PORT_COUNT
};

struct Plugin {
  float sampleRate = 48000.0f;
  float lastPort[space::P_COUNT] = {};
  bool portsSeen = false;
  space::Shared shared;
  space::Engine engine;
  space::Meters meters;
  std::unique_ptr<float*[]> ports;

  float* audioInL = nullptr;
  float* audioInR = nullptr;
  float* audioOutL = nullptr;
  float* audioOutR = nullptr;
  float* inPeakL = nullptr;
  float* inPeakR = nullptr;
  float* outPeakL = nullptr;
  float* outPeakR = nullptr;
  float* inPeak = nullptr;
  float* outPeak = nullptr;
  float* latency = nullptr;
};

LV2_Handle instantiate(const LV2_Descriptor*, double rate, const char*,
                       const LV2_Feature* const*) {
  auto* p = new Plugin();
  p->sampleRate = static_cast<float>(rate);
  p->engine.prepare(p->sampleRate);
  p->engine.setControls(space::controlsFrom(p->shared), 0.0f, false);
  return p;
}

void connectPort(LV2_Handle handle, uint32_t port, void* data) {
  auto* p = static_cast<Plugin*>(handle);
  switch (port) {
    case PORT_AUDIO_IN_L: p->audioInL = static_cast<float*>(data); break;
    case PORT_AUDIO_IN_R: p->audioInR = static_cast<float*>(data); break;
    case PORT_AUDIO_OUT_L: p->audioOutL = static_cast<float*>(data); break;
    case PORT_AUDIO_OUT_R: p->audioOutR = static_cast<float*>(data); break;
    case PORT_IN_PEAK_L: p->inPeakL = static_cast<float*>(data); break;
    case PORT_IN_PEAK_R: p->inPeakR = static_cast<float*>(data); break;
    case PORT_OUT_PEAK_L: p->outPeakL = static_cast<float*>(data); break;
    case PORT_OUT_PEAK_R: p->outPeakR = static_cast<float*>(data); break;
    case PORT_IN_PEAK: p->inPeak = static_cast<float*>(data); break;
    case PORT_OUT_PEAK: p->outPeak = static_cast<float*>(data); break;
    case PORT_LATENCY: p->latency = static_cast<float*>(data); break;
    default:
      if (!p->ports) {
        p->ports = std::make_unique<float*[]>(PORT_COUNT);
        std::memset(p->ports.get(), 0, sizeof(float*) * PORT_COUNT);
      }
      if (port < PORT_COUNT) p->ports[port] = static_cast<float*>(data);
      break;
  }
}

void activate(LV2_Handle handle) {
  auto* p = static_cast<Plugin*>(handle);
  p->engine.reset();
  if (p->latency) *p->latency = 0.0f;
}

void deactivate(LV2_Handle) {}

void run(LV2_Handle handle, uint32_t frames) {
  auto* p = static_cast<Plugin*>(handle);
  if (!p->audioInL || !p->audioOutL) return;
  if (!p->audioInR) p->audioInR = p->audioInL;
  if (!p->audioOutR) p->audioOutR = p->audioOutL;

  if (p->ports) {
    for (uint32_t i = 0; i < space::P_COUNT; ++i) {
      float* v = p->ports[PORT_FIRST_PARAM + i];
      if (!v) continue;
      if (!p->portsSeen || *v != p->lastPort[i]) {
        p->shared.setFromHost(i, static_cast<double>(*v));
        p->lastPort[i] = *v;
      }
    }
    p->portsSeen = true;
  }

  p->engine.setControls(space::controlsFrom(p->shared), 0.0f, false);

  if (p->audioOutL != p->audioInL) {
    std::memcpy(p->audioOutL, p->audioInL, frames * sizeof(float));
  }
  if (p->audioOutR != p->audioInR) {
    std::memcpy(p->audioOutR, p->audioInR, frames * sizeof(float));
  }

  p->engine.process(p->audioOutL, p->audioOutR, static_cast<int>(frames),
                    p->meters);

  if (p->inPeakL) *p->inPeakL = p->meters.inPeakLDb;
  if (p->inPeakR) *p->inPeakR = p->meters.inPeakRDb;
  if (p->outPeakL) *p->outPeakL = p->meters.outPeakLDb;
  if (p->outPeakR) *p->outPeakR = p->meters.outPeakRDb;
  if (p->inPeak) *p->inPeak = p->meters.inPeakDb;
  if (p->outPeak) *p->outPeak = p->meters.outPeakDb;
  if (p->latency) *p->latency = 0.0f;
}

void cleanup(LV2_Handle handle) { delete static_cast<Plugin*>(handle); }

const LV2_Descriptor kDescriptor = {
    kPluginUri, instantiate, connectPort, activate, run, deactivate,
    cleanup, nullptr};

} // namespace

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  return index == 0 ? &kDescriptor : nullptr;
}
