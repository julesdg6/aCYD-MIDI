#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>
#include <esp32_smartdisplay.h>
#include <esp_bt.h>
#include <lvgl.h>
#include <esp32-hal-psram.h>
#include <algorithm>
#include <cmath>
#include <esp_task_wdt.h>
#include "esp_log.h"

#include "module_arpeggiator_mode.h"
#include "module_auto_chord_mode.h"
#include "module_bouncing_ball_mode.h"
#include "common_definitions.h"
#include "splash_screen.h"
#include "module_euclidean_mode.h"
#include "module_grid_piano_mode.h"
#include "hardware_midi.h"
#include "module_keyboard_mode.h"
#include "module_lfo_mode.h"
#include "midi_utils.h"
#include "module_morph_mode.h"
#include "module_physics_drop_mode.h"
#include "module_random_generator_mode.h"
#include "remote_display.h"
#include "wifi_manager.h"
#include "module_settings_mode.h"
#include "module_raga_mode.h"
#include "module_sequencer_mode.h"
#include "screenshot.h"
#include "module_slink_mode.h"
#include "module_tb3po_mode.h"
#include "ui_elements.h"
#include "module_xy_pad_mode.h"
#include "module_grids_mode.h"
#include "esp_now_midi_module.h"

static uint32_t lv_last_tick = 0;
static lv_obj_t *render_obj = nullptr;
static bool ble_initialized = false;
static uint32_t ble_init_start_ms = 0;

TFT_eSPI tft;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
volatile bool ble_request_redraw = false;
volatile bool ble_disconnect_action = false;
uint8_t midiPacket[] = {0x80, 0x80, 0, 0, 0};
TouchState touch;
AppMode currentMode = MENU;
volatile bool needsRedraw = false;
uint16_t sharedBPM = 120;
MidiClockMaster midiClockMaster = CLOCK_INTERNAL;

#define RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

static constexpr uint16_t MENU_COLOR_TL = RGB565(255, 0, 0);    // Red (keys)
static constexpr uint16_t MENU_COLOR_TR = RGB565(255, 255, 0);  // Yellow (drop)
static constexpr uint16_t MENU_COLOR_BL = RGB565(0, 0, 255);    // Blue (raga)
static constexpr uint16_t MENU_COLOR_BR = RGB565(0, 255, 0);    // Green (slink)
static constexpr uint16_t MENU_COLOR_DROP = MENU_COLOR_TR;
static constexpr uint16_t MENU_COLOR_KEYS = MENU_COLOR_TL;
static constexpr uint16_t MENU_COLOR_RAGA = MENU_COLOR_BL;
static constexpr uint16_t MENU_COLOR_SLINK = RGB565(0, 255, 0);

enum MenuIcon {
  ICON_KEYS,
  ICON_SEQUENCER,
  ICON_ZEN,
  ICON_DROP,
  ICON_RNG,
  ICON_XY,
  ICON_ARP,
  ICON_GRID,
  ICON_CHORD,
  ICON_LFO,
  ICON_SLINK,
  ICON_TB3PO,
  ICON_GRIDS,
  ICON_RAGA,
  ICON_EUCLID,
  ICON_MORPH,
};

struct MenuTile {
  const char *label;
  MenuIcon icon;
  AppMode mode;
};

static constexpr size_t kMenuCols = 4;
static constexpr size_t kMenuRows = 4;

static const MenuTile kMenuTiles[] = {
    {"KEYS", ICON_KEYS, KEYBOARD},
    {"BEATS", ICON_SEQUENCER, SEQUENCER},
    {"ZEN", ICON_ZEN, BOUNCING_BALL},
    {"DROP", ICON_DROP, PHYSICS_DROP},
    {"RNG", ICON_RNG, RANDOM_GENERATOR},
    {"XY PAD", ICON_XY, XY_PAD},
    {"ARP", ICON_ARP, ARPEGGIATOR},
    {"GRID", ICON_GRID, GRID_PIANO},
    {"CHORD", ICON_CHORD, AUTO_CHORD},
    {"LFO", ICON_LFO, LFO},
    {"TB3PO", ICON_TB3PO, TB3PO},
    {"GRIDS", ICON_GRIDS, GRIDS},
    {"RAGA", ICON_RAGA, RAGA},
    {"EUCLID", ICON_EUCLID, EUCLID},
    {"MORPH", ICON_MORPH, MORPH},
    {"SLINK", ICON_SLINK, SLINK},
};

