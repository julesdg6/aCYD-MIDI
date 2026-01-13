// euclidean_mode.cpp
// Euclidean rhythm generator using Bjorklund's algorithm
// Inspired by Mutable Instruments Grids and Ableton Live's Euclidean sequencer
// Features: 4 independent voices, circular visualization, triplet mode

#include "euclidean_mode.h"
#include "common_definitions.h"
#include "midi_utils.h"

EuclideanState euclideanState;

// Bjorklund's algorithm for generating Euclidean rhythms
void generateEuclideanPattern(EuclideanVoice& voice) {
  // Clear pattern
  for (int i = 0; i < 32; i++) {
    voice.pattern[i] = false;
  }
  
  if (voice.events == 0 || voice.steps == 0) return;
  
  // Bjorklund's algorithm implementation
  int bucket = 0;
  for (int i = 0; i < voice.steps; i++) {
    bucket += voice.events;
    if (bucket >= voice.steps) {
      bucket -= voice.steps;
      // Apply rotation
      int pos = (i + voice.rotation) % voice.steps;
      if (pos < 0) pos += voice.steps;
      voice.pattern[pos] = true;
    }
  }
}

void initializeEuclideanMode() {
  // Initialize 4 voices with classic patterns
  // Voice 0 (Red): Kick - every 4th (4/16)
  euclideanState.voices[0] = {16, 4, 0, 36, TFT_RED, {false}};  // C1 kick
  
  // Voice 1 (Yellow): Snare - backbeat (4/16 rotated)
  euclideanState.voices[1] = {16, 4, 2, 38, TFT_YELLOW, {false}};  // D1 snare
  
  // Voice 2 (Green): Hi-hat - 8/16
  euclideanState.voices[2] = {16, 8, 0, 42, TFT_GREEN, {false}};  // F#1 closed hat
  
  // Voice 3 (Cyan): Percussion - 5/16 (interesting pattern)
  euclideanState.voices[3] = {16, 5, 0, 39, TFT_CYAN, {false}};  // D#1 clap
  
  // Generate all patterns
  for (int i = 0; i < 4; i++) {
    generateEuclideanPattern(euclideanState.voices[i]);
  }
  
  euclideanState.bpm = 120;
  euclideanState.currentStep = 0;
  euclideanState.isPlaying = false;
  euclideanState.lastStepTime = 0;
  euclideanState.selectedVoice = 0;
  euclideanState.tripletMode = false;
  
  Serial.println("Euclidean mode initialized");
  drawEuclideanMode();
}

