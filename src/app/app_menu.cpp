#include "app/app_menu.h"

#include "hardware_midi.h"

#include <Arduino.h>
#include <lvgl.h>

#include <algorithm>
#include <cctype>

#include "app/app_modes.h"
#include "app/app_menu_icons.h"
#include "color_utils.h"
#include "common_definitions.h"
#include "screenshot.h"
#include "ui_elements.h"
#include "module_settings_mode.h"
#include "module_waaave_mode.h"
#include "module_slink_mode.h"
#ifdef ENABLE_M5_8ENCODER
#include "module_encoder_panel_mode.h"
#endif

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
    {"SLINK", MenuIcon::Slink, SLINK},
};

// Experimental mode menu tiles
// Focus: experimental/newer modes only (Baby8, 8Encoder, Waaave, Fractal, Dimensions)
// Rest are empty placeholders for future expansion
static const MenuTile kExperimentalMenuTiles[] = {
#ifdef ENABLE_BABY8_EMU
  {"BABY8", MenuIcon::Baby8, BABY8},
#else
  {"", MenuIcon::Empty, MENU},
#endif
#ifdef ENABLE_M5_8ENCODER
  {"8ENC", MenuIcon::Encoder8, ENCODER_PANEL},
#else
  {"", MenuIcon::Empty, MENU},
#endif
  {"WAAAVE", MenuIcon::Waaave, WAAAVE},
  {"FRACTAL", MenuIcon::FractalEcho, FRACTAL_ECHO},
  {"DIMS", MenuIcon::Dimensions, DIMENSIONS},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
  {"", MenuIcon::Empty, MENU},
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
#ifdef ENABLE_BABY8_EMU
    {BABY8, "baby8"},
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
  // Empty tiles are drawn as blacked-out placeholders
  if (tile.icon == MenuIcon::Empty) {
    uint16_t darkBg = THEME_BG;
    uint16_t darkBorder = blendColor(THEME_SURFACE, THEME_BG, 50);
    tft.fillRoundRect(x, y, w, h, 10, darkBg);
    tft.drawRoundRect(x, y, w, h, 10, darkBorder);
    return;
  }
  
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

// Helper function to wait and render
static void waitAndRender(int delayMs = 5000) {
  // Process LVGL updates multiple times to ensure rendering is complete
  for (int i = 0; i < 10; ++i) {
    lv_timer_handler();
    delay(25);
  }
  // Additional delay for visual settling
  delay(delayMs);
}

void captureAllScreenshots() {
  AppMode previousMode = currentMode;
#if DEBUG_ENABLED
  Serial.println("Capturing all screens to SD...");
#endif

  // Vector to store documentation entries
  static const int MAX_DOCS = 60;
  const char* documentation[MAX_DOCS];
  int docCount = 0;

  // 1. Main menu
  switchMode(MENU);
  requestRedraw();
  waitAndRender();
  takeScreenshot("menu");
  documentation[docCount++] = "menu - Main menu with all mode tiles";

  // 2. Settings menu (with scrolling)
  switchMode(SETTINGS);
  requestRedraw();
  
  int settingsMaxScroll = getSettingsMaxScroll();
  if (settingsMaxScroll > 0) {
    // Capture top of settings
    setSettingsScrollOffset(0);
    requestRedraw();
    waitAndRender();
    takeScreenshot("settings_top");
    documentation[docCount++] = "settings_top - Settings menu (top section)";
    
    // Capture middle of settings if there's significant scroll
    if (settingsMaxScroll > 100) {
      setSettingsScrollOffset(settingsMaxScroll / 2);
      requestRedraw();
      waitAndRender();
      takeScreenshot("settings_middle");
      documentation[docCount++] = "settings_middle - Settings menu (middle section)";
    }
    
    // Capture bottom of settings
    setSettingsScrollOffset(settingsMaxScroll);
    requestRedraw();
    waitAndRender();
    takeScreenshot("settings_bottom");
    documentation[docCount++] = "settings_bottom - Settings menu (bottom section)";
    
    // Reset scroll
    setSettingsScrollOffset(0);
  } else {
    // Settings fit on one screen
    waitAndRender();
    takeScreenshot("settings");
    documentation[docCount++] = "settings - Settings menu";
  }

  // 3. Individual modules (single-screen modes)
  struct SimpleMode {
    AppMode mode;
    const char *label;
    const char *description;
  };

  const SimpleMode simpleModes[] = {
    {KEYBOARD, "keys", "Keyboard - Piano keyboard interface"},
    {SEQUENCER, "sequencer", "Sequencer - Step sequencer"},
    {BOUNCING_BALL, "zen", "Zen - Bouncing ball generative music"},
    {PHYSICS_DROP, "drop", "Drop - Physics-based note generator"},
    {RANDOM_GENERATOR, "rng", "RNG - Random note generator"},
    {XY_PAD, "xy_pad", "XY Pad - Two-axis MIDI controller"},
    {ARPEGGIATOR, "arp", "Arpeggiator - Note arpeggiator"},
    {GRID_PIANO, "grid", "Grid - Grid-based piano"},
    {AUTO_CHORD, "chord", "Chord - Automatic chord generator"},
    {LFO, "lfo", "LFO - Low-frequency oscillator MIDI controller"},
    {TB3PO, "tb3po", "TB3PO - TB-303 style sequencer"},
    {GRIDS, "grids", "Grids - Euclidean rhythm generator"},
    {RAGA, "raga", "Raga - Indian raga generator"},
    {EUCLID, "euclid", "Euclid - Euclidean rhythm sequencer"},
    {MORPH, "morph", "Morph - Morphing pattern generator"},
    {FRACTAL_ECHO, "fractal", "Fractal Echo - Fractal-based music generator"},
    {DIMENSIONS, "dimensions", "Dimensions - Multi-dimensional parameter space"},
#ifdef ENABLE_BABY8_EMU
    {BABY8, "baby8", "Baby8 - Vintage computer emulator music mode"},
#endif
  };

  for (const SimpleMode &mode : simpleModes) {
    switchMode(mode.mode);
    requestRedraw();
    waitAndRender();
    takeScreenshot(mode.label);
    documentation[docCount++] = mode.description;
  }

  // 4. SLINK mode (7 tabs)
  if (slink_state_ptr) {
    const char* slinkTabs[] = {
      "MAIN", "TRIGGER", "PITCH", "CLOCK", "SCALE", "MOD", "SETUP"
    };
    const char* slinkDesc[] = {
      "slink_main - SLINK Wave Engine (Main tab)",
      "slink_trigger - SLINK Wave Engine (Trigger tab)",
      "slink_pitch - SLINK Wave Engine (Pitch tab)",
      "slink_clock - SLINK Wave Engine (Clock tab)",
      "slink_scale - SLINK Wave Engine (Scale tab)",
      "slink_mod - SLINK Wave Engine (Modulation tab)",
      "slink_setup - SLINK Wave Engine (Setup tab)"
    };

    for (int tab = 0; tab < 7; tab++) {
      switchMode(SLINK);
      slink_state_ptr->current_tab = static_cast<SlinkTab>(tab);
      requestRedraw();
      waitAndRender();
      
      char label[32];
      int len = snprintf(label, sizeof(label), "slink_%s", slinkTabs[tab]);
      // Convert to lowercase
      for (int i = 0; i < len && i < (int)sizeof(label) - 1; i++) {
        label[i] = tolower((unsigned char)label[i]);
      }
      takeScreenshot(label);
      documentation[docCount++] = slinkDesc[tab];
    }
  }

  // 5. WAAAVE mode (3 pages)
  const char* waaveDesc[] = {
    "waaave_transport - WAAAVE Pool (Transport page)",
    "waaave_controls_1_4 - WAAAVE Pool (Controls channels 1-4)",
    "waaave_controls_5_8 - WAAAVE Pool (Controls channels 5-8)"
  };
  
  int numWaavePages = getWaaaveNumPages();
  for (int page = 0; page < numWaavePages; page++) {
    switchMode(WAAAVE);
    setWaaavePage(page);
    requestRedraw();
    waitAndRender();
    
    char label[32];
    if (page == 0) {
      snprintf(label, sizeof(label), "waaave_transport");
    } else {
      snprintf(label, sizeof(label), "waaave_controls_%d_%d", (page - 1) * 4 + 1, page * 4);
    }
    takeScreenshot(label);
    documentation[docCount++] = waaveDesc[page];
  }

#ifdef ENABLE_M5_8ENCODER
  // 6. Encoder Panel mode (3 pages)
  const char* encoderDesc[] = {
    "encoder_panel_page1 - 8 Encoder Panel (MIDI CC 1-8)",
    "encoder_panel_page2 - 8 Encoder Panel (MIDI CC 9-16)",
    "encoder_panel_page3 - 8 Encoder Panel (MIDI CC 17-24)"
  };

  for (int page = 0; page < 3; page++) {
    switchMode(ENCODER_PANEL);
    currentEncoderPage = page;
    requestRedraw();
    waitAndRender();
    
    char label[32];
    snprintf(label, sizeof(label), "encoder_panel_page%d", page + 1);
    takeScreenshot(label);
    documentation[docCount++] = encoderDesc[page];
  }
#endif

  // Write documentation file
  writeScreenshotDocumentation(documentation, docCount);

  // Restore previous mode
  switchMode(previousMode);
  requestRedraw();

#if DEBUG_ENABLED
  Serial.printf("Screen capture complete. Captured %d screenshots.\n", docCount);
#endif
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  drawHeader("aCYD MIDI", "", 5, false);
  drawSettingsCog();
  int dividerX = BACK_BUTTON_X + BACK_BUTTON_W + SCALE_X(8);
  tft.drawFastVLine(dividerX, SCALE_Y(5), HEADER_HEIGHT - SCALE_Y(10), THEME_PRIMARY);
  
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
