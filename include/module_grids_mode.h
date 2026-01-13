#ifndef MODULE_GRIDS_MODE_H
#define MODULE_GRIDS_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

#define GRIDS_STEPS 16
#define GRIDS_MIN_BPM 60
#define GRIDS_MAX_BPM 240

struct GridsState {
  uint8_t step;
  bool playing;
  unsigned long lastStepTime;
  unsigned long stepInterval;
  float bpm;
  uint8_t patternX;
  uint8_t patternY;
  uint8_t kickDensity;
  uint8_t snareDensity;
  uint8_t hatDensity;
  uint8_t kickNote;
  uint8_t snareNote;
  uint8_t hatNote;
  uint8_t swing;
  uint8_t accentThreshold;
  uint8_t kickPattern[GRIDS_STEPS];
  uint8_t snarePattern[GRIDS_STEPS];
  uint8_t hatPattern[GRIDS_STEPS];
};

extern GridsState grids;

void initializeGridsMode();
void drawGridsMode();
void handleGridsMode();
void regenerateGridsPattern();

#endif // MODULE_GRIDS_MODE_H
