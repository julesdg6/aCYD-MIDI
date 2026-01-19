#include "module_grids_mode.h"
#include "clock_manager.h"

#include <algorithm>
#include <pgmspace.h>

GridsState grids;

struct GridsLayout {
  int padX;
  int padY;
  int padSize;
  int controlX;
  int controlWidth;
  int buttonHeight;
  int buttonSpacing;
  int sliderY;
  int sliderW;
  int sliderH;
  int sliderSpacing;
  int sliderPositions[3];
};

static SequencerSyncState gridsSync;
static inline bool gridsIsRequested() {
  return gridsSync.playing || gridsSync.startPending;
}

static GridsLayout calculateGridsLayout() {
  GridsLayout layout;
  const int desiredControlWidth = SCALE_X(90);
  const int padMaxWidth = DISPLAY_WIDTH - desiredControlWidth - MARGIN_SMALL * 3;
  layout.padSize = std::min(SCALE_X(140), std::max(padMaxWidth, SCALE_X(110)));
  layout.padX = MARGIN_SMALL;
  layout.padY = HEADER_HEIGHT + SCALE_Y(8);
  layout.buttonHeight = SCALE_Y(38);
  layout.buttonSpacing = SCALE_Y(10);
  layout.sliderW = SCALE_X(70);
  layout.sliderSpacing = SCALE_X(8);
  layout.sliderH = SCALE_Y(18);

  const int sliderPadGap = SCALE_Y(12);
  const int sliderBottomMargin = SCALE_Y(20);
  int padVerticalLimit =
      DISPLAY_HEIGHT - layout.padY - sliderPadGap - layout.sliderH - sliderBottomMargin;
  padVerticalLimit = std::max(padVerticalLimit, 0);
  int padSizeLimit = std::min(layout.padSize, padVerticalLimit);
  if (padVerticalLimit >= SCALE_X(110)) {
    layout.padSize = std::max(padSizeLimit, SCALE_X(110));
  } else {
    layout.padSize = padSizeLimit;
  }

  layout.controlX = layout.padX + layout.padSize + MARGIN_SMALL;
  layout.controlWidth = DISPLAY_WIDTH - layout.controlX - MARGIN_SMALL;

  layout.sliderY = layout.padY + layout.padSize + sliderPadGap;
  int sliderDefaultY = DISPLAY_HEIGHT - SCALE_Y(70);
  layout.sliderY = std::max(layout.sliderY, sliderDefaultY);
  int sliderMaxY = DISPLAY_HEIGHT - layout.sliderH - sliderBottomMargin;
  layout.sliderY = std::min(layout.sliderY, sliderMaxY);

  int sliderTotalWidth = 3 * layout.sliderW + 2 * layout.sliderSpacing;
  int sliderStartX = (DISPLAY_WIDTH - sliderTotalWidth) / 2;
  for (int i = 0; i < 3; ++i) {
    layout.sliderPositions[i] = sliderStartX + i * (layout.sliderW + layout.sliderSpacing);
  }
  return layout;
}

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
  gridsSync.tryStartIfReady(!instantStartMode);
  grids.playing = gridsSync.playing;
  if (!grids.playing) {
    return;
  }

  uint32_t readySteps = gridsSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);
  if (readySteps == 0) {
    return;
  }

  for (uint32_t i = 0; i < readySteps; ++i) {
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
  }
  requestRedraw();  // Request redraw to show step progress
}

