#include "app/app_menu.h"

#include "hardware_midi.h"

#include <Arduino.h>
#include <lvgl.h>

#include <algorithm>

#include "app/app_modes.h"
#include "app/app_menu_icons.h"
#include "color_utils.h"
#include "common_definitions.h"
#include "screenshot.h"
#include "ui_elements.h"

namespace {

#define RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

static constexpr uint16_t MENU_COLOR_TL = RGB565(255, 0, 0);    // Red (keys)
static constexpr uint16_t MENU_COLOR_TR = RGB565(255, 255, 0);  // Yellow (drop)
static constexpr uint16_t MENU_COLOR_BL = RGB565(0, 0, 255);    // Blue (raga)
static constexpr uint16_t MENU_COLOR_BR = RGB565(0, 255, 0);    // Green (slink)
static constexpr uint16_t MENU_COLOR_DROP = MENU_COLOR_TR;
static constexpr uint16_t MENU_COLOR_KEYS = MENU_COLOR_TL;
static constexpr uint16_t MENU_COLOR_RAGA = MENU_COLOR_BL;
static constexpr uint16_t MENU_COLOR_SLINK = RGB565(0, 255, 0);

struct MenuTile {
  const char *label;
  MenuIcon icon;
  AppMode mode;
};

static constexpr size_t kMenuCols = 4;
static constexpr size_t kMenuRows = 4;

// Original mode menu tiles (original 16 modes)
static const MenuTile kOriginalMenuTiles[] = {
    {"KEYS", MenuIcon::Keys, KEYBOARD},
    {"BEATS", MenuIcon::Sequencer, SEQUENCER},
    {"ZEN", MenuIcon::Zen, BOUNCING_BALL},
    {"DROP", MenuIcon::Drop, PHYSICS_DROP},
    {"RNG", MenuIcon::Rng, RANDOM_GENERATOR},
    {"XY PAD", MenuIcon::Xy, XY_PAD},
    {"ARP", MenuIcon::Arp, ARPEGGIATOR},
    {"GRID", MenuIcon::Grid, GRID_PIANO},
    {"CHORD", MenuIcon::Chord, AUTO_CHORD},
    {"LFO", MenuIcon::Lfo, LFO},
    {"TB3PO", MenuIcon::Tb3po, TB3PO},
    {"GRIDS", MenuIcon::Grids, GRIDS},
    {"RAGA", MenuIcon::Raga, RAGA},
    {"EUCLID", MenuIcon::Euclid, EUCLID},
    {"MORPH", MenuIcon::Morph, MORPH},
#ifdef ENABLE_M5_8ENCODER
    {"8ENC", MenuIcon::Encoder8, ENCODER_PANEL},
#else
    {"SLINK", MenuIcon::Slink, SLINK},
#endif
};

// Experimental mode menu tiles - Optimized for video production workflows
// Primary: Waaave Pool controller (nanoKONTROL2 emulation) for video mixing
// Secondary: Common performance and generative modes for hybrid workflows
// New: Fractal Echo MIDI effect for complex echo patterns
// Ordering: Most video-relevant modes in top row, generative/utility modes below
// Note: SLINK omitted to reduce cognitive load; available in original mode
static const MenuTile kExperimentalMenuTiles[] = {
    {"WAAAVE", MenuIcon::Waaave, WAAAVE},     // Video controller (priority)
    {"KEYS", MenuIcon::Keys, KEYBOARD},        // Traditional keyboard
    {"BEATS", MenuIcon::Sequencer, SEQUENCER}, // Rhythm programming
    {"ZEN", MenuIcon::Zen, BOUNCING_BALL},     // Generative ambient
    {"DROP", MenuIcon::Drop, PHYSICS_DROP},    // Physics-based gen
    {"RNG", MenuIcon::Rng, RANDOM_GENERATOR},  // Random patterns
    {"XY PAD", MenuIcon::Xy, XY_PAD},          // Real-time XY control
    {"ARP", MenuIcon::Arp, ARPEGGIATOR},       // Arpeggiation
    {"GRID", MenuIcon::Grid, GRID_PIANO},      // Grid layout keyboard
    {"CHORD", MenuIcon::Chord, AUTO_CHORD},    // Chord progressions
    {"LFO", MenuIcon::Lfo, LFO},               // Modulation
    {"TB3PO", MenuIcon::Tb3po, TB3PO},         // Phrase generator
    {"GRIDS", MenuIcon::Grids, GRIDS},         // Multi-layer arp
    {"RAGA", MenuIcon::Raga, RAGA},            // Raga explorer
    {"EUCLID", MenuIcon::Euclid, EUCLID},      // Euclidean rhythms
    {"ECHO", MenuIcon::FractalEcho, FRACTAL_ECHO}, // Fractal echo effect
};

static_assert(sizeof(kOriginalMenuTiles) / sizeof(kOriginalMenuTiles[0]) == kMenuCols * kMenuRows,
              "Original menu tile count must match 4x4 grid.");
static_assert(sizeof(kExperimentalMenuTiles) / sizeof(kExperimentalMenuTiles[0]) == kMenuCols * kMenuRows,
              "Experimental menu tile count must match 4x4 grid.");

struct CaptureEntry {
  AppMode mode;
  const char *label;
};

static const CaptureEntry kCaptureSequence[] = {
    {MENU, "menu"},
    {SETTINGS, "settings"},
    {KEYBOARD, "keys"},
    {SEQUENCER, "sequencer"},
    {BOUNCING_BALL, "zen"},
    {PHYSICS_DROP, "drop"},
    {RANDOM_GENERATOR, "rng"},
    {XY_PAD, "xy_pad"},
    {ARPEGGIATOR, "arp"},
    {GRID_PIANO, "grid"},
    {AUTO_CHORD, "chord"},
    {LFO, "lfo"},
    {TB3PO, "tb3po"},
    {GRIDS, "grids"},
    {RAGA, "raga"},
    {EUCLID, "euclid"},
    {MORPH, "morph"},
    {SLINK, "slink"},
    {WAAAVE, "waaave"},
#ifdef ENABLE_M5_8ENCODER
    {ENCODER_PANEL, "encoder_panel"},
#endif
};

static void drawSettingsCog() {
  int cx = BACK_BUTTON_X + BACK_BUTTON_W / 2;
  int cy = BACK_BUTTON_Y + BACK_BUTTON_H / 2;
  int radius = SCALE_X(9);
  int toothHalf = SCALE_X(2);
  int toothLen = SCALE_X(5);
  tft.drawCircle(cx, cy, radius, THEME_TEXT);
  tft.fillCircle(cx, cy, SCALE_X(3), THEME_SURFACE);

  tft.fillRect(cx - toothHalf, cy - radius - SCALE_Y(2), SCALE_X(4), toothLen, THEME_TEXT);
  tft.fillRect(cx - toothHalf, cy + radius - SCALE_Y(3), SCALE_X(4), toothLen, THEME_TEXT);
  tft.fillRect(cx - radius - SCALE_X(2), cy - toothHalf, toothLen, SCALE_X(4), THEME_TEXT);
  tft.fillRect(cx + radius - SCALE_X(2), cy - toothHalf, toothLen, SCALE_X(4), THEME_TEXT);
}

static void drawMenuTile(int x, int y, int w, int h, const MenuTile &tile, uint16_t accent) {
  uint16_t bgColor = accent;
  uint16_t borderColor = blendColor(accent, THEME_BG, 150);
  uint16_t innerBorderColor = blendColor(borderColor, THEME_BG, 80);
  tft.fillRoundRect(x, y, w, h, 10, bgColor);
  tft.drawRoundRect(x, y, w, h, 10, borderColor);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, innerBorderColor);
  int minDim = std::min(w, h);
  int iconSize = std::max(SCALE_X(12), minDim - SCALE_X(18));
  int iconX = x + w / 2;
  int iconY = y + h / 2 - SCALE_Y(4);
  drawMenuIcon(iconX, iconY, iconSize, tile.icon, accent);
  tft.setTextColor(THEME_BG, bgColor);
  tft.drawCentreString(tile.label, iconX, y + h - SCALE_Y(12), 0);
}

}  // namespace

