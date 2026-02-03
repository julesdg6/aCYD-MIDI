# Duplicate MIDI Clock Emission Investigation

**Date**: 2026-02-03  
**Status**: ✅ **ISSUE NOT FOUND IN CURRENT CODEBASE**

## Summary

Investigation of reported duplicate MIDI clock emissions (0xF8 bytes) from both software and hardware timer paths reveals that **the current codebase does NOT exhibit this issue**. The implementation uses a single, well-architected clock source via the uClock library with proper ISR-safe design.

## Issue Description (From Problem Statement)

The problem statement describes:
- Duplicate MIDI clock bytes (0xF8) emitted by device
- Two competing tick sources:
  1. Software tick path in `src/clock_manager.cpp` (original `micros()`-based loop)
  2. Hardware timer handler in `src/midi_timer.cpp`
- Compile-time guard `USE_HW_TIMER` to suppress software emits
- BLE notifications showing delivery jitter

Referenced files mentioned but **NOT FOUND**:
- `src/midi_timer.cpp` - Does not exist
- `tools/ble_midi_capture.py` - Does not exist
- `tools/analysis_midi_capture.py` - Does not exist
- `tools/midi_capture.csv` - Does not exist
- `USE_HW_TIMER` flag - Not in platformio.ini or any source files

## Current Implementation Analysis

### Architecture Overview

The current implementation uses **uClock library** for hardware timer-based MIDI clock generation:

```
Hardware Timer (ESP32)
  ↓
uClock Library ISR
  ↓
onClockTickCallback() [ISR context]
  ↓
Set tickPending flag (atomic)
  ↓
midiClockTask() [FreeRTOS task - 1ms polling]
  ↓
updateClockManager() [task context]
  ↓
sendMIDIClock() [SINGLE CALL SITE]
  ↓
BLE, Hardware MIDI, ESP-NOW, WiFi outputs
```

### Single Tick Source Architecture

**1. Hardware Timer Initialization** (`src/clock_manager.cpp:113-135`):
```cpp
void initClockManager() {
  // ...
  uClock.init();
  uClock.setOutputPPQN(uClock.PPQN_24);  // 24 PPQN (MIDI standard)
  uClock.setOnOutputPPQN(onClockTickCallback);  // ISR callback
  uClock.setOnClockStart(onClockStartCallback);
  uClock.setOnClockStop(onClockStopCallback);
  uClock.setTempo(120.0);
}
```

**2. ISR-Safe Callback** (`src/clock_manager.cpp:39-46`):
```cpp
static void onClockTickCallback(uint32_t tick) {
  // Minimal ISR: record tick and mark pending for main loop processing.
  lockClockManager();
  tickCount = tick;
  tickPending = true;  // Flag for task context processing
  unlockClockManager();
  // NOTE: Does NOT call sendMIDIClock() here (ISR-safe design)
}
```

**3. Task Context Processing** (`src/clock_manager.cpp:197-208`):
```cpp
void updateClockManager() {
  // ... (state management)
  
  // Handle any pending ticks (deferred from ISR)
  while (true) {
    bool pending = false;
    lockClockManager();
    if (tickPending) {
      tickPending = false;
      pending = true;
    }
    unlockClockManager();
    if (!pending) break;
    sendMIDIClock();  // ← ONLY CALL SITE
    requestRedraw();
  }
}
```

**4. FreeRTOS Task** (`src/midi_clock_task.cpp:15-20`):
```cpp
static void midiClockTask(void *) {
  while (true) {
    updateClockManager();  // Polls tickPending flag every 1ms
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
```

### Verification: No Duplicate Paths

**Search for all `sendMIDIClock()` calls:**
```bash
$ grep -rn "sendMIDIClock()" --include="*.cpp" --include="*.h"
include/midi_utils.h:34:inline void sendMIDIClock() {
src/clock_manager.cpp:206:    sendMIDIClock();
```

**Result**: Only ONE call site in the entire codebase.

**Search for competing timer implementations:**
```bash
$ find . -name "midi_timer.*"
# No results

$ grep -r "USE_HW_TIMER" --include="*.cpp" --include="*.h" --include="*.ini"
# No results
```

