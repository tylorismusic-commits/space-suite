# Changelog

## 0.2.0 — 2026-11-18

### Preset handling

- User presets: save, duplicate, recall, rename, delete and export to
  `~/Documents/Space Suite Presets`.
- Drag a `.space-suite` file onto the panel to import it; collisions get a
  `(2)` suffix.
- Rename conflicts show inline feedback; overwriting a different existing
  preset asks for a second Enter.
- Presets recall the editor page they were saved on.
- Dirty state follows the live parameters, including host automation.
- The browser rescans on open and while open.
- Fixed hidden preset rows stealing clicks intended for the footer.

### Audit

- Added CLAP and LV2 load/process/state host tests.
- Added a format/parse round-trip test for every control.

### 0.2.0 — 2026-11-17

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
