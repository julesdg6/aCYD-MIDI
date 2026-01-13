/*******************************************************************
 TB-3PO Mode Implementation
 *******************************************************************/

#include "tb3po_mode.h"

TB3POState tb3po;

// Helper: Random bit with probability (0-100)
static bool randBit(int probability) {
  return (random(100) < probability);
}

// Reseed the random generator
static void reseed() {
  if (!tb3po.lockSeed) {
    randomSeed(millis());
    tb3po.seed = random(1, 65536);
  }
}

// Get density for note on/off (gate probability)
static int getOnOffDensity() {
  int noteDens = int(tb3po.density) - 7; // -7 to +7
  return abs(noteDens);
}

// Get density for pitch variety
static int getPitchChangeDensity() {
  return constrain(tb3po.density, 0, 8);
}

// Generate pitch sequence
static void regeneratePitches() {
  randomSeed(tb3po.seed);
  
  const Scale& scale = scales[tb3po.scaleIndex];
  int pitchChangeDens = getPitchChangeDensity();
  
  int availablePitches = 0;
  if (scale.numNotes > 0) {
    if (pitchChangeDens > 7) {
      availablePitches = scale.numNotes - 1;
    } else if (pitchChangeDens < 2) {
      availablePitches = pitchChangeDens; // Root only at lowest
    } else {
      int rangeFromScale = scale.numNotes - 3;
      if (rangeFromScale < 4) rangeFromScale = 4;
      availablePitches = 3 + ((pitchChangeDens - 3) * rangeFromScale / 4);
      availablePitches = constrain(availablePitches, 1, scale.numNotes - 1);
    }
  }
  
  // Reset octave modifiers
  tb3po.oct_ups = 0;
  tb3po.oct_downs = 0;
  
  for (int s = 0; s < TB3PO_MAX_STEPS; s++) {
    // Chance to repeat prior note (more likely at lower pitch density)
    int forceRepeatProb = 50 - (pitchChangeDens * 6);
    if (s > 0 && randBit(forceRepeatProb)) {
      tb3po.notes[s] = tb3po.notes[s - 1];
    } else {
      tb3po.notes[s] = random(0, availablePitches + 1);
      
      // Random octave shifts
      tb3po.oct_ups <<= 1;
      tb3po.oct_downs <<= 1;
      if (randBit(40)) {
        if (randBit(50)) {
          tb3po.oct_ups |= 0x1;
        } else {
          tb3po.oct_downs |= 0x1;
        }
      }
    }
  }
}

// Apply density to gates, slides, accents
static void applyDensity() {
  randomSeed(tb3po.seed + 1000); // Different seed for rhythm
  
  int onOffDens = getOnOffDensity();
  int densProb = 10 + onOffDens * 6; // 10-94% gate probability
  
  int latestSlide = 0;
  int latestAccent = 0;
  
  tb3po.gates = 0;
  tb3po.slides = 0;
  tb3po.accents = 0;
  
  for (int i = 0; i < TB3PO_MAX_STEPS; i++) {
    // Gates
    tb3po.gates <<= 1;
    if (randBit(densProb)) {
      tb3po.gates |= 1;
    }
    
    // Slides (less likely if previous was slide)
    tb3po.slides <<= 1;
    latestSlide = randBit(latestSlide ? 10 : 18);
    if (latestSlide) {
      tb3po.slides |= 1;
    }
    
    // Accents (less likely if previous was accent)
    tb3po.accents <<= 1;
    latestAccent = randBit(latestAccent ? 7 : 16);
    if (latestAccent) {
      tb3po.accents |= 1;
    }
  }
}

// Full regeneration
static void regenerateAll() {
  reseed();
  regeneratePitches();
  applyDensity();
}

// Get MIDI note for step
static int getMIDINoteForStep(int stepNum) {
  const Scale& scale = scales[tb3po.scaleIndex];
  
  int noteIndex = 60 + tb3po.notes[stepNum] + tb3po.rootNote;
  noteIndex += (tb3po.octaveOffset * scale.numNotes);
  
  // Apply octave shifts
  if (tb3po.oct_ups & (1 << stepNum)) {
    noteIndex += scale.numNotes;
  } else if (tb3po.oct_downs & (1 << stepNum)) {
    noteIndex -= scale.numNotes;
  }
  
  noteIndex = constrain(noteIndex, 0, 127);
  return getNoteInScale(tb3po.scaleIndex, noteIndex - 60, 4);
}

// Check if step is gated
static bool stepIsGated(int stepNum) {
  return (tb3po.gates & (1 << stepNum)) != 0;
}

