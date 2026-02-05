#ifndef MODULE_RAGA_MODE_H
#define MODULE_RAGA_MODE_H

#include "common_definitions.h"
#include "midi_utils.h"
#include "ui_elements.h"

#define RAGA_COLUMNS 4
#define RAGA_COUNT 8

enum RagaType {
  RAGA_BHAIRAVI = 0,
  RAGA_LALIT,
  RAGA_BHUPALI,
  RAGA_TODI,
  RAGA_MADHUVANTI,
  RAGA_MEGHMALHAR,
  RAGA_YAMAN,
  RAGA_MALKAUNS,
};

enum TalaType {
  TALA_TEENTAL = 0,
  TALA_RUPAK,
  TALA_JHAPTAL,
  TALA_COUNT,
};

struct RagaState {
  RagaType currentRaga;
  TalaType currentTala;
  uint8_t rootNote;
  bool playing;
  bool droneEnabled;
};

extern const char *const kRagaNames[RAGA_COUNT];
extern const char *const kTalaNames[TALA_COUNT];
extern RagaState raga;

void initializeRagaMode();
void drawRagaMode();
void handleRagaMode();
void toggleRagaPlayback();

#endif // MODULE_RAGA_MODE_H
