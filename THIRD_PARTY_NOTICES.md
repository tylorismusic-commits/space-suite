# Third-party notices

Space Suite is GPL-3.0-or-later.

## Included in this repository

| Component | Licence | Use |
|---|---|---|
| **CLAP** headers (`third_party/clap`) | MIT | Plugin API |
| **Jost** ExtraBold (`assets/fonts/Jost-ExtraBold.ttf`) | SIL Open Font License 1.1 | Panel typography; `assets/fonts/OFL.txt` |
| **hearth-dsp** (`third_party/hearth-dsp`) | GPL-3.0-or-later | Filters, smoothers, envelopes, oversampling, spectrum |

## Adapted from our own GPL-3.0 code

| Component | Use |
|---|---|
| **Comp Suite** | CLAP/LV2 wrappers, parameter/state/preset plumbing, panel chrome, tests, TTL generator |
| **Vocal Suite** `Space` | Reverb FDN, plate/room/hall tables, ducking, width |
| **Vocal Suite** `Echo` | Ducking/width/feedback concepts |
| **Amp Suite** `Delay` | Stereo delay, fractional read, feedback tone, ping-pong |
| **Adhesive** `Tape` | Tape wow/flutter and saturation reference for Tape delay |

## Reference-only (not vendored in 0.1.0)

Guitarix, DaisySP, Soundpipe, Airwindows, LSP and Surge XT were consulted as
open-source references for modulation and reverb behaviour. No code from these
projects is included in the 0.1.0 build. If a module is vendored later, its
original copyright and licence header will be preserved here.

## Not shipped

No commercial presets, impulse responses, artwork, trade dress or proprietary
algorithms. Real product names in documentation are descriptive only.
