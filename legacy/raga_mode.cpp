/*******************************************************************
 RAGA Mode Implementation
 Indian Classical Music scales with microtonal support
 *******************************************************************/

#include "raga_mode.h"

RagaState raga;

// Raga scale definitions
// notes[]: MIDI intervals from root (0=root, 1=minor 2nd, 2=major 2nd, etc.)
// microtonalCents[]: Fine tuning in cents (-50 to +50, 0 = no adjustment)
const RagaScale ragaScales[RAGA_COUNT] = {
  // Bhairavi - Morning raga, very devotional
  {"Bhairavi", {0, 1, 3, 5, 7, 8, 10, 12, 255, 255, 255, 255}, 8, 
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0xF800}, // Red
  
  // Lalit - Morning raga, complex and serious
  {"Lalit", {0, 1, 4, 6, 7, 9, 11, 12, 255, 255, 255, 255}, 8,
   {0, 0, 0, -20, 0, 0, 0, 0, 0, 0, 0, 0}, 0xFD00}, // Orange
  
  // Bhupali - Evening raga, pentatonic, peaceful
  {"Bhupali", {0, 2, 4, 7, 9, 12, 255, 255, 255, 255, 255, 255}, 6,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0xFFE0}, // Yellow
  
  // Todi - Morning raga, intense and passionate
  {"Todi", {0, 1, 3, 6, 7, 8, 11, 12, 255, 255, 255, 255}, 8,
   {0, -30, 0, -20, 0, -20, 0, 0, 0, 0, 0, 0}, 0x07E0}, // Green
  
  // Madhuvanti - Evening raga, romantic
  {"Madhuvanti", {0, 2, 3, 6, 7, 9, 11, 12, 255, 255, 255, 255}, 8,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0x07FF}, // Cyan
  
  // Meghmalhar - Monsoon raga, evokes rain
  {"Meghmalhar", {0, 2, 3, 5, 7, 9, 10, 12, 255, 255, 255, 255}, 8,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0x001F}, // Blue
  
  // Yaman - Evening raga, very popular and soothing
  {"Yaman", {0, 2, 4, 6, 7, 9, 11, 12, 255, 255, 255, 255}, 8,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0x781F}, // Purple
  
  // Kalavati - Night raga, tender and sweet
  {"Kalavati", {0, 2, 3, 5, 7, 9, 10, 12, 255, 255, 255, 255}, 8,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0xF81F}, // Magenta
  
  // Malkauns - Late night raga, pentatonic, meditative
  {"Malkauns", {0, 3, 5, 8, 10, 12, 255, 255, 255, 255, 255, 255}, 6,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0x8410}, // Dark gray
  
  // Bairagi - Morning raga, devotional
  {"Bairagi", {0, 1, 5, 7, 8, 12, 255, 255, 255, 255, 255, 255}, 6,
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0xFBE0}  // Light Orange
};

void initializeRagaMode() {
  Serial.println("\n=== Raga Mode Initialization ===");
  
  raga.currentRaga = BHAIRAVI;
  raga.rootNote = 60; // Middle C
  raga.playing = false;
  raga.droneEnabled = false;
  raga.currentStep = 0;
  raga.lastNoteTime = 0;
  raga.currentNote = -1;
  raga.octaveRange = 2;
  
  Serial.printf("Raga: %s\n", ragaScales[raga.currentRaga].name);
  Serial.println("Raga mode initialized");
  
  drawRagaMode();
}

