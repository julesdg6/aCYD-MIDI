#include "module_raga_mode.h"

const char *const kRagaNames[RAGA_COUNT] = {
    "Bhairavi",
    "Lalit",
    "Bhupali",
    "Todi",
    "Madhuvanti",
    "Meghmalhar",
    "Yaman",
    "Malkauns",
};

RagaState raga;

static uint8_t getRagaMidiNote() {
  return raga.rootNote + static_cast<uint8_t>(raga.currentRaga);
}

void initializeRagaMode() {
  raga.currentRaga = RAGA_BHAIRAVI;
  raga.rootNote = 60;
  raga.playing = false;
  raga.droneEnabled = false;
  drawRagaMode();
}

void drawRagaMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("RAGA", "Indian Classical Scales");

  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Root: " + getNoteNameFromMIDI(raga.rootNote), MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(10), 1);
  tft.drawString(raga.playing ? "Playing" : "Idle", DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), HEADER_HEIGHT + SCALE_Y(10), 1);

  const int rows = (RAGA_COUNT + RAGA_COLUMNS - 1) / RAGA_COLUMNS;
  const int buttonW = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - (RAGA_COLUMNS - 1) * SCALE_X(8)) / RAGA_COLUMNS;
  const int buttonH = SCALE_Y(28);
  const int startY = HEADER_HEIGHT + SCALE_Y(30);

  for (int index = 0; index < RAGA_COUNT; ++index) {
    int row = index / RAGA_COLUMNS;
    int col = index % RAGA_COLUMNS;
    int x = MARGIN_SMALL + col * (buttonW + SCALE_X(8));
    int y = startY + row * (buttonH + SCALE_Y(8));
    bool selected = (index == static_cast<int>(raga.currentRaga));
    uint16_t fillColor = selected ? THEME_ACCENT : THEME_SURFACE;
    uint16_t textColor = selected ? THEME_BG : THEME_TEXT;

    tft.fillRoundRect(x, y, buttonW, buttonH, 6, fillColor);
    tft.drawRoundRect(x, y, buttonW, buttonH, 6, selected ? THEME_PRIMARY : THEME_TEXT_DIM);
    tft.setTextColor(textColor, fillColor);
    tft.drawCentreString(kRagaNames[index], x + buttonW / 2, y + buttonH / 2 - 6, 1);
  }

  const int controlY = DISPLAY_HEIGHT - SCALE_Y(55);
  drawRoundButton(MARGIN_SMALL, controlY, BTN_MEDIUM_W, BTN_MEDIUM_H,
                  raga.playing ? "STOP" : "PLAY",
                  raga.playing ? THEME_ERROR : THEME_SUCCESS);
  drawRoundButton(MARGIN_SMALL + BTN_MEDIUM_W + SCALE_X(12), controlY, BTN_MEDIUM_W, BTN_MEDIUM_H,
                  raga.droneEnabled ? "DRONE ON" : "DRONE",
                  raga.droneEnabled ? THEME_SUCCESS : THEME_SECONDARY);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Scale: " + String(kRagaNames[static_cast<int>(raga.currentRaga)]),
                 MARGIN_SMALL, controlY - SCALE_Y(20), 1);
}

void toggleRagaPlayback() {
  raga.playing = !raga.playing;
  uint8_t note = getRagaMidiNote();
  if (raga.playing) {
    sendMIDI(0x90, note, 120);
  } else {
    sendMIDI(0x80, note, 0);
  }
  requestRedraw();
}

void handleRagaMode() {
  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  const int controlY = DISPLAY_HEIGHT - SCALE_Y(55);
  if (touch.justPressed &&
      isButtonPressed(MARGIN_SMALL, controlY, BTN_MEDIUM_W, BTN_MEDIUM_H)) {
    toggleRagaPlayback();
    return;
  }

  if (touch.justPressed &&
      isButtonPressed(MARGIN_SMALL + BTN_MEDIUM_W + SCALE_X(12), controlY, BTN_MEDIUM_W, BTN_MEDIUM_H)) {
    raga.droneEnabled = !raga.droneEnabled;
    requestRedraw();
    return;
  }

  if (!touch.justPressed) {
    return;
  }

  const int buttonW = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - (RAGA_COLUMNS - 1) * SCALE_X(8)) / RAGA_COLUMNS;
  const int buttonH = SCALE_Y(28);
  const int startY = HEADER_HEIGHT + SCALE_Y(30);

  for (int index = 0; index < RAGA_COUNT; ++index) {
    int row = index / RAGA_COLUMNS;
    int col = index % RAGA_COLUMNS;
    int x = MARGIN_SMALL + col * (buttonW + SCALE_X(8));
    int y = startY + row * (buttonH + SCALE_Y(8));

    if (isButtonPressed(x, y, buttonW, buttonH)) {
      if (raga.currentRaga != static_cast<RagaType>(index)) {
        raga.currentRaga = static_cast<RagaType>(index);
        requestRedraw();
      }
      return;
    }
  }
}