void drawGridsMode() {
  grids.playing = gridsSync.playing;
  tft.fillScreen(THEME_BG);
  drawHeader("GRIDS", "", 3);

  const GridsLayout layout = calculateGridsLayout();
  const int padX = layout.padX;
  const int padY = layout.padY;
  const int padSize = layout.padSize;
  const int controlX = layout.controlX;
  const int controlW = layout.controlWidth;
  const int buttonH = layout.buttonHeight;
  const int buttonSpacing = layout.buttonSpacing;
  const int sliderY = layout.sliderY;
  const int sliderH = layout.sliderH;
  const int sliderW = layout.sliderW;

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
    int stepIndicatorY = sliderY - SCALE_Y(20);
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

  const uint8_t densities[3] = {grids.kickDensity, grids.snareDensity, grids.hatDensity};
  const uint16_t sliderColor[3] = {THEME_ERROR, THEME_WARNING, THEME_ACCENT};
  const char *sliderLabels[3] = {"K", "S", "H"};

  for (int i = 0; i < 3; ++i) {
    int x = layout.sliderPositions[i];
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString(sliderLabels[i], x - SCALE_X(15), sliderY, 2);
    tft.drawRect(x, sliderY, sliderW, sliderH, THEME_TEXT);
    int fillWidth = (densities[i] * sliderW) / 255;
    if (fillWidth > 0) {
      tft.fillRect(x + 1, sliderY + 1, fillWidth, sliderH - 2, sliderColor[i]);
    }
  }

  int buttonY = padY;
  drawRoundButton(controlX, buttonY, controlW, buttonH, grids.playing ? "STOP" : "PLAY", THEME_PRIMARY);
  buttonY += buttonH + buttonSpacing;
  drawRoundButton(controlX, buttonY, controlW, buttonH, "BPM-", THEME_SECONDARY);
  buttonY += buttonH + buttonSpacing;
  drawRoundButton(controlX, buttonY, controlW, buttonH, "BPM+", THEME_SECONDARY);
  buttonY += buttonH + buttonSpacing;
  drawRoundButton(controlX, buttonY, controlW, buttonH, "RNDM", THEME_ACCENT);
  buttonY += buttonH + buttonSpacing;

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("BPM: " + String(sharedBPM), controlX, buttonY, 2);
}

void initializeGridsMode() {
  grids.step = 0;
  grids.playing = false;
  grids.bpm = static_cast<float>(sharedBPM);
  gridsSync.reset();
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
    if (gridsIsRequested()) {
      gridsSync.stopPlayback();
    }
    exitToMenu();
    return;
  }

  if (!touch.justPressed) {
    return;
  }

  const GridsLayout layout = calculateGridsLayout();
  const int padX = layout.padX;
  const int padY = layout.padY;
  const int padSize = layout.padSize;

  if (touch.x >= padX && touch.x < padX + padSize &&
      touch.y >= padY && touch.y < padY + padSize) {
    grids.patternX = ((touch.x - padX) * 255) / padSize;
    grids.patternY = ((touch.y - padY) * 255) / padSize;
    regenerateGridsPattern();
    requestRedraw();
    return;
  }

  const int controlX = layout.controlX;
  const int controlW = layout.controlWidth;
  const int buttonH = layout.buttonHeight;
  const int buttonSpacing = layout.buttonSpacing;
  int buttonY = layout.padY;

  bool playPressed = isButtonPressed(controlX, buttonY, controlW, buttonH);
  buttonY += buttonH + buttonSpacing;
  bool bpmDownPressed = isButtonPressed(controlX, buttonY, controlW, buttonH);
  buttonY += buttonH + buttonSpacing;
  bool bpmUpPressed = isButtonPressed(controlX, buttonY, controlW, buttonH);
  buttonY += buttonH + buttonSpacing;
  bool randomPressed = isButtonPressed(controlX, buttonY, controlW, buttonH);
  buttonY += buttonH + buttonSpacing;

  if (playPressed) {
    if (gridsIsRequested()) {
      gridsSync.stopPlayback();
    } else {
      grids.step = 0;
      gridsSync.requestStart();
    }
    requestRedraw();
    return;
  }
  if (bpmDownPressed) {
    int target = std::max<int>(GRIDS_MIN_BPM, static_cast<int>(sharedBPM) - 5);
    sharedBPM = static_cast<uint16_t>(target);
    grids.bpm = static_cast<float>(sharedBPM);
    requestRedraw();
    return;
  }
  if (bpmUpPressed) {
    int target = std::min<int>(GRIDS_MAX_BPM, static_cast<int>(sharedBPM) + 5);
    sharedBPM = static_cast<uint16_t>(target);
    grids.bpm = static_cast<float>(sharedBPM);
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

  if (touch.y >= layout.sliderY && touch.y < layout.sliderY + layout.sliderH) {
    for (int i = 0; i < 3; ++i) {
      int x = layout.sliderPositions[i];
      if (touch.x >= x && touch.x < x + layout.sliderW) {
        uint8_t value = ((touch.x - x) * 255) / layout.sliderW;
        if (i == 0) grids.kickDensity = value;
        if (i == 1) grids.snareDensity = value;
        if (i == 2) grids.hatDensity = value;
        requestRedraw();
        return;
      }
    }
  }
}
