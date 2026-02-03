#include "midi_clock_task.h"

#include "clock_manager.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
static constexpr TickType_t kClockTaskDelay = pdMS_TO_TICKS(1);
static constexpr const char *kTaskName = "MidiClock";
static constexpr UBaseType_t kTaskPriority = configMAX_PRIORITIES - 2;
static constexpr uint16_t kStackDepth = 4096;

static void midiClockTask(void * /*unused*/) {
  while (true) {
    updateClockManager();
    vTaskDelay(kClockTaskDelay);
  }
}
}  // namespace

void initMidiClockTask() {
  BaseType_t result = xTaskCreatePinnedToCore(midiClockTask, kTaskName, kStackDepth, nullptr,
                                              kTaskPriority, nullptr, 1);
  if (result != pdPASS) {
    Serial.println("[MidiClockTask] Failed to create clock task");
  }
}
