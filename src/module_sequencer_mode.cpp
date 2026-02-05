#include "module_sequencer_mode.h"
#include "clock_manager.h"
#include <cstring>

bool sequencePattern[SEQ_TRACKS][SEQ_STEPS] = {};
int currentStep = 0;
unsigned long noteOffTime[SEQ_TRACKS] = {0};
static SequencerSyncState sequencerSync;
static const uint8_t kDrumNotes[SEQ_TRACKS] = {36, 38, 42, 46};

static bool sequencerModuleRunning() {
  return sequencerSync.playing || sequencerSync.startPending;
}

static void changeSequencerTempo(int delta) {
  int target = sharedBPM + delta;
  if (target < 40) {
    target = 40;
  }
  if (target > 240) {
    target = 240;
  }
  if (target == sharedBPM) {
    return;
  }
  setSharedBPM(target);
  requestRedraw();
}

// Implementations
void initializeSequencerMode() {
  currentStep = 0;
  sequencerSync.reset();
  memset(noteOffTime, 0, sizeof(noteOffTime));
  
  // Clear all patterns
  for (int t = 0; t < SEQ_TRACKS; t++) {
    for (int s = 0; s < SEQ_STEPS; s++) {
      sequencePattern[t][s] = false;
    }
  }
}

void drawSequencerMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("BEATS", String(sharedBPM) + " BPM");
  
  drawSequencerGrid();
  
  // Transport controls - positioned to avoid overlap
  int ctrlY = SCALE_Y(200);
  // Play button: show STOP when playing, PENDING (orange) when start is pending,
  // otherwise show PLAY (green).
  const char *seqPlayLabel;
  uint16_t seqPlayColor;
  if (sequencerSync.playing) {
    seqPlayLabel = "STOP";
    seqPlayColor = THEME_ERROR;
  } else if (sequencerSync.startPending) {
    seqPlayLabel = "PENDING";
    seqPlayColor = THEME_SECONDARY;
  } else {
    seqPlayLabel = "PLAY";
    seqPlayColor = THEME_SUCCESS;
  }
  drawRoundButton(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, seqPlayLabel, seqPlayColor);
  drawRoundButton(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "CLEAR", THEME_WARNING);
  drawRoundButton(SCALE_X(130), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "BPM-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(180), ctrlY, BTN_SMALL_W, BTN_SMALL_H, "BPM+", THEME_SECONDARY);
  
  // BPM display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString(String(sharedBPM), SCALE_X(240), ctrlY + SCALE_Y(7), 2);
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
      bool current = (sequencerSync.playing && step == currentStep);
      
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
    if (sequencerModuleRunning()) {
      sequencerSync.stopPlayback();
      for (int track = 0; track < SEQ_TRACKS; track++) {
        if (noteOffTime[track] > 0) {
          sendMIDI(0x80, kDrumNotes[track], 0);
          noteOffTime[track] = 0;
        }
      }
    }
    exitToMenu();
    return;
  }
  
  int ctrlY = SCALE_Y(200);
  // Handle touch input
  if (touch.justPressed) {
    // Transport controls
    if (isButtonPressed(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      if (sequencerModuleRunning()) {
        sequencerSync.stopPlayback();
        for (int track = 0; track < SEQ_TRACKS; track++) {
          if (noteOffTime[track] > 0) {
            sendMIDI(0x80, kDrumNotes[track], 0);
            noteOffTime[track] = 0;
          }
        }
      } else {
        currentStep = 0;
        sequencerSync.requestStart();
      }
      requestRedraw();
      return;
    }
    
    if (isButtonPressed(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      // Clear all patterns
      for (int t = 0; t < SEQ_TRACKS; t++) {
        for (int s = 0; s < SEQ_STEPS; s++) {
          sequencePattern[t][s] = false;
        }
      }
      requestRedraw();
      return;
    }
    
    if (isButtonPressed(SCALE_X(130), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      changeSequencerTempo(-1);
      return;
    }
    
    if (isButtonPressed(SCALE_X(180), ctrlY, BTN_SMALL_W, BTN_SMALL_H)) {
      changeSequencerTempo(+1);
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
          requestRedraw();
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
  unsigned long now = millis();

  // Check for notes to turn off
  for (int track = 0; track < SEQ_TRACKS; track++) {
    if (noteOffTime[track] > 0 && now >= noteOffTime[track]) {
      sendMIDI(0x80, kDrumNotes[track], 0);
      noteOffTime[track] = 0;
    }
  }

  if (!sequencerModuleRunning()) {
    return;
  }

  bool wasPlaying = sequencerSync.playing;
  bool justStarted = sequencerSync.tryStartIfReady(!instantStartMode) && !wasPlaying;
  if (justStarted) {
    currentStep = 0;
  }
  if (!sequencerSync.playing) {
    return;
  }

  // Use consumeReadySteps instead of ISR callbacks for reliability
  uint32_t readySteps = sequencerSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);
  
  if (readySteps == 0) {
    return;
  }

#ifdef DEBUG_SEQUENCER
  Serial.printf("[SEQ] readySteps=%u currentStep=%u\n", readySteps, currentStep);
#endif

  for (uint32_t i = 0; i < readySteps; ++i) {
    playSequencerStep();
    currentStep = (currentStep + 1) % SEQ_STEPS;
  }
  requestRedraw();  // Request redraw to trigger render event for animation
}

void playSequencerStep() {
  if (!deviceConnected) return;

  int noteLengths[] = {200, 150, 50, 300}; // Note lengths in ms

  unsigned long now = millis();

  for (int track = 0; track < SEQ_TRACKS; track++) {
    if (sequencePattern[track][currentStep]) {
      // Turn on note
      sendMIDI(0x90, kDrumNotes[track], 100);
      // Schedule note off
      noteOffTime[track] = now + noteLengths[track];
    }
  }
}
