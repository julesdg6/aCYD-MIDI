#include "splash_screen.h"
#include "common_definitions.h"
#include "assets_splash.h"
#include "remote_display.h"

#include <algorithm>
#include <pgmspace.h>

static void drawSplashBitmap() {
  // The bitmap is stored at half resolution (128x85) in 1-bit format
  // We scale it up 2x when drawing to get 256x170 display size
  const int SCALE = 2;
  const int displayWidth = SPLASH_WIDTH * SCALE;
  const int displayHeight = SPLASH_HEIGHT * SCALE;
  
  const int startX = std::max(0, (DISPLAY_WIDTH - displayWidth) / 2);
  const int startY = std::max(0, (DISPLAY_HEIGHT - displayHeight) / 2 - SCALE_Y(10));
  
  // Decode 1-bit bitmap: 8 pixels per byte, MSB first
  // White pixels (bit=1) are drawn with THEME_ACCENT color, black (bit=0) stays background
  for (int srcY = 0; srcY < SPLASH_HEIGHT; ++srcY) {
    for (int srcX = 0; srcX < SPLASH_WIDTH; ++srcX) {
      // Calculate byte index and bit position
      int pixelIndex = srcY * SPLASH_WIDTH + srcX;
      int byteIndex = pixelIndex / 8;
      int bitIndex = 7 - (pixelIndex % 8);  // MSB first
      
      // Read bit from PROGMEM
      uint8_t byte = pgm_read_byte(&SPLASH_BITMAP[byteIndex]);
      bool isWhite = (byte & (1 << bitIndex)) != 0;
      
      // Draw 2x2 block for this pixel
      if (isWhite) {
        uint16_t color = THEME_ACCENT;  // Bright cyan for logo
        for (int dy = 0; dy < SCALE; ++dy) {
          for (int dx = 0; dx < SCALE; ++dx) {
            tft.drawPixel(startX + srcX * SCALE + dx, startY + srcY * SCALE + dy, color);
          }
        }
      }
      // Black pixels are already background - no need to draw
    }
  }
}

void showSplashScreen(const String &status, unsigned long delayMs) {
  tft.fillScreen(THEME_BG);
  drawSplashBitmap();
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString("aCYD MIDI", DISPLAY_CENTER_X, HEADER_TITLE_Y + SCALE_Y(8), 5);
  
  // Display version
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawCentreString(String("v") + ACYD_MIDI_VERSION, DISPLAY_CENTER_X, HEADER_TITLE_Y + SCALE_Y(36), 2);

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
  delay(delayMs);
}
