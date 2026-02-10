#ifndef MODULE_DIMENSIONS_MODE_H
#define MODULE_DIMENSIONS_MODE_H

#include "common_definitions.h"
#include "midi_utils.h"
#include "ui_elements.h"
#include "clock_manager.h"

// Dimensions parametric sequencer
// Ported from: https://github.com/ErikOostveen/Dimensions (Dimensions_December_18_2021)

#define DIMENSIONS_MAX_NOTES 108  // C0-B8 (MIDI 12-119)
#define DIMENSIONS_NUM_EQUATIONS 20
#define DIMENSIONS_INTERVAL_DIVISIONS 10

// Parametric equation state
struct DimensionsState {
  // Equation selection
  int fx_select;  // 1-20
  
  // Parameters (user-controllable)
  float a;
  float b;
  float c;
  float d;
  float rnd;
  
  // Time variable (advances with clock)
  float t;
  float value_t1;  // t range min
  float value_t2;  // t range max
  float value_gran;  // t increment per step
  
  // Axis offsets
  float x;  // Interval offset
  float y;  // Pitch offset
  
  // Note selection subset (which notes can be played)
  bool enabledNotes[DIMENSIONS_MAX_NOTES];  // true if note enabled
  int noteSubset[DIMENSIONS_MAX_NOTES];  // Sorted list of enabled MIDI notes
  int noteSubsetSize;  // Number of enabled notes
  
  // Interval divisions (clock divisions for x-axis mapping)
  int intervalDivisions[DIMENSIONS_INTERVAL_DIVISIONS];  // Clock ticks per step
  
  // Current step output values (px, py, pz from equations)
  float px;  // Maps to interval (x-axis)
  float py;  // Maps to note (y-axis)
  float pz;  // Maps to velocity (z-axis)
  
  // MIDI output settings
  int midiChannel;
  int octaveTranspose;
  int velocityMode;  // 0 = fixed (127), 1 = from equation
  int pz_offset;  // Velocity offset
  
  // Trigger/mask settings
  int triggerMode;  // 0 = notes above pyRest, 1 = notes below pyRest
  float pyRest;  // Rest threshold (0-127)
  
  // Playback state
  int currentClkDiv;  // Current clock division (from px mapping)
  int lastNotePlayed;  // For note-off tracking
  bool noteActive;  // Is a note currently playing?
  
  // Visual/trace
  int traceMode;  // 0 = off, 1 = on (draw path)
};

extern DimensionsState dimensionsState;

// Core functions
void initializeDimensionsMode();
void drawDimensionsMode();
void handleDimensionsMode();
void updateDimensionsSequencer();

// Engine functions
void dimensionsEvaluateEquation(int equationNum, float t, float a, float b, float c, float d, float rnd,
                                float x, float y, float &px_out, float &py_out, float &pz_out);
void dimensionsPlayNote();
void dimensionsReleaseNote();
void dimensionsResetSequencer();

#endif // MODULE_DIMENSIONS_MODE_H
