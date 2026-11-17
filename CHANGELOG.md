# Changelog

## 0.2.0 — 2026-11-17

- Delay host sync: CLAP transport tempo and LV2 `time:Position` are read; the
  BPM control is hidden while host sync is active and manual BPM remains the
  fallback.
- Spring reverb refined with a six-stage short dispersive allpass tail, stronger
  damping and deeper size/line movement.
- Fixed LV2 unit metadata for percent, dB, Hz, ms and BPM.
- Tests: host tempo override and spring tail decay.

## 0.1.0 — 2026-11-16

- First complete Space Suite build.
- CLAP + LV2 bundles, 1040×640 Comp Suite-style panel.
- Reverb: Plate, Room, Hall, Spring; size, decay, pre-delay, tone, width, duck,
  mix, freeze.
- Delay: Digital, Tape, Analog, Ping-Pong; host sync, free time, feedback,
  tone, mod, width, duck, mix.
- Modulation: Chorus, Flanger, Phaser, Dimension; rate, depth, feedback,
  width, mix.
- Global input/output/mix/bypass and chain enable strip.
- Factory presets, including **Space Ghost**.
- Typed value entry, double-click reset, preset browser.
- DSP, panel, layout, geometry and TTL tests.
