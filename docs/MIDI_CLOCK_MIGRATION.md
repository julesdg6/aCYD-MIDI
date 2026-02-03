# Migration Guide: Hardware Timer MIDI Clock

## For Module Developers

If you're developing custom modules or modifying existing ones, follow this guide to ensure compatibility with the new hardware timer-based MIDI clock.

## Required Changes

### 1. BPM Updates

**Before:**
```cpp
sharedBPM = newValue;
requestRedraw();
```

**After:**
```cpp
setSharedBPM(newValue);
requestRedraw();
```

**Why:** The hardware timer needs to be restarted with the new interval when BPM changes. `setSharedBPM()` handles this automatically.

### 2. Local BPM State

If your module maintains a local copy of BPM (e.g., for display or calculations), update it **before** calling `setSharedBPM()`:

**Before:**
```cpp
sharedBPM = target;
myModule.bpm = sharedBPM;  // Uses the value we just set
```

**After:**
```cpp
myModule.bpm = target;      // Update local state first
setSharedBPM(target);       // Then update global and timer
```

**Why:** This avoids reading back the value you just wrote, which is cleaner and more explicit.

## Examples from Real Modules

### Example 1: Simple BPM Change (Settings Mode)

**Before:**
```cpp
if (plusPressed) {
  if (sharedBPM < kBpmMax) {
    sharedBPM = (sharedBPM + kBpmStep > kBpmMax) ? kBpmMax : sharedBPM + kBpmStep;
    requestRedraw();
  }
}
```

**After:**
```cpp
if (plusPressed) {
  if (sharedBPM < kBpmMax) {
    uint16_t newBPM = (sharedBPM + kBpmStep > kBpmMax) ? kBpmMax : sharedBPM + kBpmStep;
    setSharedBPM(newBPM);
    requestRedraw();
  }
}
```

### Example 2: BPM with Local State (Euclidean Mode)

**Before:**
```cpp
if (target == sharedBPM) {
  return;
}
sharedBPM = target;
euclideanState.bpm = target;
requestRedraw();
```

**After:**
```cpp
if (target == sharedBPM) {
  return;
}
euclideanState.bpm = target;  // Update local first
setSharedBPM(target);         // Then global and timer
requestRedraw();
```

### Example 3: BPM from UI Control (Grids Mode)

**Before:**
```cpp
if (bpmUpPressed) {
  int target = std::min<int>(GRIDS_MAX_BPM, static_cast<int>(sharedBPM) + 5);
  sharedBPM = static_cast<uint16_t>(target);
  grids.bpm = static_cast<float>(sharedBPM);
  requestRedraw();
}
```

**After:**
```cpp
if (bpmUpPressed) {
  int target = std::min<int>(GRIDS_MAX_BPM, static_cast<int>(sharedBPM) + 5);
  grids.bpm = static_cast<float>(target);  // Update local using target
  setSharedBPM(static_cast<uint16_t>(target));
  requestRedraw();
}
```

## No Changes Needed For

### ✅ Reading BPM
Reading `sharedBPM` still works normally - no changes needed:
```cpp
uint16_t currentBPM = sharedBPM;  // Still works fine
Serial.printf("BPM: %u\n", sharedBPM);  // Still works fine
```

### ✅ Clock Tick Reading
Clock tick APIs unchanged:
```cpp
uint32_t tick = clockManagerGetTickCount();
bool isBar = clockManagerIsBarStart();
bool ready = sequencerSync.readyForStep();
```

### ✅ Clock State
Clock running/stopped state unchanged:
```cpp
bool running = clockManagerIsRunning();
sequencerSync.requestStart();
sequencerSync.stopPlayback();
```

### ✅ External Clock
External clock handling unchanged:
```cpp
clockManagerExternalClock();  // Still works
clockManagerExternalStart();
clockManagerExternalStop();
```

## New Features (Optional)

### Timing Statistics

Monitor clock timing accuracy:

