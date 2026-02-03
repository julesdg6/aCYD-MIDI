#include "clock_manager.h"
#include <Arduino.h>

#include "common_definitions.h"
#include "midi_utils.h"

#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <esp_timer.h>

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

// Hardware timer for precise MIDI clock
static esp_timer_handle_t clockTimer = nullptr;
static volatile bool timerActive = false;
static uint64_t timerIntervalUs = 0;

// Timing statistics
static volatile uint32_t tickIntervalMin = UINT32_MAX;
static volatile uint32_t tickIntervalMax = 0;
static volatile uint64_t tickIntervalSum = 0;
static volatile uint32_t tickIntervalCount = 0;
static uint64_t lastTickTimeUs = 0;

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

// Hardware timer callback (ISR context - must be IRAM_ATTR and minimal)
static void IRAM_ATTR onClockTimer(void* /*arg*/) {
  // Increment tick counter in ISR-safe manner
  portENTER_CRITICAL_ISR(&clockManagerMux);
  tickCount++;
  
  // Track timing statistics
  uint64_t nowUs = esp_timer_get_time();
  if (lastTickTimeUs > 0) {
    uint32_t intervalUs = (uint32_t)(nowUs - lastTickTimeUs);
    if (intervalUs < tickIntervalMin) tickIntervalMin = intervalUs;
    if (intervalUs > tickIntervalMax) tickIntervalMax = intervalUs;
    tickIntervalSum += intervalUs;
    tickIntervalCount++;
  }
  lastTickTimeUs = nowUs;
  
  portEXIT_CRITICAL_ISR(&clockManagerMux);
  
  // Note: MIDI messages are sent by updateClockManager() which polls the tick counter
  // We don't trigger anything here - just increment the counter
}

static void startHardwareTimer(uint16_t bpm) {
  if (clockTimer == nullptr) {
    return;
  }
  
  // Calculate interval in microseconds with high precision
  // 60,000,000 us/min ÷ BPM ÷ 24 ticks/quarter
  timerIntervalUs = (60000000ULL / bpm) / CLOCK_TICKS_PER_QUARTER;
  
  // Stop timer if running
  if (timerActive) {
    esp_timer_stop(clockTimer);
    timerActive = false;
  }
  
  // Reset timing statistics
  lockClockManager();
  tickIntervalMin = UINT32_MAX;
  tickIntervalMax = 0;
  tickIntervalSum = 0;
  tickIntervalCount = 0;
  lastTickTimeUs = 0;
  unlockClockManager();
  
  // Start periodic timer
  esp_err_t err = esp_timer_start_periodic(clockTimer, timerIntervalUs);
  if (err == ESP_OK) {
    timerActive = true;
    Serial.printf("[ClockManager] Hardware timer started: %u BPM, interval=%llu us (%.3f ms)\n",
                  bpm, timerIntervalUs, timerIntervalUs / 1000.0);
  } else {
    Serial.printf("[ClockManager] Failed to start timer: %d\n", err);
  }
}

static void stopHardwareTimer() {
  if (clockTimer && timerActive) {
    esp_timer_stop(clockTimer);
    timerActive = false;
    Serial.println("[ClockManager] Hardware timer stopped");
  }
}

static RunningStateDelta updateRunningState() {
  lockClockManager();
  RunningStateDelta delta = updateRunningStateLocked();
  bool shouldStartTimer = delta.running && (midiClockMaster == CLOCK_INTERNAL);
  bool shouldStopTimer = !delta.running || (midiClockMaster != CLOCK_INTERNAL);
  unlockClockManager();
  
  if (delta.sendStart) {
    Serial.printf("[ClockManager] INTERNAL START – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStart();
    if (shouldStartTimer) {
      uint16_t bpm = clampBpm(sharedBPM);
      startHardwareTimer(bpm);
    }
  } else if (delta.sendStop) {
    Serial.printf("[ClockManager] INTERNAL STOP – pending=%d active=%d\n", delta.pendingStarts,
                  delta.activeSequencers);
    sendMIDIStop();
    if (shouldStopTimer) {
      stopHardwareTimer();
    }
  } else if (shouldStartTimer && !timerActive) {
    uint16_t bpm = clampBpm(sharedBPM);
    startHardwareTimer(bpm);
  } else if (shouldStopTimer && timerActive) {
    stopHardwareTimer();
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
  timerActive = false;
  unlockClockManager();
  
  // Create high-precision hardware timer
  esp_timer_create_args_t timerConfig = {
    .callback = &onClockTimer,
    .arg = nullptr,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "midi_clock",
    .skip_unhandled_events = false
  };
  
  esp_err_t err = esp_timer_create(&timerConfig, &clockTimer);
  if (err != ESP_OK) {
    Serial.printf("[ClockManager] Failed to create timer: %d\n", err);
    clockTimer = nullptr;
  } else {
    Serial.println("[ClockManager] Hardware timer initialized");
  }
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
  
  // The hardware timer generates ticks automatically, so we just need to
  // send MIDI clock messages for any new ticks that have accumulated
  static uint32_t lastProcessedTick = 0;
  
  uint32_t currentTick = clockManagerGetTickCount();
  
  // Process all ticks that have accumulated since last call
  bool hadNewTicks = false;
  while (lastProcessedTick < currentTick) {
    lastProcessedTick++;
    sendMIDIClock();
    hadNewTicks = true;
  }
  
  // Request redraw if we processed any ticks (for clock-synced UI animations)
  if (hadNewTicks) {
    requestRedraw();
  }
}

void clockManagerRequestStart() {
  uint32_t now = millis();
  lockClockManager();
  if (pendingStarts == 0 && activeSequencers == 0) {
    tickCount = 0;
    lastTickTime = now;
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

void clockManagerUpdateBPM() {
  // Called when sharedBPM changes - restart timer with new interval if running
  if (timerActive && midiClockMaster == CLOCK_INTERNAL) {
    uint16_t bpm = clampBpm(sharedBPM);
    startHardwareTimer(bpm);
  }
}

void clockManagerGetTimingStats(uint32_t &minUs, uint32_t &maxUs, uint32_t &avgUs) {
  lockClockManager();
  minUs = tickIntervalMin;
  maxUs = tickIntervalMax;
  avgUs = (tickIntervalCount > 0) ? (uint32_t)(tickIntervalSum / tickIntervalCount) : 0;
  unlockClockManager();
}
