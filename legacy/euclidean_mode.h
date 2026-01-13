#ifndef EUCLIDEAN_MODE_H
#define EUCLIDEAN_MODE_H

#include <Arduino.h>

// Euclidean rhythm generator state
// Implements Bjorklund's algorithm for generating rhythmic patterns
// Inspired by Mutable Instruments Grids and Ableton Live's Euclidean sequencer

struct EuclideanVoice {
  uint8_t steps;        // Total steps in the sequence (1-32)
  uint8_t events;       // Number of events to distribute (0-steps)
  int8_t rotation;      // Rotation offset (-steps to +steps)
  uint8_t midiNote;     // MIDI note to play
  uint16_t color;       // Display color
  bool pattern[32];     // Generated pattern
};

struct EuclideanState {
  EuclideanVoice voices[4];  // 4 independent rhythm voices
  uint8_t bpm;               // Tempo (40-240 BPM)
  uint8_t currentStep;       // Current playback position (0-31)
  bool isPlaying;            // Playback state
  unsigned long lastStepTime; // For timing
  uint8_t selectedVoice;     // Currently selected voice for editing (0-3)
  bool tripletMode;          // false = 16th notes, true = triplet divisions
};

extern EuclideanState euclideanState;

// Core functions
void initializeEuclideanMode();
void drawEuclideanMode();
void handleEuclideanMode();

// Pattern generation using Bjorklund's algorithm
void generateEuclideanPattern(EuclideanVoice& voice);

// Playback
void updateEuclideanSequencer();
void playEuclideanStep();

#endif // EUCLIDEAN_MODE_H
