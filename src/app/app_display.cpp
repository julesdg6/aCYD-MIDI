#include "hardware_midi.h"

#include "common_definitions.h"

#include <lvgl.h>

void requestRedraw() {
  needsRedraw = true;
}

void setDisplayInversion(bool invert) {
  if (displayColorsInverted == invert) {
    return;
  }
  displayColorsInverted = invert;
  tft.setDisplayInversion(invert);
  requestRedraw();
}

void rotateDisplay180() {
  displayRotationIndex ^= 2;
  tft.setRotation(displayRotationIndex);
  requestRedraw();
}

void setSharedBPM(uint16_t bpm) {
  sharedBPM = bpm;
  // BPM update handled automatically in updateClockManager()
}

void initDisplayConfig() {
  lv_display_t *display = lv_display_get_default();
  if (!display) {
    return;
  }

  displayConfig.width = lv_display_get_horizontal_resolution(display);
  displayConfig.height = lv_display_get_vertical_resolution(display);
  displayConfig.scaleX = (float)displayConfig.width / (float)DISPLAY_REF_WIDTH;
  displayConfig.scaleY = (float)displayConfig.height / (float)DISPLAY_REF_HEIGHT;

#if DEBUG_ENABLED
  Serial.printf("Display Config: %dx%d (scale: %.2fx, %.2fy)\n",
                displayConfig.width, displayConfig.height,
                displayConfig.scaleX, displayConfig.scaleY);
#endif
}

