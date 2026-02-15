# Clocked Module Framework - Test Specification

## Unit Tests (for future implementation)

This document outlines tests that should be implemented to validate the framework.

### MidiOutBuffer Tests

#### Test: Buffer Enqueueing
```cpp
TEST(MidiOutBuffer, EnqueueDequeue) {
  MidiOutBuffer buffer;
  buffer.init();
  
  // Enqueue note on
  ASSERT_TRUE(buffer.noteOn(0, 60, 100));
  ASSERT_EQ(buffer.getQueuedCount(), 1);
  
  // Allow task to process
  vTaskDelay(pdMS_TO_TICKS(10));
  
  // Should be empty after processing
  ASSERT_EQ(buffer.getQueuedCount(), 0);
  
  buffer.shutdown();
}
```

#### Test: Buffer Full
```cpp
TEST(MidiOutBuffer, BufferFull) {
  MidiOutBuffer buffer;
  buffer.init();
  
  // Fill buffer
  for (size_t i = 0; i < 255; ++i) {
    ASSERT_TRUE(buffer.noteOn(0, 60, 100));
  }
  
  // Next should fail
  ASSERT_FALSE(buffer.noteOn(0, 60, 100));
  
  buffer.shutdown();
}
```

#### Test: Scheduled Note-Offs
```cpp
TEST(MidiOutBuffer, ScheduledNoteOffs) {
  MidiOutBuffer buffer;
  buffer.init();
  
  uint32_t currentTick = 100;
  
  // Schedule note with 10-tick duration
  buffer.note(0, 60, 100, 10);
  
  // Should have 1 active note
  ASSERT_EQ(buffer.getActiveNoteCount(), 1);
  
  // Update without reaching off time
  buffer.updateScheduledNotes(105);
  ASSERT_EQ(buffer.getActiveNoteCount(), 1);
  
  // Update past off time
  buffer.updateScheduledNotes(111);
  ASSERT_EQ(buffer.getActiveNoteCount(), 0);
  
  buffer.shutdown();
}
```

#### Test: Panic
```cpp
TEST(MidiOutBuffer, Panic) {
  MidiOutBuffer buffer;
  buffer.init();
  
  // Add some scheduled notes
  buffer.note(0, 60, 100, 10);
  buffer.note(1, 62, 100, 10);
  ASSERT_EQ(buffer.getActiveNoteCount(), 2);
  
  // Panic should clear all
  buffer.panic();
  
  // Allow time for CC messages to process
  vTaskDelay(pdMS_TO_TICKS(10));
  
  ASSERT_EQ(buffer.getActiveNoteCount(), 0);
  
  buffer.shutdown();
}
```

### ClockRuntime Tests

#### Test: Transport State Machine
```cpp
TEST(ClockRuntime, StateMachine) {
  ClockRuntime runtime;
  runtime.init();
  
  // Initial state
  ASSERT_EQ(runtime.getState(), TransportState::STOPPED);
  
  // Request start
  runtime.requestStart();
  ASSERT_EQ(runtime.getState(), TransportState::PENDING_START);
  
  // Process ticks until start
  runtime.setStartQuantize(QuantizeMode::IMMEDIATE);
  runtime.processTick(1);
  ASSERT_EQ(runtime.getState(), TransportState::RUNNING);
  
  // Request stop
  runtime.requestStop();
  ASSERT_EQ(runtime.getState(), TransportState::PENDING_STOP);
  
  // Force stop
  runtime.forceStop();
  ASSERT_EQ(runtime.getState(), TransportState::STOPPED);
  
  runtime.shutdown();
}
```

