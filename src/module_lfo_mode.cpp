#include "module_lfo_mode.h"

LFOParams lfo;
String waveNames[] = {"SINE", "TRI", "SQR", "SAW"};

// Implementations
void initializeLFOMode() {
  lfo.rate = 1.0;
  lfo.amount = 64;
  lfo.ccTarget = 1; // Modulation wheel by default
  lfo.isRunning = false;
  lfo.phase = 0.0;
  lfo.waveform = 0;
  lfo.lastUpdate = 0;
  lfo.lastValue = 64;
  lfo.pitchWheelMode = false;
}

void drawLFOMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("LFO MOD", lfo.pitchWheelMode ? "Pitchwheel" : ("CC " + String(lfo.ccTarget)));
  
  drawLFOControls();
  drawWaveform();
}

void drawLFOControls() {
  int y = HEADER_HEIGHT + SCALE_Y(10);
  int lineSpacing = SCALE_Y(40);
  
  // Row 1: Start/Stop and Waveform selector  
  drawRoundButton(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(30), 
                  lfo.isRunning ? "STOP" : "START", 
                  lfo.isRunning ? THEME_ERROR : THEME_SUCCESS, false, 2);
  
  drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), y, SCALE_X(80), SCALE_Y(30),
                  waveNames[lfo.waveform], THEME_ACCENT, false, 2);
  
  y += lineSpacing;
  
  // Row 2: Rate slider
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setTextFont(2);
  tft.drawString("Rate", MARGIN_SMALL, y, 2);
  tft.setTextFont(4);  // Larger for value
  tft.drawString(String(lfo.rate, 1) + " Hz", MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // Rate slider bar
  int sliderX = DISPLAY_WIDTH / 2 - SCALE_X(10);
  int sliderY = y + SCALE_Y(10);
  int sliderW = SCALE_X(130);
  int sliderH = SCALE_Y(20);
  tft.drawRoundRect(sliderX, sliderY, sliderW, sliderH, 3, THEME_TEXT_DIM);
  
  float rateNorm = (lfo.rate - 0.1) / (10.0 - 0.1);  // Normalize to 0-1
  int fillW = (int)((sliderW - 4) * rateNorm);
  if (fillW > 0) {
    tft.fillRoundRect(sliderX + 2, sliderY + 2, fillW, sliderH - 4, 2, THEME_PRIMARY);
  }
  
  y += lineSpacing;
  
  // Row 3: Amount slider
  tft.setTextFont(2);
  tft.drawString("Amount", MARGIN_SMALL, y, 2);
  tft.setTextFont(4);  // Larger for value  
  tft.drawString(String(lfo.amount), MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // Amount slider bar
  sliderY = y + SCALE_Y(10);
  tft.drawRoundRect(sliderX, sliderY, sliderW, sliderH, 3, THEME_TEXT_DIM);
  fillW = ((sliderW - 4) * lfo.amount) / 127;
  if (fillW > 0) {
    tft.fillRoundRect(sliderX + 2, sliderY + 2, fillW, sliderH - 4, 2, THEME_SUCCESS);
  }
  
  y += lineSpacing;
  
  // Row 4: Target selection
  tft.setTextFont(2);
  tft.drawString("Target", MARGIN_SMALL, y, 2);
  
  String targetText;
  if (lfo.pitchWheelMode) {
    targetText = "PITCH";
  } else {
    targetText = "CC " + String(lfo.ccTarget);
  }
  tft.setTextFont(4);
  tft.drawString(targetText, MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // CC increment/decrement buttons (small, right side)
  if (!lfo.pitchWheelMode) {
    drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(70), y + SCALE_Y(10), 
                    SCALE_X(30), SCALE_Y(25), "-", THEME_SECONDARY, false, 1);
    drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(35), y + SCALE_Y(10), 
                    SCALE_X(30), SCALE_Y(25), "+", THEME_SECONDARY, false, 1);
  }
  
  // Pitch mode toggle
  drawRoundButton(DISPLAY_WIDTH / 2 - SCALE_X(40), y + SCALE_Y(10), SCALE_X(80), SCALE_Y(25),
                  "PITCH", lfo.pitchWheelMode ? THEME_PRIMARY : THEME_WARNING, 
                  lfo.pitchWheelMode, 2);
  
  y += lineSpacing + SCALE_Y(5);
  
  // Current value display (prominent)
  tft.setTextFont(2);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Output:", MARGIN_SMALL, y, 2);
  tft.setTextFont(4);
  tft.setTextColor(THEME_ACCENT, THEME_BG);
  tft.drawString(String(lfo.lastValue), MARGIN_SMALL + SCALE_X(60), y, 4);
  
  // Status indicator
  int indicatorX = DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(30);
  if (lfo.isRunning) {
    tft.fillCircle(indicatorX, y + SCALE_Y(10), SCALE_X(10), THEME_SUCCESS);
    tft.drawCircle(indicatorX, y + SCALE_Y(10), SCALE_X(10), THEME_TEXT);
  } else {
    tft.drawCircle(indicatorX, y + SCALE_Y(10), SCALE_X(10), THEME_TEXT_DIM);
  }
}

