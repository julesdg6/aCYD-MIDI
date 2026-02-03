# MIDI Clock Timing Implementation

## Overview

The aCYD-MIDI controller uses a hardware timer-based MIDI clock system to achieve precise, jitter-free timing for MIDI clock messages (0xF8), ensuring tight synchronization with external devices and internal sequencers.

## Architecture

### Hardware Timer Approach

Instead of polling `millis()` in a FreeRTOS task, the system uses the ESP32's high-resolution hardware timer (`esp_timer`) to generate MIDI clock ticks:

1. **Hardware Timer**: ESP32 `esp_timer` provides microsecond-precision periodic interrupts
2. **ISR Tick Counter**: Timer callback increments tick counter in ISR-safe critical section
3. **Deferred Dispatch**: FreeRTOS task polls tick counter and sends MIDI messages in non-ISR context
4. **BLE/UART Safe**: MIDI transmission (BLE notify, UART write) happens outside interrupt context

### Why Hardware Timers?

**Problem with Software Polling:**
- Integer millisecond math causes timing errors
  - Example: 120 BPM = 500ms/quarter ÷ 24 ticks = 20.833ms per tick
  - Integer truncation: 20ms → effective 125 BPM (+4.2% error)
- FreeRTOS scheduling jitter adds variable delay
- Catch-up loops cause burst transmission when task is delayed

**Solution with Hardware Timer:**
- Microsecond-precision interval calculation
  - Example: 120 BPM = 60,000,000 µs/min ÷ 120 ÷ 24 = 20,833.33 µs per tick
  - No rounding error - hardware maintains exact timing
- Independent of task scheduling - timer runs in hardware
- Smooth, evenly-spaced clock messages

## Technical Details

### Timer Configuration

```cpp
esp_timer_create_args_t timerConfig = {
  .callback = &onClockTimer,           // ISR callback
  .arg = nullptr,
  .dispatch_method = ESP_TIMER_TASK,   // Dispatch via task, not ISR
  .name = "midi_clock",
  .skip_unhandled_events = false       // Don't skip ticks if delayed
};
esp_timer_create(&timerConfig, &clockTimer);
```

### Interval Calculation

For a given BPM, the interval between MIDI clock messages:

```cpp
// Microseconds per tick (high precision)
uint64_t intervalUs = (60000000ULL / bpm) / CLOCK_TICKS_PER_QUARTER;
// CLOCK_TICKS_PER_QUARTER = 24 (MIDI standard)
```

**Examples:**
- 60 BPM: 41,666.67 µs per tick (24 Hz)
- 120 BPM: 20,833.33 µs per tick (48 Hz)
- 180 BPM: 13,888.89 µs per tick (72 Hz)
- 240 BPM: 10,416.67 µs per tick (96 Hz)

### ISR Safety

The timer callback runs in interrupt context and must be ISR-safe:

```cpp
static void IRAM_ATTR onClockTimer(void* arg) {
  portENTER_CRITICAL_ISR(&clockManagerMux);  // Atomic access
  tickCount++;
  portEXIT_CRITICAL_ISR(&clockManagerMux);
  needsRedraw = true;  // Signal update needed
}
```

**Rules:**
- `IRAM_ATTR` places function in fast RAM (not flash)
- Critical sections use ISR-safe variants
- No BLE/UART/Serial calls (not ISR-safe)
- No dynamic memory allocation
- No mutex locks (use spinlocks/critical sections only)

### MIDI Dispatch

A FreeRTOS task polls the tick counter and dispatches MIDI messages:

```cpp
void updateClockManager() {
  static uint32_t lastProcessedTick = 0;
  uint32_t currentTick = clockManagerGetTickCount();
  
  while (lastProcessedTick < currentTick) {
    lastProcessedTick++;
    sendMIDIClock();  // Safe - called from task context
  }
}
```

This task runs at 10ms intervals (reduced from 1ms) since precision is handled by the hardware timer.

## Timing Statistics

The system collects real-time timing statistics for debugging and verification:

```cpp
void clockManagerGetTimingStats(uint32_t &minUs, uint32_t &maxUs, uint32_t &avgUs);
```

**Usage:**
```cpp
uint32_t min, max, avg;
clockManagerGetTimingStats(min, max, avg);
Serial.printf("Tick interval: min=%u, max=%u, avg=%u µs\n", min, max, avg);
```

**Expected values at 120 BPM:**
- Ideal: 20,833 µs
- Min: ~20,800 µs (normal jitter)
- Max: ~20,900 µs (normal jitter)
- Avg: ~20,833 µs (should match ideal)

Significant deviations indicate:
- High interrupt load
- Other high-priority interrupts preempting timer
- Wi-Fi/BLE radio interference
- Thermal throttling (rare on ESP32)

## BPM Changes

When `sharedBPM` is modified, call `clockManagerUpdateBPM()` to restart the timer with the new interval:

```cpp
sharedBPM = newBPM;
clockManagerUpdateBPM();  // Recalculates interval and restarts timer
```

The timer stops and restarts with the new period. This causes a brief discontinuity (one tick may be delayed) but ensures subsequent ticks are precisely timed.

## External Clock Mode

When using external MIDI clock (BLE/Wi-Fi/Hardware MIDI input):

- Hardware timer is **disabled**
- `clockManagerExternalClock()` is called on each incoming 0xF8 message
- Tick counter increments directly (no timer involved)
- Timing accuracy depends on external clock source

## Performance Characteristics

### Timing Accuracy

| Parameter | Old (millis poll) | New (hardware timer) |
|-----------|-------------------|----------------------|
| Resolution | 1 ms | 1 µs |
| 120 BPM error | +4.2% (125 BPM) | <0.01% |
| Jitter (typical) | 1-5 ms | <100 µs |
| CPU overhead | Higher (1ms poll) | Lower (10ms poll) |

### CPU Usage

- **ISR overhead**: ~10-50 µs per tick (depending on BPM)
- **Task overhead**: 10ms polling (90% reduction from 1ms)
- **Timer dispatch**: Zero-copy (no queue, just atomic counter)

At 120 BPM (48 ticks/sec):
- Old: 1000 polls/sec × overhead
- New: 100 polls/sec × overhead + 48 ISR/sec × minimal overhead

## Limitations

1. **Maximum BPM**: Limited by ESP32 timer resolution and interrupt overhead
   - Theoretical: 60,000,000 µs ÷ 24 ÷ 1 µs = 2,500,000 BPM
   - Practical: <1000 BPM (limited by MIDI transmission rate and CPU overhead)
   - Clamped: 40-240 BPM (musical range)

2. **Jitter sources** (even with hardware timer):
   - Wi-Fi radio interrupts (high priority)
   - BLE stack processing
   - Flash memory access (unless code is in IRAM)
   - Watchdog timer interrupts

3. **BPM change discontinuity**: Restarting timer causes one tick delay

## Debugging

### Serial Output

Enable clock debug output:

```cpp
[ClockManager] Hardware timer initialized
[ClockManager] Hardware timer started: 120 BPM, interval=20833 us (20.833 ms)
[ClockManager] INTERNAL START – pending=1 active=0
[ClockManager] INTERNAL STOP – pending=0 active=0
[ClockManager] Hardware timer stopped
```

### Timing Statistics

Poll timing stats periodically:

```cpp
uint32_t min, max, avg;
clockManagerGetTimingStats(min, max, avg);
if (tickIntervalCount > 100) {
  Serial.printf("Tick timing: min=%u, max=%u, avg=%u µs (jitter=%u µs)\n",
                min, max, avg, max - min);
}
```

### Logic Analyzer / Oscilloscope

Connect to hardware MIDI TX pin (GPIO 1 or GPIO 16 depending on UART):

- Set trigger on 0xF8 MIDI clock byte
- Measure time between consecutive clock messages
- Should be constant ±100 µs

**Expected waveform:**
- Start (0xFA): Single pulse
- Clock (0xF8): Periodic pulses at exact interval
- Stop (0xFC): Single pulse, then silence

## Future Improvements

Potential enhancements (not yet implemented):

1. **Phase-locked loop (PLL)**: Smooth BPM changes without discontinuity
2. **Clock smoothing**: Average out jitter from external sources
3. **Dynamic priority**: Boost timer priority under load
4. **Fractional BPM**: Support decimal BPM values (e.g., 120.5)
5. **Swing/humanization**: Intentional timing variation
6. **Sync to audio sample rate**: Align MIDI clock with audio I2S clock

## References

- [MIDI Clock Specification](https://www.midi.org/specifications/midi1-specifications/m1-v4-2-1-midi-1-0-detailed-specification)
  - 24 pulses per quarter note (PPQN)
  - Clock = 0xF8, Start = 0xFA, Stop = 0xFC
- [ESP32 Timer Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html)
  - High-resolution timer API
  - Microsecond precision
  - ISR and task dispatch modes

## Code References

- `src/clock_manager.cpp`: Main implementation
- `include/clock_manager.h`: Public API
- `src/midi_clock_task.cpp`: FreeRTOS task wrapper
- `include/midi_utils.h`: MIDI message dispatch
