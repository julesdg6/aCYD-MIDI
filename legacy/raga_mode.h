/*******************************************************************
 RAGA Mode - Indian Classical Music Raga Player
 
 Features:
 - 10 traditional ragas with authentic scales
 - Microtonal slides using pitch bend
 - Automatic phrase generation
 - Drone (tanpura) support
 *******************************************************************/

#ifndef RAGA_MODE_H
#define RAGA_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

#define RAGA_COUNT 10

enum RagaType {
  BHAIRAVI = 0,
  LALIT,
  BHUPALI,
  TODI,
  MADHUVANTI,
  MEGHMALHAR,
  YAMAN,
  KALAVATI,
  MALKAUNS,
  BAIRAGI
};

struct RagaScale {
  const char* name;
  uint8_t notes[12];     // MIDI note offsets from root (255 = not used)
  uint8_t numNotes;
  int8_t microtonalCents[12]; // Microtonal adjustments in cents (-50 to +50)
  uint16_t color;        // Display color
};

struct RagaState {
  RagaType currentRaga;
  uint8_t rootNote;      // Base note (default C = 60)
  bool playing;
  bool droneEnabled;
  uint8_t tempo;         // Delay between notes (0-255)
  uint8_t currentStep;
  unsigned long lastNoteTime;
  int8_t currentNote;    // Current playing note
  uint8_t octaveRange;   // 1-3 octaves
  
  // Layout constants (calculated during draw)
  int ragaBtnW, ragaBtnH, ragaBtnStartX, ragaBtnStartY;
  int ragaBtnSpacing, ragaBtnRowSpacing;
  int sliderX, sliderY, sliderW, sliderH;
  int ctrlY, ctrlH, ctrlW;
};

extern RagaState raga;
extern const RagaScale ragaScales[RAGA_COUNT];

// Function declarations
void initializeRagaMode();
void drawRagaMode();
void handleRagaMode();
void playRagaNote(uint8_t scaleIndex, bool slide);
void startDrone();
void stopDrone();

#endif