static_assert(sizeof(kMenuTiles) / sizeof(kMenuTiles[0]) == kMenuCols * kMenuRows,
              "Menu tile count must match 4x4 grid.");

struct CaptureEntry {
  AppMode mode;
  const char *label;
};

static const CaptureEntry kCaptureSequence[] = {
    {MENU, "menu"},
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
};

// Display configuration for autoscaling
DisplayConfig displayConfig;
// UART2 instance for hardware MIDI (only used when HARDWARE_MIDI_UART == 2)
// This definition matches the extern declaration in hardware_midi.h
#if HARDWARE_MIDI_UART == 2
HardwareSerial MIDISerial(2);
#endif

// Forward declarations for functions used before their definitions
void switchMode(AppMode mode);
void requestRedraw();

class MIDICallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    deviceConnected = true;
    Serial.println("BLE connected");
    if (currentMode == MENU) {
      ble_request_redraw = true;
    }
  }

  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    Serial.println("BLE disconnected - sending All Notes Off");
    // Defer heavy disconnect handling to main loop to avoid doing work
    // inside the BLE callback/task context.
    ble_disconnect_action = true;
  }
};

void setupBLE() {
  static bool bt_mem_released = false;
  if (!bt_mem_released) {
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    bt_mem_released = true;
  }
  BLEDevice::init("aCYD MIDI");
  Serial.println("Configuring BLE security...");
  // Configure BLE security for "Just Works" pairing (no PIN/passkey) and
  // also provide a static PIN for clients that require one.
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setCapability(0x03); // IO_CAPS_NONE
  pSecurity->setStaticPIN(123456);
  Serial.println("BLESecurity: IO_CAPS_NONE, static PIN=123456 set");

  // Register security callbacks to log authentication events
  class MyBLESecurityCallbacks : public BLESecurityCallbacks {
  public:
    uint32_t onPassKeyRequest() override {
      Serial.println("BLESecurityCallbacks: onPassKeyRequest()");
      return 0;
    }
    void onPassKeyNotify(uint32_t pass_key) override {
      Serial.printf("BLESecurityCallbacks: onPassKeyNotify: %06u\n", pass_key);
    }
    bool onConfirmPIN(uint32_t pass_key) override {
      Serial.printf("BLESecurityCallbacks: onConfirmPIN: %06u\n", pass_key);
      return true;
    }
    bool onSecurityRequest() override {
      Serial.println("BLESecurityCallbacks: onSecurityRequest()");
      return true;
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t) override {
      Serial.println("BLESecurityCallbacks: onAuthenticationComplete()");
    }
  };
  BLEDevice::setSecurityCallbacks(new MyBLESecurityCallbacks());
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MIDICallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  pCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR |
      BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising initialized for aCYD MIDI");
}

inline uint16_t blendColor(uint16_t from, uint16_t to, uint8_t ratio) {
  int rf = (from >> 11) & 0x1F;
  int gf = (from >> 5) & 0x3F;
  int bf = from & 0x1F;
  int rt = (to >> 11) & 0x1F;
  int gt = (to >> 5) & 0x3F;
  int bt = to & 0x1F;
  int r = ((rf * (255 - ratio)) + rt * ratio) / 255;
  int g = ((gf * (255 - ratio)) + gt * ratio) / 255;
  int b = ((bf * (255 - ratio)) + bt * ratio) / 255;
  return (r << 11) | (g << 5) | b;
}

