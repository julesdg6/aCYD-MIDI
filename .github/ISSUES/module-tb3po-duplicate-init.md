Title: module_tb3po_mode - duplicate initialization calls

Affected files:
- src/module_tb3po_mode.cpp

Description
-----------
The initialization sequence currently calls `regenerateAll()` and sets `tb3po.useInternalClock = true` twice, leading to redundant work and potential side effects.

Suggested fix
-------------
Remove the duplicated initialization so each of the following is performed exactly once:
- `regenerateAll()`
- `tb3po.useInternalClock = true`
- `uClock.setOnStep(onTb3poStepISR, 1)` (keep once)

Severity: Low (performance/cleanliness)

Notes
-----
Simple cleanup to avoid wasted cycles.