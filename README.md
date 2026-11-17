# TÌR · CYLCH · GEALACH · AWEN — Carnac v0.1 integration slice

**Upstream commit:** `98331670f1e5ea664546746d505b7502332b3e8e` (Carnac OS). X0X submodule pinned at `5048230b86254581d991121916ede3de0d0c91f6`.

This first branch adds a **standalone, host-tested control layer**, not a playable or flashable firmware. Nothing in the normal Carnac UI, DSP, MIDI parser or save path invokes it yet. No audio output changes should occur from these files alone.

- `src/dsp/ritual.[ch]` — portable four-macro control state, cyclic season and moon interpolation, effective-value offsets, smoothing, fixed MIDI CC20–23 parser, versioned 16-byte persistence extension.
- `src/dsp/ritual_carnac_adapter.h` — checked conversion between Carnac `EngineState`, `MasterPerformancePreset` and ritual state. **Does not write to stored EngineState.**
- `src/host/test_ritual.c`, `src/host/test_ritual_adapter.c` — test core and real Carnac data layout.
- `scripts/ritual/test_ritual.sh` — run tests without changing upstream test runner.

**Local test:** `bash scripts/ritual/test_ritual.sh` from repository root. Requires a C11 compiler. Portable core tests previously passed 878 assertions with GCC and sanitizers, but real-upstream adapter and full project builds are **unverified** until the script is run in a Carnac checkout.

**Safety:** This state format tentatively uses `MasterPerformancePreset.reserved[0..7]`. Although the upstream header exposes these bytes, storage compatibility and conflicts must be checked before connecting the save path. Unknown version tags must be left untouched. Source-compatible integration needs explicit audio ownership / synchronization when MIDI or UI changes state. Do not flash to hardware.

**Next PR:** Add a performance page, guarded routed MIDI CC input, coherent audio-control snapshot and effective DSP overlays; test voice stability, preset roundtrips, and render regression before building hardware firmware.

See the parent project's licensing and attribution documentation. Added files: GPL-3.0-only.