void drawRagaMode() {
  tft.fillScreen(THEME_BG);
  
  // Use unified header
  drawModuleHeader("RAGA", true);
  
  // Current raga name and status below header
  const RagaScale& current = ragaScales[raga.currentRaga];
  tft.setTextColor(current.color, THEME_BG);
  tft.drawString(current.name, 120, CONTENT_TOP + 5, 2);
  
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String status = raga.playing ? "PLAYING" : "STOPPED";
  tft.drawRightString(status, SCREEN_WIDTH - 10, CONTENT_TOP + 5, 2);
  
  int y = CONTENT_TOP + 30;
  
  // Calculate raga selection button layout
  raga.ragaBtnW = 90;
  raga.ragaBtnH = 50;
  raga.ragaBtnSpacing = 6;
  raga.ragaBtnRowSpacing = 10;
  raga.ragaBtnStartX = (SCREEN_WIDTH - (5 * raga.ragaBtnW + 4 * raga.ragaBtnSpacing)) / 2;
  raga.ragaBtnStartY = y;
  
  // Draw 10 raga selection buttons in 2 rows of 5
  for (int i = 0; i < RAGA_COUNT; i++) {
    int row = i / 5;
    int col = i % 5;
    int x = raga.ragaBtnStartX + col * (raga.ragaBtnW + raga.ragaBtnSpacing);
    int btnY = raga.ragaBtnStartY + row * (raga.ragaBtnH + raga.ragaBtnRowSpacing);
    
    uint16_t btnColor = (i == raga.currentRaga) ? ragaScales[i].color : THEME_PRIMARY;
    bool isPressedNow = touch.isPressed && isButtonPressed(x, btnY, raga.ragaBtnW, raga.ragaBtnH);
    
    drawRoundButton(x, btnY, raga.ragaBtnW, raga.ragaBtnH, ragaScales[i].name, btnColor, isPressedNow);
  }
  
  y += (2 * raga.ragaBtnH) + raga.ragaBtnRowSpacing + 20;
  
  // Scale visualization - show notes in current raga
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Scale:", 20, y, 2);
  
  String scaleNotes = "";
  const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  for (int i = 0; i < current.numNotes; i++) {
    if (current.notes[i] != 255) {
      scaleNotes += noteNames[current.notes[i] % 12];
      if (current.microtonalCents[i] < 0) scaleNotes += "↓";
      else if (current.microtonalCents[i] > 0) scaleNotes += "↑";
      if (i < current.numNotes - 1) scaleNotes += " ";
    }
  }
  tft.setTextColor(current.color, THEME_BG);
  tft.drawString(scaleNotes, 80, y, 2);
  
  y += 30;
  
  // Control section
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Root:", 20, y, 2);
  tft.drawString(noteNames[raga.rootNote % 12], 80, y, 2);
  
  tft.drawString("Drone:", 180, y, 2);
  tft.setTextColor(raga.droneEnabled ? THEME_SUCCESS : THEME_TEXT_DIM, THEME_BG);
  tft.drawString(raga.droneEnabled ? "ON" : "OFF", 260, y, 2);
  
  y += 30;
  
  // Tempo slider - store layout for touch handling
  raga.sliderX = 100;
  raga.sliderY = y;
  raga.sliderW = SCREEN_WIDTH - 120;
  raga.sliderH = 20;
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Tempo:", 20, y, 2);
  tft.drawRect(raga.sliderX, raga.sliderY, raga.sliderW, raga.sliderH, THEME_TEXT);
  int tempoFill = (raga.tempo * raga.sliderW) / 255;
  if (tempoFill > 0) {
    tft.fillRect(raga.sliderX + 1, raga.sliderY + 1, tempoFill, raga.sliderH - 2, current.color);
  }
  
  // Display tempo as BPM (map 0-255 to 40-200 BPM)
  int bpm = 40 + ((raga.tempo * 160) / 255);
  String tempoText = String(bpm) + " BPM";
  tft.setTextColor(current.color, THEME_BG);
  tft.drawRightString(tempoText, SCREEN_WIDTH - 20, y, 2);
  
  // Bottom control buttons - store layout for touch handling
  raga.ctrlY = SCREEN_HEIGHT - 60;
  raga.ctrlH = 50;
  raga.ctrlW = (SCREEN_WIDTH - 50) / 4;  // 4 buttons with spacing
  
  int btnSpacing = 10;
  int btn1X = 10;
  int btn2X = btn1X + raga.ctrlW + btnSpacing;
  int btn3X = btn2X + raga.ctrlW + btnSpacing;
  int btn4X = btn3X + raga.ctrlW + btnSpacing;
  
  // Check if buttons are currently being pressed for visual feedback
  bool btn1Pressed = touch.isPressed && isButtonPressed(btn1X, raga.ctrlY, raga.ctrlW, raga.ctrlH);
  bool btn2Pressed = touch.isPressed && isButtonPressed(btn2X, raga.ctrlY, raga.ctrlW, raga.ctrlH);
  bool btn3Pressed = touch.isPressed && isButtonPressed(btn3X, raga.ctrlY, raga.ctrlW, raga.ctrlH);
  bool btn4Pressed = touch.isPressed && isButtonPressed(btn4X, raga.ctrlY, raga.ctrlW, raga.ctrlH);
  
  drawRoundButton(btn1X, raga.ctrlY, raga.ctrlW, raga.ctrlH, raga.playing ? "STOP" : "PLAY", THEME_PRIMARY, btn1Pressed);
  drawRoundButton(btn2X, raga.ctrlY, raga.ctrlW, raga.ctrlH, "DRONE", raga.droneEnabled ? THEME_SUCCESS : THEME_SECONDARY, btn2Pressed);
  drawRoundButton(btn3X, raga.ctrlY, raga.ctrlW, raga.ctrlH, "ROOT-", THEME_ACCENT, btn3Pressed);
  drawRoundButton(btn4X, raga.ctrlY, raga.ctrlW, raga.ctrlH, "ROOT+", THEME_ACCENT, btn4Pressed);
}

