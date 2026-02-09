#include "module_bpm_settings_mode.h"
#include "ui_elements.h"
#include "common_definitions.h"
#include "app/app_modes.h"

namespace {
static constexpr uint16_t kBpmStep = 1;
static constexpr uint16_t kBpmMin = 40;
static constexpr uint16_t kBpmMax = 240;

// Store the previous mode to return to
static AppMode previousMode = MENU;

// Dialog layout helper
struct BPMDialogLayout {
  int dialogX, dialogY, dialogW, dialogH;
  int buttonY, buttonH;
  int leftButtonX, centerButtonX, rightButtonX;
  int sideButtonW, centerButtonW;
};

static BPMDialogLayout calculateDialogLayout() {
  BPMDialogLayout layout;
  layout.dialogW = SCALE_X(240);
  layout.dialogH = SCALE_Y(160);
  layout.dialogX = (DISPLAY_WIDTH - layout.dialogW) / 2;
  layout.dialogY = (DISPLAY_HEIGHT - layout.dialogH) / 2;
  
  layout.buttonY = layout.dialogY + layout.dialogH - SCALE_Y(40);
  layout.buttonH = SCALE_Y(30);
  int buttonSpacing = SCALE_X(10);
  int totalButtonW = layout.dialogW - SCALE_X(40);
  layout.sideButtonW = SCALE_X(50);
  layout.centerButtonW = totalButtonW - 2 * layout.sideButtonW - 2 * buttonSpacing;
  
  layout.leftButtonX = layout.dialogX + SCALE_X(20);
  layout.centerButtonX = layout.leftButtonX + layout.sideButtonW + buttonSpacing;
  layout.rightButtonX = layout.centerButtonX + layout.centerButtonW + buttonSpacing;
  
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
  int bpmValueY = layout.dialogY + SCALE_Y(55);
  tft.setTextColor(THEME_ACCENT, THEME_SURFACE);
  tft.drawCentreString(String(sharedBPM), DISPLAY_CENTER_X, bpmValueY, 7);
  
  // BPM label
  int bpmLabelY = layout.dialogY + SCALE_Y(95);
  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawCentreString("BPM", DISPLAY_CENTER_X, bpmLabelY, 2);
  
  // Draw buttons
  drawRoundButton(layout.leftButtonX, layout.buttonY, layout.sideButtonW, layout.buttonH, "-", THEME_SECONDARY, false, 4);
  drawRoundButton(layout.centerButtonX, layout.buttonY, layout.centerButtonW, layout.buttonH, "CLOSE", THEME_PRIMARY, false, 2);
  drawRoundButton(layout.rightButtonX, layout.buttonY, layout.sideButtonW, layout.buttonH, "+", THEME_SECONDARY, false, 4);
}

void handleBPMSettingsMode() {
  if (!touch.justPressed) {
    return;
  }
  
  BPMDialogLayout layout = calculateDialogLayout();
  
  // Minus button
  if (isButtonPressed(layout.leftButtonX, layout.buttonY, layout.sideButtonW, layout.buttonH)) {
    if (sharedBPM > kBpmMin) {
      uint16_t newBPM = (sharedBPM - kBpmStep < kBpmMin) ? kBpmMin : sharedBPM - kBpmStep;
      setSharedBPM(newBPM);
      requestRedraw();
    }
    return;
  }
  
  // Plus button
  if (isButtonPressed(layout.rightButtonX, layout.buttonY, layout.sideButtonW, layout.buttonH)) {
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
  bool closeButton = isButtonPressed(layout.centerButtonX, layout.buttonY, layout.centerButtonW, layout.buttonH);
  
  if (closeButton || outsideDialog) {
    // Return to the previous mode using proper mode switching
    switchMode(previousMode);
    return;
  }
}
