Title: module_settings_mode - BPM row outer brace hides subsequent rows when scrolled out

Affected files:
- src/module_settings_mode.cpp

Description
-----------
In `module_settings_mode.cpp` the BPM row's outer `if` block beginning at the `bpmRowY` visibility check incorrectly wraps the rest of the UI rows. As a result, when the BPM row scrolls out of view, subsequent UI rows are not drawn.

Suggested fix
-------------
Remove the redundant outer opening brace tied to the initial `if (bpmRowY >= layout.viewTop && ...)` visibility check and keep only the inner visibility check that uses `labelY` and `bpmRowHeight()`. Remove the corresponding extra closing brace so subsequent rows render independently of the BPM row's visibility.

Severity: Low/Medium (UI layout bug)

Notes
-----
Careful edit to maintain indentation and ensure no extra braces remain.