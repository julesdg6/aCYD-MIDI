#include "module_waaave_mode.h"
#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"
#include <Arduino.h>
#include <cmath>

namespace {

// Waaave Pool controller state
struct WaaaveState {
  int currentPage = 0;  // 0=Transport, 1=Controls 1-4, 2=Controls 5-8
  
  // Transport button states
  bool recordPressed = false;
  bool playPressed = false;
  bool stopPressed = false;
  bool prevPressed = false;
  bool nextPressed = false;
  
  // Control values (8 channels)
  uint8_t knobs[8] = {64, 64, 64, 64, 64, 64, 64, 64};        // CC#16-23
  uint8_t sliders[8] = {0, 0, 0, 0, 0, 0, 0, 0};              // CC#0-7
  bool sButtons[8] = {false, false, false, false, false, false, false, false};  // CC#32-39
  bool mButtons[8] = {false, false, false, false, false, false, false, false};  // CC#48-55
  bool rButtons[8] = {false, false, false, false, false, false, false, false};  // CC#64-71
};

static WaaaveState state;

// UI Constants
static constexpr int SLIDER_BORDER_WIDTH = 1;
static constexpr int KNOB_SENSITIVITY = 3;  // Pixels of drag per value increment
static constexpr float KNOB_ROTATION_RANGE = 270.0f;  // Degrees of rotation
static constexpr float KNOB_START_ANGLE = -135.0f;    // Starting angle in degrees

// MIDI CC mappings for Korg nanoKONTROL2
static constexpr uint8_t CC_KNOB_BASE = 16;
static constexpr uint8_t CC_SLIDER_BASE = 0;
static constexpr uint8_t CC_S_BUTTON_BASE = 32;
static constexpr uint8_t CC_M_BUTTON_BASE = 48;
static constexpr uint8_t CC_R_BUTTON_BASE = 64;

// Transport CC mappings (Korg nanoKONTROL2 specific)
// Note: These are not standard MIDI Machine Control (MMC uses SysEx),
// but are the CC assignments used by the Korg nanoKONTROL2 for transport
static constexpr uint8_t CC_RECORD = 93;
static constexpr uint8_t CC_PLAY = 94;
static constexpr uint8_t CC_STOP = 95;
static constexpr uint8_t CC_PREV = 91;
static constexpr uint8_t CC_NEXT = 92;

static void sendCC(uint8_t cc, uint8_t value) {
  sendMIDI(0xB0, cc, value);  // Control Change on channel 1
}

// Page navigation helpers
static inline int numPages() { return 3; }

static void drawPageIndicator() {
  int dotSize = SCALE_X(6);
  int spacing = SCALE_X(12);
  int totalWidth = numPages() * dotSize + (numPages() - 1) * (spacing - dotSize);
  int startX = DISPLAY_CENTER_X - totalWidth / 2;
  int y = DISPLAY_HEIGHT - SCALE_Y(12);
  
  for (int i = 0; i < numPages(); ++i) {
    int x = startX + i * spacing;
    uint16_t color = (i == state.currentPage) ? THEME_PRIMARY : THEME_TEXT_DIM;
    tft.fillCircle(x, y, dotSize / 2, color);
  }
}

static void drawNavigationButtons() {
  int btnW = SCALE_X(50);
  int btnH = SCALE_Y(25);
  int btnY = DISPLAY_HEIGHT - SCALE_Y(40);
  int leftX = MARGIN_SMALL;
  int rightX = DISPLAY_WIDTH - MARGIN_SMALL - btnW;
  
  if (state.currentPage > 0) {
    drawRoundButton(leftX, btnY, btnW, btnH, "<", THEME_PRIMARY, false, 5);
  }
  
  if (state.currentPage < numPages() - 1) {
    drawRoundButton(rightX, btnY, btnW, btnH, ">", THEME_PRIMARY, false, 5);
  }
}

// Transport page (Page 0)
static void drawTransportPage() {
  int btnW = SCALE_X(60);
  int btnH = SCALE_Y(50);
  int spacing = SCALE_Y(15);
  int startY = HEADER_HEIGHT + SCALE_Y(20);
  int centerX = DISPLAY_CENTER_X;
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString("Transport Controls", centerX, startY, 2);
  
  startY += SCALE_Y(30);
  
  // Record button
  uint16_t recordColor = state.recordPressed ? THEME_ERROR : THEME_SURFACE;
  drawRoundButton(centerX - btnW / 2, startY, btnW, btnH, "REC", recordColor, false, 2);
  
  startY += btnH + spacing;
  
  // Play button
  uint16_t playColor = state.playPressed ? THEME_SUCCESS : THEME_SURFACE;
  drawRoundButton(centerX - btnW / 2, startY, btnW, btnH, "PLAY", playColor, false, 2);
  
  startY += btnH + spacing;
  
  // Stop button
  uint16_t stopColor = state.stopPressed ? THEME_WARNING : THEME_SURFACE;
  drawRoundButton(centerX - btnW / 2, startY, btnW, btnH, "STOP", stopColor, false, 2);
  
  // Previous and Next buttons side by side
  int navY = DISPLAY_HEIGHT - SCALE_Y(80);
  int navW = SCALE_X(50);
  int navH = SCALE_Y(35);
  int gap = SCALE_X(15);
  
  uint16_t prevColor = state.prevPressed ? THEME_ACCENT : THEME_SURFACE;
  drawRoundButton(centerX - navW - gap / 2, navY, navW, navH, "<<", prevColor, false, 3);
  
  uint16_t nextColor = state.nextPressed ? THEME_ACCENT : THEME_SURFACE;
  drawRoundButton(centerX + gap / 2, navY, navW, navH, ">>", nextColor, false, 3);
}

// Control page helper (draws 4 channels)
static void drawControlPage(int channelStart) {
  int numChannels = 4;
  int channelW = (DISPLAY_WIDTH - 2 * MARGIN_SMALL - SCALE_X(15)) / numChannels;
  int startY = HEADER_HEIGHT + SCALE_Y(10);
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String title = "Controls " + String(channelStart + 1) + "-" + String(channelStart + 4);
  tft.drawCentreString(title, DISPLAY_CENTER_X, startY, 2);
  
  startY += SCALE_Y(20);
  
  for (int i = 0; i < numChannels; ++i) {
    int ch = channelStart + i;
    int x = MARGIN_SMALL + i * (channelW + SCALE_X(5));
    int contentY = startY;
    
    // Channel label
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    String chLabel = "CH" + String(ch + 1);
    tft.drawCentreString(chLabel, x + channelW / 2, contentY, 0);
    contentY += SCALE_Y(12);
    
    // Knob (rotary control) - display as circular indicator
    int knobSize = SCALE_X(20);
    int knobCX = x + channelW / 2;
    int knobCY = contentY + knobSize / 2 + SCALE_Y(3);
    tft.drawCircle(knobCX, knobCY, knobSize / 2, THEME_TEXT);
    
    // Draw knob position indicator
    float angle = (state.knobs[ch] / 127.0f) * KNOB_ROTATION_RANGE + KNOB_START_ANGLE;
    float rad = angle * PI / 180.0f;
    int indicatorX = knobCX + (int)(cos(rad) * knobSize / 2);
    int indicatorY = knobCY + (int)(sin(rad) * knobSize / 2);
    tft.drawLine(knobCX, knobCY, indicatorX, indicatorY, THEME_PRIMARY);
    tft.fillCircle(knobCX, knobCY, SCALE_X(2), THEME_PRIMARY);
    
    contentY += knobSize + SCALE_Y(5);
    
    // Knob value label
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawCentreString(String(state.knobs[ch]), knobCX, contentY, 0);
    contentY += SCALE_Y(12);
    
    // Slider (vertical fader)
    int sliderW = SCALE_X(12);
    int sliderH = SCALE_Y(40);
    int sliderX = x + (channelW - sliderW) / 2;
    int sliderY = contentY;
    
    tft.drawRect(sliderX, sliderY, sliderW, sliderH, THEME_TEXT);
    int fillH = (state.sliders[ch] * sliderH) / 127;
    if (fillH > 0) {
      tft.fillRect(sliderX + SLIDER_BORDER_WIDTH, sliderY + sliderH - fillH, 
                   sliderW - 2 * SLIDER_BORDER_WIDTH, fillH, THEME_ACCENT);
    }
    
    contentY += sliderH + SCALE_Y(5);
    
    // Slider value label
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawCentreString(String(state.sliders[ch]), x + channelW / 2, contentY, 0);
    contentY += SCALE_Y(12);
    
    // S, M, R buttons
    int btnW = channelW - SCALE_X(4);
    int btnH = SCALE_Y(18);
    int btnSpacing = SCALE_Y(3);
    
    uint16_t sColor = state.sButtons[ch] ? THEME_SUCCESS : THEME_SURFACE;
    drawRoundButton(x + SCALE_X(2), contentY, btnW, btnH, "S", sColor, false, 0);
    contentY += btnH + btnSpacing;
    
    uint16_t mColor = state.mButtons[ch] ? THEME_WARNING : THEME_SURFACE;
    drawRoundButton(x + SCALE_X(2), contentY, btnW, btnH, "M", mColor, false, 0);
    contentY += btnH + btnSpacing;
    
    uint16_t rColor = state.rButtons[ch] ? THEME_ERROR : THEME_SURFACE;
    drawRoundButton(x + SCALE_X(2), contentY, btnW, btnH, "R", rColor, false, 0);
  }
}

static void handleTransportPage() {
  int btnW = SCALE_X(60);
  int btnH = SCALE_Y(50);
  int spacing = SCALE_Y(15);
  int startY = HEADER_HEIGHT + SCALE_Y(50);
  int centerX = DISPLAY_CENTER_X;
  
  // Record button
  if (isButtonPressed(centerX - btnW / 2, startY, btnW, btnH)) {
    state.recordPressed = !state.recordPressed;
    sendCC(CC_RECORD, state.recordPressed ? 127 : 0);
    requestRedraw();
  }
  startY += btnH + spacing;
  
  // Play button
  if (isButtonPressed(centerX - btnW / 2, startY, btnW, btnH)) {
    state.playPressed = !state.playPressed;
    sendCC(CC_PLAY, state.playPressed ? 127 : 0);
    requestRedraw();
  }
  startY += btnH + spacing;
  
  // Stop button
  if (isButtonPressed(centerX - btnW / 2, startY, btnW, btnH)) {
    state.stopPressed = !state.stopPressed;
    sendCC(CC_STOP, state.stopPressed ? 127 : 0);
    requestRedraw();
  }
  
  // Previous and Next buttons
  int navY = DISPLAY_HEIGHT - SCALE_Y(80);
  int navW = SCALE_X(50);
  int navH = SCALE_Y(35);
  int gap = SCALE_X(15);
  
  if (isButtonPressed(centerX - navW - gap / 2, navY, navW, navH)) {
    state.prevPressed = !state.prevPressed;
    sendCC(CC_PREV, state.prevPressed ? 127 : 0);
    requestRedraw();
  }
  
  if (isButtonPressed(centerX + gap / 2, navY, navW, navH)) {
    state.nextPressed = !state.nextPressed;
    sendCC(CC_NEXT, state.nextPressed ? 127 : 0);
    requestRedraw();
  }
}

static void handleControlPage(int channelStart) {
  // Static variable to track last knob position for drag tracking
  // This is reset when touch is released or when switching pages
  static int lastKnobX[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  static int lastActivePage = -1;
  
  // Reset tracking when switching pages
  if (lastActivePage != state.currentPage) {
    for (int i = 0; i < 8; ++i) {
      lastKnobX[i] = 0;
    }
    lastActivePage = state.currentPage;
  }
  
  int numChannels = 4;
  int channelW = (DISPLAY_WIDTH - 2 * MARGIN_SMALL - SCALE_X(15)) / numChannels;
  int startY = HEADER_HEIGHT + SCALE_Y(30);
  
  for (int i = 0; i < numChannels; ++i) {
    int ch = channelStart + i;
    int x = MARGIN_SMALL + i * (channelW + SCALE_X(5));
    int contentY = startY;
    
    contentY += SCALE_Y(12);  // Skip channel label
    
    // Knob touch area
    int knobSize = SCALE_X(20);
    int knobCX = x + channelW / 2;
    int knobCY = contentY + knobSize / 2 + SCALE_Y(3);
    int knobTouchRadius = knobSize;
    
    if (touch.isPressed &&
        abs(touch.x - knobCX) <= knobTouchRadius &&
        abs(touch.y - knobCY) <= knobTouchRadius) {
      // Adjust knob based on horizontal drag with sensitivity
      int deltaX = touch.x - lastKnobX[ch];
      if (lastKnobX[ch] == 0) {
        lastKnobX[ch] = touch.x;
        deltaX = 0;
      }
      
      if (abs(deltaX) >= KNOB_SENSITIVITY) {
        int increment = deltaX / KNOB_SENSITIVITY;
        int newValue = state.knobs[ch] + increment;
        newValue = constrain(newValue, 0, 127);
        if (newValue != state.knobs[ch]) {
          state.knobs[ch] = newValue;
          sendCC(CC_KNOB_BASE + ch, state.knobs[ch]);
          requestRedraw();
        }
        lastKnobX[ch] = touch.x;
      }
    } else {
      lastKnobX[ch] = 0;
    }
    
    contentY += knobSize + SCALE_Y(17);  // Skip knob and value
    
    // Slider touch area
    int sliderW = SCALE_X(12);
    int sliderH = SCALE_Y(40);
    int sliderX = x + (channelW - sliderW) / 2;
    int sliderY = contentY;
    
    if (isButtonPressed(sliderX, sliderY, sliderW, sliderH)) {
      int relY = touch.y - sliderY;
      int newValue = 127 - ((relY * 127) / sliderH);
      newValue = constrain(newValue, 0, 127);
      state.sliders[ch] = newValue;
      sendCC(CC_SLIDER_BASE + ch, state.sliders[ch]);
      requestRedraw();
    }
    
    contentY += sliderH + SCALE_Y(17);  // Skip slider and value
    
    // S, M, R buttons
    int btnW = channelW - SCALE_X(4);
    int btnH = SCALE_Y(18);
    int btnSpacing = SCALE_Y(3);
    
    // S button
    if (isButtonPressed(x + SCALE_X(2), contentY, btnW, btnH)) {
      state.sButtons[ch] = !state.sButtons[ch];
      sendCC(CC_S_BUTTON_BASE + ch, state.sButtons[ch] ? 127 : 0);
      requestRedraw();
    }
    contentY += btnH + btnSpacing;
    
    // M button
    if (isButtonPressed(x + SCALE_X(2), contentY, btnW, btnH)) {
      state.mButtons[ch] = !state.mButtons[ch];
      sendCC(CC_M_BUTTON_BASE + ch, state.mButtons[ch] ? 127 : 0);
      requestRedraw();
    }
    contentY += btnH + btnSpacing;
    
    // R button
    if (isButtonPressed(x + SCALE_X(2), contentY, btnW, btnH)) {
      state.rButtons[ch] = !state.rButtons[ch];
      sendCC(CC_R_BUTTON_BASE + ch, state.rButtons[ch] ? 127 : 0);
      requestRedraw();
    }
  }
}

}  // namespace

void initializeWaaaveMode() {
  state.currentPage = 0;
  
  // Reset transport buttons
  state.recordPressed = false;
  state.playPressed = false;
  state.stopPressed = false;
  state.prevPressed = false;
  state.nextPressed = false;
  
  // Reset controls to default
  for (int i = 0; i < 8; ++i) {
    state.knobs[i] = 64;
    state.sliders[i] = 0;
    state.sButtons[i] = false;
    state.mButtons[i] = false;
    state.rButtons[i] = false;
  }
}

void drawWaaaveMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("WAAAVE POOL", "Korg nanoKONTROL2", 3);
  
