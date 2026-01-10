#ifndef USER_SETUP_H
#define USER_SETUP_H

// ============================================================
// CYD MIDI Controller - Hardware Configuration
// Exact pins and settings that work in Arduino IDE
// ============================================================

// Driver - ILI9341 standard
#define ILI9341_DRIVER

// Color order for CYD
#define TFT_RGB_ORDER TFT_BGR

// Display resolution
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Pin assignments (from working Arduino version)
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  -1

// Backlight pin
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Touch CS (using XPT2046_Touchscreen library instead)
#define TOUCH_CS 33

// SPI frequencies
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

// Touch frequency
#define SPI_TOUCH_FREQUENCY  2500000

// Transaction support
#define SUPPORT_TRANSACTIONS

// Font support
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#endif // USER_SETUP_H
