#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <lvgl.h>

// SD Card pin for ESP32-2432S028R (CYD)
// Note: SD card uses same SPI bus as display
// The SD card CS pin is board-specific and may vary
// Common values: 5 (some variants) or no SD card on some boards
#define SD_CS_PIN 5  // SD card chip select pin - adjust if needed

// Screenshot functions
bool initializeSD();
bool takeScreenshot(const char *label = nullptr);
void shutdownSD();

#endif // SCREENSHOT_H
