#include "module_fractal_echo_mode.h"
#include <algorithm>

// Global state
FractalParams fractalParams = {
  .enabled = true,
  .maxEchoesPerNote = 32,
  
  // Timing defaults
  .tapsMs = {100, 200, 300, 400},
  .iterations = 3,
  .stretch = 1.5f,
  
  // Dynamics defaults
  .velocityDecay = 0.8f,
  .minVelocity = 10,
  .baseLengthMs = 200,
  .lengthDecay = 0.9f,
  
  // Offsets (semitones per iteration)
  .offsets = {0, 7, 12, -5, 5, 0}
};

MidiEvent eventQueue[FRAC_MAX_EVENTS];
int eventQueueSize = 0;
int fractalPage = 0;

void initializeFractalEchoMode() {
  // Reset event queue
  eventQueueSize = 0;
  for (int i = 0; i < FRAC_MAX_EVENTS; i++) {
    eventQueue[i].active = false;
  }
  fractalPage = 0;
}

void drawFractalEchoMode() {
  tft.fillScreen(THEME_BG);
  
  String subtitle = fractalParams.enabled ? "ACTIVE" : "DISABLED";
  drawHeader("FRACTAL ECHO", subtitle);
  
  const int contentY = HEADER_HEIGHT + SCALE_Y(10);
  const int rowHeight = SCALE_Y(35);
  const int labelX = SCALE_X(10);
  const int valueX = SCALE_X(160);
  const int btnW = SCALE_X(50);
  const int btnH = SCALE_Y(25);
  
  // Page indicator
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String pageText = "Page " + String(fractalPage + 1) + "/3";
  tft.drawString(pageText, SCALE_X(10), DISPLAY_HEIGHT - SCALE_Y(25), 2);
  
  // Draw page navigation buttons
  drawRoundButton(SCALE_X(120), DISPLAY_HEIGHT - SCALE_Y(30), SCALE_X(40), SCALE_Y(25), "<", THEME_PRIMARY, false, 2);
  drawRoundButton(SCALE_X(170), DISPLAY_HEIGHT - SCALE_Y(30), SCALE_X(40), SCALE_Y(25), ">", THEME_PRIMARY, false, 2);
  
  // Enable/Disable toggle
  String enabledText = fractalParams.enabled ? "ON" : "OFF";
  uint16_t enabledColor = fractalParams.enabled ? THEME_SUCCESS : THEME_ERROR;
  drawRoundButton(DISPLAY_WIDTH - SCALE_X(70), SCALE_Y(10), SCALE_X(60), SCALE_Y(30), 
                  enabledText, enabledColor, false, 2);
  
  // Test button to trigger a note
  drawRoundButton(SCALE_X(10), SCALE_Y(10), SCALE_X(80), SCALE_Y(30), 
                  "TEST NOTE", THEME_ACCENT, false, 2);
  
  int y = contentY;
  
  if (fractalPage == 0) {
    // Page 1: Timing
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("TIMING", labelX, y, 2);
    y += SCALE_Y(25);
    
    // Tap 1-4
    for (int i = 0; i < FRAC_MAX_TAPS; i++) {
      tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
      tft.drawString("Tap " + String(i + 1), labelX, y, 2);
      
      String tapValue = String(fractalParams.tapsMs[i]) + " ms";
      drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
      tft.setTextColor(THEME_TEXT, THEME_BG);
      tft.drawString(tapValue, valueX + SCALE_X(15), y, 2);
      drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
      
      y += rowHeight;
    }
    
    // Iterations
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Iterations", labelX, y, 2);
    String iterValue = String(fractalParams.iterations);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(iterValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    y += rowHeight;
    
    // Stretch
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Stretch", labelX, y, 2);
    String stretchValue = String(fractalParams.stretch, 2);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(stretchValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    
  } else if (fractalPage == 1) {
    // Page 2: Dynamics
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("DYNAMICS", labelX, y, 2);
    y += SCALE_Y(25);
    
    // Velocity Decay
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Vel Decay", labelX, y, 2);
    String velDecayValue = String(fractalParams.velocityDecay, 2);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(velDecayValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    y += rowHeight;
    
    // Min Velocity
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Min Vel", labelX, y, 2);
    String minVelValue = String(fractalParams.minVelocity);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(minVelValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    y += rowHeight;
    
    // Base Length
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Base Len", labelX, y, 2);
    String baseLenValue = String(fractalParams.baseLengthMs) + " ms";
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(baseLenValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    y += rowHeight;
    
    // Length Decay
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Len Decay", labelX, y, 2);
    String lenDecayValue = String(fractalParams.lengthDecay, 2);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(lenDecayValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    y += rowHeight;
    
    // Max Echoes
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Max Echoes", labelX, y, 2);
    String maxEchoValue = String(fractalParams.maxEchoesPerNote);
    drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(maxEchoValue, valueX + SCALE_X(15), y, 2);
    drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
    
  } else if (fractalPage == 2) {
    // Page 3: Offsets
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("OFFSETS (semitones)", labelX, y, 2);
    y += SCALE_Y(25);
    
    // Show offsets for current iteration count
    for (int i = 0; i < fractalParams.iterations && i < FRAC_MAX_ITER; i++) {
      tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
      tft.drawString("Iter " + String(i + 1), labelX, y, 2);
      
      String offsetValue = (fractalParams.offsets[i] >= 0 ? "+" : "") + String(fractalParams.offsets[i]);
      drawRoundButton(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH, "-", THEME_WARNING, false, 2);
      tft.setTextColor(THEME_TEXT, THEME_BG);
      tft.drawString(offsetValue, valueX + SCALE_X(15), y, 2);
      drawRoundButton(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH, "+", THEME_SUCCESS, false, 2);
      
      y += rowHeight;
    }
  }
}

void handleFractalEchoMode() {
  // Process scheduled MIDI events
  processMidiEvents();
  
  if (!touch.justPressed) {
    return;
  }
  
  // Back button
  if (isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  // Enable/Disable toggle
  if (isButtonPressed(DISPLAY_WIDTH - SCALE_X(70), SCALE_Y(10), SCALE_X(60), SCALE_Y(30))) {
    fractalParams.enabled = !fractalParams.enabled;
    requestRedraw();
    return;
  }
  
  // Test button - trigger a C4 note with fractal echo
  if (isButtonPressed(SCALE_X(10), SCALE_Y(10), SCALE_X(80), SCALE_Y(30))) {
    uint8_t testNote = 60; // C4
    uint8_t testVel = 100;
    uint8_t testChannel = 0;
    
    // Send original note
    sendMIDI(0x90 | testChannel, testNote, testVel);
    
    // Add fractal echoes
    if (fractalParams.enabled) {
      addFractalEcho(testNote, testVel, testChannel);
    }
    
    // Schedule note off for original
    if (eventQueueSize < FRAC_MAX_EVENTS) {
      eventQueue[eventQueueSize].dueTimeMs = millis() + 300;
      eventQueue[eventQueueSize].status = 0x80 | testChannel;
      eventQueue[eventQueueSize].data1 = testNote;
      eventQueue[eventQueueSize].data2 = 0;
      eventQueue[eventQueueSize].active = true;
      eventQueueSize++;
    }
    return;
  }
  
  // Page navigation
  if (isButtonPressed(SCALE_X(120), DISPLAY_HEIGHT - SCALE_Y(30), SCALE_X(40), SCALE_Y(25))) {
    fractalPage = (fractalPage + 2) % 3;  // Go back
    requestRedraw();
    return;
  }
  if (isButtonPressed(SCALE_X(170), DISPLAY_HEIGHT - SCALE_Y(30), SCALE_X(40), SCALE_Y(25))) {
    fractalPage = (fractalPage + 1) % 3;  // Go forward
    requestRedraw();
    return;
  }
  
  const int contentY = HEADER_HEIGHT + SCALE_Y(10);
  const int rowHeight = SCALE_Y(35);
  const int valueX = SCALE_X(160);
  const int btnW = SCALE_X(50);
  const int btnH = SCALE_Y(25);
  
  int y = contentY + SCALE_Y(25);
  
  if (fractalPage == 0) {
    // Page 1: Timing controls
    for (int i = 0; i < FRAC_MAX_TAPS; i++) {
      // Decrease tap
      if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
        fractalParams.tapsMs[i] = max(0, (int)fractalParams.tapsMs[i] - 10);
        requestRedraw();
        return;
      }
      // Increase tap
      if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
        fractalParams.tapsMs[i] = min(2000, (int)fractalParams.tapsMs[i] + 10);
        requestRedraw();
        return;
      }
      y += rowHeight;
    }
    
    // Iterations
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.iterations = max(1, (int)fractalParams.iterations - 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.iterations = min(FRAC_MAX_ITER, (int)fractalParams.iterations + 1);
      requestRedraw();
      return;
    }
    y += rowHeight;
    
    // Stretch
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.stretch = max(0.25f, fractalParams.stretch - 0.05f);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.stretch = min(2.0f, fractalParams.stretch + 0.05f);
      requestRedraw();
      return;
    }
    
  } else if (fractalPage == 1) {
    // Page 2: Dynamics controls
    
    // Velocity Decay
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.velocityDecay = max(0.0f, fractalParams.velocityDecay - 0.05f);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.velocityDecay = min(1.0f, fractalParams.velocityDecay + 0.05f);
      requestRedraw();
      return;
    }
    y += rowHeight;
    
    // Min Velocity
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.minVelocity = max(1, (int)fractalParams.minVelocity - 5);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.minVelocity = min(127, (int)fractalParams.minVelocity + 5);
      requestRedraw();
      return;
    }
    y += rowHeight;
    
    // Base Length
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.baseLengthMs = max(10, (int)fractalParams.baseLengthMs - 10);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.baseLengthMs = min(2000, (int)fractalParams.baseLengthMs + 10);
      requestRedraw();
      return;
    }
    y += rowHeight;
    
    // Length Decay
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.lengthDecay = max(0.0f, fractalParams.lengthDecay - 0.05f);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.lengthDecay = min(1.0f, fractalParams.lengthDecay + 0.05f);
      requestRedraw();
      return;
    }
    y += rowHeight;
    
    // Max Echoes
    if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.maxEchoesPerNote = max(1, (int)fractalParams.maxEchoesPerNote - 4);
      requestRedraw();
      return;
    }
    if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
      fractalParams.maxEchoesPerNote = min(64, (int)fractalParams.maxEchoesPerNote + 4);
      requestRedraw();
      return;
    }
    
  } else if (fractalPage == 2) {
    // Page 3: Offsets controls
    for (int i = 0; i < fractalParams.iterations && i < FRAC_MAX_ITER; i++) {
      // Decrease offset
      if (isButtonPressed(valueX - SCALE_X(40), y - SCALE_Y(3), btnW, btnH)) {
        fractalParams.offsets[i] = max(-24, (int)fractalParams.offsets[i] - 1);
        requestRedraw();
        return;
      }
      // Increase offset
      if (isButtonPressed(valueX + SCALE_X(70), y - SCALE_Y(3), btnW, btnH)) {
        fractalParams.offsets[i] = min(24, (int)fractalParams.offsets[i] + 1);
        requestRedraw();
        return;
      }
      y += rowHeight;
    }
  }
}

