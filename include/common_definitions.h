#ifndef COMMON_DEFINITIONS_H
#define COMMON_DEFINITIONS_H

#include "smartdisplay_compat.h"
#include <BLEDevice.h>

// Version
#define ACYD_MIDI_VERSION "0.1.2"

// ============================================================
// Display Configuration and Autoscaling System
// ============================================================
// Reference display dimensions (ESP32-2432S028R default)
#define DISPLAY_REF_WIDTH  320
#define DISPLAY_REF_HEIGHT 240

// Actual display dimensions (populated at runtime)
struct DisplayConfig {
  int width = DISPLAY_REF_WIDTH;
  int height = DISPLAY_REF_HEIGHT;
  float scaleX = 1.0f;
  float scaleY = 1.0f;
};

extern DisplayConfig displayConfig;

// Scaling macros - automatically scale coordinates and dimensions
// Use these instead of hardcoded pixel values
#define SCALE_X(x) ((int)((x) * displayConfig.scaleX))
#define SCALE_Y(y) ((int)((y) * displayConfig.scaleY))
#define SCALE_W(w) ((int)((w) * displayConfig.scaleX))
#define SCALE_H(h) ((int)((h) * displayConfig.scaleY))

// Common scaled dimensions (based on 320x240 reference)
#ifdef DISPLAY_WIDTH
#undef DISPLAY_WIDTH
#endif
#ifdef DISPLAY_HEIGHT
#undef DISPLAY_HEIGHT
#endif
#define DISPLAY_WIDTH  (displayConfig.width)
#define DISPLAY_HEIGHT (displayConfig.height)
#define DISPLAY_CENTER_X (displayConfig.width / 2)
#define DISPLAY_CENTER_Y (displayConfig.height / 2)

// Header dimensions
#define HEADER_HEIGHT SCALE_Y(45)
#define HEADER_TITLE_Y SCALE_Y(8)
#define HEADER_SUBTITLE_Y SCALE_Y(28)
#define BACK_BUTTON_X SCALE_X(10)
#define BACK_BUTTON_Y SCALE_Y(10)
#define BACK_BUTTON_W SCALE_X(50)
#define BACK_BUTTON_H SCALE_Y(25)

// Common button dimensions
#define BTN_SMALL_W SCALE_X(40)
#define BTN_SMALL_H SCALE_Y(25)
#define BTN_MEDIUM_W SCALE_X(50)
#define BTN_MEDIUM_H SCALE_Y(25)
#define BTN_LARGE_W SCALE_X(60)
#define BTN_LARGE_H SCALE_Y(25)

// Common spacing
#define MARGIN_SMALL SCALE_X(10)
#define MARGIN_MEDIUM SCALE_X(20)
#define GAP_SMALL SCALE_X(5)
#define GAP_MEDIUM SCALE_X(8)

// Initialize display configuration
void initDisplayConfig();

// Color scheme
#define THEME_BG         0x0000
#define THEME_SURFACE    0x2945
#define THEME_PRIMARY    0x06FF
#define THEME_SECONDARY  0xFD20
#define THEME_ACCENT     0x07FF
#define THEME_SUCCESS    0x07E0
#define THEME_WARNING    0xFFE0
#define THEME_ERROR      0xF800
#define THEME_TEXT       0xFFFF
#define THEME_TEXT_DIM   0x8410

#ifndef BOARD_NAME
#define BOARD_NAME "esp32-2432S028Rv2"
#endif

// BLE MIDI UUIDs
#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// Touch handling
struct TouchState {
  bool wasPressed = false;
  bool isPressed = false;
  bool justPressed = false;
  bool justReleased = false;
  int x = 0, y = 0;
};

// App modes
enum AppMode {
  MENU,
  SETTINGS,
  BPM_SETTINGS,
  KEYBOARD,
  SEQUENCER,
  BOUNCING_BALL,
  PHYSICS_DROP,
  RANDOM_GENERATOR,
  XY_PAD,
  ARPEGGIATOR,
  GRID_PIANO,
  AUTO_CHORD,
  LFO,
  SLINK,
  TB3PO,
  GRIDS,
  RAGA,
  EUCLID,
  MORPH,
  WAAAVE,
#ifdef ENABLE_M5_8ENCODER
  ENCODER_PANEL,
#endif
  FRACTAL_ECHO
};

enum MidiClockMaster {
  CLOCK_INTERNAL = 0,
  CLOCK_WIFI,
  CLOCK_BLE,
  CLOCK_HARDWARE,
  CLOCK_ESP_NOW
};

enum MenuMode {
  MENU_ORIGINAL = 0,
  MENU_EXPERIMENTAL
};

// Music theory
struct Scale {
  String name;
  int intervals[12];
  int numNotes;
};

// Global scale definitions
extern Scale scales[];
extern const int NUM_SCALES;

// Global objects - defined in src/app/app_state.cpp
extern TFT_eSPI tft;
extern BLECharacteristic *pCharacteristic;
extern bool deviceConnected;
extern uint8_t midiPacket[];
extern TouchState touch;
extern AppMode currentMode;

// Redraw control - to minimize unnecessary redraws
extern volatile bool needsRedraw;
extern uint16_t sharedBPM;
extern MidiClockMaster midiClockMaster;
extern bool displayColorsInverted;
extern uint8_t displayRotationIndex;
extern bool instantStartMode;
extern MenuMode currentMenuMode;

void setDisplayInversion(bool invert);
void rotateDisplay180();
void requestRedraw();
void setSharedBPM(uint16_t bpm);

#endif
