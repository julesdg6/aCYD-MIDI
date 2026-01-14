#ifndef MODULE_SEQUENCER_MODE_H
#define MODULE_SEQUENCER_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Sequencer mode variables
#define SEQ_STEPS 16
#define SEQ_TRACKS 4
extern bool sequencePattern[SEQ_TRACKS][SEQ_STEPS];
extern int currentStep;
extern unsigned long lastStepTime;
extern unsigned long noteOffTime[SEQ_TRACKS];
extern int bpm;
extern int stepInterval;
extern bool sequencerPlaying;

// Function declarations
void initializeSequencerMode();
void drawSequencerMode();
void handleSequencerMode();
void drawSequencerGrid();
void toggleSequencerStep(int track, int step);
void updateSequencer();
void playSequencerStep();

#endif // MODULE_SEQUENCER_MODE_H
