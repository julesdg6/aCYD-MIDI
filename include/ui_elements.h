#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"

void exitToMenu();

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

inline void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed = false, uint8_t textFont = 2) {
  uint16_t bgColor = pressed ? color : THEME_SURFACE;
  uint16_t borderColor = color;
  uint16_t textColor = pressed ? THEME_BG : color;
  
  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, borderColor);
  tft.drawRoundRect(x+1, y+1, w-2, h-2, 7, borderColor);
  
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(text, x + w/2, y + h/2 - 8, textFont);
}

inline void drawHeader(String title, String subtitle, uint8_t titleFont = 4) {
  tft.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, THEME_SURFACE);
  tft.drawFastHLine(0, HEADER_HEIGHT, DISPLAY_WIDTH, THEME_PRIMARY);
  
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString(title, DISPLAY_CENTER_X, HEADER_TITLE_Y, titleFont);
  
  if (subtitle.length() > 0) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
    tft.drawCentreString(subtitle, DISPLAY_CENTER_X, HEADER_SUBTITLE_Y, 2);
  }
  
  drawRoundButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H, "BACK", THEME_ERROR, false, 1);
}

inline void updateStatus() {
  // Status bar removed - no more BLE connection alerts on every screen
}

#endif
