#ifndef MODULE_ARPEGGIATOR_MODE_H
#define MODULE_ARPEGGIATOR_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Arpeggiator mode variables
struct Arpeggiator {
  int scaleType = 0; // Scale for chord generation
  int chordType = 0; // 0=Major, 1=Minor, 2=7th
  int pattern = 0; // 0=Up, 1=Down, 2=UpDown, 3=Random
  int octaves = 2;
  int speed = 8; // 16th notes
  int bpm = 120; // BPM control
  int currentStep = 0;
  int currentNote = -1; // Current single note being played
  int triggeredKey = -1; // Which piano key triggered the arp
  int triggeredOctave = 4; // Octave of the triggered key
  float stepTicks = 2.0f;
  float tickAccumulator = 0.0f;
};

#define NUM_PIANO_KEYS 12

extern Arpeggiator arp;
extern String patternNames[];
extern String chordTypeNames[];
extern int pianoOctave;

// Function declarations
void initializeArpeggiatorMode();
void drawArpeggiatorMode();
void handleArpeggiatorMode();
void drawArpControls();
void drawPianoKeys();
void updateArpeggiator();
void playArpNote();
int getArpNote();
void calculateStepInterval();

#endif // MODULE_ARPEGGIATOR_MODE_H