#### Test: Quantization
```cpp
TEST(ClockRuntime, Quantization) {
  ClockRuntime runtime;
  runtime.init();
  
  // Set to bar quantization
  runtime.setStartQuantize(QuantizeMode::NEXT_BAR);
  runtime.requestStart();
  
  // Should be pending
  ASSERT_EQ(runtime.getState(), TransportState::PENDING_START);
  
  // Process ticks until bar start (96 ticks for 4/4)
  for (uint32_t i = 1; i < 96; ++i) {
    runtime.processTick(i);
    ASSERT_EQ(runtime.getState(), TransportState::PENDING_START);
  }
  
  // At bar start, should transition
  runtime.processTick(96);
  ASSERT_EQ(runtime.getState(), TransportState::RUNNING);
  
  runtime.shutdown();
}
```

#### Test: Module Registration
```cpp
TEST(ClockRuntime, ModuleRegistration) {
  ClockRuntime runtime;
  runtime.init();
  
  DrumSeqClocked module;
  
  int slotId = runtime.registerModule(&module, 0);
  ASSERT_GE(slotId, 0);
  
  // Should be able to retrieve module
  ClockedModule* retrieved = runtime.getModule(slotId);
  ASSERT_EQ(retrieved, &module);
  
  // Unregister
  runtime.unregisterModule(slotId);
  ASSERT_EQ(runtime.getModule(slotId), nullptr);
  
  runtime.shutdown();
}
```

#### Test: Step Dispatch
```cpp
class TestModule : public ClockedModule {
public:
  int stepCount = 0;
  
  const char* typeId() const override { return "test"; }
  const char* displayName() const override { return "Test"; }
  void init() override {}
  void reset() override { stepCount = 0; }
  uint16_t ticksPerStep() const override { return 6; }
  
  void onStep(const StepContext& ctx) override {
    stepCount++;
  }
  
  void setParam(uint16_t, int32_t) override {}
  int32_t getParam(uint16_t) const override { return 0; }
};

TEST(ClockRuntime, StepDispatch) {
  ClockRuntime runtime;
  runtime.init();
  
  TestModule module;
  int slotId = runtime.registerModule(&module, 0);
  
  runtime.setStartQuantize(QuantizeMode::IMMEDIATE);
  runtime.requestStart();
  
  // Process 24 ticks (1 quarter note)
  for (uint32_t i = 1; i <= 24; ++i) {
    runtime.processTick(i);
  }
  
  // Should have received 4 steps (1/16 notes)
  ASSERT_EQ(module.stepCount, 4);
  
  runtime.shutdown();
}
```

#### Test: BPM Management
```cpp
TEST(ClockRuntime, BPMManagement) {
  ClockRuntime runtime;
  runtime.init();
  
  // Default BPM
  ASSERT_EQ(runtime.getBPM(), 120);
  
  // Change BPM
  runtime.requestTempo(140);
  ASSERT_EQ(runtime.getBPM(), 140);
  
  // Test clamping
  runtime.requestTempo(20);  // Below min
  ASSERT_EQ(runtime.getBPM(), 40);
  
  runtime.requestTempo(400);  // Above max
  ASSERT_EQ(runtime.getBPM(), 300);
  
  runtime.shutdown();
}
```

### ModuleFactory Tests

#### Test: Registration and Creation
```cpp
TEST(ModuleFactory, RegisterAndCreate) {
  // DrumSeqClocked auto-registers via REGISTER_MODULE
  
  ClockedModule* module = ModuleFactory::instance().create("drum_seq_clocked");
  ASSERT_NE(module, nullptr);
  ASSERT_STREQ(module->typeId(), "drum_seq_clocked");
  
  delete module;
}
```

#### Test: Unknown Type
```cpp
TEST(ModuleFactory, UnknownType) {
  ClockedModule* module = ModuleFactory::instance().create("nonexistent");
  ASSERT_EQ(module, nullptr);
}
```

### DrumSeqClocked Tests

#### Test: Pattern Toggle
```cpp
TEST(DrumSeqClocked, PatternToggle) {
  DrumSeqClocked module;
  module.init();
  
  // Initially all off
  ASSERT_FALSE(module.getStep(0, 0));
  
  // Toggle on
  module.toggleStep(0, 0);
  ASSERT_TRUE(module.getStep(0, 0));
  
  // Toggle off
  module.toggleStep(0, 0);
  ASSERT_FALSE(module.getStep(0, 0));
}
```

