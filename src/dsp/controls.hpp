// Space Suite — controls.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace space {

// Physical-unit controls.  The UI stores the same values in Shared, so the
// host parameter text, the panel and the DSP do not need three mappings.
struct Controls {
  // Global
  bool power = true;
  bool bypass = false;
  float inputDb = 0.0f;
  float outputDb = 0.0f;
  float mix = 1.0f;            // 0 = dry, 1 = fully processed
  float bpm = 120.0f;          // fallback when the host has no transport

  // Reverb
  bool reverbOn = true;
  int reverbMode = 0;          // 0 Plate, 1 Room, 2 Hall, 3 Spring
  float reverbSize = 50.0f;    // %
  float reverbDecay = 2.4f;    // s
  float reverbPreDelay = 25.0f;// ms
  float reverbTone = 50.0f;    // %
  float reverbWidth = 70.0f;   // %
  float reverbDuck = 30.0f;    // %
  float reverbMix = 30.0f;     // %
  bool reverbFreeze = false;

  // Delay
  bool delayOn = true;
  int delayMode = 0;           // 0 Digital, 1 Tape, 2 Analog, 3 Ping-Pong
  bool delaySync = true;
  int delayDiv = 3;            // 0 1/1 .. 8 1/8
  float delayTimeMs = 375.0f;  // free mode
  float delayFeedback = 35.0f; // %
  float delayTone = 50.0f;     // %
  float delayMod = 15.0f;      // %
  float delayWidth = 50.0f;    // %
  float delayDuck = 30.0f;     // %
  float delayMix = 25.0f;      // %

  // Modulation
  bool modOn = false;
  int modMode = 0;             // 0 Chorus, 1 Flanger, 2 Phaser, 3 Dimension
  float modRate = 0.35f;       // Hz
  float modDepth = 50.0f;      // %
  float modFeedback = 20.0f;   // %
  float modWidth = 70.0f;      // %
  float modMix = 40.0f;        // %
};

struct Meters {
  float inPeakDb = -120.0f;
  float outPeakDb = -120.0f;
  float inPeakLDb = -120.0f;
  float inPeakRDb = -120.0f;
  float outPeakLDb = -120.0f;
  float outPeakRDb = -120.0f;
  float bpm = 120.0f;
  bool hostTempo = false;
};

} // namespace space
