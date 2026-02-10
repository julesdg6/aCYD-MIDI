#ifndef MODULE_FRACTAL_ECHO_MODE_H
#define MODULE_FRACTAL_ECHO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Maximum values for arrays
#define FRAC_MAX_TAPS 4
#define FRAC_MAX_ITER 6
#define FRAC_MAX_EVENTS 128

// Fractal parameters
struct FractalParams {
  bool enabled;
  uint8_t maxEchoesPerNote;
  
  // Timing parameters
  uint16_t tapsMs[FRAC_MAX_TAPS];
  uint8_t iterations;
  float stretch;
  
  // Dynamics parameters
  float velocityDecay;
  uint8_t minVelocity;
  uint16_t baseLengthMs;
  float lengthDecay;
  
  // Offsets (per iteration)
  int8_t offsets[FRAC_MAX_ITER];
};

// Scheduled MIDI event
struct MidiEvent {
  uint32_t dueTimeMs;
  uint8_t status;
  uint8_t data1;
  uint8_t data2;
  bool active;
};

// Fractal Note Echo effect state
extern FractalParams fractalParams;
extern MidiEvent eventQueue[FRAC_MAX_EVENTS];
extern int eventQueueSize;

// UI state
extern int fractalPage;  // 0=Timing, 1=Dynamics, 2=Offsets

// Function declarations
void initializeFractalEchoMode();
void drawFractalEchoMode();
void handleFractalEchoMode();

// Effect processing
void processFractalEcho();
void addFractalEcho(uint8_t note, uint8_t velocity, uint8_t channel);
void processMidiEvents();

#endif // MODULE_FRACTAL_ECHO_MODE_H
