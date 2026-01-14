#ifndef MODULE_LFO_MODE_H
#define MODULE_LFO_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// LFO mode variables
struct LFOParams {
  float rate = 1.0;      // Hz (0.1 - 10.0)
  int amount = 64;       // 0-127
  int ccTarget = 1;      // CC number (0-127) or -1 for pitchwheel
  bool isRunning = false;
  float phase = 0.0;     // Current phase (0-2π)
  int waveform = 0;      // 0=Sine, 1=Triangle, 2=Square, 3=Sawtooth
  unsigned long lastUpdate = 0;
  int lastValue = 64;    // Last sent value
  bool pitchWheelMode = false; // Special mode for pitchwheel
};

extern LFOParams lfo;
extern String waveNames[];

// Function declarations
void initializeLFOMode();
void drawLFOMode();
void handleLFOMode();
void drawLFOControls();
void updateLFO();
float calculateLFOValue();
void sendLFOValue(int value);
void drawWaveform();

#endif // MODULE_LFO_MODE_H
