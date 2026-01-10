#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>

#include "arpeggiator_mode.h"
#include "auto_chord_mode.h"
#include "bouncing_ball_mode.h"
#include "common_definitions.h"
#include "grid_piano_mode.h"
#include "keyboard_mode.h"
#include "lfo_mode.h"
#include "midi_utils.h"
#include "physics_drop_mode.h"
#include "random_generator_mode.h"
#include "remote_display.h"
#include "sequencer_mode.h"
#include "ui_elements.h"
#include "xy_pad_mode.h"

static uint32_t lv_last_tick = 0;
static lv_obj_t *render_obj = nullptr;

TFT_eSPI tft;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t midiPacket[] = {0x80, 0x80, 0, 0, 0};
TouchState touch;
AppMode currentMode = MENU;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override { deviceConnected = true; }
  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    server->startAdvertising();
  }
};

struct MenuItem {
  const char *label;
  AppMode mode;
  uint16_t color;
};

static const MenuItem kMenuItems[] = {
  {"KEYS", KEYBOARD, THEME_PRIMARY},
  {"BEATS", SEQUENCER, THEME_ACCENT},
  {"ZEN", BOUNCING_BALL, THEME_SUCCESS},
  {"DROP", PHYSICS_DROP, THEME_WARNING},
  {"RNG", RANDOM_GENERATOR, THEME_SECONDARY},
  {"XY PAD", XY_PAD, THEME_PRIMARY},
  {"ARP", ARPEGGIATOR, THEME_ACCENT},
  {"GRID", GRID_PIANO, THEME_SUCCESS},
  {"CHORD", AUTO_CHORD, THEME_WARNING},
  {"LFO", LFO, THEME_SECONDARY},
};

void setupBLE() {
  BLEDevice::init("aCYD MIDI");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  pCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(false);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  tft.fillRect(0, 0, 320, 45, THEME_SURFACE);
  tft.drawFastHLine(0, 45, 320, THEME_PRIMARY);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString("CYD MIDI", 160, 8, 4);
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawCentreString("Select Mode", 160, 28, 2);

  const int cols = 2;
  const int btnW = 140;
  const int btnH = 28;
  const int gapX = 20;
  const int gapY = 8;
  const int startX = 10;
  const int startY = 55;

  for (size_t i = 0; i < sizeof(kMenuItems) / sizeof(kMenuItems[0]); i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (btnW + gapX);
    int y = startY + row * (btnH + gapY);
    drawRoundButton(x, y, btnW, btnH, kMenuItems[i].label, kMenuItems[i].color);
  }
}

void requestRedraw() {
  if (render_obj) {
    lv_obj_invalidate(render_obj);
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
    default:
      break;
  }
  requestRedraw();
}

void handleMenu() {
  if (!touch.justPressed) {
    return;
  }
  const int cols = 2;
  const int btnW = 140;
  const int btnH = 28;
  const int gapX = 20;
  const int gapY = 8;
  const int startX = 10;
  const int startY = 55;

  for (size_t i = 0; i < sizeof(kMenuItems) / sizeof(kMenuItems[0]); i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (btnW + gapX);
    int y = startY + row * (btnH + gapY);
    if (isButtonPressed(x, y, btnW, btnH)) {
      switchMode(kMenuItems[i].mode);
      return;
    }
  }
}

void exitToMenu() {
  switchMode(MENU);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  smartdisplay_init();
  lv_display_t *display = lv_display_get_default();
  lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
  tft.init();
  render_obj = lv_obj_create(lv_screen_active());
  lv_obj_set_size(render_obj,
                  lv_display_get_horizontal_resolution(display),
                  lv_display_get_vertical_resolution(display));
  lv_obj_set_style_bg_opa(render_obj, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(render_obj, render_event, LV_EVENT_DRAW_MAIN, NULL);
  setupBLE();
#if REMOTE_DISPLAY_ENABLED
  initRemoteDisplay();  // Initialize remote display capability
#endif
  switchMode(MENU);
  lv_last_tick = millis();
}

void loop() {
  uint32_t now = millis();
  lv_tick_inc(now - lv_last_tick);
  lv_last_tick = now;
  lv_timer_handler();

  updateTouch();

  switch (currentMode) {
    case MENU:
      handleMenu();
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
    default:
      break;
  }
  requestRedraw();
#if REMOTE_DISPLAY_ENABLED
  handleRemoteDisplay();  // Handle remote display updates
#endif
}
