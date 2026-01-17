#include "module_grids_mode.h"
#include <pgmspace.h>

GridsState grids;

static const uint8_t PROGMEM PATTERN_MAP[4][3][GRIDS_STEPS] = {
  {
    {255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0},
    {0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0},
    {150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80, 150, 80}
  },
  {
    {255, 0, 0, 180, 0, 0, 255, 0, 0, 180, 0, 0, 255, 0, 0, 0},
    {0, 0, 255, 0, 0, 0, 0, 180, 0, 0, 255, 0, 0, 0, 180, 0},
    {200, 120, 80, 150, 200, 120, 80, 150, 200, 120, 80, 150, 200, 120, 80, 150}
  },
  {
    {255, 0, 0, 0, 0, 0, 200, 0, 255, 0, 0, 0, 180, 0, 0, 0},
    {0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0},
    {180, 120, 0, 120, 180, 120, 0, 120, 180, 120, 0, 120, 180, 120, 0, 120}
  },
  {
    {255, 0, 0, 0, 0, 0, 220, 0, 180, 0, 0, 0, 0, 0, 200, 0},
    {0, 0, 0, 0, 255, 0, 0, 120, 0, 0, 0, 0, 255, 0, 0, 80},
    {150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100, 150, 100}
  }
};

static inline uint8_t lerp(uint8_t a, uint8_t b, uint8_t amount) {
  return a + (((b - a) * amount) >> 8);
}

static inline uint8_t bilinearInterpolate(uint8_t v00, uint8_t v10, uint8_t v01, uint8_t v11,
                                           uint8_t x, uint8_t y) {
  uint8_t v0 = lerp(v00, v10, x);
  uint8_t v1 = lerp(v01, v11, x);
  return lerp(v0, v1, y);
}

void regenerateGridsPattern() {
  for (int step = 0; step < GRIDS_STEPS; ++step) {
    uint8_t kick00 = pgm_read_byte(&PATTERN_MAP[0][0][step]);
    uint8_t kick10 = pgm_read_byte(&PATTERN_MAP[1][0][step]);
    uint8_t kick01 = pgm_read_byte(&PATTERN_MAP[2][0][step]);
    uint8_t kick11 = pgm_read_byte(&PATTERN_MAP[3][0][step]);
    grids.kickPattern[step] =
        bilinearInterpolate(kick00, kick10, kick01, kick11, grids.patternX, grids.patternY);

    uint8_t snare00 = pgm_read_byte(&PATTERN_MAP[0][1][step]);
    uint8_t snare10 = pgm_read_byte(&PATTERN_MAP[1][1][step]);
    uint8_t snare01 = pgm_read_byte(&PATTERN_MAP[2][1][step]);
    uint8_t snare11 = pgm_read_byte(&PATTERN_MAP[3][1][step]);
    grids.snarePattern[step] =
        bilinearInterpolate(snare00, snare10, snare01, snare11, grids.patternX, grids.patternY);

    uint8_t hat00 = pgm_read_byte(&PATTERN_MAP[0][2][step]);
    uint8_t hat10 = pgm_read_byte(&PATTERN_MAP[1][2][step]);
    uint8_t hat01 = pgm_read_byte(&PATTERN_MAP[2][2][step]);
    uint8_t hat11 = pgm_read_byte(&PATTERN_MAP[3][2][step]);
    grids.hatPattern[step] =
        bilinearInterpolate(hat00, hat10, hat01, hat11, grids.patternX, grids.patternY);
  }
}

static inline void triggerDrum(uint8_t note, bool trigger, uint8_t velocity) {
  if (!trigger) {
    return;
  }
  sendMIDI(0x90, note, velocity);
  sendMIDI(0x80, note, 0);
}

