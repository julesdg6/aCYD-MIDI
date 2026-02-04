#include "module_morph_mode.h"

#include <algorithm>
#include <pgmspace.h>

// forward-declare blendColor (defined inline in src/main.cpp)
uint16_t blendColor(uint16_t from, uint16_t to, uint8_t ratio);

MorphState morphState;

// Time between auto-played morph notes (ms)
static const unsigned long MORPH_NOTE_INTERVAL_MS = 200;
static unsigned long lastMorphNoteTime = 0;

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

struct MorphLayout {
  int sliderX;
  int sliderY;
  int sliderW;
  int sliderH;
  int slotSize;
  int slotSpacing;
  int slotStartY;
  int controlY;
  int controlButtonHeight;
};

static MorphLayout calculateMorphLayout() {
  MorphLayout layout;
  layout.sliderX = MARGIN_SMALL;
  layout.sliderY = HEADER_HEIGHT + SCALE_Y(10);
  layout.sliderW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  layout.sliderH = SCALE_Y(80);
  layout.slotSpacing = SCALE_X(8);  // Reduced spacing to fit better
  layout.slotStartY = layout.sliderY + layout.sliderH + SCALE_Y(10);
  layout.controlButtonHeight = SCALE_Y(32);

  const int controlBottomMargin = SCALE_Y(10);
  int availableHeight =
      DISPLAY_HEIGHT - layout.slotStartY - layout.controlButtonHeight - controlBottomMargin;
  availableHeight = std::max(availableHeight, 0);
  int slotCapacity = std::max(availableHeight - layout.slotSpacing, 0);
  int slotSize = slotCapacity / 2;
  const int preferredMin = SCALE_X(25);  // Increased minimum size
  if (slotCapacity >= 2 * preferredMin) {
    slotSize = std::max(slotSize, preferredMin);
  }
  slotSize = std::min(slotSize, SCALE_X(55));  // Slightly reduced max
  slotSize = std::max(slotSize, preferredMin);  // Ensure minimum size
  layout.slotSize = slotSize;

  layout.controlY = layout.slotStartY + 2 * layout.slotSize + layout.slotSpacing + SCALE_Y(8);  // Reduced gap
  int maxControlY = DISPLAY_HEIGHT - layout.controlButtonHeight - controlBottomMargin;
  layout.controlY = std::min(layout.controlY, maxControlY);
  return layout;
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

  const MorphLayout layout = calculateMorphLayout();
  const int sliderX = layout.sliderX;
  const int sliderY = layout.sliderY;
  const int sliderW = layout.sliderW;
  const int sliderH = layout.sliderH;

  tft.drawRect(sliderX, sliderY, sliderW, sliderH, THEME_TEXT_DIM);
  tft.drawFastHLine(sliderX, sliderY + sliderH / 2, sliderW, THEME_TEXT_DIM);
  tft.drawFastVLine(sliderX + sliderW / 2, sliderY, sliderH, THEME_TEXT_DIM);

  int cursorX = sliderX + static_cast<int>(morphState.morphX * sliderW);
  int cursorY = sliderY + static_cast<int>(morphState.morphY * sliderH);
  tft.fillCircle(cursorX, cursorY, SCALE_X(5), THEME_PRIMARY);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Morph X/Y", sliderX, sliderY - SCALE_Y(12), 1);

  const int slotSize = layout.slotSize;
  const int slotSpacing = layout.slotSpacing;
  const int slotStartY = layout.slotStartY;

  for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
    int row = slot / 2;
    int col = slot % 2;
    int x = MARGIN_SMALL + col * (slotSize + slotSpacing);
    int y = slotStartY + row * (slotSize + slotSpacing);
    drawSlot(slot, x, y, slotSize);
  }

  int controlY = layout.controlY;
  int controlButtonHeight = layout.controlButtonHeight;
  drawRoundButton(MARGIN_SMALL, controlY, SCALE_X(64), controlButtonHeight,
                  "PLAY", morphState.recording ? THEME_ERROR : THEME_SUCCESS, false, 2);
  drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), controlY, SCALE_X(80),
                  controlButtonHeight,
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

  const MorphLayout layout = calculateMorphLayout();

  if (touch.justPressed) {
    int controlY = layout.controlY;
    int controlButtonHeight = layout.controlButtonHeight;
    if (isButtonPressed(MARGIN_SMALL, controlY, SCALE_X(64), controlButtonHeight)) {
      playMorphNote();
      return;
    }
    if (isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), controlY, SCALE_X(80),
                        controlButtonHeight)) {
      morphState.recording = !morphState.recording;
      requestRedraw();
      return;
    }
  }

  if (touch.isPressed) {
    if (touch.x >= layout.sliderX && touch.x <= layout.sliderX + layout.sliderW &&
        touch.y >= layout.sliderY && touch.y <= layout.sliderY + layout.sliderH) {
      morphState.morphX = (float)(touch.x - layout.sliderX) / (float)layout.sliderW;
      morphState.morphY = (float)(touch.y - layout.sliderY) / (float)layout.sliderH;
      morphState.morphX = std::min(std::max(morphState.morphX, 0.0f), 1.0f);
      morphState.morphY = std::min(std::max(morphState.morphY, 0.0f), 1.0f);
      unsigned long now = millis();
      if (now - lastMorphNoteTime >= MORPH_NOTE_INTERVAL_MS) {
        playMorphNote();
        lastMorphNoteTime = now;
      }
      requestRedraw();
      return;
    }

    const int slotSize = layout.slotSize;
    const int slotSpacing = layout.slotSpacing;
    const int slotStartY = layout.slotStartY;
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
