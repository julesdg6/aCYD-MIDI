# Migration Guide: Legacy Modules → ClockedModule Framework

This guide explains how to migrate existing sequencer/generative modules from the legacy `SequencerSyncState` pattern to the new `ClockedModule` framework.

## Why Migrate?

**Benefits:**
- ✅ Centralized transport management (no per-module clock logic)
- ✅ Precise timing with no drift
- ✅ Automatic note-off management
- ✅ Consistent UI behavior across modules
- ✅ Thread-safe MIDI output
- ✅ Support for multiple concurrent modules (slot system)
- ✅ Standardized serialization/parameter API

**When to migrate:**
- Adding new sequencer/generative features
- Fixing timing issues in existing modules
- Implementing multi-module/scene support
- Preparing for song mode or advanced features

## Migration Steps

### 1. Understand Current Implementation

Typical legacy module structure:

```cpp
// Old pattern (module_sequencer_mode.cpp)
static SequencerSyncState sequencerSync;
static int currentStep = 0;
static unsigned long noteOffTime[NUM_TRACKS];

void updateSequencer() {
  bool wasPlaying = sequencerSync.playing;
  bool justStarted = sequencerSync.tryStartIfReady(!instantStartMode) && !wasPlaying;
  
  if (justStarted) {
    currentStep = 0;
  }
  
  if (!sequencerSync.playing) {
    return;
  }
  
  uint32_t readySteps = sequencerSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);
  
  for (uint32_t i = 0; i < readySteps; ++i) {
    playSequencerStep();
    currentStep = (currentStep + 1) % NUM_STEPS;
  }
}
```

### 2. Create Module Header

Create `include/module_<name>_clocked.h`:

```cpp
#ifndef MODULE_<NAME>_CLOCKED_H
#define MODULE_<NAME>_CLOCKED_H

#include "clocked_module.h"

class <Name>Clocked : public ClockedModule {
public:
  <Name>Clocked();
  virtual ~<Name>Clocked();
  
  // ClockedModule interface
  const char* typeId() const override { return "<name>_clocked"; }
  const char* displayName() const override { return "<Display Name>"; }
  
  void init() override;
  void reset() override;
  void onTransportStart() override;
  void onTransportStop() override;
  
  uint16_t ticksPerStep() const override { return 6; }  // Adjust as needed
  
  void onStep(const StepContext& ctx) override;
  
  void setParam(uint16_t paramId, int32_t value) override;
  int32_t getParam(uint16_t paramId) const override;
  
  void serialize(uint8_t* buffer, size_t maxSize, size_t& outSize) const override;
  bool deserialize(const uint8_t* buffer, size_t size) override;
  
  // Module-specific public API (for UI)
  // ...
  
private:
  // Module state
  // ...
};

#endif
```

### 3. Implement Module Class

Create `src/module_<name>_clocked.cpp`:

```cpp
#include "module_<name>_clocked.h"
#include "midi_out_buffer.h"
#include <Arduino.h>

// Register with factory
REGISTER_MODULE(<Name>Clocked, "<name>_clocked")

<Name>Clocked::<Name>Clocked() {
  // Initialize state
}

<Name>Clocked::~<Name>Clocked() {
}

void <Name>Clocked::init() {
  Serial.println("[<Name>Clocked] init()");
  reset();
}

void <Name>Clocked::reset() {
  Serial.println("[<Name>Clocked] reset()");
  // Reset playhead and state
}

void <Name>Clocked::onTransportStart() {
  Serial.println("[<Name>Clocked] onTransportStart()");
  // Reset step counter, clear gates, etc.
}

void <Name>Clocked::onTransportStop() {
  Serial.println("[<Name>Clocked] onTransportStop()");
  // Send any pending note-offs
  // midiOutBuffer handles panic, but module-specific cleanup here
}

void <Name>Clocked::onStep(const StepContext& ctx) {
  // This is where the magic happens!
  // Replace old updateSequencer() logic here
  
  size_t stepIndex = ctx.stepIndex % numSteps_;
  
  if (pattern_[stepIndex]) {
    midiOutBuffer.note(channel_, noteNumber, velocity_, gateLength_);
  }
}

void <Name>Clocked::setParam(uint16_t paramId, int32_t value) {
  // Handle parameter changes
}

int32_t <Name>Clocked::getParam(uint16_t paramId) const {
  // Return parameter values
  return 0;
}

void <Name>Clocked::serialize(uint8_t* buffer, size_t maxSize, size_t& outSize) const {
  // Optional: save state
}

bool <Name>Clocked::deserialize(const uint8_t* buffer, size_t size) {
  // Optional: restore state
  return true;
}
```

### 4. Convert Timing Logic

