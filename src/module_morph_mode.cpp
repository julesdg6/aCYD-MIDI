#include "module_morph_mode.h"

#include <algorithm>
#include <pgmspace.h>

// forward-declare blendColor (defined inline in src/main.cpp)
uint16_t blendColor(uint16_t from, uint16_t to, uint8_t ratio);

MorphState morphState;

static const uint16_t PROGMEM slotColors[MORPH_SLOTS] = {
    THEME_ERROR,
    THEME_WARNING,
    THEME_SUCCESS,
    THEME_ACCENT,
};

static void drawSlot(int index, int x, int y, int size) {
  uint16_t fill = pgm_read_word(&slotColors[index]);
  if (morphState.activeSlot == index) {
    fill = blendColor(fill, THEME_BG, 120);
  }
  tft.fillRoundRect(x, y, size, size, 6, fill);
  tft.drawRoundRect(x, y, size, size, 6, THEME_TEXT);
  tft.setTextColor(THEME_BG, fill);
  tft.drawCentreString("Slot " + String(index + 1), x + size / 2, y + size / 2 - 6, 1);
}

void playMorphNote() {
  uint8_t note = 60 + static_cast<uint8_t>(morphState.morphX * 12 + morphState.morphY * 6);
  sendMIDI(0x90, note, 100);
  sendMIDI(0x80, note, 0);
}

void initializeMorphMode() {
  morphState.morphX = 0.5f;
  morphState.morphY = 0.5f;
  morphState.activeSlot = 0;
  morphState.recording = false;
  drawMorphMode();
}

void drawMorphMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("MORPH", "Gesture morphing", 3);

  // Move XY pad higher to avoid button overlay
  int sliderX = MARGIN_SMALL;
  int sliderY = HEADER_HEIGHT + SCALE_Y(10);
  int sliderW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  int sliderH = SCALE_Y(80);

  tft.drawRect(sliderX, sliderY, sliderW, sliderH, THEME_TEXT_DIM);
  tft.drawFastHLine(sliderX, sliderY + sliderH / 2, sliderW, THEME_TEXT_DIM);
  tft.drawFastVLine(sliderX + sliderW / 2, sliderY, sliderH, THEME_TEXT_DIM);

  int cursorX = sliderX + static_cast<int>(morphState.morphX * sliderW);
  int cursorY = sliderY + static_cast<int>(morphState.morphY * sliderH);
  tft.fillCircle(cursorX, cursorY, SCALE_X(5), THEME_PRIMARY);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Morph X/Y", sliderX, sliderY - SCALE_Y(12), 1);

  const int slotSize = SCALE_X(60);
  const int slotSpacing = SCALE_X(12);
  const int slotStartY = sliderY + sliderH + SCALE_Y(10);

  for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
    int row = slot / 2;
    int col = slot % 2;
    int x = MARGIN_SMALL + col * (slotSize + slotSpacing);
    int y = slotStartY + row * (slotSize + slotSpacing);
    drawSlot(slot, x, y, slotSize);
  }

  int controlY = DISPLAY_HEIGHT - SCALE_Y(50);
  drawRoundButton(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32),
                  "PLAY", morphState.recording ? THEME_ERROR : THEME_SUCCESS, false, 2);
  drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), controlY, SCALE_X(80), SCALE_Y(32),
                  morphState.recording ? "RECORDING" : "RECORD",
                  morphState.recording ? THEME_ACCENT : THEME_SECONDARY, false, 1);

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("X: " + String(static_cast<int>(morphState.morphX * 100)) + "%",
                 MARGIN_SMALL, controlY - SCALE_Y(15), 1);
  tft.drawString("Y: " + String(static_cast<int>(morphState.morphY * 100)) + "%",
                 DISPLAY_WIDTH - SCALE_X(70), controlY - SCALE_Y(15), 1);
}

void handleMorphMode() {
  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  if (touch.justPressed) {
    int controlY = DISPLAY_HEIGHT - SCALE_Y(50);
    if (isButtonPressed(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32))) {
      playMorphNote();
      return;
    }
    if (isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), controlY, SCALE_X(80), SCALE_Y(32))) {
      morphState.recording = !morphState.recording;
      requestRedraw();
      return;
    }
  }

  if (touch.isPressed) {
    int sliderX = MARGIN_SMALL;
    int sliderY = HEADER_HEIGHT + SCALE_Y(10);
    int sliderW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
    int sliderH = SCALE_Y(80);
    if (touch.x >= sliderX && touch.x <= sliderX + sliderW &&
        touch.y >= sliderY && touch.y <= sliderY + sliderH) {
      morphState.morphX = (float)(touch.x - sliderX) / (float)sliderW;
      morphState.morphY = (float)(touch.y - sliderY) / (float)sliderH;
      morphState.morphX = std::min(std::max(morphState.morphX, 0.0f), 1.0f);
      morphState.morphY = std::min(std::max(morphState.morphY, 0.0f), 1.0f);
      playMorphNote();  // Play note as you morph
      requestRedraw();
      return;
    }

    const int slotSize = SCALE_X(60);
    const int slotSpacing = SCALE_X(12);
    const int slotStartY = sliderY + sliderH + SCALE_Y(10);
    for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
      int row = slot / 2;
      int col = slot % 2;
      int x = MARGIN_SMALL + col * (slotSize + slotSpacing);
      int y = slotStartY + row * (slotSize + slotSpacing);

      if (isButtonPressed(x, y, slotSize, slotSize)) {
        morphState.activeSlot = slot;
        requestRedraw();
        return;
      }
    }
  }
}
