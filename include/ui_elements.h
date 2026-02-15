#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"
#include "clock_runtime.h"

void exitToMenu();

// UI implementations
void updateTouch();
bool isButtonPressed(int x, int y, int w, int h);
void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed = false, uint8_t textFont = 1);
void drawHeader(String title, String subtitle, uint8_t titleFont = 4, bool showBackButton = true);
void updateStatus();
// Check if the BPM value displayed in the header has been tapped
bool isBPMValueTapped();

// ========== Transport UI Helpers ==========

/**
 * Draw a standardized transport button (Play/Stop/Pending)
 * Shows consistent state across all clocked modules
 */
void drawTransportButton(int x, int y, int w, int h, TransportState state);

/**
 * Get transport button label based on state
 */
const char* getTransportButtonLabel(TransportState state);

/**
 * Get transport button color based on state
 */
uint16_t getTransportButtonColor(TransportState state);

#endif
