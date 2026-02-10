#ifndef MODULE_BABY8_MODE_H
#define MODULE_BABY8_MODE_H

#ifdef ENABLE_BABY8_EMU

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Baby8 sequencer configuration
#define BABY8_STEPS 8
#define BABY8_MAX_PATTERNS 8
#define BABY8_MIN_NOTE 0
#define BABY8_MAX_NOTE 127

// Baby8 step structure
struct Baby8Step {
  uint8_t note;        // MIDI note value (0-127)
  uint8_t velocity;    // MIDI velocity (0-127)
  bool gate;           // Gate on/off
};

// Baby8 pattern structure
struct Baby8Pattern {
  Baby8Step steps[BABY8_STEPS];
  uint8_t patternLength;  // Actual number of active steps (1-8)
  char name[16];          // Pattern name
};

// Baby8 state
struct Baby8State {
  Baby8Pattern patterns[BABY8_MAX_PATTERNS];
  uint8_t currentPatternIndex;
  uint8_t currentStep;
  uint16_t bpm;
  uint8_t swing;           // Swing amount 0-100%
  uint8_t midiChannel;     // MIDI channel (0-15)
  bool playing;
  bool startPending;
  unsigned long lastStepTime;
  unsigned long noteOffTime;
  uint8_t activeNote;      // Currently playing note
  uint8_t selectedStep;    // Currently selected step for editing
  uint8_t selectedEncoder; // Selected virtual encoder (0-7)
};

// Function declarations
void initializeBaby8Mode();
void drawBaby8Mode();
void handleBaby8Mode();
void updateBaby8Sequencer();
void playBaby8Step();
void saveBaby8Patterns();
void loadBaby8Patterns();
void resetBaby8Pattern(uint8_t patternIndex);

// UI drawing functions
void drawBaby8StepGrid();
void drawBaby8Encoders();
void handleBaby8EncoderTouch();

// Encoder simulation functions
uint8_t getBaby8EncoderValue(uint8_t encoderIndex);
void setBaby8EncoderValue(uint8_t encoderIndex, uint8_t value);

#endif // ENABLE_BABY8_EMU

#endif // MODULE_BABY8_MODE_H
