Title: clock_manager - ISR safety, stop/start ordering, and tick handling

Affected files:
- src/clock_manager.cpp

Description
---------
Multiple issues in `clock_manager.cpp` can cause incorrect behavior or races:

- Unused local `now` variable in `clockManagerExternalContinue` (minor cleanup).
- stop-path updates `uClockRunning` before calling `uClock.stop()` leading to inconsistent state compared to start path.
- `uClock.resetCounters()` does not notify `updateClockManager()` to clear its static `lastProcessedTick`, leading to missed/duplicate ticks after counter reset.
- ISR callbacks (`onClockTickCallback`, `onClockStartCallback`, `onClockStopCallback`) perform non-ISR-safe operations (e.g., `Serial.println`, or use of non-ISR critical sections).
- `updateClockManager()`'s tick loop can miss or overrun ticks when counters wrap or after resets; `tickPending` set by the ISR is not reliably consumed.

Suggested fixes
---------------
- Remove unused `now` local.
- Ensure `uClock.stop()` is called while `uClockRunning` remains true; only flip `uClockRunning` to false after a successful stop (mirror start-path ordering).
- Introduce a `tickCountersReset` flag set when `uClock.resetCounters()` is called; have `updateClockManager()` clear it and reset `lastProcessedTick` under the clock manager lock.
- Convert any ISR callbacks to set volatile/atomic flags only; move `Serial.println` and other non-ISR work to `updateClockManager()` or main loop.
- Replace `lockClockManager()/unlockClockManager()` calls in ISR with ISR-safe critical entry/exit (e.g., portENTER_CRITICAL_ISR()/portEXIT_CRITICAL_ISR) or provide dedicated `lockClockManagerFromISR()` wrappers.
- In `updateClockManager()`, read `tickCount` and `tickPending` under the lock, detect counter wrap (if `tickCount < lastProcessedTick` treat as reset), and while `tickPending || lastProcessedTick < tickCount` increment `lastProcessedTick` and process ticks, clearing `tickPending` when consumed.

Severity: High (timing/state bugs may break MIDI clock behavior)

Notes
-----
This is a multi-part issue; propose splitting if you'd like to prioritize ISR-safety first, then state ordering and then counter-reset handling.