static void fillTriangleImpl(TFT_eSPI &tft, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  struct Pt { int x; int y; };
  Pt p[3] = {{x0, y0}, {x1, y1}, {x2, y2}};
  if (p[0].y > p[1].y) std::swap(p[0], p[1]);
  if (p[0].y > p[2].y) std::swap(p[0], p[2]);
  if (p[1].y > p[2].y) std::swap(p[1], p[2]);

  auto interp = [](Pt a, Pt b, int y) -> int {
    if (b.y == a.y) return a.x;
    return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
  };

  for (int y = p[0].y; y <= p[2].y; ++y) {
    int xa, xb;
    if (y <= p[1].y && p[1].y != p[0].y) {
      xa = interp(p[0], p[2], y);
      xb = interp(p[0], p[1], y);
    } else if (p[2].y != p[1].y) {
      xa = interp(p[0], p[2], y);
      xb = interp(p[1], p[2], y);
    } else {
      xa = interp(p[0], p[2], y);
      xb = xa;
    }
    if (xa > xb) std::swap(xa, xb);
    tft.drawFastHLine(xa, y, xb - xa + 1, color);
  }
}

void drawMenuIcon(int cx, int cy, int size, MenuIcon icon, uint16_t accent) {
  const uint16_t fg = THEME_SURFACE;
  switch (icon) {
    case ICON_KEYS: {
      int keyWidth = std::max(5, size / 5);
      int keyHeight = std::max(10, size / 2);
      int startX = cx - (5 * keyWidth) / 2;
      int topY = cy - keyHeight / 2;
      for (int i = 0; i < 5; ++i) {
        int x = startX + i * keyWidth;
        int width = std::max(2, keyWidth - 2);
        tft.fillRoundRect(x, topY, width, keyHeight, 2, fg);
        tft.drawRoundRect(x, topY, width, keyHeight, 2, accent);
      }
      break;
    }
    case ICON_SEQUENCER: {
      int block = std::max(4, size / 5);
      int gridSize = block * 3;
      int startX = cx - gridSize / 2;
      int startY = cy - gridSize / 2;
      for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
          int x = startX + col * block;
          int y = startY + row * block;
          tft.fillRect(x, y, std::max(2, block - 2), std::max(2, block - 2), accent);
        }
      }
      break;
    }
    case ICON_ZEN: {
      int radius = std::max(6, size / 3);
      tft.drawCircle(cx, cy, radius, accent);
      tft.drawCircle(cx, cy, std::max(3, radius / 2), accent);
      tft.fillCircle(cx, cy, 2, accent);
      break;
    }
    case ICON_DROP: {
      int radius = std::max(5, size / 4);
      int circleY = cy - radius / 2;
      tft.fillCircle(cx, circleY, radius, accent);
      int tipY = circleY + radius;
      fillTriangleImpl(tft, cx - radius, tipY, cx + radius, tipY, cx, tipY + radius + 2, accent);
      break;
    }
    case ICON_RNG: {
      int startX = cx - size / 2;
      int startY = cy + size / 4;
      int step = std::max(4, size / 4);
      int x = startX;
      int y = startY;
      for (int i = 0; i < 3; ++i) {
        int nextX = x + step;
        int nextY = (i % 2 == 0) ? cy - size / 6 : cy + size / 6;
        tft.drawLine(x, y, nextX, nextY, accent);
        x = nextX;
        y = nextY;
      }
      tft.drawLine(x, y, x + step, cy - size / 8, accent);
      break;
    }
    case ICON_XY:
      tft.drawLine(cx - size / 2, cy, cx + size / 2, cy, accent);
      tft.drawLine(cx, cy - size / 2, cx, cy + size / 2, accent);
      tft.fillCircle(cx, cy, 2, accent);
      break;
    case ICON_ARP: {
      int steps = 4;
      int width = std::max(12, size - 6);
      int baseX = cx - width / 2;
      int baseY = cy + size / 4;
      int prevX = baseX;
      int prevY = baseY;
      for (int i = 1; i <= steps; ++i) {
        int nextX = baseX + (width * i) / steps;
        int nextY = baseY - (size * i) / (steps * 3);
        tft.drawLine(prevX, prevY, nextX, nextY, accent);
        prevX = nextX;
        prevY = nextY;
      }
      fillTriangleImpl(tft, prevX, prevY, prevX - 4, prevY + 6, prevX + 4, prevY + 6, accent);
      break;
    }
    case ICON_GRID: {
      int side = std::max(10, size - 6);
      int left = cx - side / 2;
      int topY = cy - side / 2;
      tft.drawRect(left, topY, side, side, accent);
      tft.drawLine(left + side / 2, topY, left + side / 2, topY + side, accent);
      tft.drawLine(left, topY + side / 2, left + side, topY + side / 2, accent);
      break;
    }
    case ICON_CHORD: {
      int spacing = std::max(6, size / 3);
      int startX = cx - spacing;
      int height = std::max(14, size - 8);
      for (int i = 0; i < 3; ++i) {
        int x = startX + i * spacing;
        tft.drawLine(x, cy - height / 2, x, cy + height / 2, accent);
        tft.fillCircle(x, cy - height / 2 + 3, 3, accent);
      }
      break;
    }
    case ICON_LFO: {
      int width = std::max(12, size - 6);
      int startX = cx - width / 2;
      int offsets[5] = {0, -size / 4, 0, size / 4, 0};
      for (int i = 0; i < 4; ++i) {
        int x0 = startX + (width * i) / 4;
        int y0 = cy + offsets[i];
        int x1 = startX + (width * (i + 1)) / 4;
        int y1 = cy + offsets[i + 1];
        tft.drawLine(x0, y0, x1, y1, accent);
      }
      break;
    }
    case ICON_SLINK: {
      int width = std::max(12, size - 6);
      int startX = cx - width / 2;
      int amplitude = std::max(3, size / 5);
      for (int layer = 0; layer < 2; ++layer) {
        int yOffset = layer * 3;
        int prevX = startX;
        int prevY = cy + yOffset;
        for (int i = 1; i <= 4; ++i) {
          int nextX = startX + (width * i) / 4;
          int nextY = cy + yOffset + ((i % 2 == 0) ? -amplitude : amplitude);
          tft.drawLine(prevX, prevY, nextX, nextY, accent);
          prevX = nextX;
          prevY = nextY;
        }
      }
      break;
    }
    case ICON_TB3PO: {
      int rows = 2;
      int cols = 4;
      int cellW = std::max(6, size / (cols * 2));
      int cellH = std::max(4, size / 10);
      int gridW = cols * cellW + (cols - 1) * 2;
      int gridH = rows * cellH + (rows - 1) * 4;
      int startX = cx - gridW / 2;
      int startY = cy - gridH / 2;
      for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
          int x = startX + col * (cellW + 2);
          int y = startY + row * (cellH + 4);
          tft.fillRoundRect(x, y, cellW, cellH, 2, (col + row) % 2 ? accent : fg);
          tft.drawRoundRect(x, y, cellW, cellH, 2, accent);
        }
      }
      break;
    }
    case ICON_GRIDS: {
      int blocks = 3;
      int blockSize = std::max(6, (size - (blocks - 1) * 2) / blocks);
      int gridW = blocks * blockSize + (blocks - 1) * 2;
      int startX = cx - gridW / 2;
      int startY = cy - gridW / 2;
      for (int row = 0; row < blocks; ++row) {
        for (int col = 0; col < blocks; ++col) {
          int x = startX + col * (blockSize + 2);
          int y = startY + row * (blockSize + 2);
          tft.fillRect(x, y, blockSize, blockSize, ((row + col) % 2) ? accent : fg);
          tft.drawRect(x, y, blockSize, blockSize, THEME_BG);
        }
      }
      break;
    }
    case ICON_RAGA: {
      int baseY = cy + size / 4;
      int heights[3] = {size / 5, size / 4, size / 3};
      for (int i = 0; i < 3; ++i) {
        int x = cx - size / 3 + i * (size / 3);
        int height = heights[i];
        tft.drawLine(x, baseY, x, baseY - height, accent);
        tft.fillCircle(x, baseY - height, 3, accent);
      }
      tft.drawCircle(cx, cy - size / 6, size / 5, accent);
      break;
    }
    case ICON_EUCLID: {
      int radius = std::max(10, size / 3);
      tft.drawCircle(cx, cy, radius, accent);
      const float twoPi = 6.2831853f;
      const float startAngle = -1.5707963f;
      const int steps = 8;
      for (int i = 0; i < steps; ++i) {
        float angle = startAngle + (twoPi * i) / steps;
        int markerX = cx + static_cast<int>(std::cos(angle) * radius);
        int markerY = cy + static_cast<int>(std::sin(angle) * radius);
        tft.fillCircle(markerX, markerY, 2, (i % 2 == 0) ? accent : fg);
      }
      break;
    }
    case ICON_MORPH: {
      int width = std::max(18, size - 6);
      int height = std::max(6, size / 4);
      int offset = std::max(4, size / 8);
      int left = cx - width / 2;
      int top = cy - height / 2;
      tft.fillRoundRect(left, top, width, height, 5, fg);
      tft.fillRoundRect(left + offset, top + offset / 2, width - offset * 2, height - offset / 2, 5, accent);
      tft.fillCircle(cx - offset, cy, 3, accent);
      tft.fillCircle(cx + offset, cy, 3, accent);
      break;
    }
    default:
      tft.fillCircle(cx, cy, std::max(3, size / 4), accent);
      break;
  }
}

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

