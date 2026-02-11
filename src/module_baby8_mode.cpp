#include "module_baby8_mode.h"

#ifdef ENABLE_BABY8_EMU

#include "clock_manager.h"
#include <Preferences.h>
#include <cstring>

// Global state
static Baby8State baby8;
static Preferences baby8Prefs;
static const char* PREFS_NAMESPACE = "baby8";
static const char* PREFS_KEY_PATTERNS = "patterns";
static const char* PREFS_KEY_BPM = "bpm";
static const char* PREFS_KEY_SWING = "swing";
static const char* PREFS_KEY_CHANNEL = "channel";

// Encoder mode for UI
enum Baby8EncoderMode {
  ENCODER_MODE_STEP_SELECT,  // Select which step to edit
  ENCODER_MODE_NOTE,         // Edit note value
  ENCODER_MODE_VELOCITY,     // Edit velocity
  ENCODER_MODE_GATE,         // Toggle gate
  ENCODER_MODE_LENGTH,       // Pattern length
  ENCODER_MODE_BPM,          // Tempo
  ENCODER_MODE_SWING,        // Swing amount
  ENCODER_MODE_PATTERN       // Pattern select
};

static Baby8EncoderMode encoderModes[BABY8_STEPS] = {
  ENCODER_MODE_STEP_SELECT,  // Encoder 1: Step select
  ENCODER_MODE_NOTE,         // Encoder 2: Note
  ENCODER_MODE_VELOCITY,     // Encoder 3: Velocity
  ENCODER_MODE_GATE,         // Encoder 4: Gate on/off
  ENCODER_MODE_LENGTH,       // Encoder 5: Pattern length
  ENCODER_MODE_BPM,          // Encoder 6: BPM
  ENCODER_MODE_SWING,        // Encoder 7: Swing
  ENCODER_MODE_PATTERN       // Encoder 8: Pattern select
};

static const char* encoderLabels[BABY8_STEPS] = {
  "STEP", "NOTE", "VEL", "GATE", "LEN", "BPM", "SWING", "PTRN"
};

// Initialize Baby8 mode
void initializeBaby8Mode() {
  memset(&baby8, 0, sizeof(baby8));
  
  // Set defaults
  baby8.bpm = sharedBPM;
  baby8.swing = 50;  // 50% swing (no swing)
  baby8.midiChannel = 0;
  baby8.currentPatternIndex = 0;
  baby8.selectedStep = 0;
  baby8.selectedEncoder = 0;
  baby8.playing = false;
  baby8.startPending = false;
  baby8.activeNote = 255;  // No active note
  
  // Initialize patterns with default values
  for (uint8_t p = 0; p < BABY8_MAX_PATTERNS; p++) {
    resetBaby8Pattern(p);
  }
  
  // Load saved patterns
  loadBaby8Patterns();
}

// Reset a pattern to default values
void resetBaby8Pattern(uint8_t patternIndex) {
  if (patternIndex >= BABY8_MAX_PATTERNS) return;
  
  Baby8Pattern& pattern = baby8.patterns[patternIndex];
  pattern.patternLength = BABY8_STEPS;
  snprintf(pattern.name, sizeof(pattern.name), "PAT %d", patternIndex + 1);
  
  // Initialize steps with a simple ascending scale
  for (uint8_t s = 0; s < BABY8_STEPS; s++) {
    pattern.steps[s].note = 60 + s;  // C4 and up
    pattern.steps[s].velocity = 100;
    pattern.steps[s].gate = true;
  }
}

// Load patterns from flash
void loadBaby8Patterns() {
  if (!baby8Prefs.begin(PREFS_NAMESPACE, true)) {  // true = read-only
    return;
  }
  
  // Load patterns
  size_t patternsSize = baby8Prefs.getBytesLength(PREFS_KEY_PATTERNS);
  if (patternsSize == sizeof(baby8.patterns)) {
    baby8Prefs.getBytes(PREFS_KEY_PATTERNS, &baby8.patterns, patternsSize);
  }
  
  // Load BPM
  baby8.bpm = baby8Prefs.getUShort(PREFS_KEY_BPM, sharedBPM);
  
  // Load swing
  baby8.swing = baby8Prefs.getUChar(PREFS_KEY_SWING, 50);
  
  // Load MIDI channel
  baby8.midiChannel = baby8Prefs.getUChar(PREFS_KEY_CHANNEL, 0);
  
  baby8Prefs.end();
}

