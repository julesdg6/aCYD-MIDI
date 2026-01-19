# aCYD MIDI Controller

Touchscreen Bluetooth MIDI controller running on the ESP32-2432S028R (CYD) display. The firmware bundles 16 interactive modes, hardware/ BLE MIDI routing, and touchscreen-first controls tuned for live performance.

## Quick Start

1. Grab a pre-built firmware from the GitHub [releases page](https://github.com/julesdg6/aCYD-MIDI/releases).
2. Flash with `pio run -e esp32-2432S028Rv2 -t upload` (or the matching board when using 3248S035C/R).
3. Pair “CYD MIDI” via Bluetooth or plug in a DIN-5 breakout for hardware MIDI.
4. Use the touchscreen menu to open any mode; the UI resizes automatically thanks to the scaling helpers.

## Documentation

- `docs/README.md` explains the overall feature set, capture flow, and Wi-Fi/remote display notes.
- Implementation notes live in `docs/IMPLEMENTATION_SUMMARY.md`, `docs/SLINK_IMPLEMENTATION.md`, `docs/PERFORMANCE_OPTIMIZATIONS.md`, `docs/RTOS_IMPLEMENTATION_PLAN.md`, and the `docs/` concept files for hardware, MIDI clock, and memory guidance.
- `docs/BUILD_VERIFICATION.md` lists the verification steps when building for release or testing hardware revisions.
- `docs/DRAM_FIX_EXPLANATION.md` explains the longstanding SPIRAM workaround that keeps LVGL stable.

## Releases

- `docs/CHANGELOG.md` captures the public changelog for each version.
- `docs/RELEASE.md` documents the release process and checklist.
- `docs/RELEASE_SUMMARY.md` maps the recent release work and high-level impact.
- `docs/PR_SUMMARY.md` highlights the performance optimization branch that drives the current UI responsiveness.

## License

MIT – see `LICENSE` for full terms.
