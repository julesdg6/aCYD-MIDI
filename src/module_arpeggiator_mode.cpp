#include "module_arpeggiator_mode.h"
#include "clock_manager.h"

#include <algorithm>

Arpeggiator arp;
static SequencerSyncState arpSync;

static inline bool arpActive() {
  return arpSync.playing || arpSync.startPending;
}

String patternNames[] = {"UP", "DOWN", "UP/DN", "RAND", "CHANCE"};
String chordTypeNames[] = {"MAJ", "MIN", "7TH"};
int pianoOctave = 4;

// Implementations
void initializeArpeggiatorMode() {
  arp.scaleType = 0;
  arp.chordType = 0;
  arp.pattern = 0;
  arp.octaves = 2;
  arp.speed = 8;
  // arp.bpm removed - uses global sharedBPM
  arp.currentStep = 0;
  arp.currentNote = -1;
  arp.triggeredKey = -1;
  arp.triggeredOctave = 4;
  pianoOctave = 4;
  calculateStepInterval();
  arp.tickAccumulator = 0.0f;
  arpSync.reset();
}

void drawArpeggiatorMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("ARPEGGIATOR", "Piano Chord Arps");
  
  drawArpControls();
  drawPianoKeys();
}

void drawArpControls() {
  int y = HEADER_HEIGHT + SCALE_Y(5);
  int lineSpacing = SCALE_Y(32);
  
  // Row 1: PATTERN Group (Pattern type + navigation)
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("PATTERN", MARGIN_SMALL, y, 2);
  
  drawRoundButton(MARGIN_SMALL + SCALE_X(70), y, SCALE_X(80), SCALE_Y(28), 
                  patternNames[arp.pattern], THEME_WARNING, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(155), y, SCALE_X(30), SCALE_Y(28), 
                  "<", THEME_SECONDARY, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(190), y, SCALE_X(30), SCALE_Y(28), 
                  ">", THEME_SECONDARY, false, 2);
  
  // Chord Type (on same row, right side)
  tft.drawString("TYPE", DISPLAY_WIDTH / 2 + SCALE_X(10), y, 2);
  drawRoundButton(DISPLAY_WIDTH / 2 + SCALE_X(55), y, SCALE_X(70), SCALE_Y(28), 
                  chordTypeNames[arp.chordType], THEME_ACCENT, false, 2);
  
  y += lineSpacing;
  
  // Row 2: TIMING Group (Speed + Octave range)
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("SPEED", MARGIN_SMALL, y, 2);
  
  String speedText;
  if (arp.speed == 4) speedText = "1/4";
  else if (arp.speed == 8) speedText = "1/8";
  else if (arp.speed == 16) speedText = "1/16";
  else speedText = "1/32";
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString(speedText, MARGIN_SMALL + SCALE_X(60), y + SCALE_Y(4), 4);
  
  drawRoundButton(MARGIN_SMALL + SCALE_X(115), y, SCALE_X(30), SCALE_Y(28), 
                  "+", THEME_SECONDARY, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(150), y, SCALE_X(30), SCALE_Y(28), 
                  "-", THEME_SECONDARY, false, 2);
  
  // Octave range (right side)
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("OCTAVES", DISPLAY_WIDTH / 2 + SCALE_X(10), y, 2);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString(String(arp.octaves), DISPLAY_WIDTH / 2 + SCALE_X(90), y + SCALE_Y(4), 4);
  
  drawRoundButton(DISPLAY_WIDTH / 2 + SCALE_X(115), y, SCALE_X(30), SCALE_Y(28), 
                  "-", THEME_SECONDARY, false, 2);
  drawRoundButton(DISPLAY_WIDTH / 2 + SCALE_X(150), y, SCALE_X(30), SCALE_Y(28), 
                  "+", THEME_SECONDARY, false, 2);
  
  y += lineSpacing;
  
  // Row 3: Piano Octave + Step Indicator
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("KEYBOARD", MARGIN_SMALL, y, 2);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Oct " + String(pianoOctave), MARGIN_SMALL + SCALE_X(85), y + SCALE_Y(4), 4);
  
  drawRoundButton(MARGIN_SMALL + SCALE_X(145), y, SCALE_X(30), SCALE_Y(28), 
                  "-", THEME_SECONDARY, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(180), y, SCALE_X(30), SCALE_Y(28), 
                  "+", THEME_SECONDARY, false, 2);
  
  // Step indicator / Current status (right side)
  if (arpSync.playing && arp.triggeredKey != -1) {
    String keyName = getNoteNameFromMIDI(arp.triggeredKey);
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Playing:", DISPLAY_WIDTH / 2, y, 2);
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    tft.drawString(keyName + " " + chordTypeNames[arp.chordType], 
                   DISPLAY_WIDTH / 2, y + SCALE_Y(14), 2);
    
    // Visual step indicator (dots showing current step)
    int dotY = y + SCALE_Y(3);
    int chordLength = (arp.chordType == 2) ? 4 : 3;
    int totalSteps = chordLength * arp.octaves;
    int dotsToShow = min(totalSteps, 8);  // Max 8 dots to fit
    int dotSpacing = SCALE_X(12);
    int dotStartX = DISPLAY_WIDTH - MARGIN_SMALL - (dotsToShow * dotSpacing);
    
    for (int i = 0; i < dotsToShow; i++) {
      bool isActive = (i == (arp.currentStep % totalSteps));
      uint16_t dotColor = isActive ? THEME_ACCENT : THEME_TEXT_DIM;
      int dotSize = isActive ? SCALE_X(4) : SCALE_X(3);
      tft.fillCircle(dotStartX + (i * dotSpacing), dotY, dotSize, dotColor);
    }
  }
}

void drawPianoKeys() {
  // Larger keyboard - starts lower on screen, taller keys
  int keyY = SCALE_Y(140);
  int keyWidth = DISPLAY_WIDTH / NUM_PIANO_KEYS;
  int keyHeight = SCALE_Y(60);  // Increased from 45
  
  for (int i = 0; i < NUM_PIANO_KEYS; i++) {
    int x = i * keyWidth;
    int note = (pianoOctave * 12) + i;
    String noteName = getNoteNameFromMIDI(note);
    
    bool isPressed = (arp.triggeredKey == note) && arpActive();
    bool isSharp = (noteName.indexOf('#') != -1);
    
    // Improved contrast - different colors for active/inactive, white/black keys
    uint16_t bgColor, textColor, borderColor;
    
    if (isSharp) {
      // Black keys
      if (isPressed) {
        bgColor = THEME_ACCENT;      // Bright cyan when active
        textColor = THEME_BG;
        borderColor = THEME_ACCENT;
      } else {
        bgColor = THEME_TEXT;         // Dark gray when inactive
        textColor = THEME_SURFACE;
        borderColor = THEME_TEXT_DIM;
      }
    } else {
      // White keys
      if (isPressed) {
        bgColor = THEME_PRIMARY;      // Cyan when active
        textColor = THEME_BG;
        borderColor = THEME_PRIMARY;
      } else {
        bgColor = THEME_SURFACE;      // Light gray when inactive
        textColor = THEME_TEXT;
        borderColor = THEME_TEXT_DIM;
      }
    }
    
    // Draw key with border
    tft.fillRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, bgColor);
    tft.drawRect(x, keyY, keyWidth, keyHeight, borderColor);
    
    // Larger font for note names
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(noteName, x + keyWidth/2, keyY + keyHeight/2 - SCALE_Y(8), 2);
  }
}

void handleArpeggiatorMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    int y = HEADER_HEIGHT + SCALE_Y(5);
    int lineSpacing = SCALE_Y(32);
    
    // Row 1: Pattern navigation
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(155), y, SCALE_X(30), SCALE_Y(28))) {
      arp.pattern = (arp.pattern - 1 + 5) % 5;
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(190), y, SCALE_X(30), SCALE_Y(28))) {
      arp.pattern = (arp.pattern + 1) % 5;
      requestRedraw();
      return;
    }
    
    // Chord type
    if (isButtonPressed(DISPLAY_WIDTH / 2 + SCALE_X(55), y, SCALE_X(70), SCALE_Y(28))) {
      arp.chordType = (arp.chordType + 1) % 3;
      requestRedraw();
      return;
    }
    
    y += lineSpacing;
    
    // Row 2: Speed controls
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(115), y, SCALE_X(30), SCALE_Y(28))) {
      // Faster (+ button)
      if (arp.speed == 32) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 4;
      calculateStepInterval();
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(150), y, SCALE_X(30), SCALE_Y(28))) {
      // Slower (- button)
      if (arp.speed == 4) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 32;
      calculateStepInterval();
      requestRedraw();
      return;
    }
    
    // Octave range controls
    if (isButtonPressed(DISPLAY_WIDTH / 2 + SCALE_X(115), y, SCALE_X(30), SCALE_Y(28))) {
      arp.octaves = max(1, arp.octaves - 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(DISPLAY_WIDTH / 2 + SCALE_X(150), y, SCALE_X(30), SCALE_Y(28))) {
      arp.octaves = min(4, arp.octaves + 1);
      requestRedraw();
      return;
    }
    
    y += lineSpacing;
    
    // Row 3: Piano octave controls
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(145), y, SCALE_X(30), SCALE_Y(28))) {
      pianoOctave = max(1, pianoOctave - 1);
      drawPianoKeys();
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(180), y, SCALE_X(30), SCALE_Y(28))) {
      pianoOctave = min(7, pianoOctave + 1);
      drawPianoKeys();
      requestRedraw();
      return;
    }
    
    // Piano key handling
    int keyY = SCALE_Y(140);
    int keyWidth = DISPLAY_WIDTH / NUM_PIANO_KEYS;
    int keyHeight = SCALE_Y(60);
    
    for (int i = 0; i < NUM_PIANO_KEYS; i++) {
      int x = i * keyWidth;
      if (isButtonPressed(x, keyY, keyWidth, keyHeight)) {
        int note = (pianoOctave * 12) + i;
        
        if (arpActive() && arp.triggeredKey == note) {
          // Stop current arp
          arpSync.stopPlayback();
          if (arp.currentNote != -1) {
            sendMIDI(0x80, arp.currentNote, 0);
            arp.currentNote = -1;
          }
          arp.triggeredKey = -1;
        } else {
          // Start new arp
          if (arp.currentNote != -1) {
            sendMIDI(0x80, arp.currentNote, 0);
            arp.currentNote = -1;
          }
          arp.triggeredKey = note;
          arp.triggeredOctave = pianoOctave;
          arp.currentStep = 0;
          arp.tickAccumulator = 0.0f;
          arpSync.requestStart();
        }
        drawPianoKeys();
        drawArpControls();
        return;
      }
    }
  }
  
  // Update arpeggiator
  updateArpeggiator();
}

