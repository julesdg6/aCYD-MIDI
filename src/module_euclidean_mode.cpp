#include "module_euclidean_mode.h"
#include "clock_manager.h"

#include <algorithm>
#include <cstring>

EuclideanState euclideanState;
static SequencerSyncState euclidSync;

// Y offset from bottom of display where control buttons are drawn.
static const int CONTROL_Y_OFFSET = SCALE_Y(50);

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
  euclideanState.bpm = target;
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
  const uint8_t baseSteps[EUCLIDEAN_VOICE_COUNT] = {16, 16, 16, 16};
  const uint8_t baseEvents[EUCLIDEAN_VOICE_COUNT] = {4, 4, 8, 5};
  const int8_t rotations[EUCLIDEAN_VOICE_COUNT] = {0, 2, 0, 1};
  const uint8_t notes[EUCLIDEAN_VOICE_COUNT] = {36, 38, 42, 39};
  const uint16_t colors[EUCLIDEAN_VOICE_COUNT] = {THEME_ERROR, THEME_WARNING, THEME_SUCCESS, THEME_ACCENT};

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
  std::memset(euclideanState.pendingNoteRelease, 0, sizeof(euclideanState.pendingNoteRelease));
  drawEuclideanMode();
}

void drawEuclideanMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("EUCLID", "Euclidean Rhythm", 3);

  // Draw circular visualization with sweeping line (like screenshot 16)
  int centerX = DISPLAY_CENTER_X;
  int centerY = HEADER_HEIGHT + SCALE_Y(70);
  int radius = SCALE_Y(50);
  
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
      int layerRadius = radius - (voiceIdx * SCALE_Y(12));
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
  
  // Draw voice labels
  int labelY = centerY + radius + SCALE_Y(20);
  for (int i = 0; i < EUCLIDEAN_VOICE_COUNT; i++) {
    int x = MARGIN_SMALL + i * (DISPLAY_WIDTH / EUCLIDEAN_VOICE_COUNT);
    tft.setTextColor(euclideanState.voices[i].color, THEME_BG);
    tft.drawString("V" + String(i+1), x, labelY, 1);
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString(String(euclideanState.voices[i].events) + "/" + 
                  String(euclideanState.voices[i].steps), x, labelY + SCALE_Y(10), 1);
  }

  int controlY = DISPLAY_HEIGHT - CONTROL_Y_OFFSET;
  bool playing = euclidSync.playing;
  drawRoundButton(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32),
                  playing ? "STOP" : "PLAY",
                  playing ? THEME_ERROR : THEME_SUCCESS, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(70), controlY, SCALE_X(45), SCALE_Y(32), "BPM-", THEME_SECONDARY, false, 1);
  drawRoundButton(MARGIN_SMALL + SCALE_X(120), controlY, SCALE_X(45), SCALE_Y(32), "BPM+", THEME_SECONDARY, false, 1);
  drawRoundButton(DISPLAY_WIDTH - SCALE_X(80), controlY, SCALE_X(70), SCALE_Y(32),
                  euclideanState.tripletMode ? "TRIP" : "2/4",
                  euclideanState.tripletMode ? THEME_ACCENT : THEME_SURFACE, false, 2);

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("BPM " + String(sharedBPM), DISPLAY_WIDTH - SCALE_X(80), controlY - SCALE_Y(20), 2);
}

void updateEuclideanSequencer() {
  euclidSync.tryStartIfReady(!instantStartMode);
  if (!euclidSync.playing) {
    return;
  }
  uint32_t readySteps = euclidSync.consumeReadySteps(getEuclideanStepIntervalTicks());
  if (readySteps == 0) {
    return;
  }

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

  if (!touch.justPressed) {
    return;
  }
  if (isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    releaseEuclideanNotes();
    exitToMenu();
    return;
  }

  int controlY = DISPLAY_HEIGHT - CONTROL_Y_OFFSET;
  if (isButtonPressed(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32))) {
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

  if (isButtonPressed(MARGIN_SMALL + SCALE_X(70), controlY, SCALE_X(45), SCALE_Y(32))) {
    adjustEuclidTempo(-5);
    return;
  }

  if (isButtonPressed(MARGIN_SMALL + SCALE_X(120), controlY, SCALE_X(45), SCALE_Y(32))) {
    adjustEuclidTempo(+5);
    return;
  }

  if (isButtonPressed(DISPLAY_WIDTH - SCALE_X(80), controlY, SCALE_X(70), SCALE_Y(32))) {
    euclideanState.tripletMode = !euclideanState.tripletMode;
    requestRedraw();
    return;
  }
}
