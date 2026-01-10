#ifndef SMARTDISPLAY_COMPAT_H
#define SMARTDISPLAY_COMPAT_H

#include <Arduino.h>
#include <lvgl.h>

class TFT_eSPI {
public:
  TFT_eSPI() = default;

  void begin() { init(); }
  void init() {}

  void setRotation(uint8_t rotation) {
    lv_display_t *display = lv_display_get_default();
    if (!display) {
      return;
    }
    switch (rotation & 3) {
      case 0:
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
        break;
      case 1:
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        break;
      case 2:
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
        break;
      default:
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
        break;
    }
  }

  int16_t width() const { return width_; }
  int16_t height() const { return height_; }
  bool isReady() const { return layer_ != nullptr; }

  void setLayer(lv_layer_t *layer, int16_t width, int16_t height) {
    layer_ = layer;
    width_ = width;
    height_ = height;
  }

  void setTextColor(uint16_t fg, uint16_t bg) {
    text_color_ = colorFrom565_(fg);
    text_bg_color_ = colorFrom565_(bg);
  }

  void fillScreen(uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = colorFrom565_(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_opa = LV_OPA_TRANSP;
    dsc.radius = 0;
    lv_area_t coords = {0, 0, static_cast<lv_coord_t>(width_ - 1), static_cast<lv_coord_t>(height_ - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = colorFrom565_(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_opa = LV_OPA_TRANSP;
    dsc.radius = 0;
    lv_area_t coords = {x, y, static_cast<lv_coord_t>(x + w - 1), static_cast<lv_coord_t>(y + h - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_color = colorFrom565_(color);
    dsc.border_opa = LV_OPA_COVER;
    dsc.border_width = 1;
    dsc.radius = 0;
    lv_area_t coords = {x, y, static_cast<lv_coord_t>(x + w - 1), static_cast<lv_coord_t>(y + h - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = colorFrom565_(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_opa = LV_OPA_TRANSP;
    dsc.radius = r;
    lv_area_t coords = {x, y, static_cast<lv_coord_t>(x + w - 1), static_cast<lv_coord_t>(y + h - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_color = colorFrom565_(color);
    dsc.border_opa = LV_OPA_COVER;
    dsc.border_width = 1;
    dsc.radius = r;
    lv_area_t coords = {x, y, static_cast<lv_coord_t>(x + w - 1), static_cast<lv_coord_t>(y + h - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = colorFrom565_(color);
    dsc.width = 1;
    dsc.opa = LV_OPA_COVER;
    dsc.p1 = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
    dsc.p2 = {static_cast<lv_coord_t>(x + w - 1), static_cast<lv_coord_t>(y)};
    lv_draw_line(layer_, &dsc);
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = colorFrom565_(color);
    dsc.width = 1;
    dsc.opa = LV_OPA_COVER;
    dsc.p1 = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
    dsc.p2 = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y + h - 1)};
    lv_draw_line(layer_, &dsc);
  }

  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = colorFrom565_(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_opa = LV_OPA_TRANSP;
    dsc.radius = LV_RADIUS_CIRCLE;
    lv_area_t coords = {static_cast<lv_coord_t>(x - r), static_cast<lv_coord_t>(y - r),
                        static_cast<lv_coord_t>(x + r - 1), static_cast<lv_coord_t>(y + r - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!layer_) {
      return;
    }
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_color = colorFrom565_(color);
    dsc.border_opa = LV_OPA_COVER;
    dsc.border_width = 1;
    dsc.radius = LV_RADIUS_CIRCLE;
    lv_area_t coords = {static_cast<lv_coord_t>(x - r), static_cast<lv_coord_t>(y - r),
                        static_cast<lv_coord_t>(x + r - 1), static_cast<lv_coord_t>(y + r - 1)};
    lv_draw_rect(layer_, &dsc, &coords);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) {
    fillRect(x, y, 1, 1, color);
  }

  void drawString(const String &text, int16_t x, int16_t y, uint8_t font) {
    drawText_(text.c_str(), x, y, font, false);
  }

  void drawCentreString(const String &text, int16_t x, int16_t y, uint8_t font) {
    drawText_(text.c_str(), x, y, font, true);
  }

private:
  void drawText_(const char *text, int16_t x, int16_t y, uint8_t font, bool centered) {
    if (!layer_ || !text) {
      return;
    }
    const lv_font_t *font_ptr = fontFor_(font);
    lv_point_t size;
    lv_text_get_size(&size, text, font_ptr, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (size.x <= 0 || size.y <= 0) {
      return;
    }
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = text;
    dsc.color = text_color_;
    dsc.font = font_ptr;
    dsc.opa = LV_OPA_COVER;
    lv_coord_t draw_x = x;
    lv_coord_t draw_y = y;
    if (centered) {
      draw_x = x - size.x / 2;
      draw_y = y - size.y / 2;
    }
    lv_draw_rect_dsc_t bg_dsc;
    lv_draw_rect_dsc_init(&bg_dsc);
    bg_dsc.bg_color = text_bg_color_;
    bg_dsc.bg_opa = LV_OPA_COVER;
    bg_dsc.border_opa = LV_OPA_TRANSP;
    bg_dsc.radius = 0;
    lv_area_t bg_coords = {draw_x, draw_y,
                           static_cast<lv_coord_t>(draw_x + size.x - 1),
                           static_cast<lv_coord_t>(draw_y + size.y - 1)};
    lv_draw_rect(layer_, &bg_dsc, &bg_coords);
    lv_draw_label(layer_, &dsc, &bg_coords);
  }

  static lv_color_t colorFrom565_(uint16_t color) {
    uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (color & 0x1F) * 255 / 31;
    return lv_color_make(r, g, b);
  }

  static const lv_font_t *fontFor_(uint8_t font) {
    switch (font) {
      case 4:
        return &lv_font_montserrat_32;
      case 2:
        return &lv_font_montserrat_20;
      default:
        return &lv_font_montserrat_14;
    }
  }

  lv_layer_t *layer_ = nullptr;
  int16_t width_ = 0;
  int16_t height_ = 0;
  lv_color_t text_color_ = lv_color_white();
  lv_color_t text_bg_color_ = lv_color_black();
};

#endif