void drawMenuTile(int x, int y, int w, int h, const MenuTile &tile, uint16_t accent) {
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

void drawMenu() {
  tft.fillScreen(THEME_BG);
  drawHeader("aCYD MIDI", "", 5, false);
  drawSettingsCog();
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
    switch (kMenuTiles[i].icon) {
      case ICON_KEYS:
        accent = MENU_COLOR_KEYS;
        break;
      case ICON_DROP:
        accent = MENU_COLOR_DROP;
        break;
      case ICON_RAGA:
        accent = MENU_COLOR_RAGA;
        break;
      case ICON_SLINK:
        accent = MENU_COLOR_SLINK;
        break;
      default:
        break;
    }
    drawMenuTile(x, y, tileW, tileH, kMenuTiles[i], accent);
  }
}

// Forward declarations used by captureAllScreenshots
void switchMode(AppMode mode);
void requestRedraw();

void captureAllScreenshots() {
  AppMode previousMode = currentMode;
  Serial.println("Capturing all screens to SD...");
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
  Serial.println("Screen capture complete.");
}

void requestRedraw() {
  needsRedraw = true;
}

void processRedraw() {
  if (needsRedraw && render_obj) {
    lv_obj_invalidate(render_obj);
    needsRedraw = false;
  }
}

static void render_event(lv_event_t *event) {
  lv_layer_t *layer = lv_event_get_layer(event);
  lv_display_t *display = lv_display_get_default();
  if (!layer || !display) {
    return;
  }
  tft.setLayer(layer,
               lv_display_get_horizontal_resolution(display),
               lv_display_get_vertical_resolution(display));

  switch (currentMode) {
    case MENU:
      drawMenu();
      break;
    case SETTINGS:
      drawSettingsMode();
      break;
    case KEYBOARD:
      drawKeyboardMode();
      break;
    case SEQUENCER:
      drawSequencerMode();
      break;
    case BOUNCING_BALL:
      drawBouncingBallMode();
      break;
    case PHYSICS_DROP:
      drawPhysicsDropMode();
      break;
    case RANDOM_GENERATOR:
      drawRandomGeneratorMode();
      break;
    case XY_PAD:
      drawXYPadMode();
      break;
    case ARPEGGIATOR:
      drawArpeggiatorMode();
      break;
    case GRID_PIANO:
      drawGridPianoMode();
      break;
    case AUTO_CHORD:
      drawAutoChordMode();
      break;
    case LFO:
      drawLFOMode();
      break;
    case SLINK:
      drawSlinkMode();
      break;
    case TB3PO:
      drawTB3POMode();
      break;
    case GRIDS:
      drawGridsMode();
      break;
    case RAGA:
      drawRagaMode();
      break;
    case EUCLID:
      drawEuclideanMode();
      break;
    case MORPH:
      drawMorphMode();
      break;
    default:
      break;
  }
}

