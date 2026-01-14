#include "splash_screen.h"
#include "common_definitions.h"
#include "assets_splash.h"
#include "remote_display.h"

#include <algorithm>

static void drawSplashBitmap() {
  const int startX = std::max(0, (DISPLAY_WIDTH - SPLASH_WIDTH) / 2);
  const int startY = std::max(0, (DISPLAY_HEIGHT - SPLASH_HEIGHT) / 2 - SCALE_Y(10));
  for (int y = 0; y < SPLASH_HEIGHT; ++y) {
    int rowIndex = y * SPLASH_WIDTH;
    for (int x = 0; x < SPLASH_WIDTH; ++x) {
      uint16_t color = SPLASH_PIXELS[rowIndex + x];
      tft.drawPixel(startX + x, startY + y, color);
    }
  }
}

void showSplashScreen(const String &status) {
  tft.fillScreen(THEME_BG);
  drawSplashBitmap();
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString("aCYD MIDI", DISPLAY_CENTER_X, HEADER_TITLE_Y + SCALE_Y(8), 5);
  
  // Display version
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawCentreString("v" ACYD_MIDI_VERSION, DISPLAY_CENTER_X, HEADER_TITLE_Y + SCALE_Y(36), 2);

  String message;
  if (status.length()) {
    message = status;
  } else if (isRemoteDisplayConnected()) {
    message = "WiFi: " + getRemoteDisplayIP();
  } else {
    message = "WiFi: Connecting...";
  }

  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString(message, DISPLAY_CENTER_X, DISPLAY_HEIGHT - SCALE_Y(32), 2);
  delay(800);
}