  switch (state.currentPage) {
    case 0:
      drawTransportPage();
      break;
    case 1:
      drawControlPage(0);  // Channels 1-4
      break;
    case 2:
      drawControlPage(4);  // Channels 5-8
      break;
  }
  
  drawPageIndicator();
  drawNavigationButtons();
}

void handleWaaaveMode() {
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (!touch.justPressed && !touch.isPressed) {
    return;
  }
  
  // Handle page navigation
  int btnW = SCALE_X(50);
  int btnH = SCALE_Y(25);
  int btnY = DISPLAY_HEIGHT - SCALE_Y(40);
  int leftX = MARGIN_SMALL;
  int rightX = DISPLAY_WIDTH - MARGIN_SMALL - btnW;
  
  if (touch.justPressed) {
    if (state.currentPage > 0 && isButtonPressed(leftX, btnY, btnW, btnH)) {
      state.currentPage--;
      requestRedraw();
      return;
    }
    
    if (state.currentPage < numPages() - 1 && isButtonPressed(rightX, btnY, btnW, btnH)) {
      state.currentPage++;
      requestRedraw();
      return;
    }
  }
  
  // Handle page-specific interactions
  if (touch.justPressed) {
    switch (state.currentPage) {
      case 0:
        handleTransportPage();
        break;
      case 1:
        handleControlPage(0);  // Channels 1-4
        break;
      case 2:
        handleControlPage(4);  // Channels 5-8
        break;
    }
  } else if (touch.isPressed) {
    // Handle continuous interactions (slider dragging)
    if (state.currentPage == 1) {
      handleControlPage(0);
    } else if (state.currentPage == 2) {
      handleControlPage(4);
    }
  }
}
