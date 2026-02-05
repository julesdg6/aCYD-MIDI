Title: module_euclidean_mode - triplet timing helper usage and persistent triplet accumulator

Affected files:
- src/module_euclidean_mode.cpp

Description
-----------
Two maintainability/timing issues in `module_euclidean_mode.cpp`:

1. `getEuclideanStepIntervalTicks()` helper is defined but unused; either use it in `updateEuclideanSequencer()` to compute step interval (respects `euclideanState.tripletMode`) or remove it.
2. A `static` local `tripletAccumulator` retains state across mode re-initializations causing stale timing; move this accumulator into `euclideanState` (e.g., `tripletAccumulator`) and reset it in `initializeEuclideanMode()`.

Suggested fixes
---------------
- Replace hardcoded `CLOCK_TICKS_PER_SIXTEENTH` usage with `getEuclideanStepIntervalTicks()` inside the sequencer timing path, or delete the helper if not needed.
- Add `tripletAccumulator` to persistent mode state, and zero it in the mode initialization function.

Severity: Low/Medium

Notes
-----
Small refactor to improve correctness when switching modes or re-entering the Euclidean mode.