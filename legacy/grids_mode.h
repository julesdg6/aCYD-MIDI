/*******************************************************************
 GRIDS Mode - Mutable Instruments Grids Drum Sequencer
 Adapted for MIDI based on MI Grids by Emilie Gillet
 
 Features:
 - X/Y pad for pattern selection and variation
 - 3 drum voices (Kick, Snare, Hi-hat)
 - Pattern map interpolation
 - Density/Fill control per voice
 - BPM control
 *******************************************************************/

#ifndef GRIDS_MODE_H
#define GRIDS_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

#define GRIDS_STEPS 16
#define GRIDS_MIN_BPM 60
#define GRIDS_MAX_BPM 240

struct GridsState {
  // Playback
  uint8_t step = 0;
  bool playing = false;
  unsigned long lastStepTime = 0;
  unsigned long stepInterval = 125; // milliseconds per step
  float bpm = 120.0;
  
  // Pattern control (X/Y coordinates, 0-255)
  uint8_t patternX = 128;  // X position in pattern map
  uint8_t patternY = 128;  // Y position in pattern map
  
  // Per-voice density (0-255, controls fill amount)
  uint8_t kickDensity = 200;
  uint8_t snareDensity = 150;
  uint8_t hatDensity = 180;
  
  // MIDI note assignments
  uint8_t kickNote = 36;   // C1 - Kick
  uint8_t snareNote = 38;  // D1 - Snare
  uint8_t hatNote = 42;    // F#1 - Closed Hi-hat
  
  // Swing amount (0-100%)
  uint8_t swing = 0;
  
  // Accent threshold (0-255)
  uint8_t accentThreshold = 200;
  
  // Current pattern buffers (0-255 per step)
  uint8_t kickPattern[GRIDS_STEPS];
  uint8_t snarePattern[GRIDS_STEPS];
  uint8_t hatPattern[GRIDS_STEPS];
};

extern GridsState grids;

// Function declarations
void initializeGridsMode();
void drawGridsMode();
void handleGridsMode();
void regenerateGridsPattern();

#endif