**Old pattern:**
```cpp
uint32_t readySteps = sequencerSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);

for (uint32_t i = 0; i < readySteps; ++i) {
  playSequencerStep();
  currentStep = (currentStep + 1) % NUM_STEPS;
}
```

**New pattern:**
```cpp
void MyModule::onStep(const StepContext& ctx) {
  // ctx.stepIndex is monotonically increasing
  size_t currentStep = ctx.stepIndex % NUM_STEPS;
  
  // Play step
  if (pattern_[currentStep]) {
    midiOutBuffer.note(0, noteNumber, velocity, gateLength);
  }
}
```

**Key differences:**
- No manual step counting (`ctx.stepIndex` provided)
- No loop over multiple steps (called once per step)
- No `consumeReadySteps()` needed

### 5. Convert MIDI Output

**Old pattern:**
```cpp
sendMIDI(0x90, note, velocity);
noteOffTime[track] = millis() + duration_ms;

// Later, in update loop:
if (noteOffTime[track] > 0 && millis() >= noteOffTime[track]) {
  sendMIDI(0x80, note, 0);
  noteOffTime[track] = 0;
}
```

**New pattern:**
```cpp
// Automatic note-off after duration (in ticks)
midiOutBuffer.note(channel, note, velocity, durationTicks);

// Or manual control:
midiOutBuffer.noteOn(channel, note, velocity);
// ... later ...
midiOutBuffer.noteOff(channel, note);
```

**Key differences:**
- No manual note-off timers
- Time in ticks instead of milliseconds
- Thread-safe enqueueing

### 6. Update Transport Control

**Old pattern:**
```cpp
if (touch.justPressed && isButtonPressed(playButtonX, playButtonY, w, h)) {
  if (sequencerSync.playing) {
    sequencerSync.stopPlayback();
  } else {
    sequencerSync.requestStart();
  }
}
```

**New pattern:**
```cpp
// Module doesn't manage transport directly
// Instead, it's registered with ClockRuntime

// In UI code (not in module):
if (touch.justPressed && isButtonPressed(playButtonX, playButtonY, w, h)) {
  if (clockRuntime.isRunning()) {
    clockRuntime.requestStop();
  } else {
    clockRuntime.requestStart();
  }
}

// Module automatically receives callbacks:
// - onTransportStart()
// - onStep(ctx)
// - onTransportStop()
```

### 7. Update UI Code

**Old pattern:**
```cpp
void drawSequencerMode() {
  const char* label;
  uint16_t color;
  
  if (sequencerSync.playing) {
    label = "STOP";
    color = THEME_ERROR;
  } else if (sequencerSync.startPending) {
    label = "PENDING";
    color = THEME_SECONDARY;
  } else {
    label = "PLAY";
    color = THEME_SUCCESS;
  }
  
  drawRoundButton(x, y, w, h, label, color);
}
```

**New pattern:**
```cpp
#include "ui_elements.h"

void drawSequencerMode() {
  TransportState state = clockRuntime.getState();
  drawTransportButton(x, y, w, h, state);
}
```

### 8. Module Registration

Add to mode initialization:

```cpp
// In initializeSequencerMode() or similar
static DrumSeqClocked* drumModule = nullptr;
static int drumSlotId = -1;

void initializeSequencerMode() {
  if (drumModule == nullptr) {
    drumModule = new DrumSeqClocked();
    drumSlotId = clockRuntime.registerModule(drumModule, 0);  // Channel 0
  }
  
  // Or create from factory:
  // drumModule = static_cast<DrumSeqClocked*>(
  //   ModuleFactory::instance().create("drum_seq_clocked")
  // );
}

// In cleanup/exit
void cleanupSequencerMode() {
  if (drumSlotId >= 0) {
    clockRuntime.unregisterModule(drumSlotId);
    drumSlotId = -1;
  }
  delete drumModule;
  drumModule = nullptr;
}
```

## Common Patterns

### Pattern: Step Probability

**Old:**
```cpp
if (random(100) < probability[step]) {
  sendMIDI(0x90, note, velocity);
}
```

**New:**
```cpp
void onStep(const StepContext& ctx) {
  size_t step = ctx.stepIndex % numSteps_;
  if (random(100) < probability_[step]) {
    midiOutBuffer.note(channel_, note, velocity_, gateLength_);
  }
}
```

### Pattern: Parameter Changes

**Old:**
```cpp
if (touch.justPressed && isButtonPressed(...)) {
  velocity = newValue;
}
```

**New:**
```cpp
// In UI code:
if (touch.justPressed && isButtonPressed(...)) {
  module->setParam(PARAM_VELOCITY, newValue);
}

// In module:
void MyModule::setParam(uint16_t paramId, int32_t value) {
  if (paramId == PARAM_VELOCITY) {
    velocity_ = static_cast<uint8_t>(value & 0x7F);
  }
}
```