```cpp
#include "clock_timing_debug.h"

// In your update/draw function:
static uint32_t lastStatsTime = 0;
if (millis() - lastStatsTime > 5000) {
  lastStatsTime = millis();
  
  // Get quality string
  const char* quality = ClockTimingDebug::getTimingQuality();
  Serial.printf("Clock quality: %s\n", quality);
  
  // Or get raw stats
  uint32_t minUs, maxUs, avgUs;
  clockManagerGetTimingStats(minUs, maxUs, avgUs);
  Serial.printf("Timing: %u/%u/%u µs (min/avg/max)\n", minUs, avgUs, maxUs);
}
```

### Check Timing Accuracy

```cpp
#include "clock_timing_debug.h"

if (!ClockTimingDebug::isTimingAccurate()) {
  Serial.println("Warning: MIDI clock jitter detected!");
}
```

## Compatibility Notes

### Header Inclusion
No new headers required for basic BPM changes:
```cpp
#include "common_definitions.h"  // setSharedBPM() declared here
```

For timing debug features:
```cpp
#include "clock_timing_debug.h"  // Optional - only if using debug utils
```

### Compile-Time Compatibility
The change is **source-compatible**:
- Old code that reads `sharedBPM` still works
- Old code that writes `sharedBPM = value` still compiles (but won't update timer)
- No breaking API changes

**However**, you should update to `setSharedBPM()` to ensure proper timer behavior.

## Testing Your Changes

After migrating your module:

1. **Compile test**: Verify no errors
2. **BPM change test**: Change BPM and verify it takes effect
3. **Timing test**: Check Serial Monitor for timer restart message:
   ```
   [ClockManager] Hardware timer started: 120 BPM, interval=20833 us (20.833 ms)
   ```
4. **Sequencer test**: Verify sequencer playback matches expected tempo

## Common Mistakes

### ❌ Forgetting to call setSharedBPM()
```cpp
// WRONG - timer won't update!
sharedBPM = 140;
```

```cpp
// CORRECT
setSharedBPM(140);
```

### ❌ Reading back sharedBPM after setting
```cpp
// Suboptimal - redundant read
myState.bpm = newBPM;
setSharedBPM(newBPM);
myOtherState.bpm = sharedBPM;  // Just use newBPM!
```

```cpp
// Better
myState.bpm = newBPM;
setSharedBPM(newBPM);
myOtherState.bpm = newBPM;  // Use local variable
```

### ❌ Multiple updates in tight loop
```cpp
// Inefficient - restarts timer many times
for (int i = 60; i <= 180; i++) {
  setSharedBPM(i);  // Don't do this!
  delay(10);
}
```

```cpp
// Better - update once
uint16_t finalBPM = calculateDesiredBPM();
setSharedBPM(finalBPM);
```

## Module Checklist

When updating a module, verify:

- [ ] All `sharedBPM = value` replaced with `setSharedBPM(value)`
- [ ] Local BPM state updated before calling `setSharedBPM()`
- [ ] No redundant reads of `sharedBPM` after setting
- [ ] Module compiles without warnings
- [ ] BPM changes work correctly in module
- [ ] Sequencer timing matches expected BPM
- [ ] Serial Monitor shows timer restart on BPM change

## Need Help?

If you encounter issues during migration:

1. Check that you're including `common_definitions.h`
2. Verify `setSharedBPM()` is declared and linked
3. Look for Serial output: `[ClockManager] Hardware timer started...`
4. Compare your changes to examples in this guide

## Summary

**Minimal changes required:**
- Replace `sharedBPM = value` → `setSharedBPM(value)`
- Update local BPM copies before calling `setSharedBPM()`

**Everything else stays the same:**
- Clock tick APIs unchanged
- Sequencer sync APIs unchanged  
- External clock handling unchanged
- Reading `sharedBPM` unchanged

This migration ensures your module benefits from the improved timing accuracy with minimal code changes!