**Result**: No competing timer implementations found.

### sendMIDIClock() Implementation

Location: `include/midi_utils.h:34-50`

```cpp
inline void sendMIDIClock() {
  // BLE MIDI
  if (deviceConnected) {
    midiPacket[2] = 0xF8;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();  // ← BLE notification
  }
  
  // Hardware MIDI (DIN-5 connector via UART)
  sendHardwareMIDISingle(0xF8);
  
  // ESP-NOW MIDI
  #if ESP_NOW_ENABLED
  if (espNowState.initialized && espNowState.mode != ESP_NOW_OFF) {
    sendEspNowMidi(0xF8, 0x00, 0x00);
  }
  #endif
  
  // WiFi MIDI
  uint8_t wifiClock = 0xF8;
  sendWiFiMidiMessage(&wifiClock, 1);
}
```

This function broadcasts to all MIDI transports from a **single source**, preventing duplicates.

## Why No Duplicates Can Occur

1. **Single hardware timer source**: uClock library manages the ESP32 hardware timer
2. **ISR-safe design**: Callback only sets flags, doesn't send MIDI
3. **Deferred processing**: sendMIDIClock() called from task context only
4. **Single call site**: Only `clock_manager.cpp:206` calls sendMIDIClock()
5. **No software tick loop**: No `micros()`-based timing found
6. **No competing timers**: No other timer ISRs found

## BLE Notification Delivery

The issue mentions "BLE notifications show delivery jitter". This is **NOT a duplicate emission problem**, but rather a characteristic of BLE transport:

### BLE Notification Characteristics
- **Connection interval**: BLE has ~7.5-30ms connection intervals (configurable)
- **Packet queuing**: Multiple MIDI messages may be queued before transmission
- **Radio scheduling**: ESP32 BLE stack manages radio time with WiFi/BT coexistence
- **OS buffering**: FreeRTOS and BLE stack introduce variable buffering delays

### Why Jitter Is Expected
At 120 BPM with 24 PPQN:
- MIDI clock interval = (60000ms / 120) / 24 = **20.833ms**
- BLE connection interval ≈ 7.5-30ms (comparable to clock rate)
- Result: Notifications may appear bursty (multiple clocks in one BLE packet) or delayed

This is **normal BLE behavior**, not a code bug. Hardware MIDI (31.25 kbaud UART) has ~1ms latency and no jitter.

## Evidence from Documentation

### UCLOCK_SOLUTION_SUMMARY.md
The repository includes `UCLOCK_SOLUTION_SUMMARY.md` which documents the uClock integration as the **final solution** for MIDI clock timing. Key points:

- ✅ Hardware timer precision via uClock library
- ✅ ESP32 Core 6.10.0 compatible (julesdg6/uClock fork)
- ✅ Perfect accuracy (no integer truncation errors)
- ✅ ISR-safe callbacks with FreeRTOS integration
- ✅ Status: **READY FOR PRODUCTION**

This confirms the current implementation is intentional and correct.

### MIDI_CLOCK_FIX.md
Documents the migration from millisecond-based software timing (with 4.2% error at 120 BPM) to uClock library. No mention of duplicate emissions.

## Possible Explanations

### Scenario 1: Issue Was Already Fixed
The duplicate emission problem may have existed in an earlier version and was resolved when uClock was integrated. The current codebase reflects the fixed state.

### Scenario 2: Issue From Different Branch
The files mentioned (midi_timer.cpp, USE_HW_TIMER) may exist in a different branch or were part of an experimental implementation that was not merged.

### Scenario 3: Issue Description Is Outdated
The problem statement may describe a known issue with a solution already implemented, and this investigation confirms the fix is in place.

### Scenario 4: Tools Need Creation
The capture/analysis tools mentioned may need to be created to verify BLE timing behavior, but the duplicate emission issue itself doesn't exist.

## Recommendations

### Immediate Actions
1. ✅ **Confirm with user**: Ask if duplicate clocks are observed on actual hardware
2. ✅ **Check branches**: Verify if midi_timer.cpp exists in other branches
3. ✅ **Test BLE**: If jitter is the concern, test BLE connection parameters

### If Issue Still Occurs on Hardware
If the user reports seeing duplicate 0xF8 bytes on actual hardware:

