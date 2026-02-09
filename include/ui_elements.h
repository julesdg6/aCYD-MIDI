#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"

void exitToMenu();

// UI implementations
void updateTouch();
bool isButtonPressed(int x, int y, int w, int h);
void drawRoundButton(int x, int y, int w, int h, String text, uint16_t color, bool pressed = false, uint8_t textFont = 1);
void drawHeader(String title, String subtitle, uint8_t titleFont = 4, bool showBackButton = true);
void updateStatus();
// Check if the BPM value displayed in the header has been tapped
bool isBPMValueTapped();

#endif
