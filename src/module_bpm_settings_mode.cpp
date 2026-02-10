#include "module_bpm_settings_mode.h"
#include "ui_elements.h"
#include "common_definitions.h"
#include "app/app_modes.h"
#include "clock_manager.h"

namespace {
static constexpr uint16_t kBpmStep = 1;
static constexpr uint16_t kBpmMin = 40;
static constexpr uint16_t kBpmMax = 240;

// Store the previous mode to return to
static AppMode previousMode = MENU;

// Dialog layout helper
struct BPMDialogLayout {
  int dialogX, dialogY, dialogW, dialogH;
  int bpmButtonY, bpmButtonH;
  int leftBpmButtonX, centerBpmButtonX, rightBpmButtonX;
  int sideBpmButtonW, centerBpmButtonW;
  int transportButtonY, transportButtonH;
  int playButtonX, stopButtonX;
  int transportButtonW;
};

static BPMDialogLayout calculateDialogLayout() {
  BPMDialogLayout layout;
  layout.dialogW = SCALE_X(240);
  layout.dialogH = SCALE_Y(180);  // Increased height for transport controls
  layout.dialogX = (DISPLAY_WIDTH - layout.dialogW) / 2;
  layout.dialogY = (DISPLAY_HEIGHT - layout.dialogH) / 2;
  
  // Transport control buttons (play/stop)
  layout.transportButtonH = SCALE_Y(30);
  layout.transportButtonY = layout.dialogY + SCALE_Y(105);
  int transportButtonSpacing = SCALE_X(10);
  layout.transportButtonW = SCALE_X(50);
  int totalTransportWidth = 2 * layout.transportButtonW + transportButtonSpacing;
  int transportStartX = (DISPLAY_WIDTH - totalTransportWidth) / 2;
  layout.playButtonX = transportStartX;
  layout.stopButtonX = transportStartX + layout.transportButtonW + transportButtonSpacing;
  
  // BPM adjustment buttons (-, CLOSE, +)
  layout.bpmButtonY = layout.dialogY + layout.dialogH - SCALE_Y(40);
  layout.bpmButtonH = SCALE_Y(30);
  int buttonSpacing = SCALE_X(10);
  int totalButtonW = layout.dialogW - SCALE_X(40);
  layout.sideBpmButtonW = SCALE_X(50);
  layout.centerBpmButtonW = totalButtonW - 2 * layout.sideBpmButtonW - 2 * buttonSpacing;
  
  layout.leftBpmButtonX = layout.dialogX + SCALE_X(20);
  layout.centerBpmButtonX = layout.leftBpmButtonX + layout.sideBpmButtonW + buttonSpacing;
  layout.rightBpmButtonX = layout.centerBpmButtonX + layout.centerBpmButtonW + buttonSpacing;
  
  return layout;
}
}

void setPreviousModeForBPMSettings(AppMode mode) {
  previousMode = mode;
}

void initializeBPMSettingsMode() {
  // previousMode is set via setPreviousModeForBPMSettings before switchMode is called
  // This ensures we capture the correct previous mode
}

