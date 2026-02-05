#ifndef MODULE_TB3PO_MODE_H
#define MODULE_TB3PO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

#define TB3PO_MAX_STEPS 16
#define TB3PO_MIN_BPM 60
#define TB3PO_MAX_BPM 240

// TB-3PO state structure
struct TB3POState {
  uint16_t gates = 0;
  uint16_t slides = 0;
  uint16_t accents = 0;
  uint16_t oct_ups = 0;
  uint16_t oct_downs = 0;
  uint8_t notes[TB3PO_MAX_STEPS] = {0};

  uint8_t step = 0;
  uint8_t numSteps = TB3PO_MAX_STEPS;
  int currentNote = -1;

  uint16_t seed = 12345;
  bool lockSeed = false;
  uint8_t density = 7;
  uint8_t scaleIndex = 0;
  uint8_t rootNote = 0;
  int8_t octaveOffset = 0;
  float bpm = 120.0f;
  bool useInternalClock = true;
  bool readyForInput = false;
};

extern TB3POState tb3po;

void initializeTB3POMode();
void drawTB3POMode();
void handleTB3POMode();
void updateTB3POPlayback();
void registerTb3poStepCallback();

#endif // MODULE_TB3PO_MODE_H
