#ifndef MODULE_RANDOM_GENERATOR_MODE_H
#define MODULE_RANDOM_GENERATOR_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Random Generator mode variables
struct RandomGen {
  int rootNote = 60; // C4
  int scaleType = 0; // Major
  int minOctave = 3;
  int maxOctave = 6;
  int probability = 50; // 0-100%
  int subdivision = 4; // 4=quarter, 8=eighth, 16=sixteenth
  int currentNote = -1;
};

extern RandomGen randomGen;

// Function declarations
void initializeRandomGeneratorMode();
void drawRandomGeneratorMode();
void handleRandomGeneratorMode();
void drawRandomGenControls();
void updateRandomGenerator();
void playRandomNote();
void registerRandomStepCallback();

#endif // MODULE_RANDOM_GENERATOR_MODE_H