void drawBPMSettingsMode() {
  // Draw modal dialog (replaces screen content)
  tft.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, THEME_BG);
  
  BPMDialogLayout layout = calculateDialogLayout();
  
  // Dialog background with border
  tft.fillRoundRect(layout.dialogX, layout.dialogY, layout.dialogW, layout.dialogH, 12, THEME_SURFACE);
  tft.drawRoundRect(layout.dialogX, layout.dialogY, layout.dialogW, layout.dialogH, 12, THEME_PRIMARY);
  tft.drawRoundRect(layout.dialogX + 1, layout.dialogY + 1, layout.dialogW - 2, layout.dialogH - 2, 11, THEME_PRIMARY);
  
  // Title
  int titleY = layout.dialogY + SCALE_Y(15);
  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawCentreString("TEMPO", DISPLAY_CENTER_X, titleY, 4);
  
  // BPM value - large display
  int bpmValueY = layout.dialogY + SCALE_Y(45);
  tft.setTextColor(THEME_ACCENT, THEME_SURFACE);
  tft.drawCentreString(String(sharedBPM), DISPLAY_CENTER_X, bpmValueY, 7);
  
  // BPM label
  int bpmLabelY = layout.dialogY + SCALE_Y(75);
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawCentreString("BPM", DISPLAY_CENTER_X, bpmLabelY, 2);
  
  // Transport controls (play/stop)
  // Note: Mutually exclusive highlighting - only the active button is highlighted
  // When playing, PLAY is highlighted; when stopped, STOP is highlighted
  // Both buttons remain interactive regardless of state - user can press either at any time
  bool isPlaying = clockManagerIsRunning();
  drawRoundButton(layout.playButtonX, layout.transportButtonY, layout.transportButtonW, layout.transportButtonH, 
                  "PLAY", THEME_SUCCESS, isPlaying, 2);
  drawRoundButton(layout.stopButtonX, layout.transportButtonY, layout.transportButtonW, layout.transportButtonH, 
                  "STOP", THEME_ERROR, !isPlaying, 2);
  
  // BPM adjustment buttons
  drawRoundButton(layout.leftBpmButtonX, layout.bpmButtonY, layout.sideBpmButtonW, layout.bpmButtonH, "-", THEME_SECONDARY, false, 4);
  drawRoundButton(layout.centerBpmButtonX, layout.bpmButtonY, layout.centerBpmButtonW, layout.bpmButtonH, "CLOSE", THEME_PRIMARY, false, 2);
  drawRoundButton(layout.rightBpmButtonX, layout.bpmButtonY, layout.sideBpmButtonW, layout.bpmButtonH, "+", THEME_SECONDARY, false, 4);
}

void handleBPMSettingsMode() {
  if (!touch.justPressed) {
    return;
  }
  
  BPMDialogLayout layout = calculateDialogLayout();
  
  // Play button
  if (isButtonPressed(layout.playButtonX, layout.transportButtonY, layout.transportButtonW, layout.transportButtonH)) {
    if (!clockManagerIsRunning()) {
      clockManagerRequestStart();
      requestRedraw();
    }
    return;
  }
  
  // Stop button
  if (isButtonPressed(layout.stopButtonX, layout.transportButtonY, layout.transportButtonW, layout.transportButtonH)) {
    if (clockManagerIsRunning()) {
      clockManagerExternalStop();
      requestRedraw();
    }
    return;
  }
  
  // Minus button
  if (isButtonPressed(layout.leftBpmButtonX, layout.bpmButtonY, layout.sideBpmButtonW, layout.bpmButtonH)) {
    if (sharedBPM > kBpmMin) {
      uint16_t newBPM = (sharedBPM - kBpmStep < kBpmMin) ? kBpmMin : sharedBPM - kBpmStep;
      setSharedBPM(newBPM);
      requestRedraw();
    }
    return;
  }
  
  // Plus button
  if (isButtonPressed(layout.rightBpmButtonX, layout.bpmButtonY, layout.sideBpmButtonW, layout.bpmButtonH)) {
    if (sharedBPM < kBpmMax) {
      uint16_t newBPM = (sharedBPM + kBpmStep > kBpmMax) ? kBpmMax : sharedBPM + kBpmStep;
      setSharedBPM(newBPM);
      requestRedraw();
    }
    return;
  }
  
  // Close button or tap outside dialog
  bool outsideDialog = touch.x < layout.dialogX || touch.x > layout.dialogX + layout.dialogW ||
                       touch.y < layout.dialogY || touch.y > layout.dialogY + layout.dialogH;
  bool closeButton = isButtonPressed(layout.centerBpmButtonX, layout.bpmButtonY, layout.centerBpmButtonW, layout.bpmButtonH);
  
  if (closeButton || outsideDialog) {
    // Return to the previous mode using proper mode switching
    // Note: We don't stop the sequencer/clock when closing the BPM dialog
    switchMode(previousMode);
    return;
  }
}
