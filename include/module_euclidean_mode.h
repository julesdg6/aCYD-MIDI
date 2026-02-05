#ifndef MODULE_EUCLIDEAN_MODE_H
#define MODULE_EUCLIDEAN_MODE_H

#include "common_definitions.h"
#include "midi_utils.h"
#include "ui_elements.h"

#define EUCLIDEAN_MAX_STEPS 32
#define EUCLIDEAN_VOICE_COUNT 4

struct EuclideanVoice {
  uint8_t steps;
  uint8_t events;
  int8_t rotation;
  uint8_t midiNote;
  uint16_t color;
  bool pattern[EUCLIDEAN_MAX_STEPS];
};

struct EuclideanState {
  EuclideanVoice voices[EUCLIDEAN_VOICE_COUNT];
  uint8_t bpm;
  uint8_t currentStep;
  bool tripletMode;
  bool pendingNoteRelease[EUCLIDEAN_VOICE_COUNT];
};

extern EuclideanState euclideanState;

void initializeEuclideanMode();
void drawEuclideanMode();
void handleEuclideanMode();
void updateEuclideanSequencer();
void registerEuclidStepCallback();

#endif // MODULE_EUCLIDEAN_MODE_H