#### Test: Clear All
```cpp
TEST(DrumSeqClocked, ClearAll) {
  DrumSeqClocked module;
  module.init();
  
  // Set some steps
  module.toggleStep(0, 0);
  module.toggleStep(1, 5);
  module.toggleStep(2, 10);
  
  ASSERT_TRUE(module.getStep(0, 0));
  ASSERT_TRUE(module.getStep(1, 5));
  
  // Clear all
  module.clearAll();
  
  ASSERT_FALSE(module.getStep(0, 0));
  ASSERT_FALSE(module.getStep(1, 5));
  ASSERT_FALSE(module.getStep(2, 10));
}
```

#### Test: Step Playback
```cpp
TEST(DrumSeqClocked, StepPlayback) {
  DrumSeqClocked module;
  module.init();
  
  // Create a pattern: kick on 0, 4, 8, 12
  for (size_t i = 0; i < 16; i += 4) {
    module.toggleStep(0, i);
  }
  
  // Simulate steps
  StepContext ctx;
  ctx.tick = 0;
  ctx.stepIndex = 0;
  ctx.ticksPerStep = 6;
  
  for (size_t i = 0; i < 16; ++i) {
    ctx.stepIndex = i;
    ctx.tick = i * 6;
    module.onStep(ctx);
    // Would need to verify MIDI output here
  }
}
```

#### Test: Serialization
```cpp
TEST(DrumSeqClocked, Serialization) {
  DrumSeqClocked module1;
  module1.init();
  
  // Set pattern
  module1.toggleStep(0, 0);
  module1.toggleStep(1, 5);
  module1.setParam(PARAM_VELOCITY, 90);
  
  // Serialize
  uint8_t buffer[256];
  size_t size;
  module1.serialize(buffer, sizeof(buffer), size);
  ASSERT_GT(size, 0);
  
  // Create new module and deserialize
  DrumSeqClocked module2;
  module2.init();
  ASSERT_TRUE(module2.deserialize(buffer, size));
  
  // Verify state
  ASSERT_TRUE(module2.getStep(0, 0));
  ASSERT_TRUE(module2.getStep(1, 5));
  ASSERT_EQ(module2.getParam(PARAM_VELOCITY), 90);
}
```

## Integration Tests

### Test: Full Framework Integration

```cpp
TEST(Integration, FullWorkflow) {
  // Initialize framework
  midiOutBuffer.init();
  clockRuntime.init();
  
  // Create and register module
  DrumSeqClocked* module = new DrumSeqClocked();
  int slotId = clockRuntime.registerModule(module, 0);
  
  // Set up pattern
  module->toggleStep(0, 0);
  module->toggleStep(0, 4);
  module->toggleStep(0, 8);
  module->toggleStep(0, 12);
  
  // Start transport
  clockRuntime.setStartQuantize(QuantizeMode::IMMEDIATE);
  clockRuntime.requestStart();
  
  // Simulate 96 ticks (1 bar in 4/4)
  for (uint32_t tick = 1; tick <= 96; ++tick) {
    clockRuntime.processTick(tick);
    midiOutBuffer.updateScheduledNotes(tick);
  }
  
  // Stop transport
  clockRuntime.requestStop();
  clockRuntime.processTick(97);
  
  // Cleanup
  clockRuntime.unregisterModule(slotId);
  delete module;
  
  clockRuntime.shutdown();
  midiOutBuffer.shutdown();
  
  // Verify no hanging notes
  ASSERT_EQ(midiOutBuffer.getActiveNoteCount(), 0);
}
```

### Test: Multi-Module Slot System

```cpp
TEST(Integration, MultiModule) {
  midiOutBuffer.init();
  clockRuntime.init();
  
  // Create two modules
  DrumSeqClocked* drums = new DrumSeqClocked();
  TestModule* test = new TestModule();
  
  int drumSlot = clockRuntime.registerModule(drums, 0);
  int testSlot = clockRuntime.registerModule(test, 1);
  
  // Start transport
  clockRuntime.setStartQuantize(QuantizeMode::IMMEDIATE);
  clockRuntime.requestStart();
  
  // Run for 24 ticks
  for (uint32_t tick = 1; tick <= 24; ++tick) {
    clockRuntime.processTick(tick);
  }
  
  // Both should have received steps
  ASSERT_GT(test->stepCount, 0);
  
  // Test mute
  clockRuntime.setModuleMute(drumSlot, true);
  test->stepCount = 0;
  
  for (uint32_t tick = 25; tick <= 48; ++tick) {
    clockRuntime.processTick(tick);
  }
  
  // Test module still receives steps when muted
  ASSERT_GT(test->stepCount, 0);
  
  // Cleanup
  clockRuntime.shutdown();
  midiOutBuffer.shutdown();
  
  delete drums;
  delete test;
}
```

## Manual Test Procedures

### Manual Test 1: Basic Playback

1. Flash firmware to device
2. Navigate to test module
3. Set some pattern steps
4. Press PLAY
5. Verify:
   - ✅ MIDI notes output correctly
   - ✅ Visual feedback shows current step
   - ✅ Timing is correct at various BPMs

### Manual Test 2: Transport Controls

1. Press PLAY
2. Verify state transitions:
   - STOPPED → PENDING_START (if quantized)
   - PENDING_START → RUNNING (at bar start)
3. Press STOP while running
4. Verify:
   - RUNNING → PENDING_STOP (if quantized)
   - PENDING_STOP → STOPPED (at bar end)
   - No hanging notes after stop

### Manual Test 3: BPM Changes

1. Start playback at 120 BPM
2. Change to 140 BPM
3. Verify timing adjusts immediately
4. Change to 60 BPM
5. Verify still playing correctly

### Manual Test 4: External Clock Sync

1. Set clock master to HARDWARE or BLE
2. Send external MIDI clock
3. Verify module syncs to external clock
4. Stop external clock
5. Verify module stops cleanly

## Performance Benchmarks

### Benchmark: Tick Processing Time

Target: <1ms per tick at 120 BPM

```cpp
void benchmark_tick_processing() {
  uint32_t start = micros();
  
  for (int i = 0; i < 1000; ++i) {
    clockRuntime.processTick(i);
  }
  
  uint32_t elapsed = micros() - start;
  Serial.printf("Avg tick time: %lu us\n", elapsed / 1000);
}
```

### Benchmark: MIDI Output Latency

Target: <5ms from onStep() to wire

```cpp
void benchmark_midi_latency() {
  // Measure time from midiOutBuffer.noteOn() to actual transmission
  // Requires logic analyzer or oscilloscope on MIDI output
}
```

## Test Results Template

```
Framework Version: v1.0.0
Test Date: YYYY-MM-DD
Hardware: ESP32-2432S028Rv2
Build Config: default

MidiOutBuffer Tests:
  [PASS] Enqueue/Dequeue
  [PASS] Buffer Full
  [PASS] Scheduled Note-Offs
  [PASS] Panic

ClockRuntime Tests:
  [PASS] State Machine
  [PASS] Quantization
  [PASS] Module Registration
  [PASS] Step Dispatch
  [PASS] BPM Management

ModuleFactory Tests:
  [PASS] Register and Create
  [PASS] Unknown Type

DrumSeqClocked Tests:
  [PASS] Pattern Toggle
  [PASS] Clear All
  [PASS] Step Playback
  [PASS] Serialization

Integration Tests:
  [PASS] Full Workflow
  [PASS] Multi-Module

Manual Tests:
  [PASS] Basic Playback
  [PASS] Transport Controls
  [PASS] BPM Changes
  [PASS] External Clock Sync

Performance:
  Tick processing: 0.3ms avg
  MIDI latency: 2.1ms avg

Overall: PASS
```