void switchMode(AppMode mode) {
  currentMode = mode;
  switch (mode) {
    case MENU:
      stopAllModes();
      break;
    case SETTINGS:
      stopAllModes();
      initializeSettingsMode();
      break;
    case KEYBOARD:
      initializeKeyboardMode();
      break;
    case SEQUENCER:
      initializeSequencerMode();
      break;
    case BOUNCING_BALL:
      initializeBouncingBallMode();
      break;
    case PHYSICS_DROP:
      initializePhysicsDropMode();
      break;
    case RANDOM_GENERATOR:
      initializeRandomGeneratorMode();
      break;
    case XY_PAD:
      initializeXYPadMode();
      break;
    case ARPEGGIATOR:
      initializeArpeggiatorMode();
      break;
    case GRID_PIANO:
      initializeGridPianoMode();
      break;
    case AUTO_CHORD:
      initializeAutoChordMode();
      break;
    case LFO:
      initializeLFOMode();
      break;
    case SLINK:
      initializeSlinkMode();
      break;
    case TB3PO:
      initializeTB3POMode();
      break;
    case GRIDS:
      initializeGridsMode();
      break;
    case RAGA:
      initializeRagaMode();
      break;
    case EUCLID:
      initializeEuclideanMode();
      break;
    case MORPH:
      initializeMorphMode();
      break;
    default:
      break;
  }
  requestRedraw();
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

  for (size_t i = 0; i < kMenuCols * kMenuRows; ++i) {
    int col = i % kMenuCols;
    int row = i / kMenuCols;
    int x = startX + col * (tileW + gapX);
    int y = startY + row * (tileH + gapY);
    if (isButtonPressed(x, y, tileW, tileH)) {
      const MenuTile &tile = kMenuTiles[i];
      switchMode(tile.mode);
      return;
    }
  }
}

