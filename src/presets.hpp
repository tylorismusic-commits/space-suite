// Space Suite — presets.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>

#include "params.hpp"

namespace space {

struct PresetValue {
  int param;
  double value;
};

struct Preset {
  const char* category;
  const char* name;
  int page;  // 0 Reverb, 1 Delay, 2 Mod, for the list badge
  const PresetValue* values;
  int count;
};

#define PV(p, v) {static_cast<int>(p), (v)}

inline constexpr PresetValue kInitValues[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_INPUT, 0), PV(P_OUTPUT, 0),
    PV(P_MIX, 100), PV(P_REVERB_ON, 0), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kPlateVocal[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 0),
    PV(P_REVERB_SIZE, 45), PV(P_REVERB_DECAY, 2.2), PV(P_REVERB_PREDELAY, 20),
    PV(P_REVERB_TONE, 55), PV(P_REVERB_WIDTH, 65), PV(P_REVERB_DUCK, 45),
    PV(P_REVERB_MIX, 28), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kPlateDrum[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 0),
    PV(P_REVERB_SIZE, 35), PV(P_REVERB_DECAY, 1.2), PV(P_REVERB_PREDELAY, 10),
    PV(P_REVERB_TONE, 60), PV(P_REVERB_WIDTH, 50), PV(P_REVERB_DUCK, 25),
    PV(P_REVERB_MIX, 22), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kHallWide[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 2),
    PV(P_REVERB_SIZE, 70), PV(P_REVERB_DECAY, 4.5), PV(P_REVERB_PREDELAY, 35),
    PV(P_REVERB_TONE, 45), PV(P_REVERB_WIDTH, 80), PV(P_REVERB_DUCK, 20),
    PV(P_REVERB_MIX, 30), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kRoomTight[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 1),
    PV(P_REVERB_SIZE, 40), PV(P_REVERB_DECAY, 1.0), PV(P_REVERB_PREDELAY, 8),
    PV(P_REVERB_TONE, 55), PV(P_REVERB_WIDTH, 45), PV(P_REVERB_DUCK, 40),
    PV(P_REVERB_MIX, 20), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kSpringGuitar[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 3),
    PV(P_REVERB_SIZE, 55), PV(P_REVERB_DECAY, 2.0), PV(P_REVERB_PREDELAY, 0),
    PV(P_REVERB_TONE, 50), PV(P_REVERB_WIDTH, 60), PV(P_REVERB_DUCK, 10),
    PV(P_REVERB_MIX, 30), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kSlapMono[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 0), PV(P_DELAY_ON, 1),
    PV(P_DELAY_MODE, 0), PV(P_DELAY_SYNC, 0), PV(P_DELAY_TIME, 90),
    PV(P_DELAY_FEEDBACK, 10), PV(P_DELAY_TONE, 55), PV(P_DELAY_MOD, 5),
    PV(P_DELAY_WIDTH, 0), PV(P_DELAY_DUCK, 0), PV(P_DELAY_MIX, 25),
    PV(P_MOD_ON, 0)};

inline constexpr PresetValue kTapeDotted[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 0), PV(P_DELAY_ON, 1),
    PV(P_DELAY_MODE, 1), PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 8),
    PV(P_DELAY_FEEDBACK, 35), PV(P_DELAY_TONE, 40), PV(P_DELAY_MOD, 35),
    PV(P_DELAY_WIDTH, 60), PV(P_DELAY_DUCK, 35), PV(P_DELAY_MIX, 30),
    PV(P_MOD_ON, 0)};

inline constexpr PresetValue kAnalogDark[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 0), PV(P_DELAY_ON, 1),
    PV(P_DELAY_MODE, 2), PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 2),
    PV(P_DELAY_FEEDBACK, 40), PV(P_DELAY_TONE, 30), PV(P_DELAY_MOD, 20),
    PV(P_DELAY_WIDTH, 50), PV(P_DELAY_DUCK, 20), PV(P_DELAY_MIX, 30),
    PV(P_MOD_ON, 0)};

inline constexpr PresetValue kPingPongWide[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 0), PV(P_DELAY_ON, 1),
    PV(P_DELAY_MODE, 3), PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 3),
    PV(P_DELAY_FEEDBACK, 45), PV(P_DELAY_TONE, 55), PV(P_DELAY_MOD, 15),
    PV(P_DELAY_WIDTH, 100), PV(P_DELAY_DUCK, 20), PV(P_DELAY_MIX, 35),
    PV(P_MOD_ON, 0)};

inline constexpr PresetValue kChorusWide[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_MOD_ON, 1), PV(P_MOD_MODE, 0),
    PV(P_MOD_RATE, 0.35), PV(P_MOD_DEPTH, 55), PV(P_MOD_FEEDBACK, 0),
    PV(P_MOD_WIDTH, 85), PV(P_MOD_MIX, 40), PV(P_REVERB_ON, 0),
    PV(P_DELAY_ON, 0)};

