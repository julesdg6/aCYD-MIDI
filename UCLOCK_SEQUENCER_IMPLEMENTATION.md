# uClock Step Sequencer Extension Implementation

## Overview

This document describes the implementation of uClock's step sequencer extension across all sequencer and generative modules in the aCYD-MIDI project.

## Background

The uClock library includes a built-in step sequencer extension specifically designed for creating step sequencers for synthesizers and drum machines. The extension provides:

- **16th note orientation**: Natural step sequencer workflow
- **Multi-track support**: Independent sequences with individual callbacks
- **Per-track shuffle**: Each track can have its own groove (future feature)
- **Hardware-timed precision**: ISR-driven timing for rock-solid accuracy

## Implementation Pattern

Each module follows the same pattern established by the TB3PO mode:

### 1. ISR-Safe Step Counter
```cpp
// ISR-safe step counter from uClock step extension
static volatile uint32_t moduleStepCount = 0;
static const uint8_t moduleTrackIndex = N;  // Unique track number
```

### 2. ISR Callback
```cpp
// ISR callback for uClock step sequencer extension
static void onModuleStepISR(uint32_t step, uint8_t track) {
  (void)step;
  if (track == moduleTrackIndex) {
    moduleStepCount++;
  }
}
```

**Important**: The ISR callback must be minimal and ISR-safe. It:
- Does NOT call `sendMIDI()` (not ISR-safe)
- Does NOT call `Serial.println()` (not ISR-safe)
- Does NOT call `requestRedraw()` (not ISR-safe)
- ONLY increments a volatile counter

### 3. Registration in Init
```cpp
void initializeModuleMode() {
  // ... other initialization ...
  
  // Register uClock step callback (ISR-safe) and allocate 1 track slot.
  uClock.setOnStep(onModuleStepISR, 1);
}
```

### 4. Step Processing in Main Loop
```cpp
void updateModule() {
  // ... handle start/stop ...
  
  if (!moduleSync.playing) {
    return;
  }
  
  // Get steps from uClock step extension (ISR-safe)
  uint32_t readySteps = 0;
  noInterrupts();
  readySteps = moduleStepCount;
  moduleStepCount = 0;
  interrupts();
  
  if (readySteps == 0) {
    return;
  }
  
  // Process steps in main loop (safe to call sendMIDI, etc.)
  for (uint32_t i = 0; i < readySteps; ++i) {
    playModuleStep();
  }
}
```

## Track Assignments

Each module is assigned a unique track index to avoid conflicts:

| Module | Track Index | Purpose |
|--------|-------------|---------|
| TB3PO | 0 | TB-303 style sequencer |
| Arpeggiator | 1 | Piano keyboard arpeggiator |
| Euclidean | 2 | Euclidean rhythm generator |
| Random Generator | 3 | Random note generator |
| Raga | 4 | Indian classical music generator |
| Sequencer | 5 | 16-step drum sequencer |

## Module-Specific Implementations

### Sequencer Mode (module_sequencer_mode.cpp)
- **Purpose**: 16-step drum sequencer (TR-808 style)
- **Timing**: Straight 16th notes
- **Special handling**: Tracks note off times separately

### Arpeggiator Mode (module_arpeggiator_mode.cpp)
- **Purpose**: Piano keyboard arpeggiator
- **Timing**: Uses `stepTicks` accumulator for variable note rates (4th, 8th, 16th, 32nd)
- **Special handling**: Accumulates ticks and plays notes when threshold reached

### Euclidean Mode (module_euclidean_mode.cpp)
- **Purpose**: Euclidean rhythm patterns
- **Timing**: Straight 16th notes or triplets
- **Special handling**: Triplet mode uses accumulator to divide steps by 3

### Random Generator Mode (module_random_generator_mode.cpp)
- **Purpose**: Random melodic note generator
- **Timing**: Configurable subdivision (quarter, eighth, sixteenth)
- **Special handling**: Subdivision accumulator divides 16th notes by factor

### Raga Mode (module_raga_mode.cpp)
- **Purpose**: Indian classical music phrase generator
- **Timing**: Straight 16th notes with tala (rhythm cycle) tracking
- **Special handling**: Generates musical phrases based on raga scales

## Benefits of uClock Step Extension

### Before (Manual Timing)
- Software-based timing using `SequencerSyncState::consumeReadySteps()`
- Relied on MIDI clock ticks from main clock manager
- Potential for jitter and timing drift
- More complex code with tick calculations

### After (uClock Step Extension)
- Hardware-timed ISR callbacks at exact 16th note intervals
- Sub-microsecond precision from ESP32 hardware timer
- Minimal ISR code reduces interrupt latency
- Simpler, more maintainable code

## Thread Safety

The implementation uses proper thread safety mechanisms:

1. **Volatile counters**: Step counters are `volatile` to prevent compiler optimization issues
2. **Interrupt disable**: `noInterrupts()` / `interrupts()` protect counter reads
3. **ISR-safe operations**: ISR only increments counters, all MIDI/serial/display work in main loop

## Consistency with Existing Code

This implementation:
- Follows the exact pattern used by TB3PO mode (already in production)
- Uses the same uClock library and API
- Maintains the same `SequencerSyncState` for start/stop logic
- Preserves all existing functionality while improving timing

## Testing Recommendations

### Build Testing
```bash
# Test all board variants
pio run -e esp32-2432S028Rv2
pio run -e esp32-3248S035C
pio run -e esp32-3248S035R
```

### Runtime Testing
1. **Timing accuracy**
   - Test each sequencer mode at various BPMs (60, 120, 180, 240)
   - Verify step timing is precise and consistent
   - Check for drift over extended periods (1+ hour)

2. **Multi-mode switching**
   - Switch between different sequencer modes
   - Verify each mode registers its track correctly
   - Check for no conflicts or timing glitches

3. **Synchronization**
   - Run multiple modes simultaneously (if supported in future)
   - Verify all stay in sync
   - Test start/stop behavior

4. **Edge cases**
   - Very fast BPM (240)
   - Very slow BPM (40)
   - Rapid start/stop cycling
   - Mode switching while playing

## Future Enhancements

The uClock step extension supports additional features that could be implemented:

1. **Per-track shuffle**: Each sequencer could have its own groove setting
2. **Per-track shift**: Offset patterns in time for polyrhythmic effects
3. **Per-track direction**: Forward, reverse, ping-pong playback

## References

- **uClock Library**: https://github.com/julesdg6/uClock
- **Step Extension Docs**: See README in uClock repository
- **TB3PO Implementation**: `src/module_tb3po_mode.cpp` (reference implementation)
- **Original Timing Fix**: `UCLOCK_SOLUTION_SUMMARY.md`