void updateGridsPlayback() {
  if (!grids.playing) {
    return;
  }
  unsigned long now = millis();
  grids.stepInterval = (60000.0 / grids.bpm) / 4.0;
  if (now - grids.lastStepTime < grids.stepInterval) {
    return;
  }
  grids.lastStepTime = now;

  bool kickTrigger = grids.kickPattern[grids.step] >= (255 - grids.kickDensity);
  bool snareTrigger = grids.snarePattern[grids.step] >= (255 - grids.snareDensity);
  bool hatTrigger = grids.hatPattern[grids.step] >= (255 - grids.hatDensity);

  uint8_t kickVel = grids.kickPattern[grids.step] >= grids.accentThreshold ? 127 : 100;
  uint8_t snareVel = grids.snarePattern[grids.step] >= grids.accentThreshold ? 127 : 100;
  uint8_t hatVel = grids.hatPattern[grids.step] >= grids.accentThreshold ? 127 : 90;

  triggerDrum(grids.kickNote, kickTrigger, kickVel);
  triggerDrum(grids.snareNote, snareTrigger, snareVel);
  triggerDrum(grids.hatNote, hatTrigger, hatVel);

  grids.step = (grids.step + 1) % GRIDS_STEPS;
  requestRedraw();  // Request redraw to show step progress
}

void drawGridsMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("GRIDS", "", 3);

  const int padSize = SCALE_X(140);
  const int padX = (DISPLAY_WIDTH - padSize) / 2;
  const int padY = HEADER_HEIGHT + SCALE_Y(8);

  tft.fillRect(padX, padY, padSize, padSize, THEME_SURFACE);
  tft.drawRect(padX, padY, padSize, padSize, THEME_TEXT);
  tft.drawFastVLine(padX + padSize / 2, padY, padSize, THEME_TEXT_DIM);
  tft.drawFastHLine(padX, padY + padSize / 2, padSize, THEME_TEXT_DIM);

  int markerX = padX + (grids.patternX * padSize) / 255;
  int markerY = padY + (grids.patternY * padSize) / 255;
  tft.fillCircle(markerX, markerY, SCALE_X(5), THEME_TEXT);
  tft.drawCircle(markerX, markerY, SCALE_X(6), THEME_TEXT);
  
  // Show current step position if playing
  if (grids.playing) {
    int stepIndicatorY = padY + padSize + SCALE_Y(5);
    int stepW = SCALE_X(18);
    int stepSpacing = SCALE_X(1);
    int stepStartX = padX + (padSize - (GRIDS_STEPS * (stepW + stepSpacing) - stepSpacing)) / 2;
    
    for (int i = 0; i < GRIDS_STEPS; i++) {
      int x = stepStartX + i * (stepW + stepSpacing);
      bool isCurrent = (i == grids.step);
      uint16_t color = isCurrent ? THEME_ACCENT : THEME_SURFACE;
      tft.fillRect(x, stepIndicatorY, stepW, SCALE_Y(4), color);
    }
  }

  const int sliderY = padY + padSize + SCALE_Y(18);
  const int sliderW = SCALE_X(80);
  const int sliderH = SCALE_Y(18);
  const int sliderSpacing = SCALE_X(8);
  const int sliderStartX = (DISPLAY_WIDTH - (3 * sliderW + 2 * sliderSpacing)) / 2;
  const int sliderPositions[3] = {
      sliderStartX,
      sliderStartX + sliderW + sliderSpacing,
      sliderStartX + 2 * (sliderW + sliderSpacing),
  };

  const uint8_t densities[3] = {grids.kickDensity, grids.snareDensity, grids.hatDensity};
  const uint16_t sliderColor[3] = {THEME_ERROR, THEME_WARNING, THEME_ACCENT};
  const char *sliderLabels[3] = {"K", "S", "H"};

  for (int i = 0; i < 3; ++i) {
    int x = sliderPositions[i];
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString(sliderLabels[i], x - SCALE_X(15), sliderY, 2);
    tft.drawRect(x, sliderY, sliderW, sliderH, THEME_TEXT);
    int fillWidth = (densities[i] * sliderW) / 255;
    if (fillWidth > 0) {
      tft.fillRect(x + 1, sliderY + 1, fillWidth, sliderH - 2, sliderColor[i]);
    }
  }

  const int btnY = DISPLAY_HEIGHT - SCALE_Y(50);
  const int btnH = SCALE_Y(40);
  const int btnSpacing = SCALE_X(8);
  const int btnW = (DISPLAY_WIDTH - (5 * btnSpacing)) / 4;

  drawRoundButton(MARGIN_SMALL, btnY, btnW, btnH, grids.playing ? "STOP" : "PLAY", THEME_PRIMARY);
  drawRoundButton(MARGIN_SMALL + (btnW + btnSpacing), btnY, btnW, btnH, "BPM-", THEME_SECONDARY);
  drawRoundButton(MARGIN_SMALL + 2 * (btnW + btnSpacing), btnY, btnW, btnH, "BPM+", THEME_SECONDARY);
  drawRoundButton(MARGIN_SMALL + 3 * (btnW + btnSpacing), btnY, btnW, btnH, "RNDM", THEME_ACCENT);

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("BPM: " + String((int)grids.bpm), MARGIN_SMALL, btnY - SCALE_Y(15), 2);
}

