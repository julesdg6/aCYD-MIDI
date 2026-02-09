#ifndef MODULE_MORPH_MODE_H
#define MODULE_MORPH_MODE_H

#include "common_definitions.h"
#include "midi_utils.h"
#include "ui_elements.h"

#define MORPH_SLOTS 4
#define MORPH_MAX_POINTS 96

struct MorphPoint {
  uint8_t x;
  uint8_t y;
};

struct MorphSlot {
  bool hasData;
  uint16_t length;
  MorphPoint points[MORPH_MAX_POINTS];
};

struct MorphState {
  float morphX;
  float morphY;
  uint8_t activeSlot;
  bool recording;
  bool recordGestureActive;
  bool playing;
  uint16_t playIndex;
  uint32_t lastRecordSampleMs;
  uint32_t lastPlayStepMs;
  uint8_t outputX;
  uint8_t outputY;
  MorphSlot slots[MORPH_SLOTS];
};

extern MorphState morphState;

void initializeMorphMode();
void drawMorphMode();
void handleMorphMode();
void playMorphNote();

#endif // MODULE_MORPH_MODE_H