// Add fractal echoes for a note
void addFractalEcho(uint8_t note, uint8_t velocity, uint8_t channel) {
  if (!fractalParams.enabled) {
    return;
  }
  
  uint32_t now = millis();
  int echoCount = 0;
  
  // For each iteration
  for (int iter = 0; iter < fractalParams.iterations && iter < FRAC_MAX_ITER; iter++) {
    float iterStretch = pow(fractalParams.stretch, iter);
    float iterVelocityScale = pow(fractalParams.velocityDecay, iter);
    float iterLengthScale = pow(fractalParams.lengthDecay, iter);
    
    int iterNote = note + fractalParams.offsets[iter];
    if (iterNote < 0) iterNote = 0;
    if (iterNote > 127) iterNote = 127;
    
    int iterVelocity = (int)(velocity * iterVelocityScale);
    if (iterVelocity < fractalParams.minVelocity) {
      break;  // Stop if velocity too low
    }
    if (iterVelocity > 127) iterVelocity = 127;
    
    uint16_t iterLength = (uint16_t)(fractalParams.baseLengthMs * iterLengthScale);
    
    // For each tap
    for (int tap = 0; tap < FRAC_MAX_TAPS; tap++) {
      if (fractalParams.tapsMs[tap] == 0) continue;
      
      uint32_t delay = (uint32_t)(fractalParams.tapsMs[tap] * iterStretch);
      
      // Add Note On event
      if (eventQueueSize < FRAC_MAX_EVENTS) {
        eventQueue[eventQueueSize].dueTimeMs = now + delay;
        eventQueue[eventQueueSize].status = 0x90 | channel;
        eventQueue[eventQueueSize].data1 = iterNote;
        eventQueue[eventQueueSize].data2 = iterVelocity;
        eventQueue[eventQueueSize].active = true;
        eventQueueSize++;
        echoCount++;
      }
      
      // Add Note Off event
      if (eventQueueSize < FRAC_MAX_EVENTS) {
        eventQueue[eventQueueSize].dueTimeMs = now + delay + iterLength;
        eventQueue[eventQueueSize].status = 0x80 | channel;
        eventQueue[eventQueueSize].data1 = iterNote;
        eventQueue[eventQueueSize].data2 = 0;
        eventQueue[eventQueueSize].active = true;
        eventQueueSize++;
      }
      
      // Check max echoes limit
      if (echoCount >= fractalParams.maxEchoesPerNote) {
        return;
      }
    }
  }
}

// Process scheduled MIDI events
void processMidiEvents() {
  uint32_t now = millis();
  
  for (int i = 0; i < eventQueueSize; i++) {
    if (!eventQueue[i].active) continue;
    
    if (now >= eventQueue[i].dueTimeMs) {
      // Send the MIDI event
      sendMIDI(eventQueue[i].status, eventQueue[i].data1, eventQueue[i].data2);
      
      // Mark as inactive
      eventQueue[i].active = false;
    }
  }
  
  // Compact the queue (remove inactive events)
  int writeIdx = 0;
  for (int readIdx = 0; readIdx < eventQueueSize; readIdx++) {
    if (eventQueue[readIdx].active) {
      if (writeIdx != readIdx) {
        eventQueue[writeIdx] = eventQueue[readIdx];
      }
      writeIdx++;
    }
  }
  eventQueueSize = writeIdx;
}
