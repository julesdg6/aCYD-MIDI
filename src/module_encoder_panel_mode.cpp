#include "module_encoder_panel_mode.h"

#ifdef ENABLE_M5_8ENCODER

#include <Preferences.h>

// Encoder panel state
int currentEncoderPage = 0;
bool encoderFineMode = true;

// Define encoder pages (3 pages of 8 encoders each)
EncoderPage encoderPages[3] = {
  {
    "MIDI CC 1-8",
    {
      {PARAM_MIDI_CC, "CC 1", 1, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 2", 2, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 3", 3, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 4", 4, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 5", 5, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 6", 6, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 7", 7, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 8", 8, 0, 0, 127, 64, 1}
    }
  },
  {
    "MIDI CC 11-18",
    {
      {PARAM_MIDI_CC, "CC 11", 11, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 12", 12, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 13", 13, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 14", 14, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 15", 15, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 16", 16, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 17", 17, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "CC 18", 18, 0, 0, 127, 64, 1}
    }
  },
  {
    "Custom",
    {
      {PARAM_MIDI_CC, "Volume", 7, 0, 0, 127, 100, 1},
      {PARAM_MIDI_CC, "Pan", 10, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "Cutoff", 74, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "Resonance", 71, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "Attack", 73, 0, 0, 127, 0, 1},
      {PARAM_MIDI_CC, "Release", 72, 0, 0, 127, 64, 1},
      {PARAM_MIDI_CC, "Reverb", 91, 0, 0, 127, 40, 1},
      {PARAM_MIDI_CC, "Delay", 92, 0, 0, 127, 0, 1}
    }
  }
};

const int NUM_ENCODER_PAGES = 3;

void initializeEncoderPanelMode() {
  // Try to initialize the encoder hardware
  if (!encoder8.isConnected()) {
    encoder8.begin();
  }
  
  // Load saved mappings from preferences
  loadEncoderMappings();
  
  // Reset encoder page
  currentEncoderPage = 0;
  encoderFineMode = true;
  
  // Reset all encoder counters
  encoder8.resetAllEncoders();
}

void drawEncoderPanelMode() {
  tft.fillScreen(THEME_BG);
  
  // Header with page name
  String subtitle = encoderPages[currentEncoderPage].name;
  if (encoderFineMode) {
    subtitle += " [FINE]";
  } else {
    subtitle += " [COARSE]";
  }
  drawHeader("ENCODER", subtitle);
  
  // Draw encoder controls in a 4x2 grid
  int cols = 4;
  int rows = 2;
  int spacing = SCALE_X(5);
  int topMargin = HEADER_HEIGHT + SCALE_Y(10);
  int bottomMargin = SCALE_Y(50);
  
  int availableWidth = DISPLAY_WIDTH - (spacing * (cols + 1));
  int availableHeight = DISPLAY_HEIGHT - topMargin - bottomMargin;
  
  int encWidth = availableWidth / cols;
  int encHeight = availableHeight / rows;
  
  for (int i = 0; i < 8; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = spacing + col * (encWidth + spacing);
    int y = topMargin + row * (encHeight + spacing);
    
    drawEncoderControl(i, x, y, encWidth, encHeight);
  }
  
  // Bottom controls
  int btnY = DISPLAY_HEIGHT - SCALE_Y(35);
  drawRoundButton(SCALE_X(10), btnY, BTN_MEDIUM_W, BTN_SMALL_H, "PAGE", THEME_SECONDARY);
  drawRoundButton(SCALE_X(70), btnY, BTN_MEDIUM_W, BTN_SMALL_H, encoderFineMode ? "FINE" : "COARSE", THEME_ACCENT);
  drawRoundButton(SCALE_X(130), btnY, BTN_MEDIUM_W, BTN_SMALL_H, "SAVE", THEME_SUCCESS);
  drawRoundButton(SCALE_X(190), btnY, BTN_MEDIUM_W, BTN_SMALL_H, "RESET", THEME_WARNING);
  
  // Show connection status
  if (!encoder8.isConnected()) {
    tft.setTextColor(THEME_ERROR, THEME_BG);
    tft.drawCentreString("8ENCODER NOT CONNECTED", DISPLAY_CENTER_X, btnY - SCALE_Y(15), 1);
  }
}

void drawEncoderControl(int encoderIndex, int x, int y, int w, int h) {
  EncoderMapping &enc = encoderPages[currentEncoderPage].encoders[encoderIndex];
  
  // Background
  uint16_t bgColor = THEME_SURFACE;
  uint16_t borderColor = THEME_PRIMARY;
  tft.fillRoundRect(x, y, w, h, 5, bgColor);
  tft.drawRoundRect(x, y, w, h, 5, borderColor);
  
  // Encoder number
  tft.setTextColor(THEME_TEXT_DIM, bgColor);
  tft.drawString(String(encoderIndex + 1), x + SCALE_X(3), y + SCALE_Y(2), 1);
  
  // Label
  tft.setTextColor(THEME_TEXT, bgColor);
  int labelY = y + h / 2 - SCALE_Y(12);
  tft.drawCentreString(enc.label, x + w / 2, labelY, 1);
  
  // Value
  String valueStr = String(enc.currentValue);
  tft.setTextColor(THEME_ACCENT, bgColor);
  int valueY = y + h / 2;
  tft.drawCentreString(valueStr, x + w / 2, valueY, 2);
  
  // Range indicator
  if (enc.maxValue > enc.minValue) {
    int barWidth = w - SCALE_X(8);
    int barHeight = SCALE_Y(3);
    int barX = x + SCALE_X(4);
    int barY = y + h - SCALE_Y(8);
    
    // Background bar
    tft.fillRect(barX, barY, barWidth, barHeight, THEME_BG);
    
    // Value bar
    int range = enc.maxValue - enc.minValue;
    int valuePos = enc.currentValue - enc.minValue;
    int fillWidth = (barWidth * valuePos) / range;
    tft.fillRect(barX, barY, fillWidth, barHeight, THEME_PRIMARY);
  }
}

void handleEncoderPanelMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  int btnY = DISPLAY_HEIGHT - SCALE_Y(35);
  
  // Touch controls
  if (touch.justPressed) {
    // Page button
    if (isButtonPressed(SCALE_X(10), btnY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      currentEncoderPage = (currentEncoderPage + 1) % NUM_ENCODER_PAGES;
      requestRedraw();
      return;
    }
    
    // Fine/Coarse toggle
    if (isButtonPressed(SCALE_X(70), btnY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      encoderFineMode = !encoderFineMode;
      requestRedraw();
      return;
    }
    
    // Save mappings
    if (isButtonPressed(SCALE_X(130), btnY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      saveEncoderMappings();
      requestRedraw();
      return;
    }
    
    // Reset to defaults
    if (isButtonPressed(SCALE_X(190), btnY, BTN_MEDIUM_W, BTN_SMALL_H)) {
      setDefaultMappings();
      requestRedraw();
      return;
    }
  }
  
  // Poll encoder hardware
  if (encoder8.isConnected() && encoder8.poll()) {
    bool needsUpdate = false;
    
    for (int i = 0; i < 8; i++) {
      EncoderEvent event = encoder8.getEvent(i);
      
      // Handle encoder rotation
      if (event.delta != 0) {
        updateEncoderValue(i, event.delta);
        sendEncoderMIDI(i);
        needsUpdate = true;
      }
      
      // Handle button press (toggle fine/coarse or page)
      if (event.buttonJustPressed) {
        if (i == 0) {
          // First encoder button cycles pages
          currentEncoderPage = (currentEncoderPage + 1) % NUM_ENCODER_PAGES;
          needsUpdate = true;
        } else if (i == 1) {
          // Second encoder button toggles fine/coarse
          encoderFineMode = !encoderFineMode;
          needsUpdate = true;
        }
      }
    }
    
    // Reset encoder deltas after processing
    encoder8.resetAllEncoders();
    
    if (needsUpdate) {
      requestRedraw();
    }
  }
}

void updateEncoderValue(int encoderIndex, int delta) {
  if (encoderIndex >= 8) {
    return;
  }
  
  EncoderMapping &enc = encoderPages[currentEncoderPage].encoders[encoderIndex];
  
  // Apply step multiplier based on fine/coarse mode
  int step = encoderFineMode ? enc.step : (enc.step * 10);
  int change = delta * step;
  
  // Update value with clamping
  enc.currentValue += change;
  enc.currentValue = constrain(enc.currentValue, enc.minValue, enc.maxValue);
}

void sendEncoderMIDI(int encoderIndex) {
  if (encoderIndex >= 8) {
    return;
  }
  
  EncoderMapping &enc = encoderPages[currentEncoderPage].encoders[encoderIndex];
  
  if (enc.type == PARAM_MIDI_CC) {
    // Send MIDI CC message
    uint8_t status = 0xB0 | (enc.midiChannel & 0x0F);
    uint8_t value = constrain(enc.currentValue, 0, 127);
    sendMIDI(status, enc.midiCC, value);
  }
  // Add handling for PARAM_INTERNAL if needed in the future
}

void loadEncoderMappings() {
  Preferences prefs;
  if (!prefs.begin("encoder8", true)) {  // true = read-only
    setDefaultMappings();
    return;
  }
  
  // Load each encoder mapping
  for (int page = 0; page < NUM_ENCODER_PAGES; page++) {
    for (int enc = 0; enc < 8; enc++) {
      String key = "p" + String(page) + "e" + String(enc);
      
      // Load current value (other params are in the default config)
      int value = prefs.getInt((key + "v").c_str(), encoderPages[page].encoders[enc].currentValue);
      encoderPages[page].encoders[enc].currentValue = value;
    }
  }
  
  prefs.end();
}

void saveEncoderMappings() {
  Preferences prefs;
  if (!prefs.begin("encoder8", false)) {  // false = read-write
    return;
  }
  
  // Save each encoder mapping
  for (int page = 0; page < NUM_ENCODER_PAGES; page++) {
    for (int enc = 0; enc < 8; enc++) {
      String key = "p" + String(page) + "e" + String(enc);
      
      // Save current value
      prefs.putInt((key + "v").c_str(), encoderPages[page].encoders[enc].currentValue);
    }
  }
  
  prefs.end();
}

void setDefaultMappings() {
  // Reset all values to their defaults (already defined in the array initialization)
  for (int page = 0; page < NUM_ENCODER_PAGES; page++) {
    for (int enc = 0; enc < 8; enc++) {
      switch (page) {
        case 0: // MIDI CC 1-8
          encoderPages[page].encoders[enc].currentValue = 64;
          break;
        case 1: // MIDI CC 11-18
          encoderPages[page].encoders[enc].currentValue = 64;
          break;
        case 2: // Custom - set specific defaults
          if (enc == 0) encoderPages[page].encoders[enc].currentValue = 100; // Volume
          else if (enc == 4) encoderPages[page].encoders[enc].currentValue = 0; // Attack
          else if (enc == 6) encoderPages[page].encoders[enc].currentValue = 40; // Reverb
          else if (enc == 7) encoderPages[page].encoders[enc].currentValue = 0; // Delay
          else encoderPages[page].encoders[enc].currentValue = 64;
          break;
      }
    }
  }
}

#endif // ENABLE_M5_8ENCODER
