#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"
#include "wifi_manager.h"

void exitToMenu();
void requestRedraw();

// UI implementations
inline void updateTouch() {
  touch.wasPressed = touch.isPressed;
  lv_indev_t *indev = lv_indev_get_next(NULL);
  touch.isPressed = indev && (lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED);
  touch.justPressed = touch.isPressed && !touch.wasPressed;
  touch.justReleased = !touch.isPressed && touch.wasPressed;
  
  if (touch.isPressed) {
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    touch.x = p.x;
    touch.y = p.y;
  }
}

inline bool isButtonPressed(int x, int y, int w, int h) {
  return touch.x >= x && touch.x <= x + w && touch.y >= y && touch.y <= y + h;
}

inline void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed = false, uint8_t textFont = 1) {
  uint16_t bgColor = pressed ? color : THEME_SURFACE;
  uint16_t borderColor = color;
  uint16_t textColor;
  
  if (pressed) {
    textColor = THEME_BG;
  } else {
    // For unpressed buttons with THEME_SURFACE background, use a lighter text color for contrast
    if (color == THEME_SURFACE) {
      textColor = THEME_TEXT_DIM;
    } else {
      textColor = color;
    }
  }

  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, borderColor);
  tft.drawRoundRect(x+1, y+1, w-2, h-2, 7, borderColor);

  tft.setTextColor(textColor, bgColor);
  int textY = y + h/2 - SCALE_Y(2);
  if (textFont == 0 || textFont == 1) {
    textY = y + h/2 - SCALE_Y(3);
  }
  tft.drawCentreString(text, x + w/2, textY, textFont);
}

static inline void drawWifiIndicator(int x, int y, uint16_t color) {
  const int radii[] = {SCALE_X(3), SCALE_X(5), SCALE_X(7)};
  for (int radius : radii) {
    tft.drawCircle(x, y, radius, color);
  }
  tft.fillCircle(x, y, SCALE_X(2), color);
  tft.fillRect(x - radii[2], y, radii[2] * 2 + 1, radii[2], THEME_SURFACE);
}

static inline void drawBluetoothIndicator(int x, int y, uint16_t color) {
  int halfHeight = SCALE_Y(7);
  int diagOffset = SCALE_X(6);
  int stemX = x - SCALE_X(1);
  tft.drawLine(stemX, y - halfHeight, stemX, y + halfHeight, color);
  tft.drawLine(stemX, y - halfHeight, x + diagOffset, y - SCALE_Y(2), color);
  tft.drawLine(stemX, y + halfHeight, x + diagOffset, y + SCALE_Y(2), color);
  tft.drawLine(x + diagOffset, y - SCALE_Y(2), x + diagOffset, y + SCALE_Y(2), color);
  tft.drawLine(x + diagOffset, y - SCALE_Y(2), stemX, y, color);
  tft.drawLine(x + diagOffset, y + SCALE_Y(2), stemX, y, color);
}

static inline void drawStatusIndicators() {
  String bpmLabel = String(sharedBPM) + " BPM";
  int textX = DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(70);
  int textY = HEADER_TITLE_Y + SCALE_Y(2);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawString(bpmLabel, textX, textY, 2);

  const int iconSpacing = SCALE_X(6);
  const int iconWidth = SCALE_X(16);
  int bluetoothX = textX - iconSpacing - iconWidth;
  int wifiX = bluetoothX - iconSpacing - iconWidth;
  int iconY = textY + SCALE_Y(4);
  uint16_t bluetoothColor = deviceConnected ? THEME_SUCCESS : THEME_TEXT_DIM;
  uint16_t wifiColor = isWiFiConnected() ? THEME_SUCCESS : THEME_TEXT_DIM;
  drawBluetoothIndicator(bluetoothX, iconY, bluetoothColor);
  drawWifiIndicator(wifiX, iconY, wifiColor);
}

inline void drawHeader(String title, String subtitle, uint8_t titleFont = 4, bool showBackButton = true) {
  tft.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, THEME_SURFACE);
  tft.drawFastHLine(0, HEADER_HEIGHT, DISPLAY_WIDTH, THEME_PRIMARY);
  
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString(title, DISPLAY_CENTER_X, HEADER_TITLE_Y, titleFont);
  
  if (subtitle.length() > 0) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    tft.drawCentreString(subtitle, DISPLAY_CENTER_X, HEADER_SUBTITLE_Y, 2);
  }
  
  if (showBackButton) {
    drawRoundButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H, "BACK", THEME_ERROR, false, 1);
  }
  drawStatusIndicators();
}

inline void updateStatus() {
  // Status bar removed - no more BLE connection alerts on every screen
}

#endif
