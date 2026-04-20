#include "module_zynthian_pad_mode.h"

#include "midi_utils.h"
#include "ui_elements.h"

#include <Arduino.h>

namespace {

static constexpr int kCols = 4;
static constexpr int kRows = 5;
static constexpr uint8_t kActionChannel = 15;  // MIDI channel 16
static constexpr uint8_t kToggleRecordStateCC = 119;

struct PadButton {
  const char *line1;
  const char *line2;
  ZynthianPadAction action;
};

static const PadButton kPadButtons[kRows][kCols] = {
    {{"OPT", "ADMIN", ZynthianPadAction::SCREEN_ADMIN},
     {"MIX", "LEVEL", ZynthianPadAction::SCREEN_AUDIO_MIXER},
     {"CTRL", "PRESET", ZynthianPadAction::PRESET},
     {"ZS3", "SHOT", ZynthianPadAction::SCREEN_SNAPSHOT}},
    {{"ALT", "", ZynthianPadAction::ALT_LAYER},
     {"LEARN", "", ZynthianPadAction::ENTER_MIDI_LEARN},
     {"PAD", "STEP", ZynthianPadAction::SCREEN_ZYNPAD},
     {"F1", "", ZynthianPadAction::ZYNSWITCH_1}},
    {{"REC", "", ZynthianPadAction::TOGGLE_MIDI_RECORD},
     {"MREC", "", ZynthianPadAction::SCREEN_MIDI_RECORDER},
     {"PLAY", "", ZynthianPadAction::TOGGLE_MIDI_PLAY},
     {"F2", "", ZynthianPadAction::ZYNSWITCH_2}},
    {{"BACK", "NO", ZynthianPadAction::BACK_NO},
     {"UP", "", ZynthianPadAction::ARROW_UP},
     {"SEL", "YES", ZynthianPadAction::MENU},
     {"F3", "", ZynthianPadAction::ZYNSWITCH_3}},
    {{"LEFT", "", ZynthianPadAction::ARROW_LEFT},
     {"DOWN", "", ZynthianPadAction::ARROW_DOWN},
     {"RIGHT", "", ZynthianPadAction::ARROW_RIGHT},
     {"F4", "", ZynthianPadAction::SCREEN_ALSA_MIXER}},
};

struct PadState {
  bool transportRunning = false;
  bool recordArmed = false;
  int pressedRow = -1;
  int pressedCol = -1;
  uint32_t pressedUntilMs = 0;
} gPadState;

static uint8_t actionKeycode(ZynthianPadAction action) {
  switch (action) {
    case ZynthianPadAction::PRESET:
      return 64;
    case ZynthianPadAction::SCREEN_AUDIO_MIXER:
      return 9;
    case ZynthianPadAction::SCREEN_ADMIN:
      return 92;
    case ZynthianPadAction::SCREEN_ZYNPAD:
      return 106;
    case ZynthianPadAction::SCREEN_SNAPSHOT:
      return 79;
    case ZynthianPadAction::SCREEN_MIDI_RECORDER:
      return 81;
    case ZynthianPadAction::TOGGLE_MIDI_PLAY:
      return 86;
    case ZynthianPadAction::TOGGLE_MIDI_RECORD:
      return 82;
    case ZynthianPadAction::MENU:
      return 91;
    case ZynthianPadAction::ENTER_MIDI_LEARN:
      return 89;
    case ZynthianPadAction::ARROW_UP:
      return 80;
    case ZynthianPadAction::ARROW_DOWN:
      return 88;
    case ZynthianPadAction::ARROW_LEFT:
      return 83;
    case ZynthianPadAction::ARROW_RIGHT:
      return 85;
    case ZynthianPadAction::ZYNSWITCH_1:
      return 22;
    case ZynthianPadAction::ZYNSWITCH_2:
      return 95;
    case ZynthianPadAction::ZYNSWITCH_3:
      return 104;
    case ZynthianPadAction::ALT_LAYER:
      return 84;
    case ZynthianPadAction::BACK_NO:
      return 94;
    case ZynthianPadAction::SCREEN_ALSA_MIXER:
      return 93;
    case ZynthianPadAction::NONE:
    default:
      return 0;
  }
}

static void pulseActionMidi(ZynthianPadAction action) {
  uint8_t keycode = actionKeycode(action);
  if (keycode == 0) {
    return;
  }
  sendMIDI(static_cast<uint8_t>(0x90 | kActionChannel), keycode, 127);
  sendMIDI(static_cast<uint8_t>(0x80 | kActionChannel), keycode, 0);
}

static void performAction(ZynthianPadAction action, bool emitMidi) {
  if (action == ZynthianPadAction::NONE) {
    return;
  }

  if (emitMidi) {
    pulseActionMidi(action);
  }

  if (action == ZynthianPadAction::BACK_NO) {
    exitToMenu();
    return;
  }

  switch (action) {
    case ZynthianPadAction::TOGGLE_MIDI_PLAY:
      gPadState.transportRunning = !gPadState.transportRunning;
      if (gPadState.transportRunning) {
        sendMIDIStart();
      } else {
        sendMIDIStop();
      }
      requestRedraw();
      break;
    case ZynthianPadAction::TOGGLE_MIDI_RECORD:
      gPadState.recordArmed = !gPadState.recordArmed;
      sendMIDI(static_cast<uint8_t>(0xB0 | kActionChannel), kToggleRecordStateCC,
               gPadState.recordArmed ? 127 : 0);
      requestRedraw();
      break;
    default:
      break;
  }
}

static bool findButtonForAction(ZynthianPadAction action, int &rowOut, int &colOut) {
  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      if (kPadButtons[row][col].action == action) {
        rowOut = row;
        colOut = col;
        return true;
      }
    }
  }
  return false;
}