void drawWaveform() {
  // Draw LARGER waveform visualization at bottom
  int waveX = MARGIN_SMALL;
  int waveY = DISPLAY_HEIGHT - SCALE_Y(50);
  int waveW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  int waveH = SCALE_Y(40);  // Much larger!
  
  tft.drawRoundRect(waveX, waveY, waveW, waveH, 3, THEME_TEXT_DIM);
  
  // Draw center line
  tft.drawFastHLine(waveX + 1, waveY + waveH / 2, waveW - 2, THEME_TEXT_DIM);
  
  // Draw waveform based on type
  for (int x = 0; x < waveW - 2; x++) {
    float phase = (x / (float)(waveW - 2)) * 2 * PI;
    float value = 0;
    
    switch (lfo.waveform) {
      case 0: // Sine
        value = sin(phase);
        break;
      case 1: // Triangle
        value = (phase <= PI) ? (2 * phase / PI - 1) : (3 - 2 * phase / PI);
        break;
      case 2: // Square
        value = (phase <= PI) ? 1 : -1;
        break;
      case 3: // Sawtooth
        value = 2 * phase / (2 * PI) - 1;
        break;
    }
    
    int y = waveY + waveH/2 - (int)(value * (waveH/2 - 3));
    tft.drawPixel(waveX + 1 + x, y, THEME_PRIMARY);
    
    // Draw thicker line for better visibility
    if (y > 0 && y < DISPLAY_HEIGHT) {
      tft.drawPixel(waveX + 1 + x, y - 1, THEME_PRIMARY);
    }
  }
  
  // Current phase indicator
  if (lfo.isRunning) {
    float phaseNorm = lfo.phase / (2 * PI);
    int phaseX = waveX + 1 + (int)(phaseNorm * (waveW - 2));
    tft.drawFastVLine(phaseX, waveY + 1, waveH - 2, THEME_ACCENT);
  }
}

void handleLFOMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    lfo.isRunning = false;
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    int y = HEADER_HEIGHT + SCALE_Y(10);
    int lineSpacing = SCALE_Y(40);
    
    // Start/Stop button
    if (isButtonPressed(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(30))) {
      lfo.isRunning = !lfo.isRunning;
      if (lfo.isRunning) {
        lfo.phase = 0.0;
        lfo.lastUpdate = millis();
      }
      requestRedraw();
      return;
    }
    
    // Waveform selector
    if (isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), y, SCALE_X(80), SCALE_Y(30))) {
      lfo.waveform = (lfo.waveform + 1) % 4;
      requestRedraw();
      return;
    }
    
    y += lineSpacing;
    
    // Rate slider
    int sliderX = DISPLAY_WIDTH / 2 - SCALE_X(10);
    int sliderY = y + SCALE_Y(10);
    int sliderW = SCALE_X(130);
    int sliderH = SCALE_Y(20);
    if (isButtonPressed(sliderX, sliderY, sliderW, sliderH)) {
      float normX = (touch.x - sliderX) / (float)sliderW;
      normX = constrain(normX, 0.0f, 1.0f);
      lfo.rate = 0.1 + normX * (10.0 - 0.1);
      requestRedraw();
      return;
    }
    
    y += lineSpacing;
    
    // Amount slider
    sliderY = y + SCALE_Y(10);
    if (isButtonPressed(sliderX, sliderY, sliderW, sliderH)) {
      float normX = (touch.x - sliderX) / (float)sliderW;
      normX = constrain(normX, 0.0f, 1.0f);
      lfo.amount = (int)(normX * 127);
      requestRedraw();
      return;
    }
    
    y += lineSpacing;
    
    // CC +/- buttons (if not in pitch mode)
    if (!lfo.pitchWheelMode) {
      if (isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(70), y + SCALE_Y(10), 
                          SCALE_X(30), SCALE_Y(25))) {
        lfo.ccTarget = max(0, lfo.ccTarget - 1);
        requestRedraw();
        return;
      }
      if (isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(35), y + SCALE_Y(10), 
                          SCALE_X(30), SCALE_Y(25))) {
        lfo.ccTarget = min(127, lfo.ccTarget + 1);
        requestRedraw();
        return;
      }
    }
    
    // Pitch mode toggle
    if (isButtonPressed(DISPLAY_WIDTH / 2 - SCALE_X(40), y + SCALE_Y(10), 
                        SCALE_X(80), SCALE_Y(25))) {
      lfo.pitchWheelMode = !lfo.pitchWheelMode;
      requestRedraw();
      return;
    }
  }
  
  // Update LFO
  updateLFO();
}

void updateLFO() {
  if (!lfo.isRunning) return;
  
  unsigned long now = millis();
  float deltaTime = (now - lfo.lastUpdate) / 1000.0; // Convert to seconds
  lfo.lastUpdate = now;
  
  // Update phase
  lfo.phase += 2 * PI * lfo.rate * deltaTime;
  while (lfo.phase >= 2 * PI) {
    lfo.phase -= 2 * PI;
  }
  
  // Calculate LFO value
  float lfoValue = calculateLFOValue();
  
  // Calculate output value based on target type
  int outputValue;
  if (lfo.pitchWheelMode) {
    // For pitch bend: 8192 center, scale by amount
    outputValue = 8192 + (lfoValue * lfo.amount * 64); // Scale for pitch bend range
    outputValue = constrain(outputValue, 0, 16383);
  } else {
    // For CC: 64 center, scale by amount  
    outputValue = 64 + (lfoValue * lfo.amount / 2);
    outputValue = constrain(outputValue, 0, 127);
  }
  
  // Send if value changed significantly (reduce MIDI spam)
  if (abs(outputValue - lfo.lastValue) >= 1) {
    sendLFOValue(outputValue);
    lfo.lastValue = outputValue;
    
    // Update display every few cycles
    static int displayUpdateCounter = 0;
    if (++displayUpdateCounter >= 10) {
      drawLFOControls();
      drawWaveform();
      displayUpdateCounter = 0;
    }
  }
}

float calculateLFOValue() {
  switch (lfo.waveform) {
    case 0: // Sine
      return sin(lfo.phase);
    case 1: // Triangle
      return (lfo.phase <= PI) ? (2 * lfo.phase / PI - 1) : (3 - 2 * lfo.phase / PI);
    case 2: // Square
      return (lfo.phase <= PI) ? 1 : -1;
    case 3: // Sawtooth
      return 2 * lfo.phase / (2 * PI) - 1;
    default:
      return 0;
  }
}

void sendLFOValue(int value) {
  if (!deviceConnected) return;
  
  if (lfo.pitchWheelMode) {
    // Send pitchwheel (14-bit value already calculated)
    byte lsb = value & 0x7F;
    byte msb = (value >> 7) & 0x7F;
    
    // Pitchwheel: 0xE0, LSB, MSB
    midiPacket[2] = 0xE0;
    midiPacket[3] = lsb;
    midiPacket[4] = msb;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  } else {
    // Send regular CC
    sendMIDI(0xB0, lfo.ccTarget, value);
  }
}
