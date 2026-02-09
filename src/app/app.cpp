#include "app/app.h"

#include "hardware_midi.h"

#include <Arduino.h>
#include <esp32-hal-psram.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>

#include "esp_log.h"

#include "app/app_ble_midi.h"
#include "app/app_modes.h"
#include "app/app_renderer.h"
#include "app/app_serial_cli.h"
#include "clock_manager.h"
#include "header_capture.h"
#include "midi_clock_task.h"
#include "midi_transport.h"
#include "remote_display.h"
#include "splash_screen.h"
#include "ui_elements.h"
#include "wifi_manager.h"

void appSetup() {
  // Initialize USB Serial for debugging (only if not using UART0 for MIDI)
#if DEBUG_ENABLED
  Serial.begin(115200);
  delay(200);
  Serial.println("aCYD MIDI Controller Starting...");
  Serial.printf("Hardware MIDI: %s (UART%d)\n",
                HARDWARE_MIDI_ENABLED ? "Enabled" : "Disabled",
                HARDWARE_MIDI_UART);
  Serial.printf("PSRAM: found=%s size=%u free=%u\n",
                psramFound() ? "yes" : "no",
                ESP.getPsramSize(),
                ESP.getFreePsram());
  Serial.printf("Heap pre-init: dma_free=%u dma_largest=%u int_free=%u int_largest=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif

  // Increase task watchdog timeout to 10s to allow diagnostic logging
#ifndef DISABLE_TASK_WDT
  esp_task_wdt_init(10, true);
#if DEBUG_ENABLED
  Serial.println("Task WDT timeout set to 10s for diagnostics");
#endif
#else
#if DEBUG_ENABLED
  Serial.println("Task WDT disabled for this build (display initializes on CYD 35)");
#endif
#endif

  // Increase ESP log verbosity to capture BT stack debug output for diagnosis
  esp_log_level_set("*", ESP_LOG_DEBUG);
  esp_log_level_set("BT", ESP_LOG_DEBUG);
#if DEBUG_ENABLED
  Serial.println("ESP log level set to DEBUG for BT stack");
#endif

#if DEBUG_ENABLED
  Serial.printf("Heap post-init: dma_free=%u dma_largest=%u int_free=%u int_largest=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif

  smartdisplay_init();
  lv_display_t *display = lv_display_get_default();
  if (display) {
    tft.setRotation(displayRotationIndex);
  }

  // Initialize display configuration for autoscaling
  initDisplayConfig();

#if DEBUG_ENABLED
  Serial.printf("LVGL buffer pixels: %d\n", LVGL_BUFFER_PIXELS);
#ifdef ILI9341_SPI_CONFIG_PCLK_HZ
  Serial.printf("ILI9341 PCLK Hz: %d\n", ILI9341_SPI_CONFIG_PCLK_HZ);
#endif
#ifdef ILI9341_SPI_CONFIG_TRANS_QUEUE_DEPTH
  Serial.printf("ILI9341 queue depth: %d\n", ILI9341_SPI_CONFIG_TRANS_QUEUE_DEPTH);
#endif
#ifdef ILI9341_SPI_BUS_MAX_TRANSFER_SZ
  Serial.printf("ILI9341 max transfer: %d\n", ILI9341_SPI_BUS_MAX_TRANSFER_SZ);
#endif
#endif

  tft.init();

  // Must happen before the first splash screen draw.
  appRendererInit();

  showSplashScreen("Booting...", 400);

  bleMidiBegin();
  initHardwareMIDI();  // Initialize hardware MIDI output
  // Ensure all modules register uClock step callbacks before uClock is initialized
  registerAllStepCallbacks();
  initClockManager();
  initMidiClockTask();
  initWiFi();  // Prepare WiFi (used by remote display and clock master suppliers)
  initMidiTransports();

#if REMOTE_DISPLAY_ENABLED
  initRemoteDisplay();  // Initialize remote display capability
#endif

  showSplashScreen(String(), 500);
  switchMode(MENU);

#if DEBUG_ENABLED
  Serial.println("Setup complete!");
#endif
}

void appLoop() {
  uint32_t now = millis();

  appRendererLoopTick(now);
  bleMidiLoop(now);

  updateTouch();
  updateHeaderCapture();

  // Process simple serial CLI commands for automated testing
  processSerialCommands();

#if WIFI_ENABLED
  handleWiFi();
#endif

  handleMidiTransports();

  appHandleCurrentMode();

  // Process any pending redraws after handling logic
  appRendererProcessRedraw();

#if REMOTE_DISPLAY_ENABLED
  handleRemoteDisplay();  // Handle remote display updates
#endif
}

