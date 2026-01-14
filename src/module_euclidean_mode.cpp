#include "module_euclidean_mode.h"

#include <algorithm>
#include <cstring>

EuclideanState euclideanState;

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

  euclideanState.bpm = 120;
  euclideanState.currentStep = 0;
  euclideanState.isPlaying = false;
  euclideanState.tripletMode = false;
  euclideanState.lastStepTime = millis();
  drawEuclideanMode();
}

void drawEuclideanMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("EUCLID", "Euclidean Rhythm");

  int y = HEADER_HEIGHT + SCALE_Y(10);
  for (int voiceIdx = 0; voiceIdx < EUCLIDEAN_VOICE_COUNT; ++voiceIdx) {
    const EuclideanVoice &voice = euclideanState.voices[voiceIdx];
    int totalSteps = voice.steps ? voice.steps : 1;
    int rowHeight = SCALE_Y(14);
    int stepSpacing = SCALE_X(2);
    int availableWidth = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
    int stepW = std::max(SCALE_X(6), (availableWidth - (totalSteps - 1) * stepSpacing) / totalSteps);

    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Voice " + String(voiceIdx + 1), MARGIN_SMALL, y - SCALE_Y(14), 1);

    int stepY = y;
    for (int step = 0; step < totalSteps; ++step) {
      int x = MARGIN_SMALL + step * (stepW + stepSpacing);
      bool active = voice.pattern[step];
      uint16_t fill = active ? voice.color : THEME_SURFACE;
      tft.fillRect(x, stepY, stepW, rowHeight, fill);
      tft.drawRect(x, stepY, stepW, rowHeight, THEME_BG);
    }
    y += rowHeight + SCALE_Y(10);
  }

  int controlY = DISPLAY_HEIGHT - SCALE_Y(60);
  drawRoundButton(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32),
                  euclideanState.isPlaying ? "STOP" : "PLAY",
                  euclideanState.isPlaying ? THEME_ERROR : THEME_SUCCESS, false, 2);
  drawRoundButton(MARGIN_SMALL + SCALE_X(70), controlY, SCALE_X(45), SCALE_Y(32), "BPM-", THEME_SECONDARY, false, 1);
  drawRoundButton(MARGIN_SMALL + SCALE_X(120), controlY, SCALE_X(45), SCALE_Y(32), "BPM+", THEME_SECONDARY, false, 1);
  drawRoundButton(DISPLAY_WIDTH - SCALE_X(80), controlY, SCALE_X(70), SCALE_Y(32),
                  euclideanState.tripletMode ? "TRIP" : "2/4",
                  euclideanState.tripletMode ? THEME_ACCENT : THEME_SURFACE, false, 2);

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("BPM " + String(euclideanState.bpm), DISPLAY_WIDTH - SCALE_X(80), controlY - SCALE_Y(20), 2);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString(euclideanState.tripletMode ? "Triplet feel" : "16th feel",
                 DISPLAY_WIDTH - SCALE_X(80), controlY - SCALE_Y(32), 1);
}

void updateEuclideanSequencer() {
  if (!euclideanState.isPlaying) {
    return;
  }

  unsigned long now = millis();
  unsigned long stepDuration = euclideanState.tripletMode
                                  ? ((60000UL / euclideanState.bpm) / 6)
                                  : ((60000UL / euclideanState.bpm) / 4);
  if (now - euclideanState.lastStepTime < stepDuration) {
    return;
  }

  euclideanState.lastStepTime = now;
  for (int voiceIdx = 0; voiceIdx < EUCLIDEAN_VOICE_COUNT; ++voiceIdx) {
    EuclideanVoice &voice = euclideanState.voices[voiceIdx];
    if (voice.steps == 0) {
      continue;
    }
    int stepIndex = euclideanState.currentStep % voice.steps;
    if (voice.pattern[stepIndex]) {
      sendMIDI(0x90, voice.midiNote, 110);
      sendMIDI(0x80, voice.midiNote, 0);
    }
  }
  euclideanState.currentStep = (euclideanState.currentStep + 1) % EUCLIDEAN_MAX_STEPS;
}

void handleEuclideanMode() {
  updateEuclideanSequencer();

  if (!touch.justPressed) {
    return;
  }
  if (isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  int controlY = DISPLAY_HEIGHT - SCALE_Y(60);
  if (isButtonPressed(MARGIN_SMALL, controlY, SCALE_X(64), SCALE_Y(32))) {
    euclideanState.isPlaying = !euclideanState.isPlaying;
    if (euclideanState.isPlaying) {
      euclideanState.lastStepTime = millis();
    }
    drawEuclideanMode();
    return;
  }

  if (isButtonPressed(MARGIN_SMALL + SCALE_X(70), controlY, SCALE_X(45), SCALE_Y(32))) {
    euclideanState.bpm = std::max(60U, (unsigned int)euclideanState.bpm - 5);
    drawEuclideanMode();
    return;
  }

  if (isButtonPressed(MARGIN_SMALL + SCALE_X(120), controlY, SCALE_X(45), SCALE_Y(32))) {
    euclideanState.bpm = std::min(240U, (unsigned int)euclideanState.bpm + 5);
    drawEuclideanMode();
    return;
  }

  if (isButtonPressed(DISPLAY_WIDTH - SCALE_X(80), controlY, SCALE_X(70), SCALE_Y(32))) {
    euclideanState.tripletMode = !euclideanState.tripletMode;
    drawEuclideanMode();
    return;
  }
}
