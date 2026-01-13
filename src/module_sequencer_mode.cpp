#include "module_sequencer_mode.h"

bool sequencePattern[SEQ_TRACKS][SEQ_STEPS] = {};
int currentStep = 0;
unsigned long lastStepTime = 0;
unsigned long noteOffTime[SEQ_TRACKS] = {0};
int bpm = 120;
int stepInterval = 0;
bool sequencerPlaying = false;

// Implementations
void initializeSequencerMode() {
  bpm = 120;
  stepInterval = 60000 / bpm / 4; // 16th notes
  sequencerPlaying = false;
  currentStep = 0;
  
  // Clear all patterns
  for (int t = 0; t < SEQ_TRACKS; t++) {
    for (int s = 0; s < SEQ_STEPS; s++) {
      sequencePattern[t][s] = false;
    }
  }
}

void drawSequencerMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("BEATS", String(bpm) + " BPM");
  
  drawSequencerGrid();
  
  // Transport controls - positioned to avoid overlap
  int ctrlY = SCALE_Y(200);
  drawRoundButton(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, sequencerPlaying ? "STOP" : "PLAY", 
                 sequencerPlaying ? THEME_ERROR : THEME_SUCCESS);
  drawRoundButton(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "CLEAR", THEME_WARNING);
  drawRoundButton(SCALE_X(130), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "BPM-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(180), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "BPM+", THEME_SECONDARY);
  
  // BPM display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString(String(bpm), SCALE_X(240), ctrlY + SCALE_Y(7), 2);
}

void drawSequencerGrid() {
  int gridX = MARGIN_SMALL;
  int gridY = HEADER_HEIGHT + SCALE_Y(5);
  int cellW = SCALE_X(15);
  int cellH = SCALE_Y(28);
  int spacing = SCALE_X(1);
  
  // 808-style track labels and colors
  String trackLabels[] = {"KICK", "SNRE", "HHAT", "OPEN"};
  uint16_t trackColors[] = {THEME_ERROR, THEME_WARNING, THEME_PRIMARY, THEME_ACCENT};
  
  for (int track = 0; track < SEQ_TRACKS; track++) {
    int y = gridY + track * (cellH + spacing + SCALE_Y(3));
    
    // Track name with color coding
    tft.setTextColor(trackColors[track], THEME_BG);
    tft.drawString(trackLabels[track], gridX, y + SCALE_Y(12), 1);
    
    // Steps - 16 steps in 808 style
    for (int step = 0; step < SEQ_STEPS; step++) {
      int x = gridX + SCALE_X(35) + step * (cellW + spacing);
      
      bool active = sequencePattern[track][step];
      bool current = (sequencerPlaying && step == currentStep);
      
      uint16_t color;
      if (current && active) color = THEME_TEXT;
      else if (current) color = trackColors[track];
      else if (active) color = trackColors[track];
      else color = THEME_SURFACE;
      
      // Highlight every 4th step (like 808)
      if (step % 4 == 0) {
        tft.drawRect(x-1, y-1, cellW+2, cellH+2, THEME_TEXT_DIM);
      }
      
      tft.fillRect(x, y, cellW, cellH, color);
      tft.drawRect(x, y, cellW, cellH, THEME_TEXT_DIM);
    }
  }
}

void handleSequencerMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    sequencerPlaying = false;
    exitToMenu();
    return;
  }
  
  int ctrlY = SCALE_Y(200);
  // Handle touch input
  if (touch.justPressed) {
    // Transport controls
    if (isButtonPressed(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      sequencerPlaying = !sequencerPlaying;
      if (sequencerPlaying) {
        currentStep = 0;
        lastStepTime = millis();
      }
      drawSequencerMode();
      return;
    }
    
    if (isButtonPressed(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      // Clear all patterns
      for (int t = 0; t < SEQ_TRACKS; t++) {
        for (int s = 0; s < SEQ_STEPS; s++) {
          sequencePattern[t][s] = false;
        }
      }
      drawSequencerGrid();
      return;
    }
    
    if (isButtonPressed(SCALE_X(130), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      bpm = max(60, bpm - 1);
      stepInterval = 60000 / bpm / 4;
      drawSequencerMode();
      return;
    }
    
    if (isButtonPressed(SCALE_X(180), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      bpm = min(200, bpm + 1);
      stepInterval = 60000 / bpm / 4;
      drawSequencerMode();
      return;
    }
    
    // Grid interaction
    int gridX = MARGIN_SMALL + SCALE_X(35);
    int gridY = HEADER_HEIGHT + SCALE_Y(5);
    int cellW = SCALE_X(15);
    int cellH = SCALE_Y(28);
    int spacing = SCALE_X(1);
    
    for (int track = 0; track < SEQ_TRACKS; track++) {
      for (int step = 0; step < SEQ_STEPS; step++) {
        int x = gridX + step * (cellW + spacing);
        int y = gridY + track * (cellH + spacing + SCALE_Y(3));
        
        if (isButtonPressed(x, y, cellW, cellH)) {
          toggleSequencerStep(track, step);
          drawSequencerGrid();
          return;
        }
      }
    }
  }
  
  // Update sequencer timing
  updateSequencer();
}

void toggleSequencerStep(int track, int step) {
  sequencePattern[track][step] = !sequencePattern[track][step];
}

void updateSequencer() {
  if (!sequencerPlaying) return;
  
  unsigned long now = millis();
  
  // Check for notes to turn off
  int drumNotes[] = {36, 38, 42, 46};
  for (int track = 0; track < SEQ_TRACKS; track++) {
    if (noteOffTime[track] > 0 && now >= noteOffTime[track]) {
      sendMIDI(0x80, drumNotes[track], 0);
      noteOffTime[track] = 0;
    }
  }
  
  if (now - lastStepTime >= stepInterval) {
    playSequencerStep();
    currentStep = (currentStep + 1) % SEQ_STEPS;
    lastStepTime = now;
    drawSequencerGrid();
  }
}

void playSequencerStep() {
  if (!deviceConnected) return;
  
  int drumNotes[] = {36, 38, 42, 46}; // Kick, Snare, Hi-hat, Open Hi-hat
  int noteLengths[] = {200, 150, 50, 300}; // Note lengths in ms
  
  unsigned long now = millis();
  
  for (int track = 0; track < SEQ_TRACKS; track++) {
    if (sequencePattern[track][currentStep]) {
      // Turn on note
      sendMIDI(0x90, drumNotes[track], 100);
      // Schedule note off
      noteOffTime[track] = now + noteLengths[track];
    }
  }
}
