// morph_mode.cpp
// MORPH - Gesture recording and morphing sequencer
// Records touch gestures, stores in 4 memory slots, and morphologically
// interpolates between them to create evolving musical patterns

#include "morph_mode.h"
#include "common_definitions.h"
#include "midi_utils.h"

MorphState morphState;

// Memory slot colors (4 corners)
const uint16_t SLOT_COLORS[NUM_MEMORY_SLOTS] = {
  TFT_RED,      // Top-left
  TFT_YELLOW,   // Top-right
  TFT_GREEN,    // Bottom-left
  TFT_CYAN      // Bottom-right
};

// Catmull-Rom spline interpolation for smooth curves
float catmullRom(float p0, float p1, float p2, float p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  
  return 0.5f * (
    (2.0f * p1) +
    (-p0 + p2) * t +
    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
  );
}

// Interpolate a point along a gesture using Catmull-Rom splines
GesturePoint interpolateGesture(const Gesture& gesture, float t) {
  if (gesture.numPoints == 0) {
    return {0.5f, 0.5f, 0, 0, 0};
  }
  
  if (gesture.numPoints == 1) {
    return gesture.points[0];
  }
  
  // Find which segment we're in
  float segmentFloat = t * (gesture.numPoints - 1);
  int segment = (int)segmentFloat;
  float localT = segmentFloat - segment;
  
  if (segment >= gesture.numPoints - 1) {
    return gesture.points[gesture.numPoints - 1];
  }
  
  // Get 4 control points for Catmull-Rom
  int i0 = max(0, segment - 1);
  int i1 = segment;
  int i2 = min(gesture.numPoints - 1, segment + 1);
  int i3 = min(gesture.numPoints - 1, segment + 2);
  
  GesturePoint result;
  result.x = catmullRom(
    gesture.points[i0].x,
    gesture.points[i1].x,
    gesture.points[i2].x,
    gesture.points[i3].x,
    localT
  );
  result.y = catmullRom(
    gesture.points[i0].y,
    gesture.points[i1].y,
    gesture.points[i2].y,
    gesture.points[i3].y,
    localT
  );
  
  // Interpolate velocity
  result.velocity = gesture.points[i1].velocity * (1.0f - localT) + 
                    gesture.points[i2].velocity * localT;
  result.pressure = result.velocity;
  result.time = millis();
  
  return result;
}

// Bilinear interpolation between 4 corner gestures
void morphGestures() {
  // Clear morphed gesture
  morphState.morphedGesture.numPoints = 0;
  morphState.morphedGesture.isValid = false;
  
  // Count valid memories
  int validCount = 0;
  for (int i = 0; i < NUM_MEMORY_SLOTS; i++) {
    if (morphState.memories[i].isValid) validCount++;
  }
  
  if (validCount == 0) return;
  
  // Get maximum point count from all gestures
  int maxPoints = 0;
  for (int i = 0; i < NUM_MEMORY_SLOTS; i++) {
    if (morphState.memories[i].isValid && morphState.memories[i].numPoints > maxPoints) {
      maxPoints = morphState.memories[i].numPoints;
    }
  }
  
  if (maxPoints == 0) return;
  
  // Sample and blend gestures
  int sampleCount = min(maxPoints, MAX_GESTURE_POINTS);
  
  for (int i = 0; i < sampleCount; i++) {
    float t = (float)i / (float)(sampleCount - 1);
    
    // Sample each corner gesture at time t
    GesturePoint corners[4];
    for (int slot = 0; slot < NUM_MEMORY_SLOTS; slot++) {
      if (morphState.memories[slot].isValid) {
        corners[slot] = interpolateGesture(morphState.memories[slot], t);
      } else {
        // Use center point if gesture doesn't exist
        corners[slot] = {0.5f, 0.5f, 0, 0, 0};
      }
    }
    
    // Bilinear interpolation
    // Top-left(0), Top-right(1), Bottom-left(2), Bottom-right(3)
    float topX = corners[0].x * (1.0f - morphState.morphX) + corners[1].x * morphState.morphX;
    float topY = corners[0].y * (1.0f - morphState.morphX) + corners[1].y * morphState.morphX;
    
    float bottomX = corners[2].x * (1.0f - morphState.morphX) + corners[3].x * morphState.morphX;
    float bottomY = corners[2].y * (1.0f - morphState.morphX) + corners[3].y * morphState.morphX;
    
    GesturePoint morphed;
    morphed.x = topX * (1.0f - morphState.morphY) + bottomX * morphState.morphY;
    morphed.y = topY * (1.0f - morphState.morphY) + bottomY * morphState.morphY;
    
    // Blend velocity
    float topVel = corners[0].velocity * (1.0f - morphState.morphX) + corners[1].velocity * morphState.morphX;
    float bottomVel = corners[2].velocity * (1.0f - morphState.morphX) + corners[3].velocity * morphState.morphX;
    morphed.velocity = topVel * (1.0f - morphState.morphY) + bottomVel * morphState.morphY;
    morphed.pressure = morphed.velocity;
    morphed.time = millis();
    
    morphState.morphedGesture.points[i] = morphed;
  }
  
  morphState.morphedGesture.numPoints = sampleCount;
  morphState.morphedGesture.isValid = true;
  
  // Apply mutation if enabled
  if (morphState.mutationAmount > 0) {
    mutateGesture(morphState.morphedGesture, morphState.mutationAmount);
  }
}

