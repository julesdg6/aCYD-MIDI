#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include <algorithm>

#include "common_definitions.h"
#include "clock_manager.h"
#include "midi_transport.h"
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
  const int iconSize = SCALE_X(12);
  const int iconSpacing = SCALE_X(4);
  const int gridCols = 2;
  const int gridRows = 2;
  const int totalWidth = gridCols * iconSize + (gridCols - 1) * iconSpacing;
  const int totalHeight = gridRows * iconSize + (gridRows - 1) * iconSpacing;
  int iconsStartX = DISPLAY_WIDTH - MARGIN_SMALL - totalWidth;
  int iconsStartY = HEADER_TITLE_Y + SCALE_Y(2);

  String bpmLabel = String(sharedBPM);
  int bpmX = std::max(MARGIN_SMALL, iconsStartX - SCALE_X(70));
  int bpmY = iconsStartY + totalHeight / 2 - SCALE_Y(6);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawString(bpmLabel, bpmX, bpmY, 2);

  struct Indicator {
    int dx;
    int dy;
    uint16_t color;
  };
  Indicator icons[4] = {
      {0, 0,
       static_cast<uint16_t>(isWiFiConnected() ? THEME_SECONDARY : THEME_TEXT_DIM)},
      {iconSize + iconSpacing, 0,
       static_cast<uint16_t>(deviceConnected ? THEME_PRIMARY : THEME_TEXT_DIM)},
      {0, iconSize + iconSpacing,
       static_cast<uint16_t>(clockManagerIsRunning() ? THEME_SUCCESS : THEME_ERROR)},
      {iconSize + iconSpacing, iconSize + iconSpacing,
       static_cast<uint16_t>(
           midiTransportIsPulseActive() ? THEME_WARNING : THEME_TEXT_DIM)},
  };

  for (int i = 0; i < 4; ++i) {
    int x = iconsStartX + icons[i].dx;
    int y = iconsStartY + icons[i].dy;
    int centerX = x + iconSize / 2;
    int centerY = y + iconSize / 2;

    switch (i) {
      case 0:
        // WiFi icon (top-left)
        drawWifiIndicator(centerX, centerY, icons[i].color);
        break;
      case 1:
        // Bluetooth icon (top-right)
        drawBluetoothIndicator(centerX, centerY, icons[i].color);
        break;
      case 2:
        // Clock indicator (bottom-left) - small filled circle
        tft.fillCircle(centerX, centerY, SCALE_X(4), icons[i].color);
        break;
      case 3:
        // MIDI pulse indicator (bottom-right) - small rounded rect
        tft.fillRoundRect(centerX - SCALE_X(4), centerY - SCALE_Y(4),
                          SCALE_X(8), SCALE_Y(8), 3, icons[i].color);
        break;
    }
  }
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