void captureAllScreenshots() {
  AppMode previousMode = currentMode;
#if DEBUG_ENABLED
  Serial.println("Capturing all screens to SD...");
#endif

  for (const CaptureEntry &entry : kCaptureSequence) {
    switchMode(entry.mode);
    requestRedraw();
    for (int i = 0; i < 5; ++i) {
      lv_timer_handler();
      delay(25);
    }
    takeScreenshot(entry.label);
    delay(100);
  }

  switchMode(previousMode);
  requestRedraw();

#if DEBUG_ENABLED
  Serial.println("Screen capture complete.");
#endif
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  drawHeader("aCYD MIDI", "", 5, false);
  drawSettingsCog();
  
  // Select the appropriate tile array based on menu mode
  const MenuTile* activeMenuTiles = (currentMenuMode == MENU_EXPERIMENTAL) ? kExperimentalMenuTiles : kOriginalMenuTiles;
  
  const int gapX = SCALE_X(6);
  const int gapY = SCALE_Y(4);
  const int tileW = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - ((int)kMenuCols - 1) * gapX) / kMenuCols;
  const int tileH = SCALE_Y(40);
  const int startX = MARGIN_SMALL;
  const int startY = HEADER_HEIGHT + SCALE_Y(6);
  for (size_t i = 0; i < kMenuCols * kMenuRows; ++i) {
    int col = i % kMenuCols;
    int row = i / kMenuCols;
    int x = startX + col * (tileW + gapX);
    int y = startY + row * (tileH + gapY);
    uint8_t fx = (kMenuCols > 1) ? (uint8_t)((255 * col) / (kMenuCols - 1)) : 0;
    uint8_t fy = (kMenuRows > 1) ? (uint8_t)((255 * row) / (kMenuRows - 1)) : 0;
    uint16_t topBlend = blendColor(MENU_COLOR_TL, MENU_COLOR_TR, fx);
    uint16_t bottomBlend = blendColor(MENU_COLOR_BL, MENU_COLOR_BR, fx);
    uint16_t accent = blendColor(topBlend, bottomBlend, fy);
    switch (activeMenuTiles[i].icon) {
      case MenuIcon::Keys:
        accent = MENU_COLOR_KEYS;
        break;
      case MenuIcon::Drop:
        accent = MENU_COLOR_DROP;
        break;
      case MenuIcon::Raga:
        accent = MENU_COLOR_RAGA;
        break;
      case MenuIcon::Slink:
        accent = MENU_COLOR_SLINK;
        break;
      default:
        break;
    }
    drawMenuTile(x, y, tileW, tileH, activeMenuTiles[i], accent);
  }
}

