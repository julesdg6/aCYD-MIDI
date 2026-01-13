/*******************************************************************
 GRIDS Mode Implementation
 Simplified Grids drum pattern generator with X/Y pad interface
 *******************************************************************/

#include "grids_mode.h"

GridsState grids;

// Simplified pattern maps (16 steps, 3 voices) 
// These are inspired by classic drum patterns
// Each value is 0-255 representing trigger probability

// Pattern Map A (corner patterns) - 4 base patterns
const uint8_t PATTERN_MAP[4][3][GRIDS_STEPS] PROGMEM = {
  // Pattern 0: Four-on-the-floor house beat
  {
    {255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0},   // Kick
    {0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0},       // Snare
    {150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80}  // Hat
  },
  // Pattern 1: Funky breakbeat
  {
    {255, 0, 0, 180, 0, 0, 255, 0, 0, 180, 0, 0, 255, 0, 0, 0},  // Kick
    {0, 0, 255, 0, 0, 0, 0, 180, 0, 0, 255, 0, 0, 0, 180, 0},    // Snare
    {200, 120, 80, 150, 200, 120, 80, 150, 200, 120, 80, 150, 200, 120, 80, 150}  // Hat
  },
  // Pattern 2: Minimal techno
  {
    {255, 0, 0, 0, 0, 0, 200, 0, 255, 0, 0, 0, 180, 0, 0, 0},   // Kick
    {0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0},       // Snare
    {180, 120, 0, 120, 180, 120, 0, 120, 180, 120, 0, 120, 180, 120, 0, 120}  // Hat
  },
  // Pattern 3: Hip-hop boom-bap
  {
    {255, 0, 0, 0, 0, 0, 220, 0, 180, 0, 0, 0, 0, 0, 200, 0},   // Kick
    {0, 0, 0, 0, 255, 0, 0, 120, 0, 0, 0, 0, 255, 0, 0, 80},    // Snare
    {150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100}  // Hat
  }
};

// Linear interpolation helper
uint8_t lerp(uint8_t a, uint8_t b, uint8_t amount) {
  return a + (((b - a) * amount) >> 8);
}

// Bilinear interpolation across 4 pattern corners
uint8_t bilinearInterpolate(uint8_t v00, uint8_t v10, uint8_t v01, uint8_t v11, uint8_t x, uint8_t y) {
  uint8_t v0 = lerp(v00, v10, x);
  uint8_t v1 = lerp(v01, v11, x);
  return lerp(v0, v1, y);
}

void regenerateGridsPattern() {
  Serial.printf("Regenerating Grids pattern at X=%d Y=%d\n", grids.patternX, grids.patternY);
  
  // For each step and voice, interpolate between the 4 corner patterns
  for (int step = 0; step < GRIDS_STEPS; step++) {
    // Read corner values from PROGMEM
    uint8_t k00 = pgm_read_byte(&PATTERN_MAP[0][0][step]);
    uint8_t k10 = pgm_read_byte(&PATTERN_MAP[1][0][step]);
    uint8_t k01 = pgm_read_byte(&PATTERN_MAP[2][0][step]);
    uint8_t k11 = pgm_read_byte(&PATTERN_MAP[3][0][step]);
    grids.kickPattern[step] = bilinearInterpolate(k00, k10, k01, k11, grids.patternX, grids.patternY);
    
    uint8_t s00 = pgm_read_byte(&PATTERN_MAP[0][1][step]);
    uint8_t s10 = pgm_read_byte(&PATTERN_MAP[1][1][step]);
    uint8_t s01 = pgm_read_byte(&PATTERN_MAP[2][1][step]);
    uint8_t s11 = pgm_read_byte(&PATTERN_MAP[3][1][step]);
    grids.snarePattern[step] = bilinearInterpolate(s00, s10, s01, s11, grids.patternX, grids.patternY);
    
    uint8_t h00 = pgm_read_byte(&PATTERN_MAP[0][2][step]);
    uint8_t h10 = pgm_read_byte(&PATTERN_MAP[1][2][step]);
    uint8_t h01 = pgm_read_byte(&PATTERN_MAP[2][2][step]);
    uint8_t h11 = pgm_read_byte(&PATTERN_MAP[3][2][step]);
    grids.hatPattern[step] = bilinearInterpolate(h00, h10, h01, h11, grids.patternX, grids.patternY);
  }
}

void initializeGridsMode() {
  Serial.println("\n=== Grids Mode Initialization ===");
  
  grids.step = 0;
  grids.playing = false;
  grids.lastStepTime = 0;
  grids.bpm = 120.0;
  grids.stepInterval = (60000.0 / grids.bpm) / 4.0; // 16th notes
  
  grids.patternX = 128;
  grids.patternY = 128;
  grids.kickDensity = 200;
  grids.snareDensity = 150;
  grids.hatDensity = 180;
  grids.swing = 0;
  grids.accentThreshold = 200;
  
  regenerateGridsPattern();
  
  Serial.printf("BPM: %.1f, Pattern: (%d,%d)\n", grids.bpm, grids.patternX, grids.patternY);
  Serial.println("Grids initialized and drawn");
  
  drawGridsMode();
}

