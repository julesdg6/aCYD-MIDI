#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <lvgl.h>

// SD Card pin for ESP32-2432S028R (CYD)
// Note: SD card uses same SPI bus as display but different CS pin
#define SD_CS 5  // SD card chip select pin

// Screenshot functions
bool initializeSD();
bool takeScreenshot();
void shutdownSD();

#endif // SCREENSHOT_H
