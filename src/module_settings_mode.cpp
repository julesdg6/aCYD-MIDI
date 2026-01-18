#include "module_settings_mode.h"
#include "wifi_manager.h"
#include "ui_elements.h"

#include <algorithm>

namespace {
static inline int settingsRowSpacing() { return SCALE_Y(12); }
static inline int bpmRowHeight() { return SCALE_Y(56); }
static inline int compactRowHeight() { return SCALE_Y(44); }
static inline int statusRowHeight() { return SCALE_Y(26); }
static inline int contentPadding() { return SCALE_Y(8); }
static inline int scrollBarWidth() { return SCALE_X(6); }
static inline int viewMargin() { return MARGIN_SMALL; }
static inline int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

struct SettingsLayout {
  int viewTop;
  int viewHeight;
  int viewLeft;
  int viewWidth;
  int viewRight;
  int contentHeight;
  int bpmRowY;
  int clockRowY;
  int startModeRowY;
  int wifiRowY;
  int bluetoothRowY;
  int displayRowY;
  int scrollbarX;
};

static SettingsLayout computeSettingsLayout() {
  SettingsLayout layout;
  layout.viewTop = HEADER_HEIGHT + SCALE_Y(5);
  layout.viewLeft = viewMargin();
  layout.viewWidth = DISPLAY_WIDTH - 2 * viewMargin();
  layout.scrollbarX = DISPLAY_WIDTH - viewMargin() - scrollBarWidth();
  layout.viewWidth = layout.scrollbarX - layout.viewLeft - SCALE_X(4);
  layout.viewRight = layout.viewLeft + layout.viewWidth;
  layout.viewHeight = DISPLAY_HEIGHT - layout.viewTop - SCALE_Y(6);
  int y = layout.viewTop + contentPadding();
  layout.bpmRowY = y;
  y += bpmRowHeight() + settingsRowSpacing();
  layout.clockRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.startModeRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.wifiRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
  layout.bluetoothRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
  layout.displayRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.contentHeight = y - layout.viewTop + contentPadding();
  return layout;
}

static int settingsScrollOffset = 0;
static bool settingsScrollActive = false;
static int settingsScrollStartY = 0;
static int settingsScrollStartOffset = 0;
} // namespace

static constexpr uint16_t kBpmStep = 1;
static constexpr uint16_t kBpmMin = 40;
static constexpr uint16_t kBpmMax = 240;

static const char *const kClockMasterLabels[] = {
  "Internal Clock",
  "WiFi MIDI",
  "BLE MIDI",
  "Hardware MIDI",
};
static_assert(static_cast<int>(CLOCK_HARDWARE) + 1 == sizeof(kClockMasterLabels) / sizeof(kClockMasterLabels[0]),
              "Clock master labels must match enum size");

void initializeSettingsMode() {
  // No stateful initialization required yet.
}

void drawSettingsMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("SETTINGS", "Device preferences", 3);

  const SettingsLayout layout = computeSettingsLayout();
  tft.fillRoundRect(layout.viewLeft, layout.viewTop, layout.viewWidth, layout.viewHeight, 12, THEME_SURFACE);
  tft.drawRoundRect(layout.viewLeft, layout.viewTop, layout.viewWidth, layout.viewHeight, 12, THEME_TEXT_DIM);

  const int viewBottom = layout.viewTop + layout.viewHeight;
  const int contentLeft = layout.viewLeft + SCALE_X(6);
  const int contentRight = layout.viewRight - SCALE_X(2);
  const int rowWidth = contentRight - contentLeft;
  const int rowBgX = contentLeft - SCALE_X(4);
  const int rowBgW = rowWidth + SCALE_X(8);
  const int buttonHeight = SCALE_Y(44);
  const int buttonSpacing = SCALE_X(6);
  const int bpmRowY = layout.bpmRowY - settingsScrollOffset;
  if (bpmRowY >= layout.viewTop && bpmRowY + bpmRowHeight() > layout.viewTop && bpmRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = bpmRowY - SCALE_Y(18);
    tft.drawString("Shared Tempo", contentLeft, labelY, 2);
    int buttonY = bpmRowY + (bpmRowHeight() - buttonHeight) / 2;
    tft.fillRoundRect(contentLeft - SCALE_X(4), bpmRowY, rowWidth + SCALE_X(8), bpmRowHeight(), 12, THEME_BG);
    int narrowBtnW = max(SCALE_X(26), rowWidth / 8);
    int remainingWidth = rowWidth - narrowBtnW * 2 - buttonSpacing;
    int labelWidth = max(remainingWidth, SCALE_X(40));
    int labelX = contentLeft + narrowBtnW + buttonSpacing;
    drawRoundButton(contentLeft, buttonY, narrowBtnW, buttonHeight, "-", THEME_ERROR, false, 5);
    drawRoundButton(labelX, buttonY, labelWidth, buttonHeight, String(sharedBPM), THEME_PRIMARY, false, 4);
    drawRoundButton(labelX + labelWidth + buttonSpacing, buttonY, narrowBtnW, buttonHeight, "+", THEME_SUCCESS, false, 5);
  }

  const int clockRowY = layout.clockRowY - settingsScrollOffset;
  if (clockRowY >= layout.viewTop && clockRowY + compactRowHeight() > layout.viewTop && clockRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = clockRowY - SCALE_Y(18);
    tft.drawString("Clock Master", contentLeft, labelY, 2);
    drawRoundButton(contentLeft - SCALE_X(4), clockRowY, rowWidth + SCALE_X(8), compactRowHeight(),
                    kClockMasterLabels[static_cast<int>(midiClockMaster)], THEME_ACCENT, false, 2);
  }

  const int startModeRowY = layout.startModeRowY - settingsScrollOffset;
  if (startModeRowY >= layout.viewTop &&
      startModeRowY + compactRowHeight() > layout.viewTop &&
      startModeRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = startModeRowY - SCALE_Y(18);
    tft.drawString("Start Mode", contentLeft, labelY, 2);
    String modeLabel = instantStartMode ? "Instant" : "Quantized";
    drawRoundButton(contentLeft - SCALE_X(4), startModeRowY, rowWidth + SCALE_X(8), compactRowHeight(),
                    modeLabel, THEME_WARNING, false, 2);
  }

  const int wifiRowY = layout.wifiRowY - settingsScrollOffset;
  if (wifiRowY >= layout.viewTop && wifiRowY + statusRowHeight() > layout.viewTop && wifiRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String wifiStatus = "WiFi: " + String(isWiFiConnected() ? "Connected" : "Offline");
    tft.drawString(wifiStatus, contentLeft, wifiRowY, 2);
  }

  const int btRowY = layout.bluetoothRowY - settingsScrollOffset;
  if (btRowY >= layout.viewTop && btRowY + statusRowHeight() > layout.viewTop && btRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String btStatus = "Bluetooth MIDI: " + String(deviceConnected ? "Connected" : "Idle");
    tft.drawString(btStatus, contentLeft, btRowY, 2);
  }

  const int displayRowY = layout.displayRowY - settingsScrollOffset;
  if (displayRowY + compactRowHeight() > layout.viewTop && displayRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = displayRowY - SCALE_Y(18);
    tft.drawString("Display", contentLeft, labelY, 2);
    tft.fillRoundRect(rowBgX, displayRowY, rowBgW, compactRowHeight(), 12, THEME_BG);
    int displayButtonHeight = compactRowHeight() - SCALE_Y(10);
    int displayButtonY = displayRowY + (compactRowHeight() - displayButtonHeight) / 2;
    int displayFirstWidth = (rowWidth - buttonSpacing) / 2;
    int displaySecondWidth = rowWidth - buttonSpacing - displayFirstWidth;
    String invertLabel = displayColorsInverted ? "Colors Normal" : "Invert Colors";
    bool rotated = (displayRotationIndex == 1);
    String rotateLabel = rotated ? "Rotate Back" : "Rotate 180";
    drawRoundButton(contentLeft, displayButtonY, displayFirstWidth, displayButtonHeight, invertLabel,
                    THEME_WARNING, false, 1);
    drawRoundButton(contentLeft + displayFirstWidth + buttonSpacing, displayButtonY, displaySecondWidth,
                    displayButtonHeight, rotateLabel, THEME_PRIMARY, false, 1);
  }

  int maxScroll = std::max(0, layout.contentHeight - layout.viewHeight);
  if (maxScroll > 0) {
    int barHeight = std::max(SCALE_Y(24), (layout.viewHeight * layout.viewHeight) / layout.contentHeight);
    int trackHeight = layout.viewHeight - barHeight;
    int scrollable = maxScroll;
    int barY = layout.viewTop +
               (scrollable > 0 ? ((scrollable - settingsScrollOffset) * trackHeight) / scrollable : 0);
    tft.fillRoundRect(layout.scrollbarX, barY, scrollBarWidth(), barHeight, SCALE_X(3), THEME_TEXT_DIM);
  }
}