void initializeGridsMode() {
  grids.step = 0;
  grids.playing = false;
  grids.lastStepTime = 0;
  grids.stepInterval = (60000.0 / grids.bpm) / 4.0;
  grids.bpm = 120.0f;
  grids.patternX = 128;
  grids.patternY = 128;
  grids.kickDensity = 200;
  grids.snareDensity = 150;
  grids.hatDensity = 180;
  grids.kickNote = 36;
  grids.snareNote = 38;
  grids.hatNote = 42;
  grids.swing = 0;
  grids.accentThreshold = 200;
  regenerateGridsPattern();
  drawGridsMode();
}

void handleGridsMode() {
  updateGridsPlayback();

  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    if (grids.playing) {
      grids.playing = false;
    }
    exitToMenu();
    return;
  }

  if (!touch.justPressed) {
    return;
  }

  const int padSize = SCALE_X(140);
  const int padX = (DISPLAY_WIDTH - padSize) / 2;
  const int padY = HEADER_HEIGHT + SCALE_Y(8);

  if (touch.x >= padX && touch.x < padX + padSize &&
      touch.y >= padY && touch.y < padY + padSize) {
    grids.patternX = ((touch.x - padX) * 255) / padSize;
    grids.patternY = ((touch.y - padY) * 255) / padSize;
    regenerateGridsPattern();
    requestRedraw();
    return;
  }

  const int btnY = DISPLAY_HEIGHT - SCALE_Y(50);
  const int btnH = SCALE_Y(40);
  const int btnSpacing = SCALE_X(8);
  const int btnW = (DISPLAY_WIDTH - (5 * btnSpacing)) / 4;

  bool playPressed = isButtonPressed(MARGIN_SMALL, btnY, btnW, btnH);
  bool bpmDownPressed = isButtonPressed(MARGIN_SMALL + (btnW + btnSpacing), btnY, btnW, btnH);
  bool bpmUpPressed = isButtonPressed(MARGIN_SMALL + 2 * (btnW + btnSpacing), btnY, btnW, btnH);
  bool randomPressed = isButtonPressed(MARGIN_SMALL + 3 * (btnW + btnSpacing), btnY, btnW, btnH);

  if (playPressed) {
    grids.playing = !grids.playing;
    if (grids.playing) {
      grids.step = 0;
      grids.lastStepTime = millis();
    }
    requestRedraw();
    return;
  }
  if (bpmDownPressed) {
    grids.bpm = constrain(grids.bpm - 5, GRIDS_MIN_BPM, GRIDS_MAX_BPM);
    requestRedraw();
    return;
  }
  if (bpmUpPressed) {
    grids.bpm = constrain(grids.bpm + 5, GRIDS_MIN_BPM, GRIDS_MAX_BPM);
    requestRedraw();
    return;
  }
  if (randomPressed) {
    grids.patternX = random(256);
    grids.patternY = random(256);
    regenerateGridsPattern();
    requestRedraw();
    return;
  }

  const int sliderY = padY + padSize + SCALE_Y(18);
  const int sliderW = SCALE_X(80);
  const int sliderH = SCALE_Y(18);
  const int sliderSpacing = SCALE_X(8);
  const int sliderStartX = (DISPLAY_WIDTH - (3 * sliderW + 2 * sliderSpacing)) / 2;
  const int sliderPositions[3] = {
      sliderStartX,
      sliderStartX + sliderW + sliderSpacing,
      sliderStartX + 2 * (sliderW + sliderSpacing),
  };

  if (touch.y >= sliderY && touch.y < sliderY + sliderH) {
    for (int i = 0; i < 3; ++i) {
      int x = sliderPositions[i];
      if (touch.x >= x && touch.x < x + sliderW) {
        uint8_t value = ((touch.x - x) * 255) / sliderW;
        if (i == 0) grids.kickDensity = value;
        if (i == 1) grids.snareDensity = value;
        if (i == 2) grids.hatDensity = value;
        requestRedraw();
        return;
      }
    }
  }
}
