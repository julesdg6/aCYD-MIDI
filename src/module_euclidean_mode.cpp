#include "module_euclidean_mode.h"
#include "clock_manager.h"

#ifdef ENABLE_M5_8ENCODER
#include "drivers/m5_8encoder.h"
#endif

#include <algorithm>
#include <cstring>

EuclideanState euclideanState;
static SequencerSyncState euclidSync;

// CONTROL_Y_OFFSET removed — unused; layout uses explicit SCALE_* calls

static uint32_t getEuclideanStepIntervalTicks() {
  if (euclideanState.tripletMode) {
    return CLOCK_TICKS_PER_QUARTER / 6;
  }
  return CLOCK_TICKS_PER_SIXTEENTH;
}

static void adjustEuclidTempo(int delta) {
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
  // Use sharedBPM as single source of truth
  setSharedBPM(target);
  requestRedraw();
}

static void generateEuclideanPattern(EuclideanVoice &voice) {
  std::memset(voice.pattern, 0, sizeof(voice.pattern));
  if (voice.events == 0 || voice.steps == 0) {
    return;
  }

  int bucket = 0;
  for (int i = 0; i < voice.steps; ++i) {
    bucket += voice.events;
    if (bucket >= voice.steps) {
      bucket -= voice.steps;
      int pos = (i + voice.rotation) % voice.steps;
      if (pos < 0) {
        pos += voice.steps;
      }
      voice.pattern[pos] = true;
    }
  }
}

static void releaseEuclideanNotes() {
  for (int voiceIdx = 0; voiceIdx < EUCLIDEAN_VOICE_COUNT; ++voiceIdx) {
    if (!euclideanState.pendingNoteRelease[voiceIdx]) {
      continue;
    }
    sendMIDI(0x80, euclideanState.voices[voiceIdx].midiNote, 0);
    euclideanState.pendingNoteRelease[voiceIdx] = false;
  }
}

void initializeEuclideanMode() {
  const uint8_t baseSteps[EUCLIDEAN_VOICE_COUNT] = {16, 16, 16, 16, 16, 16, 16, 16};
  const uint8_t baseEvents[EUCLIDEAN_VOICE_COUNT] = {4, 4, 8, 5, 3, 6, 7, 2};
  const int8_t rotations[EUCLIDEAN_VOICE_COUNT] = {0, 2, 0, 1, 0, 1, 3, 2};
  const uint8_t notes[EUCLIDEAN_VOICE_COUNT] = {36, 38, 42, 39, 45, 47, 49, 51};
  const uint16_t colors[EUCLIDEAN_VOICE_COUNT] = {THEME_ERROR, THEME_WARNING, THEME_SUCCESS, THEME_ACCENT, THEME_PRIMARY, TFT_ORANGE, TFT_PURPLE, TFT_PINK};

  for (int i = 0; i < EUCLIDEAN_VOICE_COUNT; ++i) {
    euclideanState.voices[i].steps = baseSteps[i];
    euclideanState.voices[i].events = baseEvents[i];
    euclideanState.voices[i].rotation = rotations[i];
    euclideanState.voices[i].midiNote = notes[i];
    euclideanState.voices[i].color = colors[i];
    generateEuclideanPattern(euclideanState.voices[i]);
  }

  euclideanState.bpm = sharedBPM;
  euclideanState.currentStep = 0;
  euclidSync.reset();
  euclideanState.tripletMode = false;
  euclideanState.tripletAccumulator = 0;
  std::memset(euclideanState.pendingNoteRelease, 0, sizeof(euclideanState.pendingNoteRelease));
  
  drawEuclideanMode();
}

void drawEuclideanMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("EUCLID", "Euclidean Rhythm", 3);

  // Draw circular visualization on the LEFT side
  int centerX = MARGIN_SMALL + SCALE_X(65);  // Moved left
  int centerY = HEADER_HEIGHT + SCALE_Y(75);
  int radius = SCALE_Y(52);
  
  // Draw circle background
  tft.drawCircle(centerX, centerY, radius, THEME_TEXT_DIM);
  tft.drawCircle(centerX, centerY, radius - 1, THEME_TEXT_DIM);
  
  // Draw voices as segments around the circle
  for (int voiceIdx = 0; voiceIdx < EUCLIDEAN_VOICE_COUNT; ++voiceIdx) {
    const EuclideanVoice &voice = euclideanState.voices[voiceIdx];
    int totalSteps = voice.steps ? voice.steps : 1;
    
    // Draw each step as a point on the circle
    for (int step = 0; step < totalSteps; step++) {
      float angle = (2.0 * PI * step) / totalSteps - PI/2; // Start from top
      int layerRadius = radius - (voiceIdx * SCALE_Y(6));  // Reduced spacing for 8 voices
      int x = centerX + cos(angle) * layerRadius;
      int y = centerY + sin(angle) * layerRadius;
      
      bool active = voice.pattern[step];
      bool isCurrent = (euclidSync.playing &&
                       step == (euclideanState.currentStep % totalSteps));
      
      uint16_t color;
      if (isCurrent && active) {
        color = THEME_TEXT; // White for current active
      } else if (isCurrent) {
        color = THEME_ACCENT; // Cyan for current position
      } else if (active) {
        color = voice.color; // Voice color for active
      } else {
        color = THEME_SURFACE; // Dark for inactive
      }
      
      int dotSize = active ? SCALE_X(3) : SCALE_X(2);
      tft.fillCircle(x, y, dotSize, color);
    }
  }
  
  // Draw sweeping line showing current step position
  if (euclidSync.playing && euclideanState.voices[0].steps > 0) {
    int totalSteps = euclideanState.voices[0].steps;
    float angle = (2.0 * PI * (euclideanState.currentStep % totalSteps)) / totalSteps - PI/2;
    int lineEndX = centerX + cos(angle) * radius;
    int lineEndY = centerY + sin(angle) * radius;
    tft.drawLine(centerX, centerY, lineEndX, lineEndY, THEME_PRIMARY);
  }
  
  // Draw voice labels BELOW circle (more space)
  int labelY = centerY + radius + SCALE_Y(14);
  int labelSpacing = SCALE_X(20);  // Reduced spacing for 8 voices
  int labelStartX = centerX - (EUCLIDEAN_VOICE_COUNT * labelSpacing) / 2 + SCALE_X(8);
  
  for (int i = 0; i < EUCLIDEAN_VOICE_COUNT; i++) {
    int x = labelStartX + i * labelSpacing;
    tft.setTextColor(euclideanState.voices[i].color, THEME_BG);
    tft.drawString("V" + String(i+1), x, labelY, 1);  // Smaller font for 8 voices
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString(String(euclideanState.voices[i].events) + "/" + 
                  String(euclideanState.voices[i].steps), x, labelY + SCALE_Y(10), 1);  // Smaller font
  }

  // RIGHT SIDE: Controls with better grouping
  int rightX = centerX + radius + SCALE_X(20);
  int rightY = HEADER_HEIGHT + SCALE_Y(10);
  int btnW = DISPLAY_WIDTH - rightX - MARGIN_SMALL;
  int btnH = SCALE_Y(36);
  int btnSpacing = SCALE_Y(10);
  
  // Time division indicator (LARGE)
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String timeDivision = euclideanState.tripletMode ? "TRIPLET" : "2/4";
  tft.drawString(timeDivision, rightX, rightY, 4);  // Font 4 for visibility
  rightY += SCALE_Y(28);
  
  // Playback control
  bool playing = euclidSync.playing;
  drawRoundButton(rightX, rightY, btnW, btnH,
                  playing ? "STOP" : "PLAY",
                  playing ? THEME_ERROR : THEME_SUCCESS, false, 2);
  rightY += btnH + btnSpacing;
  
  // BPM controls removed - now accessible via header tap
  
  // Time division toggle
  drawRoundButton(rightX, rightY, btnW, btnH,
                  euclideanState.tripletMode ? "TRIP" : "STRAIGHT",
                  euclideanState.tripletMode ? THEME_ACCENT : THEME_SURFACE, false, 2);
}

