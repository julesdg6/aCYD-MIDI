#include "splash_screen.h"
#include "common_definitions.h"
#include "assets_splash.h"
#include "remote_display.h"

#include <algorithm>
#include <pgmspace.h>
#include <lvgl.h>

// Helper function to convert RGB565 color to LVGL color
static lv_color_t colorFrom565(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;
  return lv_color_make(r, g, b);
}

// Helper function to get LVGL font for TFT_eSPI font size
static const lv_font_t *fontFor(uint8_t font) {
  switch (font) {
    case 5:
      return &lv_font_montserrat_32;
    case 4:
      return &lv_font_montserrat_32;
    case 2:
      return &lv_font_montserrat_20;
    default:
      return &lv_font_montserrat_14;
  }
}

void showSplashScreen(const String &status, unsigned long delayMs) {
  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    delay(delayMs);
    return;
  }
  
  lv_obj_t *screen = lv_screen_active();
  
  // Clear screen with background color
  lv_obj_set_style_bg_color(screen, colorFrom565(THEME_BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  
  // Create a container for the splash screen
  lv_obj_t *splash_container = lv_obj_create(screen);
  lv_obj_set_size(splash_container, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_obj_set_pos(splash_container, 0, 0);
  lv_obj_set_style_bg_opa(splash_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(splash_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(splash_container, 0, 0);
  
  // Draw the bitmap using a canvas
  const int SCALE = 2;
  const int canvasWidth = SPLASH_WIDTH * SCALE;
  const int canvasHeight = SPLASH_HEIGHT * SCALE;
  const int startX = std::max(0, (DISPLAY_WIDTH - canvasWidth) / 2);
  const int startY = std::max(0, (DISPLAY_HEIGHT - canvasHeight) / 2 - SCALE_Y(10));
  
  // Create canvas for the logo
  lv_obj_t *canvas = lv_canvas_create(splash_container);
  lv_obj_set_pos(canvas, startX, startY);
  
  // Allocate buffer for canvas (RGB565 format: 2 bytes per pixel)
  lv_color_t *cbuf = (lv_color_t *)malloc(canvasWidth * canvasHeight * sizeof(lv_color_t));
  
  if (cbuf) {
    lv_canvas_set_buffer(canvas, cbuf, canvasWidth, canvasHeight, LV_COLOR_FORMAT_RGB565);
    
    lv_color_t bg_color = colorFrom565(THEME_BG);
    lv_color_t accent_color = colorFrom565(THEME_ACCENT);
    
    // Fill canvas with background color
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);
    
    // Decode 1-bit bitmap and draw scaled pixels
    for (int srcY = 0; srcY < SPLASH_HEIGHT; ++srcY) {
      for (int srcX = 0; srcX < SPLASH_WIDTH; ++srcX) {
        int pixelIndex = srcY * SPLASH_WIDTH + srcX;
        int byteIndex = pixelIndex / 8;
        int bitIndex = 7 - (pixelIndex % 8);
        
        uint8_t byte = pgm_read_byte(&SPLASH_BITMAP[byteIndex]);
        bool isWhite = (byte & (1 << bitIndex)) != 0;
        
        if (isWhite) {
          // Draw 2x2 block for this pixel
          for (int dy = 0; dy < SCALE; ++dy) {
            for (int dx = 0; dx < SCALE; ++dx) {
              lv_canvas_set_px(canvas, srcX * SCALE + dx, srcY * SCALE + dy, accent_color, LV_OPA_COVER);
            }
          }
        }
      }
    }
  }
  
  // Create title label
  lv_obj_t *title_label = lv_label_create(splash_container);
  lv_label_set_text(title_label, "aCYD MIDI");
  lv_obj_set_style_text_font(title_label, fontFor(5), 0);
  lv_obj_set_style_text_color(title_label, colorFrom565(THEME_TEXT), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, HEADER_TITLE_Y + SCALE_Y(8));
  
  // Create version label
  lv_obj_t *version_label = lv_label_create(splash_container);
  lv_label_set_text_fmt(version_label, "v%s", ACYD_MIDI_VERSION);
  lv_obj_set_style_text_font(version_label, fontFor(2), 0);
  lv_obj_set_style_text_color(version_label, colorFrom565(THEME_TEXT_DIM), 0);
  lv_obj_align(version_label, LV_ALIGN_TOP_MID, 0, HEADER_TITLE_Y + SCALE_Y(36));
  
  // Create status message label
  lv_obj_t *status_label = lv_label_create(splash_container);
  if (status.length()) {
    // Copy the provided status string
    lv_label_set_text_fmt(status_label, "%s", status.c_str());
  } else if (isRemoteDisplayConnected()) {
    // Copy the WiFi IP string
    String wifi_msg = "WiFi: " + getRemoteDisplayIP();
    lv_label_set_text_fmt(status_label, "%s", wifi_msg.c_str());
  } else {
    // Static string - can use directly
    lv_label_set_text_static(status_label, "WiFi: Connecting...");
  }
  lv_obj_set_style_text_font(status_label, fontFor(2), 0);
  lv_obj_set_style_text_color(status_label, colorFrom565(THEME_TEXT), 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -SCALE_Y(32));
  
  // Force LVGL to render immediately
  lv_refr_now(disp);
  
  delay(delayMs);
  
  // Clean up splash screen objects and free canvas buffer
  if (cbuf) {
    free(cbuf);
  }
  lv_obj_delete(splash_container);
  lv_refr_now(disp);
}
