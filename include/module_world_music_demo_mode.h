#ifndef MODULE_WORLD_MUSIC_DEMO_MODE_H
#define MODULE_WORLD_MUSIC_DEMO_MODE_H

#include "common_definitions.h"
#include "world_music_core.h"
#include "midi_utils.h"
#include "ui_elements.h"

// ============================================================
// World Music Demo Mode
// ============================================================
// Demonstrates usage of the world_music_core library
// Shows how to create mode bundles and generate phrases

#define WM_DEMO_MAX_PHRASE_LENGTH 32

enum WMDemoModeType {
  WM_DEMO_RAGA_YAMAN = 0,
  WM_DEMO_MAQAM_RAST,
  WM_DEMO_PENTATONIC,
  WM_DEMO_MODE_COUNT
};

struct WorldMusicDemoState {
  WMDemoModeType currentModeType;
  Mode currentMode;
  GeneratorParams genParams;
  
  // Generated phrase
  int8_t phraseNotes[WM_DEMO_MAX_PHRASE_LENGTH];
  uint8_t phraseOctaves[WM_DEMO_MAX_PHRASE_LENGTH];
  uint8_t phraseLength;
  
  // Playback state
  bool isPlaying;
  uint8_t currentStep;
  unsigned long lastNoteTime;
  uint16_t noteInterval;  // ms between notes
};

extern const char *const kWMDemoModeNames[WM_DEMO_MODE_COUNT];
extern WorldMusicDemoState wmDemo;

void initializeWorldMusicDemoMode();
void drawWorldMusicDemoMode();
void handleWorldMusicDemoMode();

// Mode creation functions
Mode createDemoRagaYaman();
Mode createDemoMaqamRast();
Mode createDemoPentatonic();

#endif // MODULE_WORLD_MUSIC_DEMO_MODE_H