1. **Add debug logging**:
   ```cpp
   void sendMIDIClock() {
     static uint32_t clockCount = 0;
     Serial.printf("[%lu] MIDI Clock #%lu\n", micros(), ++clockCount);
     // ... existing code
   }
   ```

2. **Monitor serial output**: Compare log timestamps with MIDI clock captures

3. **Create capture tools**: Implement the mentioned Python scripts:
   - `tools/ble_midi_capture.py` - BLE + serial capture
   - `tools/analysis_midi_capture.py` - Inter-arrival analysis

### If Issue Is BLE Jitter Only
If the concern is BLE notification timing (not duplicates):

1. **Document BLE limitations**: Update docs to explain BLE jitter is expected
2. **Recommend hardware MIDI**: For critical timing, use DIN-5 connector (UART)
3. **Optimize BLE parameters**: Tune connection interval if possible

### If Issue Doesn't Exist
If user confirms no duplicate clocks are observed:

1. **Close issue**: Mark as resolved/cannot reproduce
2. **Update documentation**: Add note about uClock preventing duplicates
3. **Archive investigation**: Keep this document for reference

## Testing Recommendations

If creating validation tools:

### 1. Serial Logging Test
Add temporary debug output to count sendMIDIClock() calls:
```cpp
void sendMIDIClock() {
  static unsigned long lastMicros = 0;
  unsigned long now = micros();
  unsigned long delta = now - lastMicros;
  Serial.printf("[Clock] %lu us delta\n", delta);
  lastMicros = now;
  // ... existing code
}
```

Expected: ~20833µs intervals at 120 BPM (no duplicates = no ~0µs deltas)

### 2. MIDI Monitor Test
Connect to DAW with MIDI monitor:
1. Start internal clock at 120 BPM
2. Monitor incoming MIDI clocks
3. Check for duplicate 0xF8 bytes with identical timestamps
4. Measure inter-arrival times

Expected: No duplicates, ~20.833ms intervals

### 3. Hardware Oscilloscope Test
For hardware MIDI output:
1. Connect oscilloscope to MIDI TX pin
2. Measure pulse intervals
3. Check for back-to-back transmissions

Expected: ~20.833ms intervals, no duplicates

## Conclusion

**The duplicate MIDI clock emission issue described in the problem statement DOES NOT EXIST in the current codebase.**

The current implementation:
- ✅ Uses single hardware timer source (uClock library)
- ✅ Has proper ISR-safe deferred processing design
- ✅ Only calls sendMIDIClock() from one location
- ✅ No competing software/hardware tick paths
- ✅ Professional-grade timing implementation
- ✅ Production-ready status confirmed in documentation

**BLE notification jitter** is a separate issue related to BLE transport characteristics, not duplicate clock emissions. This is expected behavior for wireless MIDI.

**Recommended action**: Confirm with user if the issue still occurs on actual hardware, or close the issue as already resolved.

---

## Appendix: Code References

### File Locations
- Clock manager: `src/clock_manager.cpp`, `include/clock_manager.h`
- MIDI utilities: `include/midi_utils.h`
- Clock task: `src/midi_clock_task.cpp`, `include/midi_clock_task.h`
- uClock library: External dependency via platformio (https://github.com/julesdg6/uClock.git)

### Key Functions
- `initClockManager()` - Initialize uClock and callbacks
- `updateClockManager()` - Process pending ticks in task context
- `onClockTickCallback()` - ISR callback from uClock
- `sendMIDIClock()` - Broadcast 0xF8 to all MIDI transports
- `midiClockTask()` - FreeRTOS task polling at 1ms

### Search Commands Used
```bash
# Find all sendMIDIClock calls
grep -rn "sendMIDIClock()" --include="*.cpp" --include="*.h"

# Find timer implementations
find . -name "*timer*"

# Find USE_HW_TIMER references
grep -r "USE_HW_TIMER" --include="*.cpp" --include="*.h" --include="*.ini"

# Find ISR implementations
grep -rn "ISR\|IRAM_ATTR\|timerBegin" --include="*.cpp" --include="*.h"
```

All searches confirm single tick source architecture.
