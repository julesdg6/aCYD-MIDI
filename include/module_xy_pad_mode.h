#ifndef MODULE_XY_PAD_MODE_H
#define MODULE_XY_PAD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// XY Pad mode variables
extern int xCC;  // CC number for X axis (Modulation Wheel by default)
extern int yCC;  // CC number for Y axis (Volume by default)
extern int xValue;  // Current X value (0-127)
extern int yValue;  // Current Y value (0-127)
extern bool padPressed;
extern int padX;
extern int padY;  // Touch position on pad
extern bool xyPadNeedsReset;  // Flag to reset static variables

// Pad area dimensions - using scaled dimensions
#define PAD_X SCALE_X(20)
#define PAD_Y (HEADER_HEIGHT + SCALE_Y(15))
#define PAD_WIDTH SCALE_X(200)
#define PAD_HEIGHT SCALE_Y(140)
#define PAD_CENTER_X (PAD_X + PAD_WIDTH/2)
#define PAD_CENTER_Y (PAD_Y + PAD_HEIGHT/2)

// Function declarations
void initializeXYPadMode();
void drawXYPadMode();
void handleXYPadMode();
void drawXYPad();
void drawCCControls();
void updateXYValues(int touchX, int touchY);
void sendXYValues();

#endif // MODULE_XY_PAD_MODE_H