// Add procedural variation to gesture
void mutateGesture(Gesture& gesture, uint8_t amount) {
  if (amount == 0 || gesture.numPoints == 0) return;
  
  float mutationScale = amount / 100.0f * 0.15f; // Max 15% deviation
  
  for (int i = 0; i < gesture.numPoints; i++) {
    // Add smooth noise (not completely random)
    float noiseX = (sin(i * 0.5f + millis() * 0.001f) * 0.5f + 0.5f) - 0.5f;
    float noiseY = (cos(i * 0.7f + millis() * 0.0015f) * 0.5f + 0.5f) - 0.5f;
    
    gesture.points[i].x += noiseX * mutationScale;
    gesture.points[i].y += noiseY * mutationScale;
    
    // Clamp to valid range
    gesture.points[i].x = constrain(gesture.points[i].x, 0.0f, 1.0f);
    gesture.points[i].y = constrain(gesture.points[i].y, 0.0f, 1.0f);
  }
}

// Start recording a gesture
void startRecording(int memorySlot) {
  morphState.isRecording = true;
  morphState.currentMemorySlot = memorySlot;
  morphState.recordPointIndex = 0;
  morphState.recordStartTime = millis();
  morphState.memories[memorySlot].numPoints = 0;
  morphState.memories[memorySlot].isValid = false;
  Serial.printf("Started recording to slot %d\n", memorySlot);
}

// Record a point during gesture capture
void recordGesturePoint(float x, float y) {
  if (!morphState.isRecording) return;
  if (morphState.recordPointIndex >= MAX_GESTURE_POINTS) return;
  
  int slot = morphState.currentMemorySlot;
  int idx = morphState.recordPointIndex;
  
  GesturePoint& point = morphState.memories[slot].points[idx];
  point.x = x;
  point.y = y;
  point.time = millis();
  
  // Calculate velocity from previous point
  if (idx > 0) {
    GesturePoint& prev = morphState.memories[slot].points[idx - 1];
    float dx = point.x - prev.x;
    float dy = point.y - prev.y;
    float distance = sqrt(dx * dx + dy * dy);
    float timeDelta = (point.time - prev.time) / 1000.0f; // seconds
    point.velocity = timeDelta > 0 ? distance / timeDelta : 0;
    point.pressure = constrain(point.velocity * 10.0f, 0.0f, 1.0f);
  } else {
    point.velocity = 0;
    point.pressure = 0.5f;
  }
  
  morphState.recordPointIndex++;
  morphState.memories[slot].numPoints = morphState.recordPointIndex;
}

// Stop recording and finalize gesture
void stopRecording() {
  if (!morphState.isRecording) return;
  
  int slot = morphState.currentMemorySlot;
  morphState.memories[slot].duration = millis() - morphState.recordStartTime;
  morphState.memories[slot].isValid = morphState.memories[slot].numPoints > 2;
  morphState.memories[slot].color = SLOT_COLORS[slot];
  morphState.isRecording = false;
  
  Serial.printf("Stopped recording slot %d: %d points, %.1fms\n",
                slot, morphState.memories[slot].numPoints, 
                morphState.memories[slot].duration);
  
  // Re-morph with new gesture
  morphGestures();
}

