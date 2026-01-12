#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp32_smartdisplay.h>
#include <esp_bt.h>
#include <lvgl.h>
#include <esp32-hal-psram.h>

#include "arpeggiator_mode.h"
#include "auto_chord_mode.h"
#include "bouncing_ball_mode.h"
#include "common_definitions.h"
#include "grid_piano_mode.h"
#include "hardware_midi.h"
#include "keyboard_mode.h"
#include "lfo_mode.h"
#include "midi_utils.h"
#include "physics_drop_mode.h"
#include "random_generator_mode.h"
#include "remote_display.h"
#include "sequencer_mode.h"
#include "screenshot.h"
#include "slink_mode.h"
#include "ui_elements.h"
#include "xy_pad_mode.h"

static uint32_t lv_last_tick = 0;
static lv_obj_t *render_obj = nullptr;
static bool ble_initialized = false;
static uint32_t ble_init_start_ms = 0;

TFT_eSPI tft;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t midiPacket[] = {0x80, 0x80, 0, 0, 0};
TouchState touch;
AppMode currentMode = MENU;

// Display configuration for autoscaling
DisplayConfig displayConfig;
// UART2 instance for hardware MIDI (only used when HARDWARE_MIDI_UART == 2)
// This definition matches the extern declaration in hardware_midi.h
#if HARDWARE_MIDI_UART == 2
HardwareSerial MIDISerial(2);
#endif

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
  {"SLINK", SLINK, THEME_ACCENT},
};

void setupBLE() {
  static bool bt_mem_released = false;
  if (!bt_mem_released) {
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    bt_mem_released = true;
  }
  BLEDevice::init("aCYD MIDI");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
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
  advertising->setScanResponse(false);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
}

void drawMenu() {
  tft.fillScreen(THEME_BG);
  tft.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, THEME_SURFACE);
  tft.drawFastHLine(0, HEADER_HEIGHT, DISPLAY_WIDTH, THEME_PRIMARY);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString("CYD MIDI", DISPLAY_CENTER_X, HEADER_TITLE_Y, 4);
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawCentreString("Select Mode", DISPLAY_CENTER_X, HEADER_SUBTITLE_Y, 2);

  const int cols = 2;
  const int btnW = SCALE_X(140);
  const int btnH = SCALE_Y(28);
  const int gapX = SCALE_X(20);
  const int gapY = SCALE_Y(8);
  const int startX = MARGIN_SMALL;
  const int startY = SCALE_Y(55);

  for (size_t i = 0; i < sizeof(kMenuItems) / sizeof(kMenuItems[0]); i++) {
    int col = i % cols;
    int row = i / cols;
    int x = startX + col * (btnW + gapX);
    int y = startY + row * (btnH + gapY);
    drawRoundButton(x, y, btnW, btnH, kMenuItems[i].label, kMenuItems[i].color);
  }
  
  // Screenshot button at the bottom
  drawRoundButton(85, 215, 150, 28, "SCREENSHOT", THEME_SECONDARY);
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
    case SLINK:
      drawSlinkMode();
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
    case SLINK:
      initializeSlinkMode();
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
  const int btnW = SCALE_X(140);
  const int btnH = SCALE_Y(28);
  const int gapX = SCALE_X(20);
  const int gapY = SCALE_Y(8);
  const int startX = MARGIN_SMALL;
  const int startY = SCALE_Y(55);

  // Check screenshot button
  if (isButtonPressed(85, 215, 150, 28)) {
    takeScreenshot();
    return;
  }

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
  render_obj = lv_obj_create(lv_screen_active());
  lv_obj_set_size(render_obj,
                  lv_display_get_horizontal_resolution(display),
                  lv_display_get_vertical_resolution(display));
  lv_obj_set_style_bg_opa(render_obj, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(render_obj, render_event, LV_EVENT_DRAW_MAIN, NULL);
  
  ble_init_start_ms = millis();
  initHardwareMIDI();  // Initialize hardware MIDI output
  
#if REMOTE_DISPLAY_ENABLED
  initRemoteDisplay();  // Initialize remote display capability
#endif
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
  }
#endif

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
    case SLINK:
      handleSlinkMode();
      break;
    default:
      break;
  }
  requestRedraw();
#if REMOTE_DISPLAY_ENABLED
  handleRemoteDisplay();  // Handle remote display updates
#endif
}