// Check if step has slide
static bool stepIsSlid(int stepNum) {
  return (tb3po.slides & (1 << stepNum)) != 0;
}

// Check if step has accent
static bool stepIsAccent(int stepNum) {
  return (tb3po.accents & (1 << stepNum)) != 0;
}

void initializeTB3POMode() {
  Serial.println("\n=== TB-3PO Mode Initialization ===");
  
  tb3po.step = 0;
  tb3po.playing = false;
  tb3po.currentNote = -1;
  tb3po.lastStepTime = 0;
  tb3po.readyForInput = false; // Wait for touch release before accepting input
  tb3po.density = 7;
  tb3po.scaleIndex = 0; // Major scale
  tb3po.rootNote = 0;   // C
  tb3po.octaveOffset = 0;
  tb3po.lockSeed = false;
  tb3po.numSteps = 16;
  tb3po.bpm = 120.0;
  tb3po.useInternalClock = true;
  
  Serial.printf("BPM: %.1f, Steps: %d, Density: %d\n", tb3po.bpm, tb3po.numSteps, tb3po.density);
  
  regenerateAll();
  
  Serial.printf("Seed: 0x%04X, Gates: 0x%04X, Slides: 0x%04X, Accents: 0x%04X\n",
                tb3po.seed, tb3po.gates, tb3po.slides, tb3po.accents);
  
  tft.fillScreen(THEME_BG);
  drawModuleHeader("TB-3PO", true);
  drawTB3POMode();
  
  Serial.println("TB-3PO initialized and drawn");
}

void drawTB3POMode() {
  tft.fillRect(0, CONTENT_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_TOP, THEME_BG);
  
  int y = CONTENT_TOP + 10;
  
  // Title and status
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString(tb3po.playing ? "PLAYING" : "STOPPED", 10, y, 2);
  
  // Seed display
  tft.drawString(tb3po.lockSeed ? "SEED LOCKED" : "SEED AUTO", 200, y, 2);
  tft.drawString("0x" + String(tb3po.seed, HEX), 350, y, 2);
  
  y += 30;
  
  // BPM
  tft.drawString("BPM: " + String((int)tb3po.bpm), 10, y, 2);
  
  // Steps
  tft.drawString("STEPS: " + String(tb3po.numSteps), 150, y, 2);
  
  // Density
  int displayDens = abs((int)tb3po.density - 7);
  String densStr = "DENS: ";
  if (tb3po.density < 7) densStr += "-";
  densStr += String(displayDens);
  tft.drawString(densStr, 300, y, 2);
  
  y += 30;
  
  // Scale info
  tft.drawString("SCALE: " + scales[tb3po.scaleIndex].name, 10, y, 2);
  tft.drawString("ROOT: " + getNoteNameFromMIDI(tb3po.rootNote), 250, y, 2);
  if (tb3po.octaveOffset != 0) {
    tft.drawString("OCT: " + String(tb3po.octaveOffset > 0 ? "+" : "") + String(tb3po.octaveOffset), 350, y, 2);
  }
  
  y += 40;
  
  // Step sequence visualization
  int stepWidth = 28;
  int stepHeight = 40;
  int startX = 10;
  
  for (int i = 0; i < tb3po.numSteps; i++) {
    int x = startX + (i * stepWidth);
    
    bool isCurrentStep = (i == tb3po.step && tb3po.playing);
    bool isGated = stepIsGated(i);
    bool isSlid = stepIsSlid(i);
    bool isAccent = stepIsAccent(i);
    
    // Step box
    uint16_t boxColor = isCurrentStep ? THEME_PRIMARY : THEME_SURFACE;
    if (!isGated) boxColor = THEME_TEXT_DIM;
    
    tft.fillRoundRect(x, y, stepWidth - 2, stepHeight, 3, boxColor);
    
    // Accent indicator (top)
    if (isAccent && isGated) {
      tft.fillCircle(x + stepWidth/2 - 1, y + 5, 3, THEME_WARNING);
    }
    
    // Slide indicator (bottom)
    if (isSlid && isGated) {
      tft.fillRect(x + 2, y + stepHeight - 6, stepWidth - 6, 4, THEME_ACCENT);
    }
    
    // Step number
    tft.setTextColor(isCurrentStep ? THEME_BG : THEME_TEXT, boxColor);
    tft.drawString(String(i + 1), x + (stepWidth/2) - 6, y + stepHeight/2 - 4, 1);
  }
  
  y += stepHeight + 20;
  
  // Control buttons - calculate from screen dimensions
  int btnSpacing = 10;
  int btnY = SCREEN_HEIGHT - 70;
  int btnH = 50;
  int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;
  
  drawRoundButton(10, btnY, btnW, btnH, tb3po.playing ? "STOP" : "PLAY", THEME_PRIMARY, false);
  drawRoundButton(110, btnY, btnW, btnH, "REGEN", THEME_SECONDARY, false);
  drawRoundButton(210, btnY, btnW, btnH, "SEED", THEME_ACCENT, false);
  drawRoundButton(310, btnY, btnW, btnH, "SCALE", THEME_SUCCESS, false);
}

// Efficient update of just the step visualization
void updateTB3POSteps() {
  int y = CONTENT_TOP + 135;  // Position matching drawTB3POMode
  int stepWidth = 28;
  int stepHeight = 40;
  int startX = 10;
  
  for (int i = 0; i < tb3po.numSteps; i++) {
    int x = startX + (i * stepWidth);
    
    bool isCurrentStep = (i == tb3po.step && tb3po.playing);
    bool isGated = stepIsGated(i);
    bool isSlid = stepIsSlid(i);
    bool isAccent = stepIsAccent(i);
    
    // Step box
    uint16_t boxColor = isCurrentStep ? THEME_PRIMARY : THEME_SURFACE;
    if (!isGated) boxColor = THEME_TEXT_DIM;
    
    tft.fillRoundRect(x, y, stepWidth - 2, stepHeight, 3, boxColor);
    
    // Accent indicator (top)
    if (isAccent && isGated) {
      tft.fillCircle(x + stepWidth/2 - 1, y + 5, 3, THEME_WARNING);
    }
    
    // Slide indicator (bottom)
    if (isSlid && isGated) {
      tft.fillRect(x + 2, y + stepHeight - 6, stepWidth - 6, 4, THEME_ACCENT);
    }
    
    // Step number
    tft.setTextColor(isCurrentStep ? THEME_BG : THEME_TEXT, boxColor);
    tft.drawString(String(i + 1), x + (stepWidth/2) - 6, y + stepHeight/2 - 4, 1);
  }
}

void handleTB3POMode() {
  updateTouch();
  
  // Wait for initial touch release before accepting button input
  if (!tb3po.readyForInput) {
    if (!touch.isPressed) {
      tb3po.readyForInput = true;
      Serial.println("TB3PO ready for input");
    }
    // Still handle playback timing even during wait
  }
  
  // Calculate button layout matching drawTB3POMode
  int btnSpacing = 10;
  int btnY = SCREEN_HEIGHT - 70;
  int btnH = 50;
  int btnW = (SCREEN_WIDTH - (5 * btnSpacing)) / 4;
  
  int btn1X = btnSpacing;
  int btn2X = btnSpacing * 2 + btnW;
  int btn3X = btnSpacing * 3 + btnW * 2;
  int btn4X = btnSpacing * 4 + btnW * 3;
  
  // Draw control buttons with press feedback
  bool playPressed = touch.isPressed && isButtonPressed(10, btnY, 90, btnH);
  bool regenPressed = touch.isPressed && isButtonPressed(110, btnY, 90, btnH);
  bool seedPressed = touch.isPressed && isButtonPressed(210, btnY, 90, btnH);
  bool scalePressed = touch.isPressed && isButtonPressed(310, btnY, 90, btnH);
  
  // Only redraw buttons when pressed
  if (playPressed || regenPressed || seedPressed || scalePressed) {
    drawRoundButton(10, btnY, 90, btnH, tb3po.playing ? "STOP" : "PLAY", THEME_PRIMARY, playPressed);
    drawRoundButton(110, btnY, 90, btnH, "REGEN", THEME_SECONDARY, regenPressed);
    drawRoundButton(210, btnY, 90, btnH, "SEED", THEME_ACCENT, seedPressed);
    drawRoundButton(310, btnY, 90, btnH, "SCALE", THEME_SUCCESS, scalePressed);
  }
  
  // Handle playback timing
  if (tb3po.playing && tb3po.useInternalClock) {
    unsigned long now = millis();
    
    // Calculate step interval from BPM (16th notes)
    tb3po.stepInterval = (60000.0 / tb3po.bpm) / 4.0;
    
    if (now - tb3po.lastStepTime >= tb3po.stepInterval) {
      tb3po.lastStepTime = now;
      
      Serial.printf("TB3PO Step %d: gate=%d accent=%d slide=%d\n", 
                    tb3po.step, stepIsGated(tb3po.step), 
                    stepIsAccent(tb3po.step), stepIsSlid(tb3po.step));
      
      // Stop previous note if playing
      if (tb3po.currentNote >= 0) {
        sendNoteOff(tb3po.currentNote);
        Serial.printf("  Note OFF: %d\n", tb3po.currentNote);
        tb3po.currentNote = -1;
      }
      
      // Play current step if gated
      if (stepIsGated(tb3po.step)) {
        int note = getMIDINoteForStep(tb3po.step);
        int velocity = stepIsAccent(tb3po.step) ? 127 : 100;
        
        Serial.printf("  Note ON: %d vel=%d\n", note, velocity);
        sendNoteOn(note, velocity);
        tb3po.currentNote = note;
      }
      
      // Advance step
      tb3po.step++;
      if (tb3po.step >= tb3po.numSteps) {
        tb3po.step = 0;
      }
      
      updateTB3POSteps();  // Only redraw step indicators, not entire screen
    }
  }
  
  // Debug touch state changes only
  static bool lastTouchState = false;
  if (touch.isPressed && !lastTouchState) {
    Serial.printf("TB3PO touch DOWN: justPressed=%d ready=%d at (%d,%d)\n", 
                  touch.justPressed, tb3po.readyForInput, touch.x, touch.y);
  }
  lastTouchState = touch.isPressed;
  
  // Only process button presses after initial touch release
  if (touch.justPressed && tb3po.readyForInput) {
    Serial.printf("TB3PO JUST PRESSED at: (%d,%d)\n", touch.x, touch.y);
    
    // Play/Stop button
    if (playPressed) {
      Serial.printf("PLAY/STOP pressed. Was playing: %d\n", tb3po.playing);
      tb3po.playing = !tb3po.playing;
      if (!tb3po.playing && tb3po.currentNote >= 0) {
        sendNoteOff(tb3po.currentNote);
        tb3po.currentNote = -1;
      }
      if (tb3po.playing) {
        tb3po.lastStepTime = millis();
        Serial.printf("Now playing. Step interval: %lu ms\n", tb3po.stepInterval);
      } else {
        Serial.println("Stopped");
      }
      drawTB3POMode();
    }
    // Regenerate button
    else if (regenPressed) {
      Serial.println("REGEN pressed");
      regenerateAll();
      drawTB3POMode();
    }
    // Seed lock button
    else if (seedPressed) {
      Serial.printf("SEED pressed. Locked: %d -> %d\n", tb3po.lockSeed, !tb3po.lockSeed);
      tb3po.lockSeed = !tb3po.lockSeed;
      if (!tb3po.lockSeed) {
        reseed();
        regenerateAll();
      }
      drawTB3POMode();
    }
    // Scale button
    else if (scalePressed) {
      Serial.printf("SCALE pressed. Index: %d -> %d\n", tb3po.scaleIndex, (tb3po.scaleIndex + 1) % NUM_SCALES);
      tb3po.scaleIndex++;
      if (tb3po.scaleIndex >= NUM_SCALES) {
        tb3po.scaleIndex = 0;
      }
      regenerateAll();
      drawTB3POMode();
    }

    // Check back button from header (standard position)
    else if (isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
      Serial.println("BACK pressed (header)");
      if (tb3po.currentNote >= 0) {
        sendNoteOff(tb3po.currentNote);
      }
      tb3po.playing = false;
      currentMode = MENU;
      tft.fillScreen(THEME_BG);
    }
    // Density control (tap on density display area)
    else if (isButtonPressed(300, CONTENT_TOP + 30, 150, 20)) {
      Serial.printf("DENSITY pressed: %d -> %d\n", tb3po.density, (tb3po.density + 1) % 15);
      tb3po.density++;
      if (tb3po.density > 14) tb3po.density = 0;
      applyDensity();
      drawTB3POMode();
    }
    // BPM control
    else if (isButtonPressed(10, CONTENT_TOP + 30, 120, 20)) {
      float newBpm = tb3po.bpm + 10;
      if (newBpm > TB3PO_MAX_BPM) newBpm = TB3PO_MIN_BPM;
      Serial.printf("BPM pressed: %.1f -> %.1f\n", tb3po.bpm, newBpm);
      tb3po.bpm = newBpm;
      drawTB3POMode();
    }
    // Root note control
    else if (isButtonPressed(250, CONTENT_TOP + 60, 80, 20)) {
      Serial.printf("ROOT pressed: %d -> %d\n", tb3po.rootNote, (tb3po.rootNote + 1) % 12);
      tb3po.rootNote++;
      if (tb3po.rootNote > 11) tb3po.rootNote = 0;
      regenerateAll();
      drawTB3POMode();
    } else {
      Serial.println("TB3PO touch - no button hit");
    }
  }
}
