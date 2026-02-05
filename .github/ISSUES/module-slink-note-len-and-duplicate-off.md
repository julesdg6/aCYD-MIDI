Title: module_slink_mode - validate note_len range and remove duplicate processVoiceNoteOffs

Affected files:
- src/module_slink_mode.cpp

Description
-----------
Two issues:

1. The button handlers that increment `slink_state.clock_engine.note_len_min` and `note_len_max` can produce an inverted range where `note_len_min > note_len_max`.
2. There is a duplicated call to `processVoiceNoteOffs()` in the update path, causing note-off processing to run twice per cycle.

Suggested fixes
---------------
- After updating `note_len_min` or `note_len_max`, validate the invariant: if `note_len_min > note_len_max` then set `note_len_max = note_len_min` (or clamp accordingly), and vice-versa for `note_len_max < note_len_min`.
- Remove the redundant `processVoiceNoteOffs()` call so it is only invoked once.

Severity: Medium

Notes
-----
Ensure the range check occurs before calling `requestRedraw()` so UI reflects a valid range.