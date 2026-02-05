#include "module_arpeggiator_mode.h"
#include "clock_manager.h"
#include <uClock.h>

#include <algorithm>

Arpeggiator arp;
static SequencerSyncState arpSync;

// ISR-safe step counter from uClock step extension
// runtime-assigned base track
static volatile uint32_t arpStepCount = 0;
static volatile uint8_t arpAssignedTrack = 0xFF;
static const uint8_t arpRequestedTracks = 1;
static volatile bool arpAssignedFlag = false;

// ISR callback for uClock step sequencer extension
// ISR callback for uClock step sequencer extension
static void onArpStepISR(uint32_t step, uint8_t track) {
  (void)step;
  if (arpAssignedTrack == 0xFF) {
    arpAssignedTrack = track;
    arpAssignedFlag = true;
    arpStepCount++;
    return;
  }
  if (track >= arpAssignedTrack && track < arpAssignedTrack + arpRequestedTracks) {
    arpStepCount++;
  }
}

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
  arp.bpm = 120;
  arp.currentStep = 0;
  arp.currentNote = -1;
  arp.triggeredKey = -1;
  arp.triggeredOctave = 4;
  pianoOctave = 4;
  calculateStepInterval();
  arp.tickAccumulator = 0.0f;
  arpSync.reset();
  // Step callback registration is done at startup via registerAllStepCallbacks().
}

void registerArpStepCallback() {
  uClock.setOnStep(onArpStepISR, 1);
}

void drawArpeggiatorMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("ARPEGGIATOR", "Piano Chord Arps");
  
  drawArpControls();
  drawPianoKeys();
}

void drawArpControls() {
  int y = 55;
  int spacing = 25;
  
  // Pattern and chord type
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Pattern:", 10, y + 6, 1);
  drawRoundButton(65, y, 60, 25, patternNames[arp.pattern], THEME_WARNING);
  drawRoundButton(130, y, 25, 25, "<", THEME_SECONDARY);
  drawRoundButton(160, y, 25, 25, ">", THEME_SECONDARY);
  
  // Chord type
  tft.drawString("Type:", SCALE_X(200), y + 6, 1);
  drawRoundButton(240, y, 50, 25, chordTypeNames[arp.chordType], THEME_ACCENT);
  
  y += spacing;
  
  // Octaves and Speed
  tft.drawString("Octaves:", 10, y + 6, 1);
  tft.drawString(String(arp.octaves), 70, y + 6, 1);
  drawRoundButton(90, y, 25, 25, "-", THEME_SECONDARY);
  drawRoundButton(120, y, 25, 25, "+", THEME_SECONDARY);
  
  // Speed
  tft.drawString("Speed:", 160, y + 6, 1);
  String speedText;
  if (arp.speed == 4) speedText = "4th";
  else if (arp.speed == 8) speedText = "8th";
  else if (arp.speed == 16) speedText = "16th";
  else if (arp.speed == 32) speedText = "32nd";
  tft.drawString(speedText, 210, y + 6, 1);
  drawRoundButton(240, y, 25, 25, "+", THEME_SECONDARY);
  drawRoundButton(270, y, 25, 25, "-", THEME_SECONDARY);
  
  y += spacing;
  
  // BPM Control
  tft.drawString("BPM:", 10, y + 6, 1);
  tft.drawString(String(arp.bpm), 50, y + 6, 1);
  drawRoundButton(80, y, 25, 25, "-", THEME_SECONDARY);
  drawRoundButton(110, y, 25, 25, "+", THEME_SECONDARY);
  
  y += spacing;
  
  // Piano octave controls
  tft.drawString("Piano Oct:", 10, y + 6, 1);
  tft.drawString(String(pianoOctave), 80, y + 6, 1);
  drawRoundButton(100, y, 25, 25, "-", THEME_SECONDARY);
  drawRoundButton(130, y, 25, 25, "+", THEME_SECONDARY);
  
  // Current status
  if (arpSync.playing && arp.triggeredKey != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    String keyName = getNoteNameFromMIDI(arp.triggeredKey);
    tft.drawString("Arping: " + keyName + " " + chordTypeNames[arp.chordType], 170, y + 6, 1);
  }
  
  y += spacing;
  
  // Current note display
  if (arp.currentNote != -1) {
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    String currentNoteName = getNoteNameFromMIDI(arp.currentNote);
    tft.drawString("♪ " + currentNoteName, 10, y + 6, 2);
  }
}

void drawPianoKeys() {
  int keyY = 160;
  int keyWidth = 320 / NUM_PIANO_KEYS;
  int keyHeight = 45;
  
  for (int i = 0; i < NUM_PIANO_KEYS; i++) {
    int x = i * keyWidth;
    int note = (pianoOctave * 12) + i;
    String noteName = getNoteNameFromMIDI(note);
    
    bool isPressed = (arp.triggeredKey == note) && arpActive();
    uint16_t bgColor = isPressed ? THEME_PRIMARY : THEME_SURFACE;
    uint16_t textColor = isPressed ? THEME_BG : THEME_TEXT;
    
    // Black key styling for sharps
    if (noteName.indexOf('#') != -1) {
      bgColor = isPressed ? THEME_ACCENT : THEME_TEXT;
      textColor = isPressed ? THEME_BG : THEME_SURFACE;
    }
    
    tft.fillRect(x + 1, keyY + 1, keyWidth - 2, keyHeight - 2, bgColor);
    tft.drawRect(x, keyY, keyWidth, keyHeight, THEME_PRIMARY);
    
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(noteName, x + keyWidth/2, keyY + keyHeight/2 - 6, 1);
  }
}

void handleArpeggiatorMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    int y = 55;
    int spacing = 25;
    
    // Pattern controls (first line)
    if (isButtonPressed(130, y, 25, 25)) {
      arp.pattern = (arp.pattern - 1 + 5) % 5;
      drawArpControls();
      return;
    }
    if (isButtonPressed(160, y, 25, 25)) {
      arp.pattern = (arp.pattern + 1) % 5;
      drawArpControls();
      return;
    }
    
    // Chord type control (first line)
    if (isButtonPressed(240, y, 50, 25)) {
      arp.chordType = (arp.chordType + 1) % 3;
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // Octave controls
    if (isButtonPressed(90, y, 25, 25)) {
      arp.octaves = max(1, arp.octaves - 1);
      drawArpControls();
      return;
    }
    if (isButtonPressed(120, y, 25, 25)) {
      arp.octaves = min(4, arp.octaves + 1);
      drawArpControls();
      return;
    }
    
    // Speed controls (+ = faster, - = slower)
    if (isButtonPressed(240, y, 25, 25)) {
      // Faster (+ button)
      if (arp.speed == 32) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 4;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (isButtonPressed(270, y, 25, 25)) {
      // Slower (- button)
      if (arp.speed == 4) arp.speed = 8;
      else if (arp.speed == 8) arp.speed = 16;
      else if (arp.speed == 16) arp.speed = 32;
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // BPM controls
    if (isButtonPressed(80, y, 25, 25)) {
      arp.bpm = max(60, arp.bpm - 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    if (isButtonPressed(110, y, 25, 25)) {
      arp.bpm = min(200, arp.bpm + 5);
      calculateStepInterval();
      drawArpControls();
      return;
    }
    
    y += spacing;
    
    // Piano octave controls
    if (isButtonPressed(100, y, 25, 25)) {
      pianoOctave = max(1, pianoOctave - 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    if (isButtonPressed(130, y, 25, 25)) {
      pianoOctave = min(7, pianoOctave + 1);
      drawPianoKeys();
      drawArpControls();
      return;
    }
    
    // Piano key handling
    int keyY = 160;
    int keyWidth = 320 / NUM_PIANO_KEYS;
    int keyHeight = 45;
    
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