// Generate MIDI from gesture point (position maps to pitch and velocity)
void generateMIDIFromGesture(const GesturePoint& point) {
  // Map Y position to pitch (higher = higher pitch)
  int pitchRange = 24; // 2 octaves
  int pitch = morphState.rootNote + (int)((1.0f - point.y) * pitchRange);
  
  // Quantize if enabled
  if (morphState.quantizeSteps > 0) {
    int semitone = pitch % 12;
    int octave = pitch / 12;
    int quantizedSemitone = (semitone * 12 / morphState.quantizeSteps) * 
                            (morphState.quantizeSteps / 12);
    pitch = octave * 12 + quantizedSemitone;
  }
  
  pitch = constrain(pitch, 0, 127);
  
  // Map velocity to pressure/velocity
  int velocity = (int)(point.pressure * 100) + 27; // 27-127 range
  velocity = constrain(velocity, 1, 127);
  
  // Send note
  sendNoteOn(pitch, velocity);
  sendNoteOff(pitch); // Immediate note off for percussive feel
  
  // Send CC based on X position (e.g., CC74 for filter)
  int ccValue = (int)(point.x * 127);
  sendControlChange(74, ccValue);
}

void initializeMorphMode() {
  // Clear all memory slots
  for (int i = 0; i < NUM_MEMORY_SLOTS; i++) {
    morphState.memories[i].numPoints = 0;
    morphState.memories[i].isValid = false;
    morphState.memories[i].color = SLOT_COLORS[i];
  }
  
  morphState.currentMemorySlot = 0;
  morphState.morphX = 0.5f;
  morphState.morphY = 0.5f;
  morphState.isPlaying = false;
  morphState.playbackPosition = 0.0f;
  morphState.isRecording = false;
  morphState.mutationAmount = 20; // 20% default mutation
  morphState.quantizeSteps = 12; // Chromatic by default
  morphState.bpm = 120;
  morphState.rootNote = 48; // C3
  morphState.trailIndex = 0;
  
  Serial.println("MORPH mode initialized");
  drawMorphMode();
}