void updateEuclideanSequencer() {
  bool wasPlaying = euclidSync.playing;
  euclidSync.tryStartIfReady(!instantStartMode);
  bool justStarted = euclidSync.playing && !wasPlaying;
  
  if (justStarted) {
    euclideanState.currentStep = 0;
    euclideanState.tripletAccumulator = 0;
  }
  
  if (!euclidSync.playing) {
    return;
  }
  
  // Use consumeReadySteps with the appropriate tick interval based on triplet mode
  uint32_t stepInterval = getEuclideanStepIntervalTicks();
  uint32_t readySteps = euclidSync.consumeReadySteps(stepInterval);
  
  // Triplet timing is handled by getEuclideanStepIntervalTicks();
  // Remove additional triplet accumulator/divisor adjustments to avoid double-adjusting timing.
  
  if (readySteps == 0) {
    return;
  }

  Serial.printf("[EUCLID] readySteps=%u currentStep=%u\n", readySteps, euclideanState.currentStep);

  for (uint32_t i = 0; i < readySteps; ++i) {
    releaseEuclideanNotes();
    for (int voiceIdx = 0; voiceIdx < EUCLIDEAN_VOICE_COUNT; ++voiceIdx) {
      EuclideanVoice &voice = euclideanState.voices[voiceIdx];
      if (voice.steps == 0) {
        continue;
      }
      int stepIndex = euclideanState.currentStep % voice.steps;
      if (!voice.pattern[stepIndex]) {
        continue;
      }
      sendMIDI(0x90, voice.midiNote, 110);
      euclideanState.pendingNoteRelease[voiceIdx] = true;
    }
    euclideanState.currentStep = (euclideanState.currentStep + 1) % EUCLIDEAN_MAX_STEPS;
  }
  requestRedraw();  // Request redraw to show step progress
}

void handleEuclideanMode() {
  updateEuclideanSequencer();

#ifdef ENABLE_M5_8ENCODER
  // Poll encoder hardware for parameter changes
  if (encoder8.isConnected() && encoder8.poll()) {
    bool needsUpdate = false;
    
    // Process 8 encoders in 4 pairs (steps/hits for each voice)
    // Encoder pairs: 0-1 (voice 0), 2-3 (voice 1), 4-5 (voice 2), 6-7 (voice 3)
    for (int voiceIdx = 0; voiceIdx < 4; voiceIdx++) {
      int stepsEnc = voiceIdx * 2;     // Even encoder for steps
      int hitsEnc = voiceIdx * 2 + 1;  // Odd encoder for hits
      
      EncoderEvent stepsEvent = encoder8.getEvent(stepsEnc);
      EncoderEvent hitsEvent = encoder8.getEvent(hitsEnc);
      
      // Adjust steps
      if (stepsEvent.delta != 0) {
        int newSteps = euclideanState.voices[voiceIdx].steps + stepsEvent.delta;
        newSteps = constrain(newSteps, 1, EUCLIDEAN_MAX_STEPS);
        euclideanState.voices[voiceIdx].steps = newSteps;
        generateEuclideanPattern(euclideanState.voices[voiceIdx]);
        needsUpdate = true;
      }
      
      // Adjust hits (events)
      if (hitsEvent.delta != 0) {
        int newHits = euclideanState.voices[voiceIdx].events + hitsEvent.delta;
        newHits = constrain(newHits, 0, euclideanState.voices[voiceIdx].steps);
        euclideanState.voices[voiceIdx].events = newHits;
        generateEuclideanPattern(euclideanState.voices[voiceIdx]);
        needsUpdate = true;
      }
    }
    
    // Reset encoder deltas after processing
    encoder8.resetAllEncoders();
    
    if (needsUpdate) {
      requestRedraw();
    }
  }
#endif

  if (!touch.justPressed) {
    return;
  }
  if (isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    releaseEuclideanNotes();
    exitToMenu();
    return;
  }

  // Calculate right-side control positions
  int centerX = MARGIN_SMALL + SCALE_X(65);
  int radius = SCALE_Y(52);
  int rightX = centerX + radius + SCALE_X(20);
  int rightY = HEADER_HEIGHT + SCALE_Y(10);
  int btnW = DISPLAY_WIDTH - rightX - MARGIN_SMALL;
  int btnH = SCALE_Y(36);
  int btnSpacing = SCALE_Y(10);
  
  // Skip time division label
  rightY += SCALE_Y(28);
  
  // PLAY/STOP button
  if (isButtonPressed(rightX, rightY, btnW, btnH)) {
    if (euclidSync.playing || euclidSync.startPending) {
      euclidSync.stopPlayback();
      releaseEuclideanNotes();
    } else {
      euclideanState.currentStep = 0;
      euclidSync.requestStart();
    }
    requestRedraw();
    return;
  }
  rightY += btnH + btnSpacing;
  
  // BPM control handlers removed - now accessible via header tap
  
  // Time division toggle
  if (isButtonPressed(rightX, rightY, btnW, btnH)) {
    euclideanState.tripletMode = !euclideanState.tripletMode;
    requestRedraw();
    return;
  }
}
