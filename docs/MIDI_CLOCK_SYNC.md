# Shared MIDI Clock Reference

## Motivation
The sequencer/timing issue tracked in [issue #62](https://github.com/julesdg6/aCYD-MIDI/issues/62) showed that every mode was trying to keep pace with the clock by polling `SequencerSyncState::readyForStep()`, but the underlying `tickCount` could jump multiple 6‑tick (16th note) intervals while the UI spent time redrawing. When that happened, the modulo check missed the intermediate boundaries and the sequencers fell out of sync.

## Clock Manager Responsibilities
- `midiClockTask()` runs on its own FreeRTOS task and calls `updateClockManager()` every millisecond (`src/midi_clock_task.cpp:1-24`).
- `updateClockManager()` stays locked to the MIDI-spec 24 pulses per quarter note (`CLOCK_TICKS_PER_QUARTER`) and only increments `tickCount` when the sequencer is interested, sending `sendMIDIClock()` and redrawing as needed (`src/clock_manager.cpp:105-124`).
- The clock still tracks internal/external masters, start/stop state, and external clock pulses via `clockManagerSequencerStarted/Stopped/External*` helpers, so the global timing model remains centralised.

## Sequencer Sync API
- `SequencerSyncState` now exposes `consumeReadySteps(stepIntervalTicks)` (`include/clock_manager.h:95-119`), which returns how many `stepIntervalTicks` have elapsed since the last call and advances `lastTick` accordingly.
- Every module that previously called `readyForStep()` now loops over the returned `readySteps` so it plays the appropriate number of steps even when multiple 16ths have occurred between updates.
- A convenience `readyForStep()` wrapper remains for compatibility, but the new API is what each mode should use moving forward.

## Module Integration
| Module | Interval | Notes |
| --- | --- | --- |
| BEATS / Sequencer (`src/module_sequencer_mode.cpp`) | 1x 16th | Uses `consumeReadySteps()` to play every pending step and advances the grid/BPM display once per redraw.
| TB3PO (`src/module_tb3po_mode.cpp`) | 1x 16th | Logs every tick for diagnostics when debugging and gates notes according to the shared clock stream.
| GRIDS (`src/module_grids_mode.cpp`) | 1x 16th | Triggers each drum voice through the shared ticks and keeps the visual step indicator aligned.
| RNG (`src/module_random_generator_mode.cpp`) | Variable (1/4, 1/8, 1/16) | Converts the requested subdivision into tick counts via `getRandomStepIntervalTicks()` and drains every ready step.
| EUCLIDEAN (`src/module_euclidean_mode.cpp`) | 1x 16th or triplet | Uses the `tripletMode` helper to set the proper interval and releases notes after each ready step.
| ARPEGGIATOR (`src/module_arpeggiator_mode.cpp`) | Free-form (driven by `arp.stepTicks`) | Adds the `readySteps` to the existing tick accumulator so the note playback stays synced to the shared clock.
| RAGA (`src/module_raga_mode.cpp`) | 1x 16th | Drains ticks before scheduling the next tala note, ensuring the drone + phrase stay in time.

## Testing
1. `pio run -e esp32-2432S028Rv2` (default build) ensures the refactor compiles cleanly.
2. Run multiple sequencer modules back-to-back while capturing serial logs (`DEBUG_ENABLED=true`) to verify that `consumeReadySteps()` reports multi-tick delivery only when the UI is busy.
3. Send MIDI Clock/BLE/Hardware clock at 24 ppqn and observe that each module now plays exactly at the shared 16th boundary even when drawing heavy screens.

## Future Work
- Consider forwarding the shape of the upcoming ticks to a generic listener so UI/LED indicators can subscribe without polling `clockManagerHasTickAdvanced()` repeatedly.
- Add regression tests for `SequencerSyncState` to assert that `consumeReadySteps()` never misses ticks under simulated jump scenarios.

