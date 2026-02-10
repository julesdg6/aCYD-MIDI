#include "app/app_modes.h"

#include "app/app_menu.h"

#include "midi_utils.h"
#include "module_arpeggiator_mode.h"
#include "module_auto_chord_mode.h"
#include "module_bouncing_ball_mode.h"
#include "module_bpm_settings_mode.h"
#include "module_euclidean_mode.h"
#include "module_fractal_echo_mode.h"
#include "module_grid_piano_mode.h"
#include "module_grids_mode.h"
#include "module_keyboard_mode.h"
#include "module_lfo_mode.h"
#include "module_morph_mode.h"
#include "module_physics_drop_mode.h"
#include "module_raga_mode.h"
#include "module_random_generator_mode.h"
#include "module_sequencer_mode.h"
#include "module_settings_mode.h"
#include "module_slink_mode.h"
#include "module_tb3po_mode.h"
#include "module_waaave_mode.h"
#include "module_xy_pad_mode.h"

namespace {

using ModeFn = void (*)();

struct ModeEntry {
  ModeFn init;
  ModeFn draw;
  ModeFn handle;
};

static void initMenuMode() {
  stopAllModes();
}

static void initSettingsMode() {
  stopAllModes();
  initializeSettingsMode();
}

constexpr size_t kModeCount = static_cast<size_t>(FRACTAL_ECHO) + 1;

static void initBPMSettingsMode() {
  // Don't stop playback - allow BPM adjustment during playback
  initializeBPMSettingsMode();
}

constexpr ModeEntry kModeTable[kModeCount] = {
    /* MENU */ {initMenuMode, drawMenu, handleMenu},
    /* SETTINGS */ {initSettingsMode, drawSettingsMode, handleSettingsMode},
    /* BPM_SETTINGS */ {initBPMSettingsMode, drawBPMSettingsMode, handleBPMSettingsMode},
    /* KEYBOARD */ {initializeKeyboardMode, drawKeyboardMode, handleKeyboardMode},
    /* SEQUENCER */ {initializeSequencerMode, drawSequencerMode, handleSequencerMode},
    /* BOUNCING_BALL */ {initializeBouncingBallMode, drawBouncingBallMode, handleBouncingBallMode},
    /* PHYSICS_DROP */ {initializePhysicsDropMode, drawPhysicsDropMode, handlePhysicsDropMode},
    /* RANDOM_GENERATOR */ {initializeRandomGeneratorMode, drawRandomGeneratorMode, handleRandomGeneratorMode},
    /* XY_PAD */ {initializeXYPadMode, drawXYPadMode, handleXYPadMode},
    /* ARPEGGIATOR */ {initializeArpeggiatorMode, drawArpeggiatorMode, handleArpeggiatorMode},
    /* GRID_PIANO */ {initializeGridPianoMode, drawGridPianoMode, handleGridPianoMode},
    /* AUTO_CHORD */ {initializeAutoChordMode, drawAutoChordMode, handleAutoChordMode},
    /* LFO */ {initializeLFOMode, drawLFOMode, handleLFOMode},
    /* SLINK */ {initializeSlinkMode, drawSlinkMode, handleSlinkMode},
    /* TB3PO */ {initializeTB3POMode, drawTB3POMode, handleTB3POMode},
    /* GRIDS */ {initializeGridsMode, drawGridsMode, handleGridsMode},
    /* RAGA */ {initializeRagaMode, drawRagaMode, handleRagaMode},
    /* EUCLID */ {initializeEuclideanMode, drawEuclideanMode, handleEuclideanMode},
    /* MORPH */ {initializeMorphMode, drawMorphMode, handleMorphMode},
    /* WAAAVE */ {initializeWaaaveMode, drawWaaaveMode, handleWaaaveMode},
    /* FRACTAL_ECHO */ {initializeFractalEchoMode, drawFractalEchoMode, handleFractalEchoMode},
};

static_assert(sizeof(kModeTable) / sizeof(kModeTable[0]) == kModeCount,
              "Mode table must cover all AppMode values.");

static const ModeEntry *getModeEntry(AppMode mode) {
  size_t index = static_cast<size_t>(mode);
  if (index >= kModeCount) {
    return nullptr;
  }
  return &kModeTable[index];
}

}  // namespace

void registerAllStepCallbacks() {
  // Register step callbacks for modules that use uClock step extension.
  // Only TB-3PO currently uses ISR-based step counting.
  // All other modules use consumeReadySteps() for direct clock manager queries.
  registerTB3POStepCallback();
}

void appDrawCurrentMode() {
  const ModeEntry *entry = getModeEntry(currentMode);
  if (entry && entry->draw) {
    entry->draw();
  }
}

void switchMode(AppMode mode) {
  currentMode = mode;
  const ModeEntry *entry = getModeEntry(mode);
  if (entry && entry->init) {
    entry->init();
  }
  requestRedraw();
}

void exitToMenu() {
  switchMode(MENU);
}

void appHandleCurrentMode() {
  const ModeEntry *entry = getModeEntry(currentMode);
  if (entry && entry->handle) {
    entry->handle();
  }
}
