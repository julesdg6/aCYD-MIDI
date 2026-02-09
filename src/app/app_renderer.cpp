#include "app/app_renderer.h"

#include <Arduino.h>
#include <lvgl.h>

#include "app/app_modes.h"
#include "common_definitions.h"

namespace {

static uint32_t lv_last_tick = 0;
static lv_obj_t *render_obj = nullptr;

static void render_event(lv_event_t *event) {
  lv_layer_t *layer = lv_event_get_layer(event);
  lv_display_t *display = lv_display_get_default();
  if (!layer || !display) {
    return;
  }

  tft.setLayer(layer,
               lv_display_get_horizontal_resolution(display),
               lv_display_get_vertical_resolution(display));
  appDrawCurrentMode();
}

}  // namespace

void appRendererInit() {
  lv_display_t *display = lv_display_get_default();
  if (!display) {
    return;
  }

  // Create render object and register event callback BEFORE showing splash screen.
  // This ensures tft.setLayer() is called via render_event before splash screen draws.
  render_obj = lv_obj_create(lv_screen_active());
  lv_obj_set_size(render_obj,
                  lv_display_get_horizontal_resolution(display),
                  lv_display_get_vertical_resolution(display));
  lv_obj_set_style_bg_opa(render_obj, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(render_obj, render_event, LV_EVENT_DRAW_MAIN, NULL);

  // Force initial render to initialize the TFT layer before splash screen.
  lv_obj_invalidate(render_obj);
  lv_refr_now(display);
}

void appRendererLoopTick(uint32_t now) {
  if (lv_last_tick == 0) {
    lv_last_tick = now;
  } else {
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
  }

  lv_timer_handler();
}

void appRendererProcessRedraw() {
  if (needsRedraw && render_obj) {
    lv_obj_invalidate(render_obj);
    needsRedraw = false;
  }
}

