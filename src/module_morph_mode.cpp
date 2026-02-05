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

static void drawSlot(int index, int x, int y, int w, int h) {
  uint16_t fill = pgm_read_word(&slotColors[index]);
  uint16_t border = THEME_TEXT_DIM;
  
  // Active slot gets brighter border
  if (morphState.activeSlot == index) {
    border = THEME_TEXT;
    fill = blendColor(fill, THEME_TEXT, 200);
  }
  
  tft.fillRoundRect(x, y, w, h, 4, fill);
  tft.drawRoundRect(x, y, w, h, 4, border);
  
  // Slot label
  tft.setTextColor(THEME_BG, fill);
  tft.drawCentreString(String(index + 1), x + w / 2, y + h / 2 - 8, 2);
}

struct MorphLayout {
  int padX;
  int padY;
  int padW;
  int padH;
  int slotX;
  int slotY;
  int slotW;
  int slotH;
  int slotSpacing;
  int controlX;
  int controlY;
  int controlW;
  int controlH;
};

static MorphLayout calculateMorphLayout() {
  MorphLayout layout;
  
  // XY Pad - maximize size, leave room for controls at bottom
  layout.padX = MARGIN_SMALL;
  layout.padY = HEADER_HEIGHT + SCALE_Y(10);
  layout.padW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  layout.padH = SCALE_Y(110);  // Larger pad area
  
  // Control row at bottom
  layout.controlH = SCALE_Y(35);
  layout.controlY = DISPLAY_HEIGHT - layout.controlH - SCALE_Y(8);
  
  // Slot controls - bottom left (compact horizontal row)
  layout.slotY = layout.controlY;
  layout.slotX = MARGIN_SMALL;
  layout.slotW = SCALE_X(40);
  layout.slotH = layout.controlH;
  layout.slotSpacing = SCALE_X(6);
  
  // PLAY/RECORD buttons - bottom right (grouped together)
  layout.controlX = DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(140);
  layout.controlW = SCALE_X(140);
  
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
  
  // Draw XY Pad area
  tft.drawRect(layout.padX, layout.padY, layout.padW, layout.padH, THEME_TEXT_DIM);
  tft.drawFastHLine(layout.padX, layout.padY + layout.padH / 2, layout.padW, THEME_TEXT_DIM);
  tft.drawFastVLine(layout.padX + layout.padW / 2, layout.padY, layout.padH, THEME_TEXT_DIM);
  
  // Draw cursor on pad
  int cursorX = layout.padX + static_cast<int>(morphState.morphX * layout.padW);
  int cursorY = layout.padY + static_cast<int>(morphState.morphY * layout.padH);
  tft.fillCircle(cursorX, cursorY, SCALE_X(6), THEME_PRIMARY);
  tft.drawCircle(cursorX, cursorY, SCALE_X(6), THEME_TEXT);
  
  // XY readouts - LARGER and higher contrast (below pad)
  int readoutY = layout.padY + layout.padH + SCALE_Y(8);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String xStr = "X: " + String(static_cast<int>(morphState.morphX * 100)) + "%";
  String yStr = "Y: " + String(static_cast<int>(morphState.morphY * 100)) + "%";
  tft.drawString(xStr, layout.padX, readoutY, 4);
  tft.drawString(yStr, DISPLAY_WIDTH - SCALE_X(100), readoutY, 4);
  
  // Slot controls - bottom left panel (horizontal row)
  for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
    int x = layout.slotX + slot * (layout.slotW + layout.slotSpacing);
    drawSlot(slot, x, layout.slotY, layout.slotW, layout.slotH);
  }
  
  // PLAY and RECORD buttons - grouped together on right
  int playBtnW = SCALE_X(65);
  int recordBtnW = layout.controlW - playBtnW - SCALE_X(6);
  
  // PLAY button
  drawRoundButton(layout.controlX, layout.controlY, playBtnW, layout.controlH,
                  "PLAY", THEME_SUCCESS, false, 2);
  
  // RECORD button (visual state indicator)
  uint16_t recordColor = morphState.recording ? THEME_ERROR : THEME_SECONDARY;
  String recordText = morphState.recording ? "STOP" : "REC";
  drawRoundButton(layout.controlX + playBtnW + SCALE_X(6), layout.controlY, 
                  recordBtnW, layout.controlH,
                  recordText, recordColor, morphState.recording, 2);
}

void handleMorphMode() {
  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  const MorphLayout layout = calculateMorphLayout();

  if (touch.justPressed) {
    // PLAY button
    int playBtnW = SCALE_X(65);
    if (isButtonPressed(layout.controlX, layout.controlY, playBtnW, layout.controlH)) {
      playMorphNote();
      return;
    }
    
    // RECORD button
    int recordBtnW = layout.controlW - playBtnW - SCALE_X(6);
    int recordBtnX = layout.controlX + playBtnW + SCALE_X(6);
    if (isButtonPressed(recordBtnX, layout.controlY, recordBtnW, layout.controlH)) {
      morphState.recording = !morphState.recording;
      requestRedraw();
      return;
    }
    
    // Slot buttons
    for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
      int x = layout.slotX + slot * (layout.slotW + layout.slotSpacing);
      if (isButtonPressed(x, layout.slotY, layout.slotW, layout.slotH)) {
        morphState.activeSlot = slot;
        requestRedraw();
        return;
      }
    }
  }

  // XY Pad interaction
  if (touch.isPressed) {
    if (touch.x >= layout.padX && touch.x <= layout.padX + layout.padW &&
        touch.y >= layout.padY && touch.y <= layout.padY + layout.padH) {
      morphState.morphX = (float)(touch.x - layout.padX) / (float)layout.padW;
      morphState.morphY = (float)(touch.y - layout.padY) / (float)layout.padH;
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
  }
}