inline constexpr PresetValue kFlangerJet[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_MOD_ON, 1), PV(P_MOD_MODE, 1),
    PV(P_MOD_RATE, 0.2), PV(P_MOD_DEPTH, 65), PV(P_MOD_FEEDBACK, 55),
    PV(P_MOD_WIDTH, 60), PV(P_MOD_MIX, 45), PV(P_REVERB_ON, 0),
    PV(P_DELAY_ON, 0)};

inline constexpr PresetValue kPhaserSlow[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_MOD_ON, 1), PV(P_MOD_MODE, 2),
    PV(P_MOD_RATE, 0.12), PV(P_MOD_DEPTH, 70), PV(P_MOD_FEEDBACK, 35),
    PV(P_MOD_WIDTH, 70), PV(P_MOD_MIX, 40), PV(P_REVERB_ON, 0),
    PV(P_DELAY_ON, 0)};

inline constexpr PresetValue kDimensionSubtle[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_MOD_ON, 1), PV(P_MOD_MODE, 3),
    PV(P_MOD_RATE, 0.25), PV(P_MOD_DEPTH, 40), PV(P_MOD_FEEDBACK, 0),
    PV(P_MOD_WIDTH, 80), PV(P_MOD_MIX, 35), PV(P_REVERB_ON, 0),
    PV(P_DELAY_ON, 0)};

inline constexpr PresetValue kBloom[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 2),
    PV(P_REVERB_SIZE, 75), PV(P_REVERB_DECAY, 6.0), PV(P_REVERB_PREDELAY, 40),
    PV(P_REVERB_TONE, 45), PV(P_REVERB_WIDTH, 80), PV(P_REVERB_DUCK, 0),
    PV(P_REVERB_MIX, 35), PV(P_DELAY_ON, 1), PV(P_DELAY_MODE, 1),
    PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 8), PV(P_DELAY_FEEDBACK, 40),
    PV(P_DELAY_TONE, 40), PV(P_DELAY_MOD, 30), PV(P_DELAY_WIDTH, 60),
    PV(P_DELAY_DUCK, 20), PV(P_DELAY_MIX, 25), PV(P_MOD_ON, 1),
    PV(P_MOD_MODE, 0), PV(P_MOD_RATE, 0.2), PV(P_MOD_DEPTH, 50),
    PV(P_MOD_WIDTH, 90), PV(P_MOD_MIX, 30)};

inline constexpr PresetValue kInfinite[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 2),
    PV(P_REVERB_SIZE, 80), PV(P_REVERB_DECAY, 9.0), PV(P_REVERB_PREDELAY, 20),
    PV(P_REVERB_TONE, 50), PV(P_REVERB_WIDTH, 85), PV(P_REVERB_DUCK, 0),
    PV(P_REVERB_MIX, 45), PV(P_REVERB_FREEZE, 0), PV(P_DELAY_ON, 0),
    PV(P_MOD_ON, 1), PV(P_MOD_MODE, 3), PV(P_MOD_RATE, 0.1),
    PV(P_MOD_DEPTH, 30), PV(P_MOD_WIDTH, 100), PV(P_MOD_MIX, 25)};

inline constexpr PresetValue kSpaceGhost[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_INPUT, 0), PV(P_OUTPUT, 0),
    PV(P_MIX, 100), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 0),
    PV(P_REVERB_SIZE, 55), PV(P_REVERB_DECAY, 3.5), PV(P_REVERB_PREDELAY, 30),
    PV(P_REVERB_TONE, 65), PV(P_REVERB_WIDTH, 85), PV(P_REVERB_DUCK, 55),
    PV(P_REVERB_MIX, 35), PV(P_DELAY_ON, 1), PV(P_DELAY_MODE, 1),
    PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 8), PV(P_DELAY_FEEDBACK, 30),
    PV(P_DELAY_TONE, 45), PV(P_DELAY_MOD, 25), PV(P_DELAY_WIDTH, 80),
    PV(P_DELAY_DUCK, 60), PV(P_DELAY_MIX, 18), PV(P_MOD_ON, 1),
    PV(P_MOD_MODE, 3), PV(P_MOD_RATE, 0.18), PV(P_MOD_DEPTH, 35),
    PV(P_MOD_FEEDBACK, 0), PV(P_MOD_WIDTH, 95), PV(P_MOD_MIX, 30)};

inline constexpr PresetValue kVocalSpace[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 0),
    PV(P_REVERB_SIZE, 40), PV(P_REVERB_DECAY, 2.5), PV(P_REVERB_PREDELAY, 25),
    PV(P_REVERB_TONE, 55), PV(P_REVERB_WIDTH, 60), PV(P_REVERB_DUCK, 60),
    PV(P_REVERB_MIX, 22), PV(P_DELAY_ON, 1), PV(P_DELAY_MODE, 1),
    PV(P_DELAY_SYNC, 1), PV(P_DELAY_DIV, 8), PV(P_DELAY_FEEDBACK, 25),
    PV(P_DELAY_TONE, 45), PV(P_DELAY_MOD, 20), PV(P_DELAY_WIDTH, 50),
    PV(P_DELAY_DUCK, 50), PV(P_DELAY_MIX, 18), PV(P_MOD_ON, 0)};

