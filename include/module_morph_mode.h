#ifndef MODULE_MORPH_MODE_H
#define MODULE_MORPH_MODE_H

#include "common_definitions.h"
#include "midi_utils.h"
#include "ui_elements.h"

#define MORPH_SLOTS 4

struct MorphState {
  float morphX;
  float morphY;
  uint8_t activeSlot;
  bool recording;
};

extern MorphState morphState;

void initializeMorphMode();
void drawMorphMode();
void handleMorphMode();
void playMorphNote();

#endif // MODULE_MORPH_MODE_H
