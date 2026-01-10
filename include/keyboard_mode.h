#ifndef KEYBOARD_MODE_H
#define KEYBOARD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Keyboard mode variables
#define NUM_KEYS 10  // More keys per row
#define NUM_ROWS 2   // Two rows
int keyboardOctave = 4;
int keyboardScale = 0;
int keyboardKey = 0;  // Key signature (C=0, C#=1, D=2, etc.)
int lastKey = -1;
int lastRow = -1;

// Function declarations
void initializeKeyboardMode();
void drawKeyboardMode();
void handleKeyboardMode();
void drawKeyboardKey(int row, int keyIndex, bool pressed);
void playKeyboardNote(int row, int keyIndex, bool on);

// Implementations
void initializeKeyboardMode() {
  keyboardOctave = 4;
  keyboardScale = 0;
  keyboardKey = 0;
  lastKey = -1;
  lastRow = -1;
}

void drawKeyboardMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("KEYS", scales[keyboardScale].name + " Key " + getNoteNameFromMIDI(keyboardKey));
  
  // Draw keys - two rows
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int i = 0; i < NUM_KEYS; i++) {
      drawKeyboardKey(row, i, false);
    }
  }
  
  // Control layout - using scaled dimensions
  int ctrlY = SCALE_Y(180);
  drawRoundButton(SCALE_X(10), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "OCT-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(60), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "OCT+", THEME_SECONDARY);
  drawRoundButton(SCALE_X(110), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "SCALE", THEME_ACCENT);
  drawRoundButton(SCALE_X(170), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "KEY-", THEME_WARNING);
  drawRoundButton(SCALE_X(220), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "KEY+", THEME_WARNING);
  
  // Status display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Octave " + String(keyboardOctave) + " | " + 
               scales[keyboardScale].name + " in " + getNoteNameFromMIDI(keyboardKey), MARGIN_SMALL, SCALE_Y(215), 1);
}

void drawKeyboardKey(int row, int keyIndex, bool pressed) {
  int keyWidth = DISPLAY_WIDTH / NUM_KEYS;
  int keyHeight = SCALE_Y(55);  // More vertical space
  int keyY = HEADER_HEIGHT + SCALE_Y(15) + (row * (keyHeight + SCALE_Y(3)));
  int x = keyIndex * keyWidth;
  
  uint16_t bgColor = pressed ? THEME_PRIMARY : THEME_SURFACE;
  uint16_t borderColor = THEME_PRIMARY;
  uint16_t textColor = pressed ? THEME_BG : THEME_TEXT;
  
  tft.fillRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, bgColor);
  tft.drawRect(x, keyY, keyWidth, keyHeight, borderColor);
  
  // Row 0 = base octave, Row 1 = octave higher
  // Apply key signature transpose
  int note = getNoteInScale(keyboardScale, keyIndex, keyboardOctave + row) + keyboardKey;
  String noteName = getNoteNameFromMIDI(note);
  
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(noteName, x + keyWidth/2, keyY + keyHeight/2 - 6, 1);
}

void handleKeyboardMode() {
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  int ctrlY = SCALE_Y(180);
  if (touch.justPressed) {
    if (isButtonPressed(SCALE_X(10), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      keyboardOctave = max(1, keyboardOctave - 1);
      drawKeyboardMode();
      return;
    }
    if (isButtonPressed(SCALE_X(60), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      keyboardOctave = min(8, keyboardOctave + 1);
      drawKeyboardMode();
      return;
    }
    if (isButtonPressed(SCALE_X(110), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      keyboardScale = (keyboardScale + 1) % NUM_SCALES;
      drawKeyboardMode();
      return;
    }
    if (isButtonPressed(SCALE_X(170), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      keyboardKey = (keyboardKey - 1 + 12) % 12;
      drawKeyboardMode();
      return;
    }
    if (isButtonPressed(SCALE_X(220), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      keyboardKey = (keyboardKey + 1) % 12;
      drawKeyboardMode();
      return;
    }
  }
  
  // Key sliding support - handle two rows
  int key = -1;
  int row = -1;
  
  // Check which key and row is being touched
  if (touch.isPressed) {
    int keyWidth = DISPLAY_WIDTH / NUM_KEYS;
    int keyHeight = SCALE_Y(55);
    
    for (int r = 0; r < NUM_ROWS; r++) {
      int keyY = HEADER_HEIGHT + SCALE_Y(15) + (r * (keyHeight + SCALE_Y(3)));
      if (touch.y >= keyY && touch.y < keyY + keyHeight) {
        row = r;
        key = touch.x / keyWidth;
        if (key >= NUM_KEYS) key = NUM_KEYS - 1;
        break;
      }
    }
  }
  
  if (touch.isPressed && key != -1 && row != -1) {
    if (key != lastKey || row != lastRow) {
      if (lastKey != -1 && lastRow != -1) {
        playKeyboardNote(lastRow, lastKey, false);
        drawKeyboardKey(lastRow, lastKey, false);
      }
      playKeyboardNote(row, key, true);
      drawKeyboardKey(row, key, true);
      lastKey = key;
      lastRow = row;
    }
  } else if (touch.justReleased && lastKey != -1 && lastRow != -1) {
    playKeyboardNote(lastRow, lastKey, false);
    drawKeyboardKey(lastRow, lastKey, false);
    lastKey = -1;
    lastRow = -1;
  }
}

void playKeyboardNote(int row, int keyIndex, bool on) {
  int note = getNoteInScale(keyboardScale, keyIndex, keyboardOctave + row) + keyboardKey;
  sendMIDI(on ? 0x90 : 0x80, note, on ? 100 : 0);
  
  MIDI_DEBUG("Key R%d:%d: %s %s\n", row, keyIndex, getNoteNameFromMIDI(note).c_str(), on ? "ON" : "OFF");
}

#endif