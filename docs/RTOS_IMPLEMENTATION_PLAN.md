# RTOS Implementation Plan for Real-Time Touch-to-MIDI Performance

## Overview

This document outlines a plan to implement FreeRTOS task prioritization for aCYD-MIDI to achieve truly real-time touch-to-MIDI response with guaranteed latency bounds.

## Current Architecture

The current implementation uses a single-threaded event loop:
```
loop() {
  lv_timer_handler()
  updateTouch()
  handleMode()  // MIDI + UI updates
  processRedraw()
}
```

**Current optimizations:**
- Removed unconditional redraws (✓ completed)
- Added dirty flag system for batched redraws (✓ completed)
- MIDI sent before visual updates in critical paths (✓ completed)

**Remaining limitations:**
- LVGL timer handler can block the loop
- No priority separation between MIDI and UI
- Jitter from variable loop timing
- Display updates can delay MIDI processing

## Proposed RTOS Architecture

### Task Structure

**Task 1: MIDI Processing (Priority: HIGH - 3)**
- Process touch input
- Calculate and send MIDI messages
- Update internal state
- No display operations
- Target: <5ms latency from touch to MIDI

**Task 2: UI Rendering (Priority: NORMAL - 1)**
- Process LVGL timer handler
- Handle display redraws
- Update visual indicators
- Can be preempted by MIDI task
- Target: 30-60 FPS visual updates

**Task 3: BLE Management (Priority: MEDIUM - 2)**
- Handle BLE callbacks
- Manage connection state
- Process incoming MIDI (if needed)

### Implementation Steps

#### Phase 1: Proof of Concept
1. Create separate task for MIDI processing
2. Move touch reading and MIDI sending to high-priority task
3. Keep existing UI code in default task
4. Measure latency improvements

#### Phase 2: Inter-task Communication
1. Implement lock-free FIFO queue for touch events
2. Use FreeRTOS queue for state updates
3. Add semaphores for shared resources (display state)

#### Phase 3: Optimization
1. Fine-tune task priorities and stack sizes
2. Optimize queue depths
3. Add latency monitoring/telemetry
4. Benchmark against single-threaded version

#### Phase 4: Polish
1. Add configurable task priorities (for different use cases)
2. Document real-time guarantees
3. Update user guide with performance characteristics

## Code Structure

### Example Task Implementation

```cpp
// High-priority MIDI task
void midiTask(void *parameter) {
  TouchEvent event;
  const TickType_t xDelay = pdMS_TO_TICKS(1); // 1ms loop
  
  while (1) {
    // Read touch with minimal overhead
    if (readTouchEvent(&event)) {
      // Process and send MIDI immediately
      processMIDI(event);
      
      // Queue state update for UI task
      xQueueSend(uiUpdateQueue, &event, 0);
    }
    
    vTaskDelay(xDelay);
  }
}

// Normal-priority UI task
void uiTask(void *parameter) {
  TouchEvent event;
  
  while (1) {
    // Handle LVGL timer
    lv_timer_handler();
    
    // Check for state updates from MIDI task
    if (xQueueReceive(uiUpdateQueue, &event, 0)) {
      updateDisplay(event);
    }
    
    // Process any pending redraws
    processRedraw();
    
    vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS
  }
}
```

### Shared State Management

```cpp
// Thread-safe state for critical data using atomics
// Remember to `#include <atomic>` when applying this pattern in C++ code.
struct MIDIState {
  std::atomic<int> currentNote;
  std::atomic<int> currentVelocity;
  std::atomic<bool> noteActive;
};

// UI state (protected by mutex if needed)
struct UIState {
  int selectedScale;
  int selectedKey;
  int octave;
  // etc.
};
```

## Expected Benefits

### Latency Reduction
- **Current**: 16-50ms (variable, depends on display updates)
- **Target with RTOS**: <5ms (bounded, guaranteed by task priority)

### Predictability
- Deterministic worst-case latency
- No interference from display rendering
- Consistent performance regardless of visual complexity

### Responsiveness
- Touch feels immediate
- No lag spikes during complex animations
- Suitable for fast playing techniques (arpeggios, trills, etc.)

## Testing Plan

### Metrics to Measure
1. **Touch-to-MIDI latency**: Time from touch detection to MIDI packet sent
2. **Jitter**: Variation in latency over time
3. **Throughput**: Maximum notes per second
4. **UI frame rate**: Display updates per second
5. **CPU utilization**: Per-task CPU usage

### Test Scenarios
1. Rapid note playing on keyboard mode
2. Fast XY pad movements
3. Complex animations (bouncing ball) during MIDI input
4. Maximum polyphony stress test
5. Long-running stability test

## Rollout Strategy

### Compatibility
- Make RTOS implementation optional via build flag
- Keep single-threaded mode for backwards compatibility
- Default to RTOS for new builds

### Configuration
```ini
# platformio.ini
build_flags = 
    -D USE_RTOS_TASKS=1
    -D MIDI_TASK_PRIORITY=3
    -D UI_TASK_PRIORITY=1
    -D MIDI_TASK_STACK_SIZE=4096
    -D UI_TASK_STACK_SIZE=8192
```

## Resources & References

- [ESP-IDF FreeRTOS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [FreeRTOS Task Priorities](https://www.freertos.org/RTOS-task-priorities.html)
- [ESP32 Arduino FreeRTOS Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples)
- Real-time audio processing best practices

## Next Steps

1. Create feature branch: `feature/rtos-midi-tasks`
2. Implement proof-of-concept with basic task separation
3. Measure baseline latency with current implementation
4. Compare RTOS vs single-threaded latency
5. Iterate on implementation based on measurements
6. Document final performance characteristics

## Success Criteria

- [ ] Touch-to-MIDI latency <5ms (99th percentile)
- [ ] Jitter <1ms
- [ ] UI maintains 30+ FPS during active MIDI
- [ ] No regression in functionality
- [ ] Code remains maintainable and documented
- [ ] Optional build flag for backwards compatibility

---

**Status**: Planning phase  
**Priority**: High (performance-critical for musical use)  
**Estimated Effort**: 2-3 weeks development + testing  
**Risk Level**: Medium (requires careful synchronization)