### Pattern: Multi-track Sequencer

**Old:**
```cpp
for (int track = 0; track < NUM_TRACKS; track++) {
  if (pattern[track][step]) {
    sendMIDI(0x90, notes[track], velocity);
  }
}
```

**New:**
```cpp
void onStep(const StepContext& ctx) {
  size_t step = ctx.stepIndex % kNumSteps;
  
  for (size_t track = 0; track < kNumTracks; track++) {
    if (pattern_[track][step]) {
      midiOutBuffer.note(channel_, notes_[track], velocity_, gateLength_);
    }
  }
}
```

### Pattern: Euclidean Rhythm

**Old:**
```cpp
bool euclideanPattern[steps];
generateEuclidean(steps, hits, rotation, euclideanPattern);

if (euclideanPattern[currentStep]) {
  sendMIDI(0x90, note, velocity);
}
```

**New:**
```cpp
void onStep(const StepContext& ctx) {
  size_t step = (ctx.stepIndex + rotation_) % steps_;
  
  if (euclideanPattern_[step]) {
    midiOutBuffer.note(channel_, note_, velocity_, gateLength_);
  }
}

// Regenerate pattern when parameters change
void setParam(uint16_t paramId, int32_t value) {
  if (paramId == PARAM_STEPS || paramId == PARAM_HITS) {
    generateEuclidean(steps_, hits_, rotation_, euclideanPattern_);
  }
}
```

## Testing Checklist

After migration, verify:

- [ ] Module initializes without errors
- [ ] Transport start/stop works correctly
- [ ] Quantization modes work (IMMEDIATE, NEXT_BAR, etc.)
- [ ] Steps trigger at correct timing
- [ ] MIDI notes output correctly
- [ ] No hanging notes after stop
- [ ] BPM changes apply correctly
- [ ] UI shows correct transport state
- [ ] Multiple instances can run simultaneously (if slot system used)
- [ ] Serialization/deserialization works (if implemented)

## Troubleshooting

### "Module not receiving onStep() calls"

**Possible causes:**
1. Module not registered: Check `clockRuntime.registerModule()` called
2. Module disabled: Check `enabled` flag in slot
3. Transport not running: Check `clockRuntime.getState()`
4. Wrong ticksPerStep: Verify `ticksPerStep()` divides 24 evenly

**Debug:**
```cpp
void onStep(const StepContext& ctx) {
  Serial.printf("[MyModule] onStep tick=%u stepIndex=%u\n", ctx.tick, ctx.stepIndex);
  // ...
}
```

### "Timing is wrong"

**Possible causes:**
1. Wrong `ticksPerStep()`: Check resolution matches intent
2. Step index math error: Use `ctx.stepIndex % numSteps`
3. Bar/beat calculation error: Use provided context fields

**Debug:**
```cpp
void onStep(const StepContext& ctx) {
  Serial.printf("tick=%u bar=%u tickInBar=%u stepInBar=%u\n",
                ctx.tick, ctx.barIndex, ctx.tickInBar, ctx.stepInBar);
}
```

### "Notes keep playing after stop"

**Possible causes:**
1. Not implementing `onTransportStop()`
2. Not using `midiOutBuffer.note()` with duration

**Fix:**
```cpp
void onTransportStop() {
  // Send explicit note-offs for active notes
  for (auto note : activeNotes_) {
    midiOutBuffer.noteOff(channel_, note);
  }
  activeNotes_.clear();
}
```

## Performance Tips

1. **Keep onStep() fast**: Avoid heavy computation
2. **Pre-compute patterns**: Generate in `setParam()`, not `onStep()`
3. **Use scheduled notes**: Prefer `note()` over manual timers
4. **Batch MIDI**: Send multiple notes in same callback if needed
5. **Avoid allocations**: Use fixed-size arrays, not dynamic allocation

## Example: Complete Migration

See `module_drum_seq_clocked.h/cpp` for a complete reference implementation.

Key files:
- `/include/module_drum_seq_clocked.h` - Header with interface
- `/src/module_drum_seq_clocked.cpp` - Implementation
- `/docs/CLOCKED_MODULE_FRAMEWORK.md` - Architecture documentation

## Getting Help

If you encounter issues during migration:

1. Review `DrumSeqClocked` reference implementation
2. Check framework documentation
3. Enable debug output in `onStep()`
4. Test with single module before multi-slot
5. Verify MIDI output with external monitor

## Next Steps

After successful migration:

1. Test thoroughly on hardware
2. Update module documentation
3. Consider adding new features (probability, humanization, etc.)
4. Contribute improvements back to framework

---

**Note**: Legacy modules will continue to work during migration period. The framework is designed to coexist with old patterns until all modules are migrated.
