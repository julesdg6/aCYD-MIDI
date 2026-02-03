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
static uint32_t lastTickTime = 0;
static volatile int pendingStarts = 0;
static volatile int activeSequencers = 0;
static volatile bool running = false;
static volatile bool externalClockActive = false;
static portMUX_TYPE clockManagerMux = portMUX_INITIALIZER_UNLOCKED;
static const char *const kMasterNames[] = {"INTERNAL", "WIFI", "BLE", "HARDWARE"};

// uClock integration
static bool uClockInitialized = false;
static bool uClockRunning = false;

static inline void lockClockManager() {
  portENTER_CRITICAL(&clockManagerMux);
}

static inline void unlockClockManager() {
  portEXIT_CRITICAL(&clockManagerMux);
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

// uClock callback - called on every MIDI clock tick (24 PPQN)
static void onClockTickCallback(uint32_t tick) {
  // Increment our tick counter
  lockClockManager();
  tickCount++;
  unlockClockManager();
  
  // Send MIDI clock message
  sendMIDIClock();
  requestRedraw();
}

// uClock callback - called when clock starts
static void onClockStartCallback() {
  Serial.println("[ClockManager] uClock started");
}

// uClock callback - called when clock stops
static void onClockStopCallback() {
  Serial.println("[ClockManager] uClock stopped");
}

static RunningStateDelta updateRunningState() {
  lockClockManager();
  RunningStateDelta delta = updateRunningStateLocked();
  bool shouldStart = delta.running && (midiClockMaster == CLOCK_INTERNAL);
  bool shouldStop = !delta.running || (midiClockMaster != CLOCK_INTERNAL);
  unlockClockManager();
  
  if (delta.sendStart) {
    Serial.printf("[ClockManager] INTERNAL START – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStart();
    if (shouldStart && uClockInitialized) {
      uClock.start();
      uClockRunning = true;
    }
  } else if (delta.sendStop) {
    Serial.printf("[ClockManager] INTERNAL STOP – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStop();
    if (shouldStop && uClockRunning) {
      uClock.stop();
      uClockRunning = false;
    }
  } else if (shouldStart && !uClockRunning && uClockInitialized) {
    uClock.start();
    uClockRunning = true;
  } else if (shouldStop && uClockRunning) {
    uClock.stop();
    uClockRunning = false;
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
  unlockClockManager();
  
  // Initialize uClock library
  // Set to 24 PPQN (standard MIDI clock)
  uClock.setOutputPPQN(uClock.PPQN_24);
  
  // Set initial tempo
  uClock.setTempo(120.0);
  
  // Register callbacks
  uClock.setOnOutputPPQN(onClockTickCallback);
  uClock.setOnClockStart(onClockStartCallback);
  uClock.setOnClockStop(onClockStopCallback);
  
  // Initialize uClock (but don't start it yet)
  uClock.init();
  uClockInitialized = true;
  
  Serial.println("[ClockManager] uClock initialized at 24 PPQN");
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
  // State management and start/stop control
  RunningStateDelta delta = updateRunningState();
  
  // Update BPM if it changed
  if (uClockInitialized) {
    uint16_t bpm = clampBpm(sharedBPM);
    float currentTempo = uClock.getTempo();
    if (abs(currentTempo - (float)bpm) > 0.1f) {
      uClock.setTempo((float)bpm);
    }
  }
}

void clockManagerRequestStart() {
  uint32_t now = millis();
  lockClockManager();
  if (pendingStarts == 0 && activeSequencers == 0) {
    tickCount = 0;
    lastTickTime = now;
    lastTickTimeMicros = 0;  // Will be initialized on first update
    microAccumulator = 0;
  }
  pendingStarts++;
  int pending = pendingStarts;
  int active = activeSequencers;
  unlockClockManager();
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
