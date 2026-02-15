# Clocked Sequencer & Generative Module Framework

## Architecture Overview

The aCYD-MIDI Clocked Module Framework provides a unified, thread-safe architecture for implementing step-based sequencers and generative music modules with precise MIDI timing.

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                      │
│  (UI, Mode Switching, User Interaction)                    │
└────────────────┬────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────┐
│                     ClockRuntime                            │
│  • Transport State Machine                                  │
│  • Module Registration & Dispatch                           │
│  • Step Boundary Computation                                │
│  • Quantized Start/Stop                                     │
│  • BPM Management                                           │
└────────┬──────────────────────────────────┬─────────────────┘
         │                                  │
         │ onStep(ctx)                      │ MIDI Events
         │                                  │
┌────────▼──────────────┐          ┌────────▼─────────────────┐
│  ClockedModule (Base) │          │    MidiOutBuffer         │
│  • init() / reset()   │          │  • Ring Buffer           │
│  • onStep(ctx)        │──MIDI──▶ │  • Output Task           │
│  • Lifecycle hooks    │          │  • Scheduled Note-Offs   │
└───────────────────────┘          └──────────┬───────────────┘
         ▲                                    │
         │ Derived modules:                   │
         │ DrumSeqClocked                     ▼
         │ EuclideanClocked         ┌─────────────────────┐
         │ etc.                     │  MIDI Transports    │
                                    │  • BLE MIDI         │
                                    │  • Hardware DIN-5   │
                                    │  • WiFi UDP         │
                                    │  • ESP-NOW          │
                                    └─────────────────────┘
```

## Key Concepts

### Transport States

The framework uses a state machine with precise quantization:

- **STOPPED**: No clock, all modules idle
- **PENDING_START**: Waiting for quantized start boundary
- **RUNNING**: Clock active, modules receive `onStep()` calls
- **PENDING_STOP**: Waiting for quantized stop boundary

### Quantization Modes

Start and stop can be quantized independently:

- **IMMEDIATE**: Transition immediately
- **NEXT_STEP**: Wait for next step boundary (default 1/16 note)
- **NEXT_BAR**: Wait for next bar start
- **END_OF_BAR**: Wait for end of current bar (start of next)

### Step Context

Every `onStep()` call receives a `StepContext` with timing information:

```cpp
struct StepContext {
  uint32_t tick;          // Current MIDI clock tick (24 PPQN)
  uint16_t bpm_x10;       // Tempo in fixed-point (1200 = 120.0 BPM)
  uint8_t  ppqn;          // Always 24 for MIDI clock
  uint32_t barIndex;      // Current bar number
  uint32_t tickInBar;     // Position within bar (0..95 for 4/4)
  uint32_t stepIndex;     // Absolute step count
  uint16_t stepInBar;     // Step position within bar
  uint16_t ticksPerStep;  // Module's step resolution
  bool isBarStart;        // True if at bar start
};
```

## Thread Safety

The framework is designed for FreeRTOS multi-tasking:

### Tasks

1. **MIDI Clock Task** (Priority: MAX-2)
   - Runs `updateClockManager()` at 1ms intervals
   - Calls `clockRuntime.processTick()`
   - Dispatches step callbacks to modules

2. **MIDI Output Task** (Priority: MAX-1)
   - Dedicated to sending MIDI messages
   - Processes ring buffer at 1ms intervals
   - Handles scheduled note-offs

3. **Main Loop Task**
   - UI rendering and touch input
   - Mode switching
   - Module parameter updates

### Synchronization

- **Lock-free ring buffer** for MIDI events (256 slots)
- **Mutex-protected** module registration and slot management
- **ISR-safe** tick processing from uClock callbacks
- **Atomic operations** where appropriate

## MIDI Output Flow

All MIDI messages flow through `MidiOutBuffer`:

```cpp
// Immediate note on/off
midiOutBuffer.noteOn(channel, note, velocity);
midiOutBuffer.noteOff(channel, note);

// Note with automatic off after duration
midiOutBuffer.note(channel, note, velocity, durationTicks);

// Control change
midiOutBuffer.controlChange(channel, cc, value);

// Transport (handled by ClockRuntime)
midiOutBuffer.midiClock();
midiOutBuffer.midiStart();
midiOutBuffer.midiStop();
```

### Scheduled Note-Offs

The framework automatically manages note-offs:

1. `note(ch, note, vel, duration)` sends note-on immediately
2. Schedules note-off for `currentTick + duration`
3. `MidiOutBuffer::updateScheduledNotes()` checks every tick
4. Sends note-off when time expires

### Panic/All-Notes-Off

On transport stop or reset:

```cpp
midiOutBuffer.panic();  // Sends CC 123/120 on all 16 channels
```

## Module Lifecycle

### Registration

```cpp
// In module implementation
REGISTER_MODULE(DrumSeqClocked, "drum_seq_clocked")

// Runtime creation
ClockedModule* module = ModuleFactory::instance().create("drum_seq_clocked");
int slotId = clockRuntime.registerModule(module, midiChannel);
```

### Initialization

```cpp
module->init();           // Called once after creation
module->reset();          // Called on reset/stop
```

### Transport Events

```cpp
// When transport starts
module->onTransportStart();

