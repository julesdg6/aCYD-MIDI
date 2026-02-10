#ifndef MODULE_ENCODER_PANEL_MODE_H
#define MODULE_ENCODER_PANEL_MODE_H

#include "common_definitions.h"

#ifdef ENABLE_M5_8ENCODER

#include "ui_elements.h"
#include "midi_utils.h"
#include "drivers/m5_8encoder.h"

// Parameter types that can be mapped to encoders
enum ParamType {
  PARAM_MIDI_CC,      // MIDI Control Change
  PARAM_INTERNAL,     // Internal parameter (like BPM, octave, etc.)
  PARAM_DISABLED      // No mapping
};

// Parameter mapping for a single encoder
struct EncoderMapping {
  ParamType type;
  String label;        // Display label (e.g., "Cutoff", "BPM", "Volume")
  uint8_t midiCC;      // MIDI CC number (0-127) when type == PARAM_MIDI_CC
  uint8_t midiChannel; // MIDI channel (0-15)
  int minValue;        // Minimum value
  int maxValue;        // Maximum value
  int currentValue;    // Current value
  int step;            // Increment step (1 for fine, 10 for coarse)
};

// Page of 8 encoder mappings
struct EncoderPage {
  String name;
  EncoderMapping encoders[8];
};

// Encoder panel mode state
extern int currentEncoderPage;
extern bool encoderFineMode;
extern EncoderPage encoderPages[];
extern const int NUM_ENCODER_PAGES;

// Mode functions
void initializeEncoderPanelMode();
void drawEncoderPanelMode();
void handleEncoderPanelMode();

// Mapping management
void loadEncoderMappings();
void saveEncoderMappings();
void setDefaultMappings();

// Helper functions
void updateEncoderValue(int encoderIndex, int delta);
void sendEncoderMIDI(int encoderIndex);
void drawEncoderControl(int encoderIndex, int x, int y, int w, int h);

#endif // ENABLE_M5_8ENCODER

#endif // MODULE_ENCODER_PANEL_MODE_H