void drawEuclideanMode() {
  tft.fillScreen(THEME_BG);
  
  // Unified header with BLE, SD, and BPM indicators
  drawModuleHeader("EUCLIDEAN");
  
  // Draw circular visualization - calculated from screen dimensions
  int centerX = SCREEN_WIDTH / 3;
  int centerY = (SCREEN_HEIGHT - CONTENT_TOP) / 2 + CONTENT_TOP;
  int radius = min(SCREEN_WIDTH / 4, (SCREEN_HEIGHT - CONTENT_TOP) / 3);
  
  // Draw concentric circles for each voice
  int radiusStep = radius / 5;
  for (int v = 3; v >= 0; v--) {
    int r = radius - (v * radiusStep);
    tft.drawCircle(centerX, centerY, r, euclideanState.voices[v].color);
    
    // Draw step markers
    for (int s = 0; s < euclideanState.voices[v].steps; s++) {
      float angle = (s * TWO_PI / euclideanState.voices[v].steps) - HALF_PI;
      int x = centerX + cos(angle) * r;
      int y = centerY + sin(angle) * r;
      
      if (euclideanState.voices[v].pattern[s]) {
        // Event marker - filled circle
        tft.fillCircle(x, y, 4, euclideanState.voices[v].color);
      } else {
        // Non-event - small dot
        tft.drawPixel(x, y, euclideanState.voices[v].color);
      }
      
      // Highlight current step
      if (euclideanState.isPlaying && s == euclideanState.currentStep) {
        tft.drawCircle(x, y, 6, TFT_WHITE);
      }
    }
  }
  
  // Control panel (right side) - calculated from screen dimensions
  int controlX = SCREEN_WIDTH - 90;
  int controlY = CONTENT_TOP;
  int availableHeight = SCREEN_HEIGHT - CONTENT_TOP - 20;
  int rowHeight = availableHeight / 4;
  
  for (int v = 0; v < 4; v++) {
    int y = controlY + (v * rowHeight);
    
    // Voice indicator
    tft.fillRect(controlX - 15, y + 5, 8, 45, euclideanState.voices[v].color);
    
    // Steps control
    tft.setTextSize(1);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.setCursor(controlX, y);
    tft.print("Steps");
    tft.fillRoundRect(controlX, y + 12, 35, 20, 3, 
                      v == euclideanState.selectedVoice ? euclideanState.voices[v].color : THEME_ACCENT);
    tft.setTextColor(THEME_BG, v == euclideanState.selectedVoice ? euclideanState.voices[v].color : THEME_ACCENT);
    tft.setCursor(controlX + 8, y + 17);
    tft.print(euclideanState.voices[v].steps);
    
    // Events control
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.setCursor(controlX, y + 35);
    tft.print("Events");
    tft.fillRoundRect(controlX + 40, y + 12, 35, 20, 3, THEME_ACCENT);
    tft.setTextColor(THEME_BG, THEME_ACCENT);
    tft.setCursor(controlX + 48, y + 17);
    tft.print(euclideanState.voices[v].events);
    
    // Rotation control
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.setCursor(controlX + 40, y + 35);
    tft.print("Rot");
    tft.fillRoundRect(controlX + 63, y + 35, 25, 17, 3, THEME_ACCENT);
    tft.setTextColor(THEME_BG, THEME_ACCENT);
    tft.setCursor(controlX + 67, y + 38);
    if (euclideanState.voices[v].rotation >= 0) tft.print("+");
    tft.print(euclideanState.voices[v].rotation);
  }
  
  // Global controls at bottom
  int bottomY = 280;
  
  // Play/Stop button
  tft.fillRoundRect(10, bottomY, 70, 35, 5, euclideanState.isPlaying ? TFT_RED : TFT_GREEN);
  tft.setTextColor(THEME_BG, euclideanState.isPlaying ? TFT_RED : TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(euclideanState.isPlaying ? 22 : 18, bottomY + 10);
  tft.print(euclideanState.isPlaying ? "STOP" : "PLAY");
  
  // BPM control
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setTextSize(1);
  tft.setCursor(90, bottomY);
  tft.print("BPM");
  tft.fillRoundRect(90, bottomY + 12, 60, 23, 3, THEME_ACCENT);
  tft.setTextColor(THEME_BG, THEME_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(100, bottomY + 16);
  tft.print(euclideanState.bpm);
  
  // Triplet mode toggle
  tft.fillRoundRect(160, bottomY, 80, 35, 5, euclideanState.tripletMode ? THEME_ACCENT : 0x4208);
  tft.setTextColor(euclideanState.tripletMode ? THEME_BG : THEME_TEXT, 
                   euclideanState.tripletMode ? THEME_ACCENT : 0x4208);
  tft.setTextSize(1);
  tft.setCursor(165, bottomY + 5);
  tft.print("Triplets");
  tft.setTextSize(2);
  tft.setCursor(173, bottomY + 18);
  tft.print(euclideanState.tripletMode ? "x3" : "x2");
  
  // Re-Sync button
  tft.fillRoundRect(250, bottomY, 70, 35, 5, THEME_ACCENT);
  tft.setTextColor(THEME_BG, THEME_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(258, bottomY + 13);
  tft.print("Re-Sync");
}

void playEuclideanStep() {
  // Play all voices that have events at current step
  for (int v = 0; v < 4; v++) {
    if (euclideanState.currentStep < euclideanState.voices[v].steps &&
        euclideanState.voices[v].pattern[euclideanState.currentStep]) {
      sendNoteOn(euclideanState.voices[v].midiNote, 100);
      sendNoteOff(euclideanState.voices[v].midiNote);
    }
  }
}

void updateEuclideanSequencer() {
  if (!euclideanState.isPlaying) return;
  
  // Calculate step duration based on BPM and triplet mode
  unsigned long stepDuration;
  if (euclideanState.tripletMode) {
    // Triplet 16th notes (24 per quarter note)
    stepDuration = (60000 / euclideanState.bpm) / 6; // 6 triplets per quarter
  } else {
    // Regular 16th notes
    stepDuration = (60000 / euclideanState.bpm) / 4; // 4 sixteenths per quarter
  }
  
  unsigned long currentTime = millis();
  if (currentTime - euclideanState.lastStepTime >= stepDuration) {
    euclideanState.lastStepTime = currentTime;
    
    playEuclideanStep();
    
    // Advance to next step (use max steps from all voices)
    euclideanState.currentStep++;
    uint8_t maxSteps = 0;
    for (int v = 0; v < 4; v++) {
      if (euclideanState.voices[v].steps > maxSteps) {
        maxSteps = euclideanState.voices[v].steps;
      }
    }
    if (euclideanState.currentStep >= maxSteps) {
      euclideanState.currentStep = 0;
    }
    
    // Update display (just the step markers)
    drawEuclideanMode();
  }
}

void handleEuclideanMode() {
  // Check for touch input
  if (touch.justPressed) {
    int touchX = touch.x;
    int touchY = touch.y;
    
    // Play/Stop button
    if (touchX >= 10 && touchX <= 80 && touchY >= 280 && touchY <= 315) {
      euclideanState.isPlaying = !euclideanState.isPlaying;
      if (euclideanState.isPlaying) {
        euclideanState.currentStep = 0;
        euclideanState.lastStepTime = millis();
      }
      drawEuclideanMode();
    }
    
    // BPM control
    else if (touchX >= 90 && touchX <= 150 && touchY >= 280 && touchY <= 315) {
      euclideanState.bpm += 5;
      if (euclideanState.bpm > 240) euclideanState.bpm = 40;
      drawEuclideanMode();
    }
    
    // Triplet mode toggle
    else if (touchX >= 160 && touchX <= 240 && touchY >= 280 && touchY <= 315) {
      euclideanState.tripletMode = !euclideanState.tripletMode;
      drawEuclideanMode();
    }
    
    // Re-Sync button
    else if (touchX >= 250 && touchX <= 320 && touchY >= 280 && touchY <= 315) {
      euclideanState.currentStep = 0;
      euclideanState.lastStepTime = millis();
      drawEuclideanMode();
    }
    
    // Voice controls
    int controlX = 390;
    int controlY = 40;
    int rowHeight = 62;
    
    for (int v = 0; v < 4; v++) {
      int y = controlY + (v * rowHeight);
      
      // Steps control
      if (touchX >= controlX && touchX <= controlX + 35 && touchY >= y + 12 && touchY <= y + 32) {
        euclideanState.selectedVoice = v;
        euclideanState.voices[v].steps++;
        if (euclideanState.voices[v].steps > 32) euclideanState.voices[v].steps = 1;
        if (euclideanState.voices[v].events > euclideanState.voices[v].steps) {
          euclideanState.voices[v].events = euclideanState.voices[v].steps;
        }
        generateEuclideanPattern(euclideanState.voices[v]);
        drawEuclideanMode();
      }
      
      // Events control
      else if (touchX >= controlX + 40 && touchX <= controlX + 75 && touchY >= y + 12 && touchY <= y + 32) {
        euclideanState.voices[v].events++;
        if (euclideanState.voices[v].events > euclideanState.voices[v].steps) {
          euclideanState.voices[v].events = 0;
        }
        generateEuclideanPattern(euclideanState.voices[v]);
        drawEuclideanMode();
      }
      
      // Rotation control
      else if (touchX >= controlX + 63 && touchX <= controlX + 88 && touchY >= y + 35 && touchY <= y + 52) {
        euclideanState.voices[v].rotation++;
        if (euclideanState.voices[v].rotation > euclideanState.voices[v].steps) {
          euclideanState.voices[v].rotation = -euclideanState.voices[v].steps;
        }
        generateEuclideanPattern(euclideanState.voices[v]);
        drawEuclideanMode();
      }
    }
    
    // Back button
    if (isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
      currentMode = MENU;
      return;
    }
  }
  
  // Update sequencer if playing
  updateEuclideanSequencer();
}