void exitToMenu() {
  switchMode(MENU);
}

void initDisplayConfig() {
  lv_display_t *display = lv_display_get_default();
  if (display) {
    displayConfig.width = lv_display_get_horizontal_resolution(display);
    displayConfig.height = lv_display_get_vertical_resolution(display);
    displayConfig.scaleX = (float)displayConfig.width / (float)DISPLAY_REF_WIDTH;
    displayConfig.scaleY = (float)displayConfig.height / (float)DISPLAY_REF_HEIGHT;
    
    Serial.printf("Display Config: %dx%d (scale: %.2fx, %.2fy)\n", 
                  displayConfig.width, displayConfig.height,
                  displayConfig.scaleX, displayConfig.scaleY);
  }
}

void setup() {
  // Initialize USB Serial for debugging (only if not using UART0 for MIDI)
#if DEBUG_ENABLED
  Serial.begin(115200);
  delay(200);
  Serial.println("aCYD MIDI Controller Starting...");
  Serial.printf("Hardware MIDI: %s (UART%d)\n", 
                HARDWARE_MIDI_ENABLED ? "Enabled" : "Disabled",
                HARDWARE_MIDI_UART);
  Serial.printf("PSRAM: found=%s size=%u free=%u\n",
                psramFound() ? "yes" : "no",
                ESP.getPsramSize(),
                ESP.getFreePsram());
  Serial.printf("Heap pre-init: dma_free=%u dma_largest=%u int_free=%u int_largest=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif

  // Increase task watchdog timeout to 10s to allow diagnostic logging
  esp_task_wdt_init(10, true);
  Serial.println("Task WDT timeout set to 10s for diagnostics");

  // Increase ESP log verbosity to capture BT stack debug output for diagnosis
  esp_log_level_set("*", ESP_LOG_DEBUG);
  esp_log_level_set("BT", ESP_LOG_DEBUG);
  Serial.println("ESP log level set to DEBUG for BT stack");

  smartdisplay_init();
#if DEBUG_ENABLED
  Serial.printf("Heap post-init: dma_free=%u dma_largest=%u int_free=%u int_largest=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif
  lv_display_t *display = lv_display_get_default();
  lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
  
  // Initialize display configuration for autoscaling
  initDisplayConfig();

#if DEBUG_ENABLED
  Serial.printf("LVGL buffer pixels: %d\n", LVGL_BUFFER_PIXELS);
#ifdef ILI9341_SPI_CONFIG_PCLK_HZ
  Serial.printf("ILI9341 PCLK Hz: %d\n", ILI9341_SPI_CONFIG_PCLK_HZ);
#endif
#ifdef ILI9341_SPI_CONFIG_TRANS_QUEUE_DEPTH
  Serial.printf("ILI9341 queue depth: %d\n", ILI9341_SPI_CONFIG_TRANS_QUEUE_DEPTH);
#endif
#ifdef ILI9341_SPI_BUS_MAX_TRANSFER_SZ
  Serial.printf("ILI9341 max transfer: %d\n", ILI9341_SPI_BUS_MAX_TRANSFER_SZ);
#endif
#endif
  
  tft.init();
  showSplashScreen("Booting...", 400);
  render_obj = lv_obj_create(lv_screen_active());
  lv_obj_set_size(render_obj,
                  lv_display_get_horizontal_resolution(display),
                  lv_display_get_vertical_resolution(display));
  lv_obj_set_style_bg_opa(render_obj, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(render_obj, render_event, LV_EVENT_DRAW_MAIN, NULL);
  
  ble_init_start_ms = millis();
  initHardwareMIDI();  // Initialize hardware MIDI output
  initWiFi();  // Prepare WiFi (used by remote display and clock master suppliers)

  #if REMOTE_DISPLAY_ENABLED
  initRemoteDisplay();  // Initialize remote display capability
  #endif
  showSplashScreen(String(), 500);
  switchMode(MENU);
  lv_last_tick = millis();
  
#if DEBUG_ENABLED
  Serial.println("Setup complete!");
#endif
}

void loop() {
  uint32_t now = millis();
  lv_tick_inc(now - lv_last_tick);
  lv_last_tick = now;
  lv_timer_handler();

#if BLE_ENABLED
  if (!ble_initialized && (now - ble_init_start_ms) > 5000) {
    setupBLE();
    ble_initialized = true;
    
#if ESP_NOW_ENABLED
    // Initialize ESP-NOW after BLE to avoid conflicts
    // Note: ESP-NOW is disabled by default, enabled via Settings mode
    Serial.println("ESP-NOW MIDI available (disabled by default)");
#endif
  }
#endif

  updateTouch();

#if WIFI_ENABLED
  handleWiFi();
#endif

  // Handle deferred BLE actions set by BLE callbacks (run in main loop)
  if (ble_disconnect_action) {
    ble_disconnect_action = false;
    Serial.println("Handling BLE disconnect in main loop: stopping modes and restarting advertising");
    stopAllModes();
    requestRedraw();
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising restarted for reconnection");
  }
  if (ble_request_redraw) {
    ble_request_redraw = false;
    requestRedraw();
  }

  switch (currentMode) {
    case MENU:
      handleMenu();
      break;
    case SETTINGS:
      handleSettingsMode();
      break;
    case KEYBOARD:
      handleKeyboardMode();
      break;
    case SEQUENCER:
      handleSequencerMode();
      break;
    case BOUNCING_BALL:
      handleBouncingBallMode();
      break;
    case PHYSICS_DROP:
      handlePhysicsDropMode();
      break;
    case RANDOM_GENERATOR:
      handleRandomGeneratorMode();
      break;
    case XY_PAD:
      handleXYPadMode();
      break;
    case ARPEGGIATOR:
      handleArpeggiatorMode();
      break;
    case GRID_PIANO:
      handleGridPianoMode();
      break;
    case AUTO_CHORD:
      handleAutoChordMode();
      break;
    case LFO:
      handleLFOMode();
      break;
    case SLINK:
      handleSlinkMode();
      break;
    case TB3PO:
      handleTB3POMode();
      break;
    case GRIDS:
      handleGridsMode();
      break;
    case RAGA:
      handleRagaMode();
      break;
    case EUCLID:
      handleEuclideanMode();
      break;
    case MORPH:
      handleMorphMode();
      break;
    default:
      break;
  }
  
  // Process any pending redraws after handling logic
  processRedraw();
  
#if REMOTE_DISPLAY_ENABLED
  handleRemoteDisplay();  // Handle remote display updates
#endif
}
