/*******************************************************************
 TB-3PO Mode - TB-303 Acid Pattern Generator
 Based on TB-3PO by Logarhythm for O_C
 Generates random TB-303 style acid patterns with:
 - Euclidean-style rhythm generation
 - Pitch slides, accents, and gate patterns
 - Density control for note/rest balance and pitch variety
 - Scale quantization with root and octave controls
 *******************************************************************/

#ifndef TB3PO_MODE_H
#define TB3PO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

#define TB3PO_MAX_STEPS 16
#define TB3PO_MIN_BPM 60
#define TB3PO_MAX_BPM 240

struct TB3POState {
  // Sequence data
  uint16_t gates = 0;      // Bitfield of gates
  uint16_t slides = 0;     // Bitfield of slides
  uint16_t accents = 0;    // Bitfield of accents
  uint16_t oct_ups = 0;    // Bitfield of octave ups
  uint16_t oct_downs = 0;  // Bitfield of octave downs
  uint8_t notes[TB3PO_MAX_STEPS] = {0}; // Note indices in scale
  
  // Playback
  uint8_t step = 0;
  uint8_t numSteps = 16;
  bool playing = false;
  unsigned long lastStepTime = 0;
  unsigned long stepInterval = 125; // milliseconds per step (120 BPM, 16th notes)
  int currentNote = -1;
  
  // Generation parameters
  uint16_t seed = 12345;
  bool lockSeed = false;
  uint8_t density = 7;      // 0-14, mapped to -7 to +7
  int scaleIndex = 0;       // Index into scales[] array
  uint8_t rootNote = 0;     // Root note 0-11 (C-B)
  int8_t octaveOffset = 0;  // -3 to +3
  
  // BPM control
  float bpm = 120.0;
  bool useInternalClock = true;
  
  // Touch handling
  bool readyForInput = false; // Wait for initial touch release before accepting input
};

extern TB3POState tb3po;

// Function declarations
void initializeTB3POMode();
void drawTB3POMode();
void updateTB3POSteps();  // Efficient partial redraw
void handleTB3POMode();

#endif
