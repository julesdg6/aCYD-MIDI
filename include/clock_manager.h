#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H

#include <stdint.h>
#include <Arduino.h>

constexpr uint8_t CLOCK_TICKS_PER_QUARTER = 24;
constexpr uint8_t CLOCK_TICKS_PER_SIXTEENTH = CLOCK_TICKS_PER_QUARTER / 4;
constexpr uint8_t CLOCK_QUARTERS_PER_BAR = 4;
constexpr uint16_t CLOCK_TICKS_PER_BAR = CLOCK_TICKS_PER_QUARTER * CLOCK_QUARTERS_PER_BAR;

void initClockManager();
void updateClockManager();
void clockManagerRequestStart();
void clockManagerCancelStart();
void clockManagerSequencerStarted();
void clockManagerSequencerStopped();
void clockManagerExternalStart();
void clockManagerExternalStop();
void clockManagerExternalClock();
uint32_t clockManagerGetTickCount();
bool clockManagerHasTickAdvanced(uint32_t &lastSeenTick);
bool clockManagerIsBarStart();
bool clockManagerIsSixteenthTick(uint32_t tick);
bool clockManagerIsRunning();

struct SequencerSyncState {
  bool playing = false;
  bool startPending = false;
  uint32_t lastTick = UINT32_MAX;
  uint32_t startRequestTick = UINT32_MAX;
  uint32_t startRequestMs = UINT32_MAX;
  bool quantizeOverride = false;

  void reset() {
    playing = false;
    startPending = false;
    lastTick = UINT32_MAX;
  }

  void requestStart() {
    if (playing || startPending) {
      return;
    }
    startPending = true;
    lastTick = UINT32_MAX;
    startRequestTick = clockManagerGetTickCount();
    startRequestMs = millis();
    quantizeOverride = false;
    clockManagerRequestStart();
  }

  void stopPlayback() {
    if (playing) {
      playing = false;
      clockManagerSequencerStopped();
    }
    if (startPending) {
      startPending = false;
      clockManagerCancelStart();
    }
    lastTick = UINT32_MAX;
    startRequestTick = UINT32_MAX;
    startRequestMs = UINT32_MAX;
    quantizeOverride = false;
  }

  bool tryStartIfReady(bool requireBarStart = true) {
    if (!startPending) {
      return playing;
    }
    uint32_t tickNow = clockManagerGetTickCount();
    if (requireBarStart && !clockManagerIsBarStart()) {
      if (!quantizeOverride && startRequestTick != UINT32_MAX) {
        static constexpr uint32_t kQuantizeForceTicks = CLOCK_TICKS_PER_BAR * 2;
        if (tickNow - startRequestTick >= kQuantizeForceTicks) {
          quantizeOverride = true;
        }
      }
      if (!quantizeOverride) {
        return false;
      }
    }
    startPending = false;
    playing = true;
    lastTick = clockManagerGetTickCount();
    startRequestTick = UINT32_MAX;
    startRequestMs = UINT32_MAX;
    quantizeOverride = false;
    clockManagerSequencerStarted();
    return true;
  }

  bool readyForStep(uint32_t stepIntervalTicks = CLOCK_TICKS_PER_SIXTEENTH) {
    if (!playing) {
      return false;
    }
    if (!clockManagerHasTickAdvanced(lastTick)) {
      return false;
    }
    if (stepIntervalTicks == 0) {
      return false;
    }
    if ((lastTick % stepIntervalTicks) != 0) {
      return false;
    }
    return true;
  }
};

#endif // CLOCK_MANAGER_H
