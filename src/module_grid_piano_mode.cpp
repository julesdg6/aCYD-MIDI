#include "module_grid_piano_mode.h"

int gridOctave = 3;
int gridPressedNote = -1;
int gridLayout[GRID_ROWS][GRID_COLS];

// Implementations
void initializeGridPianoMode() {
  gridOctave = 3;
  gridPressedNote = -1;
  calculateGridLayout();
}

void calculateGridLayout() {
  // Linnstrument-style all 4ths layout
  // Each row is 5 semitones (perfect 4th) higher than the row below
  // Each column is 1 semitone higher than the column to the left
  // Row 0 = bottom (lowest notes), Row 4 = top (highest notes)
  int baseNote = 36 + (gridOctave - 3) * 12; // C3 base
  
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      // Reverse row order: (GRID_ROWS - 1 - row) makes row 0 the bottom
      gridLayout[row][col] = baseNote + col + ((GRID_ROWS - 1 - row) * 5);
    }
  }
}

void drawGridPianoMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("GRID PIANO", "4ths Layout");
  
  // Grid area
  int cellW = 35;
  int cellH = 25;
  int startX = 10;
  int startY = 55;
  int spacing = 2;
  
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      drawGridCell(row, col);
    }
  }
  
  // Octave controls
  drawRoundButton(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "OCT-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "OCT+", THEME_SECONDARY);
  
  // Octave display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Oct " + String(gridOctave), SCALE_X(110), SCALE_Y(207), 1);
  
  // Current note display
  if (gridPressedNote != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString("Playing: " + getNoteNameFromMIDI(gridPressedNote), SCALE_X(180), SCALE_Y(207), 1);
  }
}

void drawGridCell(int row, int col, bool pressed) {
  int cellW = SCALE_X(35);
  int cellH = SCALE_Y(25);
  int startX = MARGIN_SMALL;
  int startY = SCALE_Y(55);
  int spacing = SCALE_X(2);
  
  int x = startX + col * (cellW + spacing);
  int y = startY + row * (cellH + spacing);
  
  int note = gridLayout[row][col];
  int noteInOctave = note % 12;
  bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);
  
  uint16_t bgColor, textColor;
  
  if (pressed) {
    bgColor = THEME_PRIMARY;
    textColor = THEME_BG;
  } else if (isBlackKey) {
    bgColor = THEME_SURFACE;
    textColor = THEME_TEXT_DIM;  // Lighter text for accidentals
  } else {
    bgColor = THEME_BG;
    textColor = THEME_TEXT;
  }
  
  tft.fillRect(x, y, cellW, cellH, bgColor);
  tft.drawRect(x, y, cellW, cellH, THEME_PRIMARY);
  
  // Note name
  String noteName = getNoteNameFromMIDI(note);
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(noteName, x + cellW/2, y + cellH/2 - 6, 1);
}

void handleGridPianoMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    // Octave controls
    if (isButtonPressed(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      gridOctave = max(1, gridOctave - 1);
      calculateGridLayout();
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      gridOctave = min(6, gridOctave + 1);
      calculateGridLayout();
      requestRedraw();
      return;
    }
  }
  
  // Grid interaction
  int cellW = SCALE_X(35);
  int cellH = SCALE_Y(25);
  int startX = MARGIN_SMALL;
  int startY = SCALE_Y(55);
  int spacing = SCALE_X(2);
  
  int pressedNote = -1;
  
  if (touch.isPressed) {
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        int x = startX + col * (cellW + spacing);
        int y = startY + row * (cellH + spacing);
        
        if (isButtonPressed(x, y, cellW, cellH)) {
          pressedNote = gridLayout[row][col];
          drawGridCell(row, col, true);
          break;
        }
      }
      if (pressedNote != -1) break;
    }
  }
  
  // Handle note changes
  if (pressedNote != gridPressedNote) {
    // Turn off old note - MIDI first (only if device connected)
    if (gridPressedNote != -1 && deviceConnected) {
      sendMIDI(0x80, gridPressedNote, 0);
    }

    // Turn on new note - MIDI first
    if (pressedNote != -1 && deviceConnected) {
      sendMIDI(0x90, pressedNote, 100);
    }
    
    // Then update visuals
    if (gridPressedNote != -1) {
      // Redraw old cell
      for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
          if (gridLayout[row][col] == gridPressedNote) {
            drawGridCell(row, col, false);
            break;
          }
        }
      }
    }
    
    gridPressedNote = pressedNote;
    
    // Update display
    if (gridPressedNote != -1) {
      tft.fillRect(SCALE_X(180), SCALE_Y(200), SCALE_X(140), SCALE_Y(16), THEME_BG);
      tft.setTextColor(THEME_PRIMARY, THEME_BG);
      tft.drawString("Playing: " + getNoteNameFromMIDI(gridPressedNote), SCALE_X(180), SCALE_Y(207), 1);
    } else {
      tft.fillRect(SCALE_X(180), SCALE_Y(200), SCALE_X(140), SCALE_Y(16), THEME_BG);
    }
  }
}
