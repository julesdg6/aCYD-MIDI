#include "header_capture.h"
#include "screenshot.h"

#include <Arduino.h>

namespace {
static const char *const kAppModeNames[] = {
    "Menu",
    "Settings",
    "Keys",
    "Beats",
    "Zen",
    "Drop",
    "RNG",
    "XYPad",
    "Arp",
    "Grid",
    "Chord",
    "LFO",
    "Slink",
    "TB3PO",
    "Grids",
    "Raga",
    "Euclid",
    "Morph",
};

static constexpr uint32_t kHeaderHoldThresholdMs = 3000;
static uint32_t headerPressStartMs = 0;
static bool headerCaptureTriggered = false;

const char *describeAppMode(AppMode mode) {
  size_t index = static_cast<size_t>(mode) - static_cast<size_t>(MENU);
  if (mode >= MENU && index < (sizeof(kAppModeNames) / sizeof(kAppModeNames[0]))) {
    return kAppModeNames[index];
  }
  return "ACYD";
}
}  // namespace

void updateHeaderCapture() {
  bool inHeader = touch.isPressed && touch.y >= 0 && touch.y <= HEADER_HEIGHT;
  if (touch.justPressed && inHeader) {
    headerPressStartMs = millis();
    headerCaptureTriggered = false;
  }

  if (touch.isPressed && inHeader && headerPressStartMs > 0 && !headerCaptureTriggered) {
    if ((millis() - headerPressStartMs) >= kHeaderHoldThresholdMs) {
      headerCaptureTriggered = true;
      if (!takeScreenshot(describeAppMode(currentMode))) {
        Serial.println("Screenshot capture failed");
      }
    }
  }

  if (!touch.isPressed || !inHeader) {
    headerPressStartMs = 0;
    headerCaptureTriggered = false;
  }
}
