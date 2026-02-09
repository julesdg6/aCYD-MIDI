#include "module_settings_mode.h"
#include "wifi_manager.h"
#include "ui_elements.h"
#include "esp_now_midi_module.h"

#include <algorithm>

namespace {
static inline int settingsRowSpacing() { return SCALE_Y(12); }
// bpmRowHeight removed - BPM now accessible via header tap
static inline int compactRowHeight() { return SCALE_Y(44); }
static inline int statusRowHeight() { return SCALE_Y(26); }
static inline int contentPadding() { return SCALE_Y(8); }
static inline int viewTopPadding() { return SCALE_Y(12); }
static inline int rowSideMargin() { return SCALE_X(5); }
static inline int scrollBarThumbWidth() { return SCALE_X(18); }
static inline int scrollBarTouchWidth() { return SCALE_X(32); }
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
  // bpmRowY removed - BPM now accessible via header tap
  int clockRowY;
  int startModeRowY;
  int wifiRowY;
  int bluetoothRowY;
  int espNowRowY;
  int espNowModeRowY;
  int espNowStatusRowY;
  int displayRowY;
  int scrollbarTouchX;
  int scrollbarTrackX;
};

static SettingsLayout computeSettingsLayout() {
  SettingsLayout layout;
  layout.viewTop = HEADER_HEIGHT + viewTopPadding();
  layout.viewLeft = viewMargin();
  layout.viewWidth = DISPLAY_WIDTH - 2 * viewMargin();
  layout.scrollbarTouchX = DISPLAY_WIDTH - viewMargin() - scrollBarTouchWidth();
  layout.scrollbarTrackX =
      layout.scrollbarTouchX + (scrollBarTouchWidth() - scrollBarThumbWidth()) / 2;
  layout.viewWidth = layout.scrollbarTouchX - layout.viewLeft - SCALE_X(4);
  layout.viewRight = layout.viewLeft + layout.viewWidth;
  layout.viewHeight = DISPLAY_HEIGHT - layout.viewTop - SCALE_Y(6);
  int y = layout.viewTop + contentPadding();
  // BPM row removed - now accessible via header tap
  layout.clockRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.startModeRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.wifiRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
  layout.bluetoothRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
#if ESP_NOW_ENABLED
  layout.espNowRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.espNowModeRowY = y;
  y += compactRowHeight() + settingsRowSpacing();
  layout.espNowStatusRowY = y;
  y += statusRowHeight() + settingsRowSpacing();
#endif
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

// BPM constants removed - now handled in BPM settings mode

static const char *const kClockMasterLabels[] = {
  "Internal Clock",
  "WiFi MIDI",
  "BLE MIDI",
  "Hardware MIDI",
  "ESP-NOW MIDI",
};
static_assert(static_cast<int>(CLOCK_ESP_NOW) + 1 == sizeof(kClockMasterLabels) / sizeof(kClockMasterLabels[0]),
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
  const int horizontalMargin = rowSideMargin();
  const int rowBgX = layout.viewLeft + horizontalMargin;
  const int rowBgW = layout.viewWidth - 2 * horizontalMargin;
  const int buttonInset = rowSideMargin();
  const int rowInnerLeft = rowBgX + buttonInset;
  const int rowInnerW = rowBgW - 2 * buttonInset;
  const int buttonHeight = SCALE_Y(44);
  const int buttonSpacing = SCALE_X(6);
  
  // BPM row removed - now accessible via header tap

  const int clockRowY = layout.clockRowY - settingsScrollOffset;
  const int clockLabelY = clockRowY - SCALE_Y(18);
  const int clockLabelH = SCALE_Y(18);
  if ((clockRowY + compactRowHeight() > layout.viewTop && clockRowY < viewBottom) ||
      (clockLabelY + clockLabelH > layout.viewTop && clockLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = clockLabelY;
    tft.drawString("Clock Master", rowInnerLeft, labelY, 2);
    drawRoundButton(rowInnerLeft, clockRowY, rowInnerW, compactRowHeight(),
                    kClockMasterLabels[static_cast<int>(midiClockMaster)], THEME_ACCENT, false, 2);
  }

  const int startModeRowY = layout.startModeRowY - settingsScrollOffset;
  const int startLabelY = startModeRowY - SCALE_Y(18);
  const int startLabelH = SCALE_Y(18);
  if ((startModeRowY + compactRowHeight() > layout.viewTop && startModeRowY < viewBottom) ||
      (startLabelY + startLabelH > layout.viewTop && startLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = startLabelY;
    tft.drawString("Start Mode", rowInnerLeft, labelY, 2);
    String modeLabel = instantStartMode ? "Instant" : "Quantized";
    drawRoundButton(rowInnerLeft, startModeRowY, rowInnerW, compactRowHeight(),
                    modeLabel, THEME_WARNING, false, 2);
  }

  const int wifiRowY = layout.wifiRowY - settingsScrollOffset;
  if (wifiRowY + statusRowHeight() > layout.viewTop && wifiRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String wifiStatus = "WiFi: " + String(isWiFiConnected() ? "Connected" : "Offline");
    tft.drawString(wifiStatus, rowInnerLeft, wifiRowY, 2);
  }

  const int btRowY = layout.bluetoothRowY - settingsScrollOffset;
  if (btRowY + statusRowHeight() > layout.viewTop && btRowY < viewBottom) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String btStatus = "Bluetooth MIDI: " + String(deviceConnected ? "Connected" : "Idle");
    tft.drawString(btStatus, rowInnerLeft, btRowY, 2);
  }

#if ESP_NOW_ENABLED
  // ESP-NOW Enable/Disable button
  const int espNowRowY = layout.espNowRowY - settingsScrollOffset;
  const int espNowLabelY = espNowRowY - SCALE_Y(18);
  const int espNowLabelH = SCALE_Y(18);
  if ((espNowRowY + compactRowHeight() > layout.viewTop && espNowRowY < viewBottom) ||
      (espNowLabelY + espNowLabelH > layout.viewTop && espNowLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = espNowLabelY;
    tft.drawString("ESP-NOW MIDI", rowInnerLeft, labelY, 2);
    const char* espNowLabel = (espNowState.mode == ESP_NOW_OFF) ? "Disabled" : "Enabled";
    uint16_t espNowColor = (espNowState.mode == ESP_NOW_OFF) ? THEME_ERROR : THEME_SUCCESS;
    drawRoundButton(rowInnerLeft, espNowRowY, rowInnerW, compactRowHeight(),
                    espNowLabel, espNowColor, false, 2);
  }

  // ESP-NOW Mode selection
  const int espNowModeRowY = layout.espNowModeRowY - settingsScrollOffset;
  const int espNowModeLabelY = espNowModeRowY - SCALE_Y(18);
  const int espNowModeLabelH = SCALE_Y(18);
  if ((espNowModeRowY + compactRowHeight() > layout.viewTop && espNowModeRowY < viewBottom) ||
      (espNowModeLabelY + espNowModeLabelH > layout.viewTop && espNowModeLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = espNowModeLabelY;
    tft.drawString("ESP-NOW Mode", rowInnerLeft, labelY, 2);
    const char* modeLabel = (espNowState.mode == ESP_NOW_OFF) ? "Off" : 
                           (espNowState.mode == ESP_NOW_BROADCAST) ? "Broadcast" : "Peer";
    uint16_t modeColor = (espNowState.mode == ESP_NOW_OFF) ? THEME_TEXT_DIM : THEME_ACCENT;
    drawRoundButton(rowInnerLeft, espNowModeRowY, rowInnerW, compactRowHeight(),
                    modeLabel, modeColor, false, 2);
  }

  // ESP-NOW Status
  const int espNowStatusRowY = layout.espNowStatusRowY - settingsScrollOffset;
  const int espNowStatusLabelY = espNowStatusRowY - SCALE_Y(0);
  if ((espNowStatusRowY + statusRowHeight() > layout.viewTop && espNowStatusRowY < viewBottom) ||
      (espNowStatusLabelY + statusRowHeight() > layout.viewTop && espNowStatusLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT, THEME_SURFACE);
    String espNowStatus = String("ESP-NOW: Peers=") + getEspNowPeerCount() + 
                         " TX=" + espNowState.messagesSent + 
                         " RX=" + espNowState.messagesReceived;
    tft.drawString(espNowStatus, rowInnerLeft, espNowStatusRowY, 2);
  }
#endif

  const int displayRowY = layout.displayRowY - settingsScrollOffset;
  const int displayLabelY = displayRowY - SCALE_Y(18);
  const int displayLabelH = SCALE_Y(18);
  if ((displayRowY + compactRowHeight() > layout.viewTop && displayRowY < viewBottom) ||
      (displayLabelY + displayLabelH > layout.viewTop && displayLabelY < viewBottom)) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    int labelY = displayLabelY;
    tft.drawString("Display", rowInnerLeft, labelY, 2);
    tft.fillRoundRect(rowBgX, displayRowY, rowBgW, compactRowHeight(), 12, THEME_BG);
    int displayButtonHeight = compactRowHeight() - SCALE_Y(10);
    int displayButtonY = displayRowY + (compactRowHeight() - displayButtonHeight) / 2;
    int displayFirstWidth = (rowInnerW - buttonSpacing) / 2;
    int displaySecondWidth = rowInnerW - buttonSpacing - displayFirstWidth;
    String invertLabel = displayColorsInverted ? "Colors Normal" : "Invert Colors";
    bool rotated = (displayRotationIndex == 1);
    String rotateLabel = rotated ? "Rotate Back" : "Rotate 180";
    drawRoundButton(rowInnerLeft, displayButtonY, displayFirstWidth, displayButtonHeight, invertLabel,
                    THEME_WARNING, false, 1);
    drawRoundButton(rowInnerLeft + displayFirstWidth + buttonSpacing, displayButtonY, displaySecondWidth,
                    displayButtonHeight, rotateLabel, THEME_PRIMARY, false, 1);
  }

  int maxScroll = std::max(0, layout.contentHeight - layout.viewHeight);
  if (maxScroll > 0) {
    int trackTop = layout.viewTop + SCALE_Y(3);
    int trackHeight = layout.viewHeight - SCALE_Y(6);
    int thumbWidth = scrollBarThumbWidth();
    int radius = SCALE_X(3);
    tft.fillRoundRect(layout.scrollbarTrackX, trackTop, thumbWidth, trackHeight, radius, THEME_BG);
    tft.drawRoundRect(layout.scrollbarTrackX, trackTop, thumbWidth, trackHeight, radius, THEME_TEXT_DIM);
    int barHeight = std::max(SCALE_Y(24), (layout.viewHeight * layout.viewHeight) / layout.contentHeight);
    barHeight = std::min(barHeight, trackHeight);
    int availableTrack = std::max(0, trackHeight - barHeight);
    int barY =
        trackTop + (settingsScrollOffset > 0 ? (settingsScrollOffset * availableTrack) / maxScroll : 0);
    tft.fillRoundRect(layout.scrollbarTrackX, barY, thumbWidth, barHeight, radius, THEME_TEXT);
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

  const int horizontalMargin = rowSideMargin();
  const int buttonInset = rowSideMargin();
  const int rowInnerLeft = layout.viewLeft + horizontalMargin + buttonInset;
  const int rowInnerW = layout.viewWidth - 2 * horizontalMargin - 2 * buttonInset;
  const int buttonHeight = SCALE_Y(44);
  const int buttonSpacing = SCALE_X(6);
  const int displayRowY = layout.displayRowY - settingsScrollOffset;
  const int displayButtonHeight = compactRowHeight() - SCALE_Y(10);
  const int displayFirstWidth = (rowInnerW - buttonSpacing) / 2;
  const int displaySecondWidth = rowInnerW - buttonSpacing - displayFirstWidth;
  const int displayButtonY = displayRowY + (compactRowHeight() - displayButtonHeight) / 2;
  const int displayRotateX = rowInnerLeft + displayFirstWidth + buttonSpacing;
  const bool displayRowVisible = displayRowY >= layout.viewTop && displayRowY + compactRowHeight() > layout.viewTop &&
                                 displayRowY < layout.viewTop + layout.viewHeight;
  // BPM button handling removed - now accessible via header tap
  const int trackTop = layout.viewTop + SCALE_Y(3);
  const int trackHeight = layout.viewHeight - SCALE_Y(6);
  const int scrollTouchRight = layout.scrollbarTouchX + scrollBarTouchWidth();

  bool handled = false;
  // BPM +/- button handling removed

  const int clockButtonY = layout.clockRowY - settingsScrollOffset;
  const int clockButtonX = rowInnerLeft;
  const int clockButtonW = rowInnerW;
  if (!handled && touch.justPressed &&
      isButtonPressed(clockButtonX, clockButtonY, clockButtonW, compactRowHeight())) {
    const int kCount = static_cast<int>(sizeof(kClockMasterLabels) / sizeof(kClockMasterLabels[0]));
    midiClockMaster = static_cast<MidiClockMaster>((static_cast<int>(midiClockMaster) + 1) % kCount);
    requestRedraw();
    handled = true;
  }

#if ESP_NOW_ENABLED
  // ESP-NOW Enable/Disable button handler
  const int espNowButtonY = layout.espNowRowY - settingsScrollOffset;
  const int espNowButtonX = rowInnerLeft;
  const int espNowButtonW = rowInnerW;
  if (!handled && touch.justPressed &&
      isButtonPressed(espNowButtonX, espNowButtonY, espNowButtonW, compactRowHeight())) {
    // Toggle ESP-NOW on/off
    if (espNowState.mode == ESP_NOW_OFF) {
      setEspNowMode(ESP_NOW_BROADCAST);
    } else {
      setEspNowMode(ESP_NOW_OFF);
    }
    requestRedraw();
    handled = true;
  }

  // ESP-NOW Mode button handler
  const int espNowModeButtonY = layout.espNowModeRowY - settingsScrollOffset;
  const int espNowModeButtonX = rowInnerLeft;
  const int espNowModeButtonW = rowInnerW;
  if (!handled && touch.justPressed &&
      isButtonPressed(espNowModeButtonX, espNowModeButtonY, espNowModeButtonW, compactRowHeight())) {
    // Cycle through modes: OFF -> BROADCAST -> PEER -> OFF
    if (espNowState.mode == ESP_NOW_OFF) {
      setEspNowMode(ESP_NOW_BROADCAST);
    } else if (espNowState.mode == ESP_NOW_BROADCAST) {
      setEspNowMode(ESP_NOW_PEER);
    } else {
      setEspNowMode(ESP_NOW_OFF);
    }
    requestRedraw();
    handled = true;
  }
#endif

  const int startRowY = layout.startModeRowY - settingsScrollOffset;
  const int startRowX = rowInnerLeft;
  const int startRowW = rowInnerW;
  if (!handled && touch.justPressed &&
      isButtonPressed(startRowX, startRowY, startRowW, compactRowHeight())) {
    instantStartMode = !instantStartMode;
    requestRedraw();
    handled = true;
  }

  if (!handled && displayRowVisible && touch.justPressed &&
      isButtonPressed(rowInnerLeft, displayButtonY, displayFirstWidth, displayButtonHeight)) {
    setDisplayInversion(!displayColorsInverted);
    handled = true;
  } else if (!handled && displayRowVisible && touch.justPressed &&
             isButtonPressed(displayRotateX, displayButtonY, displaySecondWidth, displayButtonHeight)) {
    rotateDisplay180();
    handled = true;
  }

  if (settingsScrollActive && touch.isPressed && maxScroll > 0) {
    int dy = touch.y - settingsScrollStartY;
    int target = clampInt(settingsScrollStartOffset + dy, 0, maxScroll);
    if (target != settingsScrollOffset) {
      settingsScrollOffset = target;
      requestRedraw();
    }
  }

  const bool touchInsideView =
      touch.x >= layout.viewLeft && touch.x <= layout.viewLeft + layout.viewWidth &&
      touch.y >= layout.viewTop && touch.y <= layout.viewTop + layout.viewHeight;
  const bool touchInsideScrollbar =
      touch.x >= layout.scrollbarTouchX && touch.x <= scrollTouchRight &&
      touch.y >= trackTop && touch.y <= trackTop + trackHeight;
  if (touch.justPressed && !handled && maxScroll > 0 &&
      (touchInsideView || touchInsideScrollbar)) {
    settingsScrollActive = true;
    settingsScrollStartY = touch.y;
    settingsScrollStartOffset = settingsScrollOffset;
    handled = true;
  }

  if (touch.justReleased) {
    settingsScrollActive = false;
  }
}