inline constexpr PresetValue kDrumRoom[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 1),
    PV(P_REVERB_SIZE, 30), PV(P_REVERB_DECAY, 0.9), PV(P_REVERB_PREDELAY, 5),
    PV(P_REVERB_TONE, 60), PV(P_REVERB_WIDTH, 45), PV(P_REVERB_DUCK, 20),
    PV(P_REVERB_MIX, 18), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 1),
    PV(P_MOD_MODE, 3), PV(P_MOD_RATE, 0.15), PV(P_MOD_DEPTH, 25),
    PV(P_MOD_WIDTH, 60), PV(P_MOD_MIX, 15)};

inline constexpr PresetValue kMixBusAir[] = {
    PV(P_POWER, 1), PV(P_BYPASS, 0), PV(P_REVERB_ON, 1), PV(P_REVERB_MODE, 0),
    PV(P_REVERB_SIZE, 30), PV(P_REVERB_DECAY, 1.5), PV(P_REVERB_PREDELAY, 10),
    PV(P_REVERB_TONE, 65), PV(P_REVERB_WIDTH, 50), PV(P_REVERB_DUCK, 50),
    PV(P_REVERB_MIX, 8), PV(P_DELAY_ON, 0), PV(P_MOD_ON, 1),
    PV(P_MOD_MODE, 3), PV(P_MOD_RATE, 0.1), PV(P_MOD_DEPTH, 20),
    PV(P_MOD_WIDTH, 90), PV(P_MOD_MIX, 15)};

#undef PV

inline constexpr Preset kPresets[] = {
    {"Init", "Init", 0, kInitValues, static_cast<int>(sizeof(kInitValues) / sizeof(kInitValues[0]))},
    {"Reverb", "Plate — Vocal", 0, kPlateVocal, static_cast<int>(sizeof(kPlateVocal) / sizeof(kPlateVocal[0]))},
    {"Reverb", "Plate — Drum Room", 0, kPlateDrum, static_cast<int>(sizeof(kPlateDrum) / sizeof(kPlateDrum[0]))},
    {"Reverb", "Hall — Wide", 0, kHallWide, static_cast<int>(sizeof(kHallWide) / sizeof(kHallWide[0]))},
    {"Reverb", "Room — Tight", 0, kRoomTight, static_cast<int>(sizeof(kRoomTight) / sizeof(kRoomTight[0]))},
    {"Reverb", "Spring — Guitar", 0, kSpringGuitar, static_cast<int>(sizeof(kSpringGuitar) / sizeof(kSpringGuitar[0]))},
    {"Delay", "Slap — Mono", 1, kSlapMono, static_cast<int>(sizeof(kSlapMono) / sizeof(kSlapMono[0]))},
    {"Delay", "Tape — Dotted 1/8", 1, kTapeDotted, static_cast<int>(sizeof(kTapeDotted) / sizeof(kTapeDotted[0]))},
    {"Delay", "Analog — Dark", 1, kAnalogDark, static_cast<int>(sizeof(kAnalogDark) / sizeof(kAnalogDark[0]))},
    {"Delay", "Ping-Pong — Wide", 1, kPingPongWide, static_cast<int>(sizeof(kPingPongWide) / sizeof(kPingPongWide[0]))},
    {"Mod", "Chorus — Wide", 2, kChorusWide, static_cast<int>(sizeof(kChorusWide) / sizeof(kChorusWide[0]))},
    {"Mod", "Flanger — Jet", 2, kFlangerJet, static_cast<int>(sizeof(kFlangerJet) / sizeof(kFlangerJet[0]))},
    {"Mod", "Phaser — Slow", 2, kPhaserSlow, static_cast<int>(sizeof(kPhaserSlow) / sizeof(kPhaserSlow[0]))},
    {"Mod", "Dimension — Subtle", 2, kDimensionSubtle, static_cast<int>(sizeof(kDimensionSubtle) / sizeof(kDimensionSubtle[0]))},
    {"Ambient", "Bloom", 0, kBloom, static_cast<int>(sizeof(kBloom) / sizeof(kBloom[0]))},
    {"Ambient", "Infinite", 0, kInfinite, static_cast<int>(sizeof(kInfinite) / sizeof(kInfinite[0]))},
    {"Vocal", "Space Ghost", 0, kSpaceGhost, static_cast<int>(sizeof(kSpaceGhost) / sizeof(kSpaceGhost[0]))},
    {"Vocal", "Vocal Space", 0, kVocalSpace, static_cast<int>(sizeof(kVocalSpace) / sizeof(kVocalSpace[0]))},
    {"Drums", "Drum Room", 0, kDrumRoom, static_cast<int>(sizeof(kDrumRoom) / sizeof(kDrumRoom[0]))},
    {"Mix Bus", "Mix Bus Air", 2, kMixBusAir, static_cast<int>(sizeof(kMixBusAir) / sizeof(kMixBusAir[0]))},
};

inline constexpr int kPresetCount = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));

} // namespace space
