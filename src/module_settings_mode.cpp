#include "module_settings_mode.h"
#include "wifi_manager.h"
#include "ui_elements.h"

namespace {
static inline int settingsRowSpacing() { return SCALE_Y(10); }
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
  int wifiRowY;
  int bluetoothRowY;
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
  layout.wifiRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
  layout.bluetoothRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
  layout.contentHeight = y - layout.viewTop + contentPadding();
  return layout;
}

static int settingsScrollOffset = 0;
static bool settingsScrollActive = false;
static int settingsScrollStartY = 0;
static int settingsScrollStartOffset = 0;
} // namespace

static constexpr uint16_t kBpmStep = 5;
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
  const int buttonHeight = SCALE_Y(44);
  const int buttonSpacing = SCALE_X(6);
  const int bpmRowY = layout.bpmRowY - settingsScrollOffset;
  if (bpmRowY + bpmRowHeight() > layout.viewTop && bpmRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = bpmRowY - SCALE_Y(18);
    tft.drawString("Shared Tempo", contentLeft, labelY, 2);
    int buttonY = bpmRowY + (bpmRowHeight() - buttonHeight) / 2;
    tft.fillRoundRect(contentLeft - SCALE_X(4), bpmRowY, rowWidth + SCALE_X(8), bpmRowHeight(), 12, THEME_BG);
    int segmentWidth = (rowWidth - 2 * buttonSpacing) / 3;
    drawRoundButton(contentLeft, buttonY, segmentWidth, buttonHeight, "-", THEME_ERROR, false, 5);
    drawRoundButton(contentLeft + segmentWidth + buttonSpacing, buttonY, segmentWidth, buttonHeight,
                    String(sharedBPM) + " BPM", THEME_PRIMARY, false, 4);
    drawRoundButton(contentLeft + 2 * (segmentWidth + buttonSpacing), buttonY, segmentWidth, buttonHeight,
                    "+", THEME_SUCCESS, false, 5);
  }

  const int clockRowY = layout.clockRowY - settingsScrollOffset;
  if (clockRowY + compactRowHeight() > layout.viewTop && clockRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = clockRowY - SCALE_Y(18);
    tft.drawString("Clock Master", contentLeft, labelY, 2);
    drawRoundButton(contentLeft - SCALE_X(4), clockRowY, rowWidth + SCALE_X(8), compactRowHeight(),
                    kClockMasterLabels[static_cast<int>(midiClockMaster)], THEME_ACCENT, false, 2);
  }

  const int wifiRowY = layout.wifiRowY - settingsScrollOffset;
  if (wifiRowY + statusRowHeight() > layout.viewTop && wifiRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String wifiStatus = "WiFi: " + String(isWiFiConnected() ? "Connected" : "Offline");
    tft.drawString(wifiStatus, contentLeft, wifiRowY, 2);
  }

  const int btRowY = layout.bluetoothRowY - settingsScrollOffset;
  if (btRowY + statusRowHeight() > layout.viewTop && btRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String btStatus = "Bluetooth MIDI: " + String(deviceConnected ? "Connected" : "Idle");
    tft.drawString(btStatus, contentLeft, btRowY, 2);
  }

  int maxScroll = std::max(0, layout.contentHeight - layout.viewHeight);
  if (maxScroll > 0) {
  int barHeight = std::max(SCALE_Y(24), (layout.viewHeight * layout.viewHeight) / layout.contentHeight);
    int trackHeight = layout.viewHeight - barHeight;
    int scrollable = maxScroll;
    int barY = layout.viewTop + (scrollable > 0 ? (settingsScrollOffset * trackHeight) / scrollable : 0);
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
  const int segmentWidth = (rowWidth - 2 * buttonSpacing) / 3;
  const int bpmButtonY = layout.bpmRowY - settingsScrollOffset + (bpmRowHeight() - buttonHeight) / 2;
  const int minusX = contentLeft;
  const int plusX = contentLeft + 2 * (segmentWidth + buttonSpacing);

  bool handled = false;
  if (touch.justPressed && isButtonPressed(minusX, bpmButtonY, segmentWidth, buttonHeight)) {
    if (sharedBPM > kBpmMin) {
      sharedBPM = (sharedBPM - kBpmStep < kBpmMin) ? kBpmMin : sharedBPM - kBpmStep;
      requestRedraw();
    }
    handled = true;
  } else if (touch.justPressed && isButtonPressed(plusX, bpmButtonY, segmentWidth, buttonHeight)) {
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