void drawGridsMode() {
  tft.fillScreen(THEME_BG);
  
  // Unified header with BLE, SD, and BPM indicators
  drawModuleHeader("GRIDS");
  
  int y = CONTENT_TOP;
  
  // X/Y Pad area (large touch area like Beatmap)
  int padSize = 200;
  int padX = 240 - padSize/2;
  int padY = y + 10;
  
  // Draw pad background
  tft.fillRect(padX, padY, padSize, padSize, THEME_SURFACE);
  tft.drawRect(padX, padY, padSize, padSize, THEME_TEXT);
  
  // Draw grid lines to show quadrants
  tft.drawFastVLine(padX + padSize/2, padY, padSize, THEME_TEXT_DIM);
  tft.drawFastHLine(padX, padY + padSize/2, padSize, THEME_TEXT_DIM);
  
  // Draw current position marker
  int markerX = padX + (grids.patternX * padSize / 256);
  int markerY = padY + (grids.patternY * padSize / 256);
  tft.fillCircle(markerX, markerY, 6, THEME_PRIMARY);
  tft.drawCircle(markerX, markerY, 7, THEME_TEXT);
  
  // Pattern labels at corners (small text)
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawString("HOUSE", padX + 5, padY + 5, 1);
  tft.drawRightString("FUNK", padX + padSize - 5, padY + 5, 1);
  tft.drawString("TECH", padX + 5, padY + padSize - 15, 1);
  tft.drawRightString("HIP", padX + padSize - 5, padY + padSize - 15, 1);
  
  y += padSize + 20;
  
  // Density sliders visualization (below pad)
  int sliderY = y;
  int sliderW = 120;
  int sliderH = 20;
  int sliderSpacing = 10;
  
  // Kick density
  tft.setTextColor(THEME_ERROR, THEME_BG);
  tft.drawString("K", 20, sliderY, 2);
  tft.drawRect(45, sliderY, sliderW, sliderH, THEME_TEXT);
  int kickFill = (grids.kickDensity * sliderW) / 255;
  if (kickFill > 0) tft.fillRect(46, sliderY + 1, kickFill, sliderH - 2, THEME_ERROR);
  
  // Snare density  
  tft.setTextColor(THEME_WARNING, THEME_BG);
  tft.drawString("S", 180, sliderY, 2);
  tft.drawRect(205, sliderY, sliderW, sliderH, THEME_TEXT);
  int snareFill = (grids.snareDensity * sliderW) / 255;
  if (snareFill > 0) tft.fillRect(206, sliderY + 1, snareFill, sliderH - 2, THEME_WARNING);
  
  // Hat density
  tft.setTextColor(THEME_ACCENT, THEME_BG);
  tft.drawString("H", 340, sliderY, 2);
  tft.drawRect(365, sliderY, sliderW, sliderH, THEME_TEXT);
  int hatFill = (grids.hatDensity * sliderW) / 255;
  if (hatFill > 0) tft.fillRect(366, sliderY + 1, hatFill, sliderH - 2, THEME_ACCENT);
  
  // Control buttons at bottom - calculated from screen dimensions
  int btnSpacing = 10;
  int btnY = SCREEN_HEIGHT - 60;
  int btnH = 50;
  int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;  // 4 buttons instead of 5
  
  drawRoundButton(10, btnY, btnW, btnH, grids.playing ? "STOP" : "PLAY", THEME_PRIMARY, false);
  drawRoundButton(100, btnY, btnW, btnH, "BPM-", THEME_SECONDARY, false);
  drawRoundButton(190, btnY, btnW, btnH, "BPM+", THEME_SECONDARY, false);
  drawRoundButton(280, btnY, btnW, btnH, "RNDM", THEME_ACCENT, false);
}