static ZynthianPadAction actionFromKeycode(uint8_t keycode) {
  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      if (actionKeycode(kPadButtons[row][col].action) == keycode) {
        return kPadButtons[row][col].action;
      }
    }
  }
  return ZynthianPadAction::NONE;
}

static void drawPadButton(int x, int y, int w, int h, const PadButton &button, bool pressed) {
  uint16_t bg = pressed ? THEME_PRIMARY : THEME_BG;
  uint16_t border = pressed ? THEME_PRIMARY : THEME_TEXT_DIM;
  uint16_t text = pressed ? THEME_BG : THEME_TEXT;

  if (button.action == ZynthianPadAction::TOGGLE_MIDI_RECORD && gPadState.recordArmed) {
    bg = pressed ? THEME_ERROR : THEME_SURFACE;
    border = THEME_ERROR;
    text = THEME_TEXT;
  } else if (button.action == ZynthianPadAction::TOGGLE_MIDI_PLAY && gPadState.transportRunning) {
    bg = pressed ? THEME_SUCCESS : THEME_SURFACE;
    border = THEME_SUCCESS;
    text = THEME_TEXT;
  }

  tft.fillRoundRect(x, y, w, h, SCALE_X(6), bg);
  tft.drawRoundRect(x, y, w, h, SCALE_X(6), border);

  tft.setTextColor(text, bg);
  if (button.line2 && button.line2[0] != '\0') {
    tft.drawCentreString(button.line1, x + w / 2, y + h / 2 - SCALE_Y(11), 2);
    tft.drawCentreString(button.line2, x + w / 2, y + h / 2 + SCALE_Y(4), 2);
  } else {
    tft.drawCentreString(button.line1, x + w / 2, y + h / 2 - SCALE_Y(4), 2);
  }
}

}  // namespace

void initializeZynthianPadMode() {
  gPadState.transportRunning = false;
  gPadState.recordArmed = false;
  gPadState.pressedRow = -1;
  gPadState.pressedCol = -1;
  gPadState.pressedUntilMs = 0;
  drawZynthianPadMode();
}

void drawZynthianPadMode() {
  drawHeader("KPNUM", "Zynthian v5");
  tft.fillRect(0, HEADER_HEIGHT + 1, DISPLAY_WIDTH, DISPLAY_HEIGHT - HEADER_HEIGHT - 1, THEME_BG);

  const int areaX = MARGIN_SMALL;
  const int areaY = HEADER_HEIGHT + SCALE_Y(8);
  const int areaW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  const int areaH = DISPLAY_HEIGHT - areaY - MARGIN_SMALL;
  const int gapX = SCALE_X(6);
  const int gapY = SCALE_Y(4);
  const int buttonW = (areaW - (kCols - 1) * gapX) / kCols;
  const int buttonH = (areaH - (kRows - 1) * gapY) / kRows;

  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      int x = areaX + col * (buttonW + gapX);
      int y = areaY + row * (buttonH + gapY);
      bool pressed = (row == gPadState.pressedRow && col == gPadState.pressedCol);
      drawPadButton(x, y, buttonW, buttonH, kPadButtons[row][col], pressed);
    }
  }
}

void handleZynthianPadMode() {
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  if (gPadState.pressedRow >= 0 && millis() > gPadState.pressedUntilMs) {
    gPadState.pressedRow = -1;
    gPadState.pressedCol = -1;
    requestRedraw();
  }

  if (!touch.justPressed) {
    return;
  }

  const int areaX = MARGIN_SMALL;
  const int areaY = HEADER_HEIGHT + SCALE_Y(8);
  const int areaW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  const int areaH = DISPLAY_HEIGHT - areaY - MARGIN_SMALL;
  const int gapX = SCALE_X(6);
  const int gapY = SCALE_Y(4);
  const int buttonW = (areaW - (kCols - 1) * gapX) / kCols;
  const int buttonH = (areaH - (kRows - 1) * gapY) / kRows;

  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      int x = areaX + col * (buttonW + gapX);
      int y = areaY + row * (buttonH + gapY);
      if (isButtonPressed(x, y, buttonW, buttonH)) {
        gPadState.pressedRow = row;
        gPadState.pressedCol = col;
        gPadState.pressedUntilMs = millis() + 120;
        performAction(kPadButtons[row][col].action, true);
        requestRedraw();
        return;
      }
    }
  }
}

bool triggerZynthianPadActionFromMidi(uint8_t status, uint8_t data1, uint8_t data2) {
  if (currentMode != ZYNTHIAN_PAD) {
    return false;
  }

  uint8_t messageType = status & 0xF0;
  uint8_t channel = status & 0x0F;
  if (channel != kActionChannel) {
    return false;
  }

  bool trigger = false;
  if (messageType == 0x90) {
    trigger = (data2 > 0);
  } else if (messageType == 0xB0) {
    trigger = (data2 > 0);
  } else {
    return false;
  }

  if (!trigger) {
    return false;
  }

  ZynthianPadAction action = actionFromKeycode(data1);
  if (action == ZynthianPadAction::NONE) {
    return false;
  }

  int row = -1;
  int col = -1;
  if (findButtonForAction(action, row, col)) {
    gPadState.pressedRow = row;
    gPadState.pressedCol = col;
    gPadState.pressedUntilMs = millis() + 120;
  }

  performAction(action, false);
  requestRedraw();
  return true;
}
