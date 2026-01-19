#include "splash_screen.h"
#include "common_definitions.h"
#include "assets_splash.h"
#include "remote_display.h"

#include <algorithm>
#include <pgmspace.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

// Helper to convert RGB565 to lv_color_t
static lv_color_t colorFrom565(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;
  return lv_color_make(r, g, b);
}

void showSplashScreen(const String &status, unsigned long delayMs) {
  // Create a full-screen container for the splash screen
  lv_obj_t *splash_container = lv_obj_create(lv_screen_active());
  lv_obj_set_size(splash_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(splash_container, colorFrom565(THEME_BG), 0);
  lv_obj_set_style_bg_opa(splash_container, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(splash_container, 0, 0);
  lv_obj_set_style_pad_all(splash_container, 0, 0);
  lv_obj_clear_flag(splash_container, LV_OBJ_FLAG_SCROLLABLE);
  
  // Create a canvas for the bitmap logo
  const int SCALE = 2;
  const int displayWidth = SPLASH_WIDTH * SCALE;
  const int displayHeight = SPLASH_HEIGHT * SCALE;
  const int startX = std::max(0, (DISPLAY_WIDTH - displayWidth) / 2);
  const int startY = std::max(0, (DISPLAY_HEIGHT - displayHeight) / 2 - SCALE_Y(10));
  
  // Allocate canvas buffer from heap (RGB565 format requires 2 bytes per pixel)
  size_t buf_size = displayWidth * displayHeight * sizeof(lv_color_t);
  lv_color_t *canvas_buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  if (!canvas_buf) {
    // Fallback: try regular heap if DMA heap allocation fails
    canvas_buf = (lv_color_t*)malloc(buf_size);
  }
  
  if (!canvas_buf) {
    // If allocation fails, skip bitmap rendering and just show text
    lv_obj_delete(splash_container);
    return;
  }
  
  lv_obj_t *canvas = lv_canvas_create(splash_container);
  lv_canvas_set_buffer(canvas, canvas_buf, displayWidth, displayHeight, LV_COLOR_FORMAT_RGB565);
  lv_obj_align(canvas, LV_ALIGN_CENTER, 0, -SCALE_Y(10));
  
  // Fill canvas with background color
  lv_canvas_fill_bg(canvas, colorFrom565(THEME_BG), LV_OPA_COVER);
  
  // Draw bitmap to canvas
  lv_color_t fg_color = colorFrom565(THEME_ACCENT);
  for (int srcY = 0; srcY < SPLASH_HEIGHT; ++srcY) {
    for (int srcX = 0; srcX < SPLASH_WIDTH; ++srcX) {
      int pixelIndex = srcY * SPLASH_WIDTH + srcX;
      int byteIndex = pixelIndex / 8;
      int bitIndex = 7 - (pixelIndex % 8);
      
      uint8_t byte = pgm_read_byte(&SPLASH_BITMAP[byteIndex]);
      bool isWhite = (byte & (1 << bitIndex)) != 0;
      
      if (isWhite) {
        for (int dy = 0; dy < SCALE; ++dy) {
          for (int dx = 0; dx < SCALE; ++dx) {
            lv_canvas_set_px(canvas, srcX * SCALE + dx, srcY * SCALE + dy, fg_color, LV_OPA_COVER);
          }
        }
      }
    }
  }
  
  // Title label
  lv_obj_t *title = lv_label_create(splash_container);
  lv_label_set_text(title, "aCYD MIDI");
  lv_obj_set_style_text_color(title, colorFrom565(THEME_TEXT), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_TITLE_Y + SCALE_Y(8));
  
  // Version label
  lv_obj_t *version = lv_label_create(splash_container);
  lv_label_set_text(version, (String("v") + ACYD_MIDI_VERSION).c_str());
  lv_obj_set_style_text_color(version, colorFrom565(THEME_TEXT_DIM), 0);
  lv_obj_set_style_text_font(version, &lv_font_montserrat_20, 0);
  lv_obj_align(version, LV_ALIGN_TOP_MID, 0, HEADER_TITLE_Y + SCALE_Y(36));
  
  // Status message
  String message;
  if (status.length()) {
    message = status;
  } else if (isRemoteDisplayConnected()) {
    message = "WiFi: " + getRemoteDisplayIP();
  } else {
    message = "WiFi: Connecting...";
  }
  
  lv_obj_t *status_label = lv_label_create(splash_container);
  lv_label_set_text(status_label, message.c_str());
  lv_obj_set_style_text_color(status_label, colorFrom565(THEME_TEXT), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -SCALE_Y(32));
  
  // Force LVGL to render the splash screen
  lv_refr_now(lv_display_get_default());
  
  // Wait for specified delay
  delay(delayMs);
  
  // Clean up splash screen and free canvas buffer
  free(canvas_buf);
  lv_obj_delete(splash_container);
}