void handleSettingsMode() {
  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }

  if (!touch.justPressed && !settingsScrollActive) {
    return;
  }

  const SettingsLayout layout = computeSettingsLayout();
  int maxScroll = std::max(0, layout.contentHeight - layout.viewHeight);
  settingsScrollOffset = clampInt(settingsScrollOffset, 0, maxScroll);

  const int contentLeft = layout.viewLeft + SCALE_X(6);
  const int rowWidth = layout.viewWidth - SCALE_X(12);
  const int buttonHeight = SCALE_Y(44);
  const int buttonSpacing = SCALE_X(6);
  const int displayRowY = layout.displayRowY - settingsScrollOffset;
  const int displayButtonHeight = compactRowHeight() - SCALE_Y(10);
  const int displayFirstWidth = (rowWidth - buttonSpacing) / 2;
  const int displaySecondWidth = rowWidth - buttonSpacing - displayFirstWidth;
  const int displayButtonY = displayRowY + (compactRowHeight() - displayButtonHeight) / 2;
  const int displayRotateX = contentLeft + displayFirstWidth + buttonSpacing;
  const bool displayRowVisible = displayRowY >= layout.viewTop && displayRowY + compactRowHeight() > layout.viewTop &&
                                 displayRowY < layout.viewTop + layout.viewHeight;
  const int bpmButtonY = layout.bpmRowY - settingsScrollOffset + (bpmRowHeight() - buttonHeight) / 2;
  const int narrowBtnW = max(SCALE_X(26), rowWidth / 8);
  const int bpmLabelWidth = max(rowWidth - 2 * narrowBtnW - buttonSpacing, SCALE_X(48));
  const int bpmLabelX = contentLeft + narrowBtnW + buttonSpacing;
  const int minusX = contentLeft;
  const int plusX = bpmLabelX + bpmLabelWidth + buttonSpacing;

  bool handled = false;
  if (touch.justPressed && isButtonPressed(minusX, bpmButtonY, narrowBtnW, buttonHeight)) {
    if (sharedBPM > kBpmMin) {
      sharedBPM = (sharedBPM - kBpmStep < kBpmMin) ? kBpmMin : sharedBPM - kBpmStep;
      requestRedraw();
    }
    handled = true;
  } else if (touch.justPressed && isButtonPressed(plusX, bpmButtonY, narrowBtnW, buttonHeight)) {
    if (sharedBPM < kBpmMax) {
      sharedBPM = (sharedBPM + kBpmStep > kBpmMax) ? kBpmMax : sharedBPM + kBpmStep;
      requestRedraw();
    }
    handled = true;
  }

  const int clockButtonY = layout.clockRowY - settingsScrollOffset;
  const int clockButtonX = contentLeft - SCALE_X(4);
  const int clockButtonW = rowWidth + SCALE_X(8);
  if (!handled && touch.justPressed &&
      isButtonPressed(clockButtonX, clockButtonY, clockButtonW, compactRowHeight())) {
    midiClockMaster = static_cast<MidiClockMaster>((static_cast<int>(midiClockMaster) + 1) % (CLOCK_HARDWARE + 1));
    requestRedraw();
    handled = true;
  }

  const int startRowY = layout.startModeRowY - settingsScrollOffset;
  const int startRowX = contentLeft - SCALE_X(4);
  const int startRowW = rowWidth + SCALE_X(8);
  if (!handled && touch.justPressed &&
      isButtonPressed(startRowX, startRowY, startRowW, compactRowHeight())) {
    instantStartMode = !instantStartMode;
    requestRedraw();
    handled = true;
  }

  if (!handled && displayRowVisible && touch.justPressed &&
      isButtonPressed(contentLeft, displayButtonY, displayFirstWidth, displayButtonHeight)) {
    setDisplayInversion(!displayColorsInverted);
    handled = true;
  } else if (!handled && displayRowVisible && touch.justPressed &&
             isButtonPressed(displayRotateX, displayButtonY, displaySecondWidth, displayButtonHeight)) {
    rotateDisplay180();
    handled = true;
  }

  if (settingsScrollActive && touch.isPressed && maxScroll > 0) {
    int dy = touch.y - settingsScrollStartY;
    int target = clampInt(settingsScrollStartOffset - dy, 0, maxScroll);
    if (target != settingsScrollOffset) {
      settingsScrollOffset = target;
      requestRedraw();
    }
  }

  if (touch.justPressed && !handled && maxScroll > 0 &&
      touch.x >= layout.viewLeft && touch.x <= layout.viewLeft + layout.viewWidth &&
      touch.y >= layout.viewTop && touch.y <= layout.viewTop + layout.viewHeight) {
    settingsScrollActive = true;
    settingsScrollStartY = touch.y;
    settingsScrollStartOffset = settingsScrollOffset;
  }

  if (touch.justReleased) {
    settingsScrollActive = false;
  }
}
