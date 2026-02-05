Title: module_lfo_mode - '+' button behavior when pitchWheelMode is active

Affected files:
- src/module_lfo_mode.cpp

Description
-----------
When the '+' button is pressed and `lfo.pitchWheelMode` is true, the handler currently returns early and does not perform the same transition logic as the '-' button. This causes inconsistent behavior between '+' and '-' when pitch wheel mode is active.

Suggested fix
-------------
In the '+' button handler (branch using `isButtonPressed(140, y, 25, 25)`), add the same logic that the '-' button uses when `lfo.pitchWheelMode` is true: set `lfo.pitchWheelMode = false`, set `lfo.ccTarget = 1`, call `requestRedraw()`, then return. Otherwise keep the existing increment behavior.

Severity: Low (UI inconsistency)

Notes
-----
Small behavioral fix to match user expectation.