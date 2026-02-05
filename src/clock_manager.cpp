#include "clock_manager.h"
#include <Arduino.h>

#include "common_definitions.h"
#include "midi_utils.h"

#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <uClock.h>

namespace {
struct RunningStateDelta {
  bool sendStart = false;
  bool sendStop = false;
  bool running = false;
  int pendingStarts = 0;
  int activeSequencers = 0;
};

static volatile uint32_t tickCount = 0;
static volatile bool tickPending = false;
static uint32_t lastTickTime = 0;
static volatile int pendingStarts = 0;
static volatile int activeSequencers = 0;
static volatile bool running = false;
static volatile bool externalClockActive = false;
static volatile bool uClockRunning = false;  // Track uClock running state
static volatile bool tickCountersReset = false;  // Track when resetCounters() is called
static volatile bool clockStarted = false;  // ISR flag for clock start
static volatile bool clockStopped = false;  // ISR flag for clock stop
// Aggregated tick statistics (ISR-updated, main-loop printed)
// (tick sampling moved to main-loop to avoid ISR work)
static portMUX_TYPE clockManagerMux = portMUX_INITIALIZER_UNLOCKED;
static const char *const kMasterNames[] = {"INTERNAL", "WIFI", "BLE", "HARDWARE"};

// Forward declarations
static inline void lockClockManager();
static inline void unlockClockManager();
static inline void lockClockManagerFromISR();
static inline void unlockClockManagerFromISR();

// uClock callback function
static void onClockTickCallback(uint32_t tick) {
  // Use the tick value provided by uClock to avoid double-counting
  // Minimal ISR: record tick and mark pending for main loop processing.
  lockClockManagerFromISR();
  tickCount = tick;
  tickPending = true;
  unlockClockManagerFromISR();
}

static void onClockStartCallback() {
  // Called when uClock starts - ISR-safe: just set a flag
  lockClockManagerFromISR();
  clockStarted = true;
  unlockClockManagerFromISR();
}

static void onClockStopCallback() {
  // Called when uClock stops - ISR-safe: just set a flag
  lockClockManagerFromISR();
  clockStopped = true;
  unlockClockManagerFromISR();
}

static inline void lockClockManager() {
  portENTER_CRITICAL(&clockManagerMux);
}

static inline void unlockClockManager() {
  portEXIT_CRITICAL(&clockManagerMux);
}

static inline void lockClockManagerFromISR() {
  portENTER_CRITICAL_ISR(&clockManagerMux);
}

static inline void unlockClockManagerFromISR() {
  portEXIT_CRITICAL_ISR(&clockManagerMux);
}

static RunningStateDelta updateRunningStateLocked() {
  RunningStateDelta delta;
  delta.running = running;
  delta.pendingStarts = pendingStarts;
  delta.activeSequencers = activeSequencers;
  bool hasSequencerInterest = (pendingStarts > 0 || activeSequencers > 0);
  bool shouldRun = false;
  if (midiClockMaster == CLOCK_INTERNAL) {
    shouldRun = hasSequencerInterest;
  } else {
    shouldRun = externalClockActive && hasSequencerInterest;
  }
  if (shouldRun == running) {
    return delta;
  }
  bool wasRunning = running;
  running = shouldRun;
  delta.running = running;
  delta.pendingStarts = pendingStarts;
  delta.activeSequencers = activeSequencers;
  if (midiClockMaster == CLOCK_INTERNAL) {
    if (running && !wasRunning) {
      delta.sendStart = true;
    } else if (!running && wasRunning) {
      delta.sendStop = true;
    }
  }
  return delta;
}

static RunningStateDelta updateRunningState() {
  lockClockManager();
  RunningStateDelta delta = updateRunningStateLocked();
  unlockClockManager();
  if (delta.sendStart) {
    Serial.printf("[ClockManager] INTERNAL START – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStart();
  } else if (delta.sendStop) {
    Serial.printf("[ClockManager] INTERNAL STOP – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStop();
  }
  return delta;
}
}  // namespace

void initClockManager() {
  lockClockManager();
  tickCount = 0;
  lastTickTime = 0;
  pendingStarts = 0;
  activeSequencers = 0;
  running = false;
  externalClockActive = false;
  uClockRunning = false;
  tickCountersReset = false;
  clockStarted = false;
  clockStopped = false;
  unlockClockManager();
  
  // Initialize uClock
  uClock.init();
  // Configure PPQN before setting tempo so tempo calculation uses correct resolution.
  uClock.setOutputPPQN(uClock.PPQN_24);
  uClock.setOnOutputPPQN(onClockTickCallback);
  uClock.setOnClockStart(onClockStartCallback);
  uClock.setOnClockStop(onClockStopCallback);
  uClock.setTempo(120.0);
  
  Serial.printf("[ClockManager] uClock initialized tempo=%.1f PPQN=%u\n", uClock.getTempo(),
                static_cast<unsigned>(uClock.PPQN_24));
}

static uint16_t clampBpm(uint16_t bpm) {
  if (bpm < 40U) {
    return 40U;
  }
  if (bpm > 240U) {
    return 240U;
  }
  return bpm;
}

void updateClockManager() {
  RunningStateDelta delta = updateRunningState();
  
  // Handle ISR flags for clock start/stop events (moved from ISR to task context)
  bool shouldPrintStart = false;
  bool shouldPrintStop = false;
  lockClockManager();
  if (clockStarted) {
    clockStarted = false;
    shouldPrintStart = true;
  }
  if (clockStopped) {
    clockStopped = false;
    shouldPrintStop = true;
  }
  unlockClockManager();
  
  if (shouldPrintStart) {
    Serial.println("[uClock] Clock started");
  }
  if (shouldPrintStop) {
    Serial.println("[uClock] Clock stopped");
  }
  
  // Update uClock tempo if BPM has changed
  uint16_t bpm = clampBpm(sharedBPM);
  float currentTempo = uClock.getTempo();
  if (fabsf(currentTempo - (float)bpm) > 0.1f) {
    uClock.setTempo((float)bpm);
  }
  
  // Start or stop uClock based on running state.
  // Protect reading/writing of `uClockRunning` and calls to uClock
  // with the same clock manager lock to avoid TOCTOU races.
  if (midiClockMaster == CLOCK_INTERNAL) {
    bool needStart = false;
    bool needStop = false;
    lockClockManager();
    bool shouldRun = running; // read protected state
    if (shouldRun && !uClockRunning) {
      needStart = true;
    } else if (!shouldRun && uClockRunning) {
      needStop = true;
    }
    unlockClockManager();

    if (needStart) {
      uClock.start();
      lockClockManager();
      uClockRunning = true;
      unlockClockManager();
    } else if (needStop) {
      // Set uClockRunning to false first to prevent new ticks from being processed,
      // then call uClock.stop() to halt the clock
      lockClockManager();
      uClockRunning = false;
      unlockClockManager();
      uClock.stop();
    }
  } else {
    // If master is external, ensure uClock is not left running (prevents
    // internal uClock and external clock both advancing tick counters).
    bool needStop = false;
    lockClockManager();
    if (uClockRunning) {
      needStop = true;
    }
    unlockClockManager();
    if (needStop) {
      // Set uClockRunning to false first to prevent new ticks from being processed,
      // then call uClock.stop() to halt the clock
      lockClockManager();
      uClockRunning = false;
      unlockClockManager();
      uClock.stop();
      Serial.println("[ClockManager] uClock stopped because master is not INTERNAL");
    }
  }
  
  // Handle tick counter reset flag
  static uint32_t lastProcessedTick = 0;
  bool resetDetected = false;
  lockClockManager();
  if (tickCountersReset) {
    tickCountersReset = false;
    resetDetected = true;
  }
  unlockClockManager();
  
  if (resetDetected) {
    lastProcessedTick = 0;
  }
  
  // Handle any pending ticks (deferred from ISR). Do this after start/stop logic
  // so heavy work runs in task context and won't trigger the interrupt WDT.
  // Process ticks with wrap/reset detection.
  while (true) {
    bool pending = false;
    uint32_t currentTickCount = 0;
    lockClockManager();
    pending = tickPending;
    currentTickCount = tickCount;
    unlockClockManager();
    
    // Detect counter wrap or reset: if tickCount < lastProcessedTick, treat as reset
    // (but only if we didn't just handle a reset via the flag above)
    if (!resetDetected && currentTickCount < lastProcessedTick) {
      lastProcessedTick = 0;
    }
    resetDetected = false;  // Clear after first iteration
    
    // Process ticks while we haven't caught up to current tick
    // The pending flag indicates ISR has set a new tick value
    if (lastProcessedTick < currentTickCount) {
      // Clear pending flag since we're processing the tick
      if (pending) {
        lockClockManager();
        tickPending = false;
        unlockClockManager();
      }
      
      // Process one tick
      lastProcessedTick++;
      sendMIDIClock();
      requestRedraw();
    } else {
      // We've caught up; clear pending flag if it was set
      if (pending) {
        lockClockManager();
        tickPending = false;
        unlockClockManager();
      }
      break;
    }
  }
}

void clockManagerRequestStart() {
  uint32_t now = millis();
  bool doReset = false;
  lockClockManager();
  if (pendingStarts == 0 && activeSequencers == 0) {
    tickCount = 0;
    lastTickTime = now;
    // Request reset of uClock when starting fresh (do outside lock)
    if (midiClockMaster == CLOCK_INTERNAL) {
      doReset = true;
    }
  }
  pendingStarts++;
  int pending = pendingStarts;
  int active = activeSequencers;
  unlockClockManager();
  if (doReset) {
    uClock.resetCounters();
    // Notify updateClockManager to clear lastProcessedTick
    lockClockManager();
    tickCountersReset = true;
    unlockClockManager();
  }
  uint16_t bpm = clampBpm(sharedBPM);
  uint32_t barMs = (60000UL * CLOCK_QUARTERS_PER_BAR) / bpm;
  Serial.printf("[ClockManager] request start (master=%s pending=%d active=%d bpm=%u barMs=%u startMs=%u)\n",
                kMasterNames[static_cast<uint8_t>(midiClockMaster)], pending, active, bpm, barMs,
                now);
  updateRunningState();
}

void clockManagerCancelStart() {
  lockClockManager();
  if (pendingStarts > 0) {
    pendingStarts--;
  }
  unlockClockManager();
  updateRunningState();
}

void clockManagerSequencerStarted() {
  lockClockManager();
  if (pendingStarts > 0) {
    pendingStarts--;
  }
  activeSequencers++;
  int pending = pendingStarts;
  int active = activeSequencers;
  unlockClockManager();
  Serial.printf("[ClockManager] sequencer started (master=%s pending=%d active=%d)\n",
                kMasterNames[static_cast<uint8_t>(midiClockMaster)], pending, active);
  updateRunningState();
}

void clockManagerSequencerStopped() {
  lockClockManager();
  if (activeSequencers > 0) {
    activeSequencers--;
  }
  int pending = pendingStarts;
  int active = activeSequencers;
  unlockClockManager();
  Serial.printf("[ClockManager] sequencer stopped (master=%s pending=%d active=%d)\n",
                kMasterNames[static_cast<uint8_t>(midiClockMaster)], pending, active);
  updateRunningState();
}

void clockManagerExternalStart() {
  uint32_t now = millis();
  lockClockManager();
  externalClockActive = true;
  tickCount = 0;
  lastTickTime = now;
  unlockClockManager();
  Serial.println("[ClockManager] external Start");
  updateRunningState();
}

void clockManagerExternalStop() {
  lockClockManager();
  externalClockActive = false;
  unlockClockManager();
  Serial.println("[ClockManager] external Stop");
  updateRunningState();
}

void clockManagerExternalContinue() {
  lockClockManager();
  // Continue should enable external clock without resetting tick counters
  externalClockActive = true;
  unlockClockManager();
  Serial.println("[ClockManager] external Continue");
  updateRunningState();
}

void clockManagerExternalClock() {
  lockClockManager();
  if (!externalClockActive || midiClockMaster == CLOCK_INTERNAL) {
    unlockClockManager();
    return;
  }
  tickCount++;
  unlockClockManager();
  requestRedraw();
}

uint32_t clockManagerGetTickCount() {
  lockClockManager();
  uint32_t count = tickCount;
  unlockClockManager();
  return count;
}

bool clockManagerHasTickAdvanced(uint32_t &lastSeenTick) {
  lockClockManager();
  if (tickCount != lastSeenTick) {
    lastSeenTick = tickCount;
    unlockClockManager();
    return true;
  }
  unlockClockManager();
  return false;
}

bool clockManagerIsBarStart() {
  return (clockManagerGetTickCount() % CLOCK_TICKS_PER_BAR) == 0;
}

bool clockManagerIsSixteenthTick(uint32_t tick) {
  return (tick % CLOCK_TICKS_PER_SIXTEENTH) == 0;
}

bool clockManagerIsRunning() {
  lockClockManager();
  bool isRunning = running;
  unlockClockManager();
  return isRunning;
}

void clockManagerUpdateBPM() {
  uint16_t bpm = clampBpm(sharedBPM);
  uClock.setTempo((float)bpm);
}
