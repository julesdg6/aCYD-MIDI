#ifndef MODULE_GRID_PIANO_MODE_H
#define MODULE_GRID_PIANO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Grid Piano mode variables (Linnstrument-style all 4ths layout)
#define GRID_COLS 8
#define GRID_ROWS 5
extern int gridOctave;
extern int gridPressedNote;
extern int gridLayout[GRID_ROWS][GRID_COLS];

// Function declarations
void initializeGridPianoMode();
void drawGridPianoMode();
void handleGridPianoMode();
void drawGridCell(int row, int col, bool pressed = false);
void calculateGridLayout();
int getGridNote(int row, int col);

#endif // MODULE_GRID_PIANO_MODE_H
