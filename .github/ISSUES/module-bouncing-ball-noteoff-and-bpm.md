Title: module_bouncing_ball_mode - always schedule Note Off and guard BPM zero

Affected files:
- src/module_bouncing_ball_mode.cpp

Description
-----------
In `module_bouncing_ball_mode.cpp`, the code can send a Note On without scheduling a corresponding Note Off if all `scheduledNotes` slots are active, and it divides by `sharedBPM` possibly causing division by zero when `sharedBPM == 0`.

Suggested fixes
---------------
- Guard `sharedBPM`: if `sharedBPM == 0` clamp to a minimum (e.g., 1) or early-return, so durations are calculable without divide-by-zero.
- When scheduling a new note, if no inactive `scheduledNotes` slot is found, overwrite the slot with the earliest `offTime` (oldest) or immediately schedule a Note Off after the intended duration to ensure every Note On has a Note Off.

Severity: Medium

Notes
-----
This ensures no stuck notes and avoids runtime crashes from divide-by-zero.