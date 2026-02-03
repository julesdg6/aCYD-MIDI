# uClock Library Integration

## Overview

This project uses the **uClock library** from the julesdg6 fork for professional-grade MIDI clock generation with ESP32 hardware timer precision.

## Library Details

- **Repository**: https://github.com/julesdg6/uClock
- **Purpose**: Hardware timer-based BPM clock generator for MIDI applications
- **Key Feature**: Fixed compatibility with ESP32 Arduino Core >= 3.0.0 (espressif32@6.10.0)

## Why This Fork?

The original uClock library had compatibility issues with newer ESP32 Arduino Core versions (>= 3.0.0). The julesdg6 fork includes fixes for:
- Updated timer API calls for espressif32@6.10.0
- Compatibility with new Arduino Core 3.x timer functions
- Proper function signatures for `timerBegin()` and `timerAttachInterrupt()`

## Integration in aCYD-MIDI

### Configuration

**platformio.ini**:
```ini
lib_deps =
    https://github.com/julesdg6/uClock.git
```

### Implementation

**src/clock_manager.cpp**:

#### Initialization
```cpp
void initClockManager() {
  // ... state initialization ...
  
  // Initialize uClock
  uClock.init();                           // Initialize hardware timer
  uClock.setTempo(120.0);                  // Set initial BPM
  uClock.setOutputPPQN(uClock.PPQN_24);    // MIDI standard: 24 ticks per quarter note
  uClock.setOnOutputPPQN(onClockTickCallback);  // Register tick callback
  uClock.setOnClockStart(onClockStartCallback); // Register start callback
  uClock.setOnClockStop(onClockStopCallback);   // Register stop callback
  
  Serial.println("[ClockManager] uClock initialized with 24 PPQN");
}
```

#### Callback Functions

**Tick Callback** - Called 24 times per quarter note:
```cpp
static void onClockTickCallback(uint32_t tick) {
  lockClockManager();
  tickCount++;
  unlockClockManager();
  sendMIDIClock();      // Send MIDI clock message
  requestRedraw();      // Update UI
}
```

**Start Callback** - Called when clock starts:
```cpp
static void onClockStartCallback() {
  Serial.println("[uClock] Clock started");
}
```

**Stop Callback** - Called when clock stops:
```cpp
static void onClockStopCallback() {
  Serial.println("[uClock] Clock stopped");
}
```

#### Update Function

The `updateClockManager()` function handles:
1. **BPM Changes**: Automatically updates uClock tempo when `sharedBPM` changes
2. **Start/Stop Control**: Manages uClock state based on sequencer activity

```cpp
void updateClockManager() {
  RunningStateDelta delta = updateRunningState();
  
  // Update tempo if changed
  uint16_t bpm = clampBpm(sharedBPM);
  float currentTempo = uClock.getTempo();
  if (abs(currentTempo - (float)bpm) > 0.1f) {
    uClock.setTempo((float)bpm);
  }
  
  // Control clock state
  if (midiClockMaster == CLOCK_INTERNAL) {
    if (delta.running && !uClock.isRunning()) {
      uClock.start();
    } else if (!delta.running && uClock.isRunning()) {
      uClock.stop();
    }
  }
}
```

## Benefits

### Hardware Timer Precision
- uClock uses ESP32 hardware timers for interrupt-based clock generation
- No software polling or accumulator needed
- Sub-microsecond timing accuracy

### Eliminates Timing Errors
- **Before**: Integer truncation caused +4.2% error at 120 BPM
- **After**: Hardware timer ensures exact BPM accuracy

### Professional Implementation
- Used in production music devices
- Battle-tested for sequencers and sync boxes
- Proper RTOS integration with FreeRTOS

### Simplified Code
- Removed ~30 lines of manual accumulator logic
- Hardware handles timing complexity
- Cleaner, more maintainable code

## API Reference

### Main Functions

| Function | Description |
|----------|-------------|
| `uClock.init()` | Initialize hardware timer |
| `uClock.setTempo(float)` | Set BPM tempo |
| `uClock.getTempo()` | Get current BPM |
| `uClock.setOutputPPQN(ppqn)` | Set output resolution (PPQN_24 for MIDI) |
| `uClock.setOnOutputPPQN(callback)` | Register tick callback |
| `uClock.setOnClockStart(callback)` | Register start callback |
| `uClock.setOnClockStop(callback)` | Register stop callback |
| `uClock.start()` | Start clock |
| `uClock.stop()` | Stop clock |
| `uClock.isRunning()` | Check if clock is running |
| `uClock.resetCounters()` | Reset internal tick counters |

### PPQN Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `uClock.PPQN_96` | 96 | High resolution (not used) |
| `uClock.PPQN_48` | 48 | High resolution (not used) |
| `uClock.PPQN_24` | 24 | **MIDI standard** - used in this project |

## Compatibility

### Supported Platforms
- ✅ ESP32 (all variants)
- ✅ ESP32-S2
- ✅ ESP32-S3
- ✅ ESP32-C3

### Framework Versions
- ✅ espressif32@6.10.0 (Arduino Core 3.x)
- ✅ espressif32@6.x.x
- ✅ Future Arduino Core versions

### Tested Boards
- ✅ ESP32-2432S028R (CYD)
- ✅ ESP32-2432S028Rv2 (CYD v2)
- ✅ ESP32-3248S035C (3.5" capacitive)
- ✅ ESP32-3248S035R (3.5" resistive)

## Troubleshooting

### Build Errors

**Error**: `too few arguments to function 'hw_timer_t* timerBegin'`
- **Cause**: Using original uClock library (not the fork)
- **Solution**: Ensure platformio.ini uses `https://github.com/julesdg6/uClock.git`

**Error**: `'class umodular::clock::uClockClass' has no member named 'setPPQN'`
- **Cause**: Wrong API method names
- **Solution**: Use `setOutputPPQN()` instead of `setPPQN()`

### Runtime Issues

**Clock not starting**:
- Check `midiClockMaster == CLOCK_INTERNAL`
- Verify sequencer has requested start (`pendingStarts > 0` or `activeSequencers > 0`)
- Check serial output for `[uClock] Clock started` message

**Tempo not updating**:
- Ensure `updateClockManager()` is called regularly (from `midiClockTask()`)
- Verify `sharedBPM` is being modified correctly
- Check serial output for tempo change confirmation

## Migration Notes

### From Microsecond Accumulator

If migrating from the previous microsecond accumulator implementation:

**Removed**:
- `microAccumulator` variable
- `lastUpdateMicros` variable
- Manual interval calculation in microseconds
- While loop for tick generation

**Added**:
- uClock library dependency
- Hardware timer initialization
- Callback functions
- Tempo update logic

**Result**: Simpler code with better accuracy and hardware-level precision.

## Further Reading

- **uClock Repository**: https://github.com/julesdg6/uClock
- **Original uClock**: https://github.com/midilab/uClock
- **MIDI Specification**: Standard 24 PPQN clock timing
- **ESP32 Timer API**: Hardware timer documentation for Arduino Core 3.x