void drawMorphMode() {
  tft.fillScreen(THEME_BG);
  
  // Unified header with BLE, SD, and BPM indicators
  drawModuleHeader("MORPH");
  
  // Central gesture area - calculated from screen dimensions
  int gestureX = 20;
  int gestureY = CONTENT_TOP;
  int gestureW = (SCREEN_WIDTH * 2) / 3 - 30;
  int gestureH = SCREEN_HEIGHT - CONTENT_TOP - 10;
  
  tft.drawRect(gestureX, gestureY, gestureW, gestureH, THEME_ACCENT);
  
  // Draw memory slot indicators in corners
  int slotSize = 30;
  int slotPositions[4][2] = {
    {gestureX + 5, gestureY + 5},                          // Top-left
    {gestureX + gestureW - slotSize - 5, gestureY + 5},    // Top-right
    {gestureX + 5, gestureY + gestureH - slotSize - 5},    // Bottom-left
    {gestureX + gestureW - slotSize - 5, gestureY + gestureH - slotSize - 5}  // Bottom-right
  };
  
  for (int i = 0; i < NUM_MEMORY_SLOTS; i++) {
    uint16_t color = morphState.memories[i].isValid ? SLOT_COLORS[i] : 0x4208;
    tft.fillRoundRect(slotPositions[i][0], slotPositions[i][1], 
                      slotSize, slotSize, 4, color);
    tft.setTextColor(THEME_BG, color);
    tft.setTextSize(2);
    tft.setCursor(slotPositions[i][0] + 10, slotPositions[i][1] + 8);
    tft.print(i + 1);
  }
  
  // Draw morphed gesture trail
  if (morphState.morphedGesture.isValid && morphState.morphedGesture.numPoints > 1) {
    for (int i = 1; i < morphState.morphedGesture.numPoints; i++) {
      int x1 = gestureX + (int)(morphState.morphedGesture.points[i-1].x * gestureW);
      int y1 = gestureY + (int)(morphState.morphedGesture.points[i-1].y * gestureH);
      int x2 = gestureX + (int)(morphState.morphedGesture.points[i].x * gestureW);
      int y2 = gestureY + (int)(morphState.morphedGesture.points[i].y * gestureH);
      
      // Color gradient based on velocity
      uint8_t intensity = (uint8_t)(morphState.morphedGesture.points[i].velocity * 255);
      uint16_t lineColor = tft.color565(intensity, 100, 255 - intensity);
      
      tft.drawLine(x1, y1, x2, y2, lineColor);
    }
  }
  
  // Draw playback position indicator
  if (morphState.isPlaying && morphState.morphedGesture.isValid) {
    GesturePoint current = interpolateGesture(morphState.morphedGesture, 
                                              morphState.playbackPosition);
    int px = gestureX + (int)(current.x * gestureW);
    int py = gestureY + (int)(current.y * gestureH);
    tft.fillCircle(px, py, 6, TFT_WHITE);
    tft.drawCircle(px, py, 8, TFT_YELLOW);
  }
  
  // Control panel (right side) - calculated from screen dimensions
  int ctrlX = gestureX + gestureW + 10;
  int ctrlY = CONTENT_TOP;
  
  // Morph XY control
  tft.setTextSize(1);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setCursor(ctrlX, ctrlY);
  tft.print("Morph");
  
  int morphBoxSize = 80;
  tft.drawRect(ctrlX, ctrlY + 12, morphBoxSize, morphBoxSize, THEME_ACCENT);
  
  // Draw morph position
  int morphPosX = ctrlX + (int)(morphState.morphX * morphBoxSize);
  int morphPosY = ctrlY + 12 + (int)(morphState.morphY * morphBoxSize);
  tft.fillCircle(morphPosX, morphPosY, 5, TFT_MAGENTA);
  
  // Mutation slider
  tft.setCursor(ctrlX, ctrlY + 100);
  tft.print("Mutate");
  tft.fillRoundRect(ctrlX, ctrlY + 112, 80, 20, 3, THEME_ACCENT);
  tft.setTextColor(THEME_BG, THEME_ACCENT);
  tft.setCursor(ctrlX + 25, ctrlY + 116);
  tft.print(morphState.mutationAmount);
  tft.print("%");
  
  // Quantize
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setCursor(ctrlX, ctrlY + 140);
  tft.print("Quantize");
  tft.fillRoundRect(ctrlX, ctrlY + 152, 80, 20, 3, THEME_ACCENT);
  tft.setTextColor(THEME_BG, THEME_ACCENT);
  tft.setCursor(ctrlX + 20, ctrlY + 156);
  if (morphState.quantizeSteps == 0) tft.print("OFF");
  else tft.print(morphState.quantizeSteps);
  
  // BPM
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setCursor(ctrlX, ctrlY + 180);
  tft.print("BPM");
  tft.fillRoundRect(ctrlX, ctrlY + 192, 80, 20, 3, THEME_ACCENT);
  tft.setTextColor(THEME_BG, THEME_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(ctrlX + 20, ctrlY + 194);
  tft.print(morphState.bpm);
  
  // Bottom controls
  tft.setTextSize(1);
  
  // Record buttons
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.setCursor(10, 283);
  tft.print("REC:");
  for (int i = 0; i < 4; i++) {
    int btnX = 40 + i * 40;
    bool isRecording = morphState.isRecording && morphState.currentMemorySlot == i;
    tft.fillRoundRect(btnX, 280, 35, 30, 4, isRecording ? TFT_RED : SLOT_COLORS[i]);
    tft.setTextColor(THEME_BG, isRecording ? TFT_RED : SLOT_COLORS[i]);
    tft.setTextSize(2);
    tft.setCursor(btnX + 12, 285);
    tft.print(i + 1);
  }
  
  // Play/Stop
  tft.fillRoundRect(200, 280, 60, 30, 5, morphState.isPlaying ? TFT_RED : TFT_GREEN);
  tft.setTextColor(THEME_BG, morphState.isPlaying ? TFT_RED : TFT_GREEN);
  tft.setTextSize(1);
  tft.setCursor(morphState.isPlaying ? 212 : 208, 290);
  tft.print(morphState.isPlaying ? "STOP" : "PLAY");
  
  // Clear
  tft.fillRoundRect(270, 280, 50, 30, 5, TFT_ORANGE);
  tft.setTextColor(THEME_BG, TFT_ORANGE);
  tft.setCursor(278, 290);
  tft.print("CLEAR");
}

void updateMorphPlayback() {
  if (!morphState.isPlaying || !morphState.morphedGesture.isValid) return;
  
  // Calculate step duration based on BPM
  float beatDuration = 60000.0f / morphState.bpm; // ms per beat
  float loopDuration = beatDuration * 4.0f; // 4 beats per loop
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - morphState.lastPlaybackTime;
  
  if (elapsed >= (unsigned long)(loopDuration / 32.0f)) { // 32 steps per loop
    morphState.lastPlaybackTime = currentTime;
    
    // Advance playback position
    morphState.playbackPosition += 0.03125f; // 1/32
    if (morphState.playbackPosition >= 1.0f) {
      morphState.playbackPosition = 0.0f;
      // Re-morph with potential mutation on each loop
      morphGestures();
    }
    
    // Generate MIDI
    GesturePoint current = interpolateGesture(morphState.morphedGesture, 
                                              morphState.playbackPosition);
    generateMIDIFromGesture(current);
  }
}

void handleMorphMode() {
  // Handle recording gesture in main area
  int gestureX = 20;
  int gestureY = 35;
  int gestureW = 280;
  int gestureH = 240;
  
  if (touch.isPressed && morphState.isRecording) {
    // Check if touch is in gesture area
    if (touch.x >= gestureX && touch.x <= gestureX + gestureW &&
        touch.y >= gestureY && touch.y <= gestureY + gestureH) {
      float normX = (float)(touch.x - gestureX) / (float)gestureW;
      float normY = (float)(touch.y - gestureY) / (float)gestureH;
      recordGesturePoint(normX, normY);
      drawMorphMode(); // Real-time feedback
    }
  } else if (!touch.isPressed && morphState.isRecording) {
    stopRecording();
    drawMorphMode();
  }
  
  if (touch.justPressed) {
    int touchX = touch.x;
    int touchY = touch.y;
    
    // Record buttons
    for (int i = 0; i < 4; i++) {
      int btnX = 40 + i * 40;
      if (touchX >= btnX && touchX <= btnX + 35 && touchY >= 280 && touchY <= 310) {
        startRecording(i);
        drawMorphMode();
        return;
      }
    }
    
    // Play/Stop
    if (touchX >= 200 && touchX <= 260 && touchY >= 280 && touchY <= 310) {
      morphState.isPlaying = !morphState.isPlaying;
      if (morphState.isPlaying) {
        morphState.playbackPosition = 0.0f;
        morphState.lastPlaybackTime = millis();
        morphGestures(); // Ensure we have current morph
      }
      drawMorphMode();
    }
    
    // Clear
    else if (touchX >= 270 && touchX <= 320 && touchY >= 280 && touchY <= 310) {
      initializeMorphMode();
      drawMorphMode();
    }
    
    // Morph XY control
    else if (touchX >= 310 && touchX <= 390 && touchY >= 47 && touchY <= 127) {
      morphState.morphX = (float)(touchX - 310) / 80.0f;
      morphState.morphY = (float)(touchY - 47) / 80.0f;
      morphGestures();
      drawMorphMode();
    }
    
    // Mutation
    else if (touchX >= 310 && touchX <= 390 && touchY >= 147 && touchY <= 167) {
      morphState.mutationAmount = (morphState.mutationAmount + 20) % 120;
      if (morphState.mutationAmount > 100) morphState.mutationAmount = 0;
      morphGestures();
      drawMorphMode();
    }
    
    // Quantize
    else if (touchX >= 310 && touchX <= 390 && touchY >= 187 && touchY <= 207) {
      const uint8_t quantizeOptions[] = {0, 4, 8, 12, 16};
      static int qIdx = 2; // Start at 12
      qIdx = (qIdx + 1) % 5;
      morphState.quantizeSteps = quantizeOptions[qIdx];
      drawMorphMode();
    }
    
    // BPM
    else if (touchX >= 310 && touchX <= 390 && touchY >= 227 && touchY <= 247) {
      morphState.bpm += 10;
      if (morphState.bpm > 240) morphState.bpm = 60;
      drawMorphMode();
    }
    
    // Back button
    else if (isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
      currentMode = MENU;
      return;
    }
  }
  
  // Update playback
  updateMorphPlayback();
}