// When transport stops
module->onTransportStop();
```

### Step Callbacks

```cpp
void MyModule::onStep(const StepContext& ctx) {
  // Called at each step boundary based on ticksPerStep()
  
  if (pattern_[ctx.stepIndex % numSteps]) {
    midiOutBuffer.note(0, noteNumber, velocity, gateLength);
  }
}
```

## Timing Guarantees

### MIDI Clock Output

- **24 PPQN** (pulses per quarter note)
- Clock pulses sent from high-priority output task
- Minimal jitter (<1ms typical)

### Step Accuracy

- Steps computed from tick count: `tick % ticksPerStep == 0`
- Modules called in deterministic slot order
- No drift accumulation

### Supported Step Resolutions

Must divide 24 evenly:

| Note Value | Ticks/Step | Steps/Quarter |
|------------|------------|---------------|
| 1/4 note   | 24         | 1             |
| 1/8 note   | 12         | 2             |
| 1/16 note  | 6          | 4             |
| 1/32 note  | 3          | 8             |
| 1/8 triplet| 8          | 3             |
| 1/16 triplet| 4         | 6             |

## Slot System

The `ClockRuntime` manages up to 8 concurrent module slots:

```cpp
struct Slot {
  ClockedModule* module;
  uint8_t midiChannel;  // 0-15
  bool mute;            // Skip MIDI output, advance playhead
  bool enabled;         // Skip entirely (frozen)
  // ... internal state
};
```

### Dispatch Order

Slots dispatched in ascending order (0→7) for determinism.

### Mute vs Disable

- **Muted**: Module advances, no MIDI output
- **Disabled**: Module frozen, no callbacks

## Integration with Existing Code

The framework coexists with legacy code:

### Backward Compatibility

- Existing modules continue to use `SequencerSyncState`
- `ClockManager` forwards ticks to `ClockRuntime`
- Global `sharedBPM` synchronized with `ClockRuntime::bpm_`

### Migration Path

1. Derive new module from `ClockedModule`
2. Implement required interface methods
3. Register with `ModuleFactory`
4. Create instance and register with `ClockRuntime`
5. Remove legacy timing code

See `DrumSeqClocked` for reference implementation.

## Performance Considerations

### Memory Usage

- **MidiOutBuffer**: ~2KB (256 events × 8 bytes)
- **Scheduled Notes**: ~0.5KB (64 notes × 8 bytes)
- **ClockRuntime**: ~1KB (8 slots + state)
- **Per Module**: Varies (DrumSeqClocked ~64 bytes)

### CPU Usage

- Clock task: ~1-2% (1ms tick processing)
- MIDI output task: ~1-2% (event transmission)
- Module callbacks: Depends on implementation

### Latency

- Input to MIDI output: <5ms typical
- Quantized start/stop: Depends on quantize mode
- Note-off accuracy: ±1 tick (1-2ms @120 BPM)

## Best Practices

### Module Implementation

1. **Keep onStep() fast**: No blocking, no heavy computation
2. **Use scheduled notes**: Prefer `note()` over manual on/off
3. **Clean up on stop**: Send note-offs in `onTransportStop()`
4. **Validate parameters**: Check ranges in `setParam()`

### UI Integration

1. **Use transport helpers**: `drawTransportButton(x, y, w, h, state)`
2. **Check state**: Use `clockRuntime.getState()` for UI feedback
3. **Request tempo changes**: Use `clockRuntime.requestTempo(bpm)`

### Testing

1. **Test quantization**: Verify start/stop at correct boundaries
2. **Test note-offs**: Ensure no hanging notes after stop/reset
3. **Test multi-slot**: Run multiple modules simultaneously
4. **Test external sync**: Verify with external MIDI clock

## Future Extensions

### Planned Features (v2+)

- **Continue (0xFB)** support for pause/resume
- **Variable time signatures** (3/4, 5/4, 7/8, etc.)
- **Advanced swing**: Per-module, per-track swing settings
- **Song mode**: Chain patterns, scene management
- **MIDI input**: Clock slave mode, note input for recording
- **Per-step modulation**: Probability, velocity curves, etc.

### Extraction Tasks

- Move slot management to separate `SlotEngine` class
- Add slot save/load system
- Implement slot copying and templates

## Troubleshooting

### No MIDI output

- Check `MidiOutBuffer` initialized: Look for "[MidiOutBuffer] Initialized"
- Verify BLE/Hardware MIDI connected
- Check module registered with `ClockRuntime`

### Timing drift

- Ensure `processTick()` called from clock callback
- Check for blocking code in `onStep()`
- Verify tick counts incrementing correctly

### Missing note-offs

- Check `onTransportStop()` implemented
- Verify `panic()` called on reset
- Use `note()` instead of manual on/off

### Build errors

- Include headers in correct order
- Link `midi_out_buffer.cpp`, `clock_runtime.cpp`, `clocked_module.cpp`
- Check C++11 features enabled (`-std=gnu++11`)

## References

- [MIDI Specification](https://www.midi.org/specifications)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

## License

See main project LICENSE file.