void playRagaNote(uint8_t scaleIndex, bool slide) {
  const RagaScale& current = ragaScales[raga.currentRaga];
  
  if (scaleIndex >= current.numNotes || current.notes[scaleIndex] == 255) return;
  
  // Calculate MIDI note
  uint8_t note = raga.rootNote + current.notes[scaleIndex];
  
  // Apply microtonal adjustment using pitch bend
  int16_t cents = current.microtonalCents[scaleIndex];
  if (cents != 0) {
    // MIDI pitch bend: 8192 = center, ±8191 range
    // ±2 semitones = ±200 cents typical
    int16_t bendValue = 8192 + (cents * 8192 / 200);
    bendValue = constrain(bendValue, 0, 16383);
    
    // Send pitch bend (14-bit value)
    sendPitchBend(bendValue);
  }
  
  // If sliding, send gradual pitch bend from previous note
  if (slide && raga.currentNote >= 0) {
    // Quick slide effect
    for (int i = 0; i < 5; i++) {
      int16_t slideValue = 8192 + ((i - 2) * 400);
      slideValue = constrain(slideValue, 0, 16383);
      sendPitchBend(slideValue);
      delay(10);
    }
  }
  
  // Stop previous note
  if (raga.currentNote >= 0) {
    sendNoteOff(raga.currentNote);
  }
  
  // Play new note
  sendNoteOn(note, 100);
  raga.currentNote = note;
}

void startDrone() {
  // Play drone notes (root and fifth)
  sendNoteOn(raga.rootNote, 60);
  sendNoteOn(raga.rootNote + 7, 50); // Fifth
  sendNoteOn(raga.rootNote + 12, 40); // Octave
}

void stopDrone() {
  sendNoteOff(raga.rootNote);
  sendNoteOff(raga.rootNote + 7);
  sendNoteOff(raga.rootNote + 12);
}

