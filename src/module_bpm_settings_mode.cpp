#include "module_bpm_settings_mode.h"
#include "ui_elements.h"
#include "common_definitions.h"

namespace {
static constexpr uint16_t kBpmStep = 1;
static constexpr uint16_t kBpmMin = 40;
static constexpr uint16_t kBpmMax = 240;

// Store the previous mode to return to
static AppMode previousMode = MENU;
}

void initializeBPMSettingsMode() {
  // Store the current mode so we can return to it
  previousMode = currentMode;
}

void drawBPMSettingsMode() {
  // Draw a semi-transparent overlay
  tft.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, THEME_BG);
  
  // Draw a centered dialog box
  int dialogW = SCALE_X(240);
  int dialogH = SCALE_Y(160);
  int dialogX = (DISPLAY_WIDTH - dialogW) / 2;
  int dialogY = (DISPLAY_HEIGHT - dialogH) / 2;
  
  // Dialog background with border
  tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, 12, THEME_SURFACE);
  tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, 12, THEME_PRIMARY);
  tft.drawRoundRect(dialogX + 1, dialogY + 1, dialogW - 2, dialogH - 2, 11, THEME_PRIMARY);
  
  // Title
  int titleY = dialogY + SCALE_Y(15);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString("TEMPO", DISPLAY_CENTER_X, titleY, 4);
  
  // BPM value - large display
  int bpmValueY = dialogY + SCALE_Y(55);
  tft.setTextColor(THEME_ACCENT, THEME_SURFACE);
  tft.drawCentreString(String(sharedBPM), DISPLAY_CENTER_X, bpmValueY, 7);
  
  // BPM label
  int bpmLabelY = dialogY + SCALE_Y(95);
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawCentreString("BPM", DISPLAY_CENTER_X, bpmLabelY, 2);
  
  // Buttons
  int buttonY = dialogY + dialogH - SCALE_Y(40);
  int buttonH = SCALE_Y(30);
  int buttonSpacing = SCALE_X(10);
  int totalButtonW = dialogW - SCALE_X(40);
  int sideButtonW = SCALE_X(50);
  int centerButtonW = totalButtonW - 2 * sideButtonW - 2 * buttonSpacing;
  
  int leftButtonX = dialogX + SCALE_X(20);
  int centerButtonX = leftButtonX + sideButtonW + buttonSpacing;
  int rightButtonX = centerButtonX + centerButtonW + buttonSpacing;
  
  // Draw buttons
  drawRoundButton(leftButtonX, buttonY, sideButtonW, buttonH, "-", THEME_SECONDARY, false, 4);
  drawRoundButton(centerButtonX, buttonY, centerButtonW, buttonH, "CLOSE", THEME_PRIMARY, false, 2);
  drawRoundButton(rightButtonX, buttonY, sideButtonW, buttonH, "+", THEME_SECONDARY, false, 4);
}

void handleBPMSettingsMode() {
  if (!touch.justPressed) {
    return;
  }
  
  int dialogW = SCALE_X(240);
  int dialogH = SCALE_Y(160);
  int dialogX = (DISPLAY_WIDTH - dialogW) / 2;
  int dialogY = (DISPLAY_HEIGHT - dialogH) / 2;
  
  int buttonY = dialogY + dialogH - SCALE_Y(40);
  int buttonH = SCALE_Y(30);
  int buttonSpacing = SCALE_X(10);
  int totalButtonW = dialogW - SCALE_X(40);
  int sideButtonW = SCALE_X(50);
  int centerButtonW = totalButtonW - 2 * sideButtonW - 2 * buttonSpacing;
  
  int leftButtonX = dialogX + SCALE_X(20);
  int centerButtonX = leftButtonX + sideButtonW + buttonSpacing;
  int rightButtonX = centerButtonX + centerButtonW + buttonSpacing;
  
  // Minus button
  if (isButtonPressed(leftButtonX, buttonY, sideButtonW, buttonH)) {
    if (sharedBPM > kBpmMin) {
      uint16_t newBPM = (sharedBPM - kBpmStep < kBpmMin) ? kBpmMin : sharedBPM - kBpmStep;
      setSharedBPM(newBPM);
      requestRedraw();
    }
    return;
  }
  
  // Plus button
  if (isButtonPressed(rightButtonX, buttonY, sideButtonW, buttonH)) {
    if (sharedBPM < kBpmMax) {
      uint16_t newBPM = (sharedBPM + kBpmStep > kBpmMax) ? kBpmMax : sharedBPM + kBpmStep;
      setSharedBPM(newBPM);
      requestRedraw();
    }
    return;
  }
  
  // Close button or tap outside dialog
  bool outsideDialog = touch.x < dialogX || touch.x > dialogX + dialogW ||
                       touch.y < dialogY || touch.y > dialogY + dialogH;
  bool closeButton = isButtonPressed(centerButtonX, buttonY, centerButtonW, buttonH);
  
  if (closeButton || outsideDialog) {
    // Return to the previous mode
    currentMode = previousMode;
    requestRedraw();
    return;
  }
}