void handleGridsMode() {
  updateTouch();
  
  // Handle playback timing
  if (grids.playing) {
    unsigned long now = millis();
    grids.stepInterval = (60000.0 / grids.bpm) / 4.0; // 16th notes
    
    if (now - grids.lastStepTime >= grids.stepInterval) {
      grids.lastStepTime = now;
      
      // Check each voice against its density threshold
      bool kickTrigger = grids.kickPattern[grids.step] >= (255 - grids.kickDensity);
      bool snareTrigger = grids.snarePattern[grids.step] >= (255 - grids.snareDensity);
      bool hatTrigger = grids.hatPattern[grids.step] >= (255 - grids.hatDensity);
      
      // Determine velocity (accent)
      uint8_t kickVel = grids.kickPattern[grids.step] >= grids.accentThreshold ? 127 : 100;
      uint8_t snareVel = grids.snarePattern[grids.step] >= grids.accentThreshold ? 127 : 100;
      uint8_t hatVel = grids.hatPattern[grids.step] >= grids.accentThreshold ? 127 : 90;
      
      // Send MIDI notes
      if (kickTrigger) {
        sendNoteOn(grids.kickNote, kickVel);
        sendNoteOff(grids.kickNote); // Immediate note off for drums
      }
      if (snareTrigger) {
        sendNoteOn(grids.snareNote, snareVel);
        sendNoteOff(grids.snareNote);
      }
      if (hatTrigger) {
        sendNoteOn(grids.hatNote, hatVel);
        sendNoteOff(grids.hatNote);
      }
      
      // Advance step
      grids.step = (grids.step + 1) % GRIDS_STEPS;
    }
  }
  
  if (touch.justPressed) {
    // Check back button from header first
    if (isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
      if (grids.playing) {
        grids.playing = false;
      }
      exitToMenu();
      return;
    }
    
    // Check X/Y pad (200x200 area)
    int padSize = 200;
    int padX = 240 - padSize/2;
    int padY = CONTENT_TOP + 10;
    
    if (touch.x >= padX && touch.x < padX + padSize && 
        touch.y >= padY && touch.y < padY + padSize) {
      // Update pattern position
      grids.patternX = ((touch.x - padX) * 255) / padSize;
      grids.patternY = ((touch.y - padY) * 255) / padSize;
      regenerateGridsPattern();
      drawGridsMode();
      Serial.printf("Pattern moved to (%d, %d)\n", grids.patternX, grids.patternY);
      return;
    }
    
    // Check control buttons - calculate matching drawGridsMode
    int btnSpacing = 10;
    int btnY = SCREEN_HEIGHT - 60;
    int btnH = 50;
    int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;  // 4 buttons instead of 5
    
    // Check button press states
    bool playPressed = touch.isPressed && isButtonPressed(10, btnY, btnW, btnH);
    bool bpmDownPressed = touch.isPressed && isButtonPressed(100, btnY, btnW, btnH);
    bool bpmUpPressed = touch.isPressed && isButtonPressed(190, btnY, btnW, btnH);
    bool randomPressed = touch.isPressed && isButtonPressed(280, btnY, btnW, btnH);
    
    // Only redraw buttons when pressed
    if (playPressed || bpmDownPressed || bpmUpPressed || randomPressed) {
      drawRoundButton(10, btnY, btnW, btnH, grids.playing ? "STOP" : "PLAY", THEME_PRIMARY, playPressed);
      drawRoundButton(100, btnY, btnW, btnH, "BPM-", THEME_SECONDARY, bpmDownPressed);
      drawRoundButton(190, btnY, btnW, btnH, "BPM+", THEME_SECONDARY, bpmUpPressed);
      drawRoundButton(280, btnY, btnW, btnH, "RNDM", THEME_ACCENT, randomPressed);
    }
    if (playPressed) {
      grids.playing = !grids.playing;
      if (grids.playing) {
        grids.step = 0;
        grids.lastStepTime = millis();
      }
      drawGridsMode();
      Serial.printf("Grids %s\n", grids.playing ? "started" : "stopped");
      return;
    }
    
    // BPM-
    if (bpmDownPressed) {
      grids.bpm = constrain(grids.bpm - 5, GRIDS_MIN_BPM, GRIDS_MAX_BPM);
      drawGridsMode();
      Serial.printf("BPM: %.1f\n", grids.bpm);
      return;
    }
    
    // BPM+
    if (bpmUpPressed) {
      grids.bpm = constrain(grids.bpm + 5, GRIDS_MIN_BPM, GRIDS_MAX_BPM);
      drawGridsMode();
      Serial.printf("BPM: %.1f\n", grids.bpm);
      return;
    }
    
    // RANDOM
    if (randomPressed) {
      grids.patternX = random(256);
      grids.patternY = random(256);
      regenerateGridsPattern();
      drawGridsMode();
      Serial.printf("Random pattern: (%d, %d)\n", grids.patternX, grids.patternY);
      return;
    }
    
    // Check density sliders
    int sliderY = CONTENT_TOP + 230;
    int sliderW = 120;
    int sliderH = 20;
    
    // Kick density
    if (touch.y >= sliderY && touch.y < sliderY + sliderH) {
      if (touch.x >= 45 && touch.x < 45 + sliderW) {
        grids.kickDensity = ((touch.x - 45) * 255) / sliderW;
        drawGridsMode();
        Serial.printf("Kick density: %d\n", grids.kickDensity);
        return;
      }
      // Snare density
      if (touch.x >= 205 && touch.x < 205 + sliderW) {
        grids.snareDensity = ((touch.x - 205) * 255) / sliderW;
        drawGridsMode();
        Serial.printf("Snare density: %d\n", grids.snareDensity);
        return;
      }
      // Hat density
      if (touch.x >= 365 && touch.x < 365 + sliderW) {
        grids.hatDensity = ((touch.x - 365) * 255) / sliderW;
        drawGridsMode();
        Serial.printf("Hat density: %d\n", grids.hatDensity);
        return;
      }
    }
  }
}
