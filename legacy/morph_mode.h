#ifndef MORPH_MODE_H
#define MORPH_MODE_H

#include <Arduino.h>

// MORPH - Gesture recording and morphing sequencer
// Neural-network-inspired system that records touch gestures,
// stores them in 4 memory slots, and morphologically interpolates
// between them to create evolving musical patterns

#define MAX_GESTURE_POINTS 128
#define NUM_MEMORY_SLOTS 4

struct GesturePoint {
  float x;              // 0.0-1.0 normalized position
  float y;              // 0.0-1.0 normalized position
  unsigned long time;   // Timestamp for velocity calculation
  float velocity;       // Calculated from distance/time
  float pressure;       // Simulated from velocity
};

struct Gesture {
  GesturePoint points[MAX_GESTURE_POINTS];
  int numPoints;
  bool isValid;
  float duration;       // Total duration in ms
  uint16_t color;       // Display color
};

struct MorphState {
  // Memory slots (4 corners)
  Gesture memories[NUM_MEMORY_SLOTS];
  int currentMemorySlot;     // Which slot to record to (0-3)
  
  // Morphing control (0.0-1.0)
  float morphX;              // Blend between left (0) and right (1) columns
  float morphY;              // Blend between top (0) and bottom (1) rows
  
  // Playback state
  bool isPlaying;
  float playbackPosition;    // 0.0-1.0 through current morphed gesture
  unsigned long lastPlaybackTime;
  Gesture morphedGesture;    // Current interpolated gesture
  
  // Recording state
  bool isRecording;
  int recordPointIndex;
  unsigned long recordStartTime;
  
  // Generation parameters
  uint8_t mutationAmount;    // 0-100 (amount of random variation)
  uint8_t quantizeSteps;     // 0=off, 4/8/12/16 = chromatic quantization
  uint8_t bpm;               // Playback tempo
  uint8_t rootNote;          // Base MIDI note (C3 = 48)
  
  // Visual trail
  int trailPoints[32][2];    // Recent playback positions for trail effect
  int trailIndex;
};

extern MorphState morphState;

// Core functions
void initializeMorphMode();
void drawMorphMode();
void handleMorphMode();

// Gesture recording
void startRecording(int memorySlot);
void recordGesturePoint(float x, float y);
void stopRecording();

// Gesture morphing (bilinear interpolation between 4 corners)
void morphGestures();

// Playback and MIDI generation
void updateMorphPlayback();
void generateMIDIFromGesture(const GesturePoint& point);

// Mutation (procedural variation)
void mutateGesture(Gesture& gesture, uint8_t amount);

// Spline interpolation for smooth gesture playback
GesturePoint interpolateGesture(const Gesture& gesture, float t);

#endif // MORPH_MODE_H