void updateArpeggiator() {
  bool wasPlaying = arpSync.playing;
  bool justStarted = arpSync.tryStartIfReady(!instantStartMode) && !wasPlaying;
  if (justStarted) {
    arp.currentStep = 0;
    arp.tickAccumulator = 0.0f;
    arp.currentNote = -1;
  }
  if (!arpSync.playing) {
    return;
  }
  
  // Use consumeReadySteps instead of ISR callbacks for reliability
  uint32_t readySteps = arpSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);
  
  if (readySteps == 0) {
    return;
  }
  
  Serial.printf("[ARP] readySteps=%u tickAccumulator=%.2f stepTicks=%.2f\n", readySteps, arp.tickAccumulator, arp.stepTicks);
  
  arp.tickAccumulator += static_cast<float>(readySteps);
  while (arp.tickAccumulator >= arp.stepTicks) {
    playArpNote();
    arp.tickAccumulator -= arp.stepTicks;
  }
}

void playArpNote() {
  // Turn off previous note
  if (arp.currentNote != -1) {
    sendMIDI(0x80, arp.currentNote, 0);
  }
  
  // Check if we should skip this note (for CHANCE pattern)
  if (arp.pattern == 4) { // CHANCE pattern
    if (random(100) < 30) { // 30% chance to skip
      arp.currentNote = -1;
      return; // Skip this note
    }
  }
  
  // Get next chord tone
  arp.currentNote = getArpNote();
  
  // Play single note
  sendMIDI(0x90, arp.currentNote, 100);
  
  // Update display
  drawArpControls();
}

int getArpNote() {
  // Generate chord intervals based on chord type
  int chordIntervals[4];
  int chordLength;
  
  switch (arp.chordType) {
    case 0: // Major
      chordIntervals[0] = 0; chordIntervals[1] = 4; chordIntervals[2] = 7;
      chordLength = 3;
      break;
    case 1: // Minor
      chordIntervals[0] = 0; chordIntervals[1] = 3; chordIntervals[2] = 7;
      chordLength = 3;
      break;
    case 2: // 7th (dominant)
      chordIntervals[0] = 0; chordIntervals[1] = 4; chordIntervals[2] = 7; chordIntervals[3] = 10;
      chordLength = 4;
      break;
  }
  
  int totalSteps = chordLength * arp.octaves;
  int step = 0;
  
  switch (arp.pattern) {
    case 0: // Up
      step = arp.currentStep % totalSteps;
      arp.currentStep++;
      break;
    case 1: // Down
      step = (totalSteps - 1) - (arp.currentStep % totalSteps);
      arp.currentStep++;
      break;
    case 2: // Up/Down
      {
        int cycle = (totalSteps - 1) * 2;
        int pos = arp.currentStep % cycle;
        if (pos < totalSteps) {
          step = pos;
        } else {
          step = cycle - pos;
        }
        arp.currentStep++;
      }
      break;
    case 3: // Random
      step = random(totalSteps);
      break;
    case 4: // Chance (like UP but with random skips)
      step = arp.currentStep % totalSteps;
      arp.currentStep++;
      break;
  }
  
  int octaveOffset = step / chordLength;
  int chordStep = step % chordLength;
  
  return arp.triggeredKey + chordIntervals[chordStep] + (octaveOffset * 12);
}

void calculateStepInterval() {
  float clampedSpeed = max(arp.speed, 1);
  arp.stepTicks = max(0.125f, 16.0f / clampedSpeed);
}