void handleRagaMode() {
  updateTouch();
  
  // Handle automatic phrase playback
  if (raga.playing) {
    unsigned long now = millis();
    // Map tempo (0-255) to BPM (40-200), then calculate note delay
    int bpm = 40 + ((raga.tempo * 160) / 255);
    // At 120 BPM, notes play every 500ms (8th notes)
    unsigned long noteDelay = (60000 / bpm) / 2; // Half beat = 8th note
    
    if (now - raga.lastNoteTime >= noteDelay) {
      raga.lastNoteTime = now;
      
      const RagaScale& current = ragaScales[raga.currentRaga];
      
      // Simple ascending/descending pattern with occasional slides
      bool slide = (random(100) < 30); // 30% chance of slide
      playRagaNote(raga.currentStep, slide);
      
      // Move to next note in scale
      if (random(100) < 70) {
        raga.currentStep = (raga.currentStep + 1) % current.numNotes;
      } else {
        // Sometimes go backwards
        raga.currentStep = (raga.currentStep > 0) ? raga.currentStep - 1 : current.numNotes - 1;
      }
    }
  }
  
  if (touch.justPressed) {
    Serial.printf("[RAGA] Touch at (%d, %d)\\n", touch.x, touch.y);
    
    // Check back button from header first
    if (isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
      Serial.println("[RAGA] Back button pressed");
      if (raga.playing) {
        raga.playing = false;
        if (raga.currentNote >= 0) {
          sendNoteOff(raga.currentNote);
          raga.currentNote = -1;
        }
      }
      if (raga.droneEnabled) {
        stopDrone();
        raga.droneEnabled = false;
      }
      sendPitchBend(8192);
      exitToMenu();
      return;
    }
    
    // Check raga selection buttons using stored layout
    for (int i = 0; i < RAGA_COUNT; i++) {
      int row = i / 5;
      int col = i % 5;
      int x = raga.ragaBtnStartX + col * (raga.ragaBtnW + raga.ragaBtnSpacing);
      int btnY = raga.ragaBtnStartY + row * (raga.ragaBtnH + raga.ragaBtnRowSpacing);
      
      if (isButtonPressed(x, btnY, raga.ragaBtnW, raga.ragaBtnH)) {
        Serial.printf("[RAGA] Selected raga %d: %s\n", i, ragaScales[i].name);
        raga.currentRaga = (RagaType)i;
        raga.currentStep = 0;
        if (raga.currentNote >= 0) {
          sendNoteOff(raga.currentNote);
          raga.currentNote = -1;
        }
        sendPitchBend(8192);
        drawRagaMode();
        return;
      }
    }
    
    // Check tempo slider using stored layout
    if (isButtonPressed(raga.sliderX, raga.sliderY, raga.sliderW, raga.sliderH)) {
      raga.tempo = ((touch.x - raga.sliderX) * 255) / raga.sliderW;
      raga.tempo = constrain(raga.tempo, 0, 255);
      Serial.printf("[RAGA] Tempo: %d\n", raga.tempo);
      drawRagaMode();
      return;
    }
    
    // Control buttons using stored layout
    int btnSpacing = 10;
    int btn1X = 10;
    int btn2X = btn1X + raga.ctrlW + btnSpacing;
    int btn3X = btn2X + raga.ctrlW + btnSpacing;
    int btn4X = btn3X + raga.ctrlW + btnSpacing;
    
    // PLAY/STOP
    if (isButtonPressed(btn1X, raga.ctrlY, raga.ctrlW, raga.ctrlH)) {
      Serial.println("[RAGA] Play/Stop pressed");
      raga.playing = !raga.playing;
      if (raga.playing) {
        raga.currentStep = 0;
        raga.lastNoteTime = millis();
      } else {
        if (raga.currentNote >= 0) {
          sendNoteOff(raga.currentNote);
          raga.currentNote = -1;
        }
        sendPitchBend(8192);
      }
      drawRagaMode();
      return;
    }
    
    // DRONE
    if (isButtonPressed(btn2X, raga.ctrlY, raga.ctrlW, raga.ctrlH)) {
      Serial.println("[RAGA] Drone pressed");
      raga.droneEnabled = !raga.droneEnabled;
      if (raga.droneEnabled) {
        startDrone();
      } else {
        stopDrone();
      }
      drawRagaMode();
      return;
    }
    
    // ROOT-
    if (isButtonPressed(btn3X, raga.ctrlY, raga.ctrlW, raga.ctrlH)) {
      Serial.println("[RAGA] Root- pressed");
      if (raga.droneEnabled) stopDrone();
      raga.rootNote = constrain(raga.rootNote - 1, 36, 84);
      if (raga.droneEnabled) startDrone();
      drawRagaMode();
      return;
    }
    
    // ROOT+
    if (isButtonPressed(btn4X, raga.ctrlY, raga.ctrlW, raga.ctrlH)) {
      Serial.println("[RAGA] Root+ pressed");
      if (raga.droneEnabled) stopDrone();
      raga.rootNote = constrain(raga.rootNote + 1, 36, 84);
      if (raga.droneEnabled) startDrone();
      drawRagaMode();
      return;
    }
    
    Serial.println("[RAGA] Touch - no button hit");
  }
}
