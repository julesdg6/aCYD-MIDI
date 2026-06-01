#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <lvgl.h>

// SD Card pins
// Prefer board-provided TF_* build flags where available.
#ifndef SD_CS_PIN
#ifdef TF_CS
#define SD_CS_PIN TF_CS
#else
// Fallback: common CS value for CYD variants with TF slot
#define SD_CS_PIN 5
#endif
#endif

// Screenshot functions
bool initializeSD();
bool takeScreenshot(const char *label = nullptr);
void shutdownSD();
bool writeScreenshotDocumentation(const char *documentation[], int count);

#endif // SCREENSHOT_H