// Save patterns to flash
void saveBaby8Patterns() {
  if (!baby8Prefs.begin(PREFS_NAMESPACE, false)) {  // false = read-write
    return;
  }
  
  baby8Prefs.putBytes(PREFS_KEY_PATTERNS, &baby8.patterns, sizeof(baby8.patterns));
  baby8Prefs.putUShort(PREFS_KEY_BPM, baby8.bpm);
  baby8Prefs.putUChar(PREFS_KEY_SWING, baby8.swing);
  baby8Prefs.putUChar(PREFS_KEY_CHANNEL, baby8.midiChannel);
  
  baby8Prefs.end();
}

// Draw Baby8 mode UI
void drawBaby8Mode() {
  tft.fillScreen(THEME_BG);
  
  // Header with BPM
  String subtitle = String(baby8.bpm) + " BPM | " + 
                   String(baby8.swing) + "% SW | " +
                   baby8.patterns[baby8.currentPatternIndex].name;
  drawHeader("BABY8", subtitle);
  
  // Draw step sequencer grid
  drawBaby8StepGrid();
  
  // Draw virtual encoders
  drawBaby8Encoders();
  
  // Transport controls
  int ctrlY = SCALE_Y(195);
  const char* playLabel = baby8.playing ? "STOP" : (baby8.startPending ? "PENDING" : "PLAY");
  uint16_t playColor = baby8.playing ? THEME_ERROR : (baby8.startPending ? THEME_SECONDARY : THEME_SUCCESS);
  
  drawRoundButton(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, playLabel, playColor);
  drawRoundButton(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "SAVE", THEME_WARNING);
  drawRoundButton(SCALE_X(130), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "LOAD", THEME_PRIMARY);
  drawRoundButton(SCALE_X(190), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H, "RESET", THEME_ERROR);
}

// Draw step sequencer grid
void drawBaby8StepGrid() {
  int gridX = MARGIN_SMALL;
  int gridY = HEADER_HEIGHT + SCALE_Y(5);
  int cellW = SCALE_X(35);
  int cellH = SCALE_Y(25);
  int spacing = SCALE_X(2);
  
  Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
  
  // Draw each step
  for (uint8_t step = 0; step < BABY8_STEPS; step++) {
    int x = gridX + step * (cellW + spacing);
    int y = gridY;
    
    Baby8Step& stepData = pattern.steps[step];
    bool isActive = (step < pattern.patternLength);
    bool isCurrent = (baby8.playing && step == baby8.currentStep);
    bool isSelected = (step == baby8.selectedStep);
    
    // Determine color
    uint16_t color;
    if (isCurrent && stepData.gate) {
      color = THEME_TEXT;  // White when playing
    } else if (isCurrent) {
      color = THEME_PRIMARY;  // Cyan when on current step
    } else if (isSelected) {
      color = THEME_SECONDARY;  // Orange when selected
    } else if (stepData.gate && isActive) {
      color = THEME_SUCCESS;  // Green when gate is on
    } else if (isActive) {
      color = THEME_SURFACE;  // Gray when active but gate off
    } else {
      color = THEME_BG;  // Black when step is disabled
    }
    
    // Draw step cell
    tft.fillRoundRect(x, y, cellW, cellH, 3, color);
    tft.drawRoundRect(x, y, cellW, cellH, 3, THEME_TEXT_DIM);
    
    // Draw step number
    tft.setTextColor(isActive && stepData.gate ? THEME_BG : THEME_TEXT, color);
    tft.drawCentreString(String(step + 1), x + cellW / 2, y + SCALE_Y(2), 1);
    
    // Draw note name below step number if gate is on
    if (stepData.gate && isActive) {
      String noteName = getNoteNameFromMIDI(stepData.note);
      tft.drawCentreString(noteName, x + cellW / 2, y + SCALE_Y(13), 1);
    }
  }
}

// Draw virtual encoders
void drawBaby8Encoders() {
  int encY = HEADER_HEIGHT + SCALE_Y(35);
  int encX = MARGIN_SMALL;
  int encW = SCALE_X(35);
  int encH = SCALE_Y(60);
  int spacing = SCALE_X(2);
  
  Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
  Baby8Step& currentStepData = pattern.steps[baby8.selectedStep];
  
  for (uint8_t enc = 0; enc < BABY8_STEPS; enc++) {
    int x = encX + enc * (encW + spacing);
    int y = encY;
    
    bool isSelected = (enc == baby8.selectedEncoder);
    uint16_t bgColor = isSelected ? THEME_PRIMARY : THEME_SURFACE;
    uint16_t textColor = THEME_TEXT;
    
    // Draw encoder background
    tft.fillRoundRect(x, y, encW, encH, 3, bgColor);
    tft.drawRoundRect(x, y, encW, encH, 3, THEME_TEXT_DIM);
    
    // Draw label
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(encoderLabels[enc], x + encW / 2, y + SCALE_Y(2), 1);
    
    // Draw value
    String valueStr;
    uint8_t value = getBaby8EncoderValue(enc);
    
    switch (encoderModes[enc]) {
      case ENCODER_MODE_STEP_SELECT:
        valueStr = String(baby8.selectedStep + 1);
        break;
      case ENCODER_MODE_NOTE:
        valueStr = getNoteNameFromMIDI(currentStepData.note);
        break;
      case ENCODER_MODE_VELOCITY:
        valueStr = String(currentStepData.velocity);
        break;
      case ENCODER_MODE_GATE:
        valueStr = currentStepData.gate ? "ON" : "OFF";
        break;
      case ENCODER_MODE_LENGTH:
        valueStr = String(pattern.patternLength);
        break;
      case ENCODER_MODE_BPM:
        valueStr = String(baby8.bpm);
        break;
      case ENCODER_MODE_SWING:
        valueStr = String(baby8.swing);
        break;
      case ENCODER_MODE_PATTERN:
        valueStr = String(baby8.currentPatternIndex + 1);
        break;
    }
    
    tft.drawCentreString(valueStr, x + encW / 2, y + SCALE_Y(15), 2);
    
    // Draw value bar
    int barX = x + SCALE_X(3);
    int barY = y + encH - SCALE_Y(15);
    int barW = encW - SCALE_X(6);
    int barH = SCALE_Y(10);
    
    tft.drawRect(barX, barY, barW, barH, THEME_TEXT_DIM);
    
    // Calculate bar fill
    float fillPercent = 0.0f;
    switch (encoderModes[enc]) {
      case ENCODER_MODE_STEP_SELECT:
        fillPercent = (float)baby8.selectedStep / (BABY8_STEPS - 1);
        break;
      case ENCODER_MODE_NOTE:
        fillPercent = (float)currentStepData.note / 127.0f;
        break;
      case ENCODER_MODE_VELOCITY:
        fillPercent = (float)currentStepData.velocity / 127.0f;
        break;
      case ENCODER_MODE_GATE:
        fillPercent = currentStepData.gate ? 1.0f : 0.0f;
        break;
      case ENCODER_MODE_LENGTH:
        fillPercent = (float)(pattern.patternLength - 1) / (BABY8_STEPS - 1);
        break;
      case ENCODER_MODE_BPM:
        fillPercent = (float)(baby8.bpm - 40) / (240 - 40);
        break;
      case ENCODER_MODE_SWING:
        fillPercent = (float)baby8.swing / 100.0f;
        break;
      case ENCODER_MODE_PATTERN:
        fillPercent = (float)baby8.currentPatternIndex / (BABY8_MAX_PATTERNS - 1);
        break;
    }
    
    int fillW = (int)(barW * fillPercent);
    if (fillW > 0) {
      tft.fillRect(barX, barY, fillW, barH, THEME_SUCCESS);
    }
  }
}

// Get encoder value
uint8_t getBaby8EncoderValue(uint8_t encoderIndex) {
  if (encoderIndex >= BABY8_STEPS) return 0;
  
  Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
  Baby8Step& step = pattern.steps[baby8.selectedStep];
  
  switch (encoderModes[encoderIndex]) {
    case ENCODER_MODE_STEP_SELECT: return baby8.selectedStep;
    case ENCODER_MODE_NOTE: return step.note;
    case ENCODER_MODE_VELOCITY: return step.velocity;
    case ENCODER_MODE_GATE: return step.gate ? 1 : 0;
    case ENCODER_MODE_LENGTH: return pattern.patternLength;
    case ENCODER_MODE_BPM: return (uint8_t)((baby8.bpm - 40) * 127 / (240 - 40));
    case ENCODER_MODE_SWING: return (uint8_t)(baby8.swing * 127 / 100);
    case ENCODER_MODE_PATTERN: return baby8.currentPatternIndex;
    default: return 0;
  }
}

// Set encoder value
void setBaby8EncoderValue(uint8_t encoderIndex, uint8_t value) {
  if (encoderIndex >= BABY8_STEPS) return;
  
  Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
  Baby8Step& step = pattern.steps[baby8.selectedStep];
  
  switch (encoderModes[encoderIndex]) {
    case ENCODER_MODE_STEP_SELECT:
      baby8.selectedStep = value % BABY8_STEPS;
      break;
    case ENCODER_MODE_NOTE:
      step.note = value % 128;
      break;
    case ENCODER_MODE_VELOCITY:
      step.velocity = value % 128;
      break;
    case ENCODER_MODE_GATE:
      step.gate = (value > 0);
      break;
    case ENCODER_MODE_LENGTH:
      pattern.patternLength = ((value % BABY8_STEPS) + 1);
      if (pattern.patternLength < 1) pattern.patternLength = 1;
      if (pattern.patternLength > BABY8_STEPS) pattern.patternLength = BABY8_STEPS;
      break;
    case ENCODER_MODE_BPM:
      baby8.bpm = 40 + (value * (240 - 40) / 127);
      setSharedBPM(baby8.bpm);
      break;
    case ENCODER_MODE_SWING:
      baby8.swing = (value * 100) / 127;
      break;
    case ENCODER_MODE_PATTERN:
      baby8.currentPatternIndex = value % BABY8_MAX_PATTERNS;
      break;
  }
}

// Handle touch input
void handleBaby8Mode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    if (baby8.playing) {
      baby8.playing = false;
      baby8.startPending = false;
      // Stop active note
      if (baby8.activeNote < 128) {
        sendMIDI(0x80 | baby8.midiChannel, baby8.activeNote, 0);
        baby8.activeNote = 255;
      }
    }
    exitToMenu();
    return;
  }
  
  int ctrlY = SCALE_Y(195);
  
  if (touch.justPressed) {
    // Play/Stop button
    if (isButtonPressed(MARGIN_SMALL, ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      if (baby8.playing) {
        baby8.playing = false;
        baby8.startPending = false;
        // Stop active note
        if (baby8.activeNote < 128) {
          sendMIDI(0x80 | baby8.midiChannel, baby8.activeNote, 0);
          baby8.activeNote = 255;
        }
      } else {
        baby8.currentStep = 0;
        baby8.startPending = true;
        baby8.lastStepTime = millis();
      }
      requestRedraw();
      return;
    }
    
    // Save button
    if (isButtonPressed(SCALE_X(70), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      saveBaby8Patterns();
      requestRedraw();
      return;
    }
    
    // Load button
    if (isButtonPressed(SCALE_X(130), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      loadBaby8Patterns();
      requestRedraw();
      return;
    }
    
    // Reset button
    if (isButtonPressed(SCALE_X(190), ctrlY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      resetBaby8Pattern(baby8.currentPatternIndex);
      requestRedraw();
      return;
    }
    
    // Step grid touch
    int gridX = MARGIN_SMALL;
    int gridY = HEADER_HEIGHT + SCALE_Y(5);
    int cellW = SCALE_X(35);
    int cellH = SCALE_Y(25);
    int spacing = SCALE_X(2);
    
    for (uint8_t step = 0; step < BABY8_STEPS; step++) {
      int x = gridX + step * (cellW + spacing);
      int y = gridY;
      
      if (isButtonPressed(x, y, cellW, cellH)) {
        baby8.selectedStep = step;
        requestRedraw();
        return;
      }
    }
    
    // Encoder touch
    handleBaby8EncoderTouch();
  }
  
  // Update sequencer
  updateBaby8Sequencer();
}

// Handle encoder touch
void handleBaby8EncoderTouch() {
  int encY = HEADER_HEIGHT + SCALE_Y(35);
  int encX = MARGIN_SMALL;
  int encW = SCALE_X(35);
  int encH = SCALE_Y(60);
  int spacing = SCALE_X(2);
  
  for (uint8_t enc = 0; enc < BABY8_STEPS; enc++) {
    int x = encX + enc * (encW + spacing);
    int y = encY;
    
    if (isButtonPressed(x, y, encW, encH)) {
      baby8.selectedEncoder = enc;
      
      // Increment value on touch
      uint8_t currentValue = getBaby8EncoderValue(enc);
      
      // Different increment amounts based on encoder type
      uint8_t increment = 1;
      switch (encoderModes[enc]) {
        case ENCODER_MODE_NOTE:
          increment = 1;  // One semitone
          break;
        case ENCODER_MODE_VELOCITY:
          increment = 10;  // Larger steps for velocity
          break;
        case ENCODER_MODE_BPM:
          increment = 5;  // 5 BPM increments (mapped to 127 scale)
          break;
        case ENCODER_MODE_SWING:
          increment = 10;  // 10% increments (mapped to 127 scale)
          break;
        case ENCODER_MODE_GATE:
          increment = 1;  // Toggle
          break;
        default:
          increment = 1;
          break;
      }
      
      uint8_t newValue = currentValue + increment;
      
      // Handle wrap-around or toggle for certain modes
      if (encoderModes[enc] == ENCODER_MODE_GATE) {
        newValue = currentValue ? 0 : 1;  // Toggle
      } else if (encoderModes[enc] == ENCODER_MODE_STEP_SELECT) {
        newValue = (currentValue + 1) % BABY8_STEPS;
      } else if (encoderModes[enc] == ENCODER_MODE_PATTERN) {
        newValue = (currentValue + 1) % BABY8_MAX_PATTERNS;
      } else if (encoderModes[enc] == ENCODER_MODE_LENGTH) {
        newValue = (currentValue % BABY8_STEPS) + 1;
      }
      
      setBaby8EncoderValue(enc, newValue);
      requestRedraw();
      return;
    }
  }
}

// Update sequencer timing
void updateBaby8Sequencer() {
  unsigned long now = millis();
  
  // Handle note off
  if (baby8.noteOffTime > 0 && now >= baby8.noteOffTime) {
    if (baby8.activeNote < 128) {
      sendMIDI(0x80 | baby8.midiChannel, baby8.activeNote, 0);
      baby8.activeNote = 255;
    }
    baby8.noteOffTime = 0;
  }
  
  // Check if we should start playing
  if (baby8.startPending) {
    baby8.playing = true;
    baby8.startPending = false;
    baby8.currentStep = 0;
    baby8.lastStepTime = now;
    playBaby8Step();
    return;
  }
  
  // Update sequencer if playing
  if (baby8.playing) {
    // Calculate step duration in milliseconds
    unsigned long stepDuration = (60000 / baby8.bpm) / 2;  // 16th notes at given BPM
    
    // Apply swing to odd steps
    if (baby8.currentStep % 2 == 1 && baby8.swing != 50) {
      // Swing affects timing of odd steps
      float swingFactor = (float)baby8.swing / 50.0f;  // 0.0 to 2.0
      stepDuration = (unsigned long)(stepDuration * swingFactor);
    }
    
    if (now - baby8.lastStepTime >= stepDuration) {
      baby8.lastStepTime = now;
      
      // Advance to next step
      baby8.currentStep++;
      Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
      if (baby8.currentStep >= pattern.patternLength) {
        baby8.currentStep = 0;
      }
      
      playBaby8Step();
      requestRedraw();
    }
  }
}

// Play current step
void playBaby8Step() {
  Baby8Pattern& pattern = baby8.patterns[baby8.currentPatternIndex];
  
  if (baby8.currentStep >= pattern.patternLength) {
    return;
  }
  
  Baby8Step& step = pattern.steps[baby8.currentStep];
  
  // Stop previous note if still playing
  if (baby8.activeNote < 128) {
    sendMIDI(0x80 | baby8.midiChannel, baby8.activeNote, 0);
    baby8.activeNote = 255;
  }
  
  // Play note if gate is on
  if (step.gate) {
    sendMIDI(0x90 | baby8.midiChannel, step.note, step.velocity);
    baby8.activeNote = step.note;
    
    // Schedule note off (50% gate time)
    unsigned long gateDuration = (60000 / baby8.bpm) / 4;  // Half of 16th note
    baby8.noteOffTime = millis() + gateDuration;
  }
}

#endif // ENABLE_BABY8_EMU
