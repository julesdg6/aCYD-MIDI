Title: esp_now_midi_module - midiPacket race conditions, routing inconsistencies, and duplicate stray statements

Affected files:
- src/esp_now_midi_module.cpp

Description
-----------
There are several concurrency and logic issues in the ESP-NOW MIDI module:

1. A shared global `midiPacket` buffer is mutated in `onEspNowNoteOn` while other tasks/ISRs may concurrently access it, producing data races.
2. Routing behavior differs between clock/start/stop/continue handlers and note handlers — `onEspNowClock` guards forwarding by `midiClockMaster == CLOCK_ESP_NOW`, while start/stop/continue forward unconditionally; behavior should be consistent.
3. Duplicate lines and orphaned statements exist around lines ~241-249 causing syntax errors or unexpected output (stray Serial.printf / duplicated sendHardwareMIDI calls).

Suggested fixes
---------------
- Avoid mutating a shared midiPacket: create a local stack buffer (e.g., `uint8_t localPacket[5]`) in handlers and call `pCharacteristic->setValue(localPacket, len)`/`notify()` with that, or protect the shared buffer with a FreeRTOS mutex around write+notify.
- Decide on a consistent routing policy: either only forward when `midiClockMaster == CLOCK_ESP_NOW` for all handlers, or forward unconditionally for start/stop/continue; update all handlers accordingly.
- Remove duplicate and stray statements outside function scope; ensure the Note Off handler has a single call to `sendHardwareMIDI` and a single `Serial.printf` and that braces are balanced.

Severity: High (message corruption, crashes, and non-deterministic behavior possible)

Notes
-----
This issue can be split into smaller tasks: (A) remove midiPacket races, (B) reconcile routing policy, (C) remove duplicates/syntax errors.