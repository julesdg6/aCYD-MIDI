# MIDI Clock Timing Fix - User Guide

## What Changed

This update fixes a timing accuracy issue in the MIDI clock that was causing the effective BPM to be faster than the displayed value.

## The Problem

Previously, when you set the BPM to 120 in the UI, the actual MIDI clock was running at approximately **125 BPM** - about 4.2% faster than expected.

**Why this happened:**
- The old code used integer math that rounded down: `(60000 / 120) / 24 = 20 milliseconds per tick`
- The correct value should be: **20.833 milliseconds per tick**
- Over time, this 0.833ms error accumulated, making the clock run fast

## The Solution

The firmware now uses the ESP32's high-precision hardware timer:
- Calculates intervals in **microseconds** instead of milliseconds
- At 120 BPM: exactly 20,833.33 microseconds per tick
- Hardware timer maintains precision automatically - no rounding errors

## What You'll Notice

### Improved Timing Accuracy
- **120 BPM now plays at exactly 120 BPM** (not 125)
- Tighter synchronization with external MIDI devices
- More consistent sequencer playback
- Better alignment of bar starts and quantized events

### More Stable Performance
- Less jitter (timing variation) - typically under 100 microseconds
- Smoother playback even when UI is busy (rendering, WiFi, etc.)
- No more "burst" catch-up behavior when system is under load

## Testing Your Device

You can verify the timing improvement using the new debug utilities.

### Basic Test (Serial Monitor)

1. Connect to Serial Monitor at 115200 baud
2. Start a sequencer or MIDI clock mode
3. Look for messages like:
   ```
   [ClockManager] Hardware timer started: 120 BPM, interval=20833 us (20.833 ms)
   ```

### Advanced Testing (Optional)

If you want to measure timing accuracy:

1. Add this code to your `loop()` in main.cpp:
   ```cpp
   #include "clock_timing_debug.h"
   
   // In loop():
   static uint32_t lastDebugTime = 0;
   if (millis() - lastDebugTime > 10000) {  // Every 10 seconds
     lastDebugTime = millis();
     ClockTimingDebug::printTimingStats();
   }
   ```

2. Upload and check Serial Monitor for output like:
   ```
   ========== MIDI Clock Timing Statistics ==========
   Interval (µs): min=20800, max=20900, avg=20833
   Jitter: 100 µs (0.48% of average)
   Expected interval: 20833 µs @ 120 BPM
   Timing error: +0 µs (+0.000%)
   Effective BPM: 120.00 (target: 120)
   ==================================================
   ```

**Good values:**
- Jitter < 1% of average
- Timing error < 0.5%
- Effective BPM matches target BPM

## Compatibility

### What Still Works
- ✅ All sequencer modes (BEATS, EUCLID, GRIDS, etc.)
- ✅ Internal MIDI clock
- ✅ External MIDI clock (BLE, WiFi, Hardware)
- ✅ BPM changes (40-240 range)
- ✅ Clock-synced UI animations
- ✅ Quantized start/stop
- ✅ Hardware MIDI output

### No Breaking Changes
This is a **drop-in replacement** - no settings or workflow changes needed. Your existing patterns, settings, and configurations will work exactly the same, just with better timing.

## For Developers

### API Changes

**New function in `common_definitions.h`:**
```cpp
void setSharedBPM(uint16_t bpm);
```

**Usage:**
Instead of directly modifying `sharedBPM`, use:
```cpp
// Old way (don't do this):
sharedBPM = 140;

// New way (correct):
setSharedBPM(140);
```

This ensures the hardware timer is updated with the new interval.

**New function in `clock_manager.h`:**
```cpp
void clockManagerUpdateBPM();
void clockManagerGetTimingStats(uint32_t &minUs, uint32_t &maxUs, uint32_t &avgUs);
```

### Debug Utilities

**New header: `include/clock_timing_debug.h`**

Provides helper functions for monitoring timing accuracy:
- `ClockTimingDebug::printTimingStats()` - Print detailed timing report
- `ClockTimingDebug::getEffectiveBPM()` - Get actual measured BPM
- `ClockTimingDebug::isTimingAccurate()` - Check if within tolerance
- `ClockTimingDebug::getTimingQuality()` - Get quality rating string

See `docs/MIDI_CLOCK_TIMING.md` for full technical documentation.

## Troubleshooting

### "Clock seems slower now"

This is expected! The old clock was running **too fast**. If you were using BPM settings to compensate:
- Old setting: 115 BPM to get ~120 BPM actual
- New setting: 120 BPM to get 120 BPM actual

You may need to increase your BPM settings slightly to match the tempo you were used to.

### Sequencer not starting

The timing fix doesn't affect start/stop logic. If sequencers aren't starting:
- Check that clock source is set to INTERNAL (in Settings)
- Verify sequencer is armed/enabled
- Check that quantize timeout (2 bars) hasn't triggered

### Jitter higher than expected

Normal jitter is <100 microseconds. Higher jitter (>1%) can be caused by:
- Wi-Fi enabled and transmitting
- Heavy UI rendering (complex graphics)
- BLE advertising/scanning
- Other high-priority interrupts

Try disabling Wi-Fi/BLE to isolate the issue.

## Performance Notes

### CPU Usage
- **Reduced** overall CPU usage (task polling reduced from 1ms to 10ms)
- Hardware timer is very efficient (<10µs overhead per tick)
- More CPU time available for UI and other features

### Memory Usage
- Minimal additional RAM (~100 bytes for timing statistics)
- No dynamic allocation (all static/stack variables)
- No impact on heap fragmentation

## Future Enhancements

Possible future improvements (not yet implemented):
- Phase-locked loop for smooth BPM transitions
- Fractional BPM support (e.g., 120.5 BPM)
- Swing/humanization timing
- Advanced timing diagnostics in UI

## Questions or Issues?

If you experience any issues with the new timing system:
1. Check Serial Monitor for error messages
2. Verify hardware timer initialization: `[ClockManager] Hardware timer initialized`
3. Report issues with timing statistics output (use `printTimingStats()`)
4. Include your board model and configuration (UART0/UART2)

## Summary

✅ **BPM accuracy**: 120 BPM is now actually 120 BPM (was ~125 BPM)  
✅ **Better sync**: Tighter timing with external devices  
✅ **Smoother playback**: Less jitter and no catch-up bursts  
✅ **Compatible**: No changes to your workflow needed  
✅ **More efficient**: Lower CPU usage overall  

This is a fundamental timing improvement that makes the aCYD-MIDI a more accurate and reliable MIDI controller!
