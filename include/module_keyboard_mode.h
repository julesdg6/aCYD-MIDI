#ifndef MODULE_KEYBOARD_MODE_H
#define MODULE_KEYBOARD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Keyboard mode variables
#define NUM_KEYS 10  // More keys per row
#define NUM_ROWS 2   // Two rows

extern int keyboardOctave;
extern int keyboardScale;
extern int keyboardKey;  // Key signature (C=0, C#=1, D=2, etc.)
extern int lastKey;
extern int lastRow;

void initializeKeyboardMode();
void drawKeyboardMode();
void handleKeyboardMode();
void drawKeyboardKey(int row, int keyIndex, bool pressed);
void playKeyboardNote(int row, int keyIndex, bool on);

#endif // MODULE_KEYBOARD_MODE_H