void handleMenu() {
  static const uint32_t kBackHoldDurationMs = 1500;
  static uint32_t backHoldStart = 0;
  static bool backHoldTriggered = false;
  static bool backTouchActive = false;

  bool backPressed = touch.isPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H);
  if (touch.justPressed && backPressed) {
    backTouchActive = true;
    backHoldStart = millis();
    backHoldTriggered = false;
  }
  if (backTouchActive && backPressed && !backHoldTriggered &&
      (millis() - backHoldStart >= kBackHoldDurationMs)) {
    backHoldTriggered = true;
    captureAllScreenshots();
    backTouchActive = false;
  }
  if (touch.justReleased) {
    if (backTouchActive && !backHoldTriggered &&
        isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
      switchMode(SETTINGS);
      backTouchActive = false;
      backHoldStart = 0;
      backHoldTriggered = false;
      return;
    }
    backTouchActive = false;
    backHoldStart = 0;
    backHoldTriggered = false;
  }

  if (!touch.justPressed) {
    return;
  }

  const int gapX = SCALE_X(6);
  const int gapY = SCALE_Y(4);
  const int tileW = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - ((int)kMenuCols - 1) * gapX) / kMenuCols;
  const int tileH = SCALE_Y(40);
  const int startX = MARGIN_SMALL;
  const int startY = HEADER_HEIGHT + SCALE_Y(6);

  // Select the appropriate tile array based on menu mode
  const MenuTile* activeMenuTiles = (currentMenuMode == MENU_EXPERIMENTAL) ? kExperimentalMenuTiles : kOriginalMenuTiles;

  for (size_t i = 0; i < kMenuCols * kMenuRows; ++i) {
    int col = i % kMenuCols;
    int row = i / kMenuCols;
    int x = startX + col * (tileW + gapX);
    int y = startY + row * (tileH + gapY);
    if (isButtonPressed(x, y, tileW, tileH)) {
      const MenuTile &tile = activeMenuTiles[i];
      switchMode(tile.mode);
      return;
    }
  }
}
