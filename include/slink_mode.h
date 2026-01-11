#ifndef SLINK_MODE_H
#define SLINK_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"
#include <math.h>

// ============================================================
// Constants
// ============================================================
#define SLINK_BANDS 16
#define SLINK_MAX_VOICES 16
#define SLINK_NUM_MODULATORS 6
#define SLINK_TICK_INTERVAL_MS 1

// ============================================================
// Core Data Structures
// ============================================================

// Slink Wave - represents one of the two animated wave engines
typedef struct {
    // Rate controls
    float rate_hz;        // Rate in Hz (0.0 - 4.0)
    bool  sync_mode;      // true = Sync to tempo, false = Hz
    float sync_value;     // Musical divisions (1/64 to 32 bars)
    bool  phase_inverted; // Ø button - invert direction
    bool  rate_bipolar;   // +/- (Hz mode only)
    bool  triplet;        // Triplet modifier
    bool  dotted;         // Dotted modifier
    bool  freeze;         // Freeze animation
    
    // Shape controls
    float multiply;       // 0..1
    float ripple;         // 0..1
    float offset;         // 0..1 (phase offset)
    float invert;         // -1..1 (attenuverter)
    float gravity;        // -1..1
    float scan;           // 0..1 (selects algo/waveshape)
    
    // State
    float phase;          // Current phase 0..2π
    float node_values[SLINK_BANDS]; // Current amplitude at each band 0..1
} SlinkWave;

// Band - configuration for one of the 16 bands
typedef struct {
    bool    enabled;              // Band on/off
    
    // Trigger controls (per-band)
    float   clock_divider;        // Clock speed multiplier (0.1 - 10.0)
    uint8_t trigger_mode;         // 0=Retrigger, 1=Once, 2=Slink
    
    // State
    float   last_trigger_value;   // Last sampled trigger amplitude
    bool    armed_for_once;       // For Once mode - re-arm tracking
    uint32_t last_clock_tick;     // Last time this band was clocked
} Band;

// Active Voice - tracks a currently playing MIDI note
typedef struct {
    bool     active;         // Is this slot in use?
    uint8_t  note;          // MIDI note number
    uint8_t  velocity;      // MIDI velocity
    uint8_t  channel;       // MIDI channel
    uint32_t off_time_ms;   // When to send Note Off (0 = sustain)
    uint8_t  band_index;    // Which band triggered this
} ActiveVoice;

// Trigger Engine State
typedef struct {
    float threshold;        // 0..1 - trigger threshold
    uint8_t vel_min;       // 0..127 - minimum velocity
    uint8_t vel_max;       // 0..127 - maximum velocity
    float forte;           // 0..1 - velocity curve (0=soft, 0.5=linear, 1=hard)
} TriggerEngine;

// Pitch Engine State
typedef struct {
    float spread;          // 0..1 - how much each band can roam the range
    float squish;          // 0..1 - bias distribution
    uint8_t range_semitones; // 0..72 - pitch range in semitones
} PitchEngine;

// Clock Engine State
typedef struct {
    float bpm;             // Tempo in beats per minute
    float swing;           // 0..1 - swing amount
    uint32_t note_len_min; // Minimum note length in ms
    uint32_t note_len_max; // Maximum note length in ms
    bool note_len_x10;     // Multiply lengths by 10
    bool sustain_mode;     // Hold notes until next trigger
    uint8_t max_voices;    // Polyphony limit (1-16)
    
    // State
    uint32_t last_tick_ms; // Last global clock tick
    bool swing_offset;     // Current swing state (even/odd)
} ClockEngine;

// Scale Engine State
typedef struct {
    uint8_t root_note;     // 0-11 (C-B)
    uint8_t scale_index;   // Index into scales[] array
    float color;           // 0..1 - weight of non-root degrees
    bool arp_mode;         // Use held MIDI notes as pitch pool
    bool custom_scale[12]; // Custom scale notes (when using custom)
    
    // Arp mode state
    uint8_t held_notes[128]; // Held MIDI notes from input
    uint8_t num_held_notes;  // Count of held notes
} ScaleEngine;

// Modulator (LFO)
typedef struct {
    bool enabled;          // Is this modulator active?
    uint8_t shape;         // 0=sine, 1=tri, 2=ramp, 3=square, 4=random
    float rate_hz;         // Rate in Hz
    bool sync_mode;        // Sync to tempo
    float sync_value;      // Musical divisions
    bool triplet;          // Triplet modifier
    bool dotted;           // Dotted modifier
    float range;           // Modulation depth 0..1
    
    // State
    float phase;           // Current phase 0..2π
    float output;          // Current output -1..1
    
    // Assignments - which parameters this modulates
    bool mod_multiply;     // Modulate Multiply parameter
    bool mod_ripple;       // Modulate Ripple parameter
    bool mod_offset;       // Modulate Offset parameter
    bool mod_gravity;      // Modulate Gravity parameter
    bool mod_scan;         // Modulate Scan parameter
    bool mod_threshold;    // Modulate Threshold parameter
} Modulator;

// Mod Engine State
typedef struct {
    Modulator mods[SLINK_NUM_MODULATORS]; // 6 modulators A-F
} ModEngine;

// UI State
typedef enum {
    SLINK_TAB_MAIN,
    SLINK_TAB_TRIGGER,
    SLINK_TAB_PITCH,
    SLINK_TAB_CLOCK,
    SLINK_TAB_SCALE,
    SLINK_TAB_MOD,
    SLINK_TAB_SETUP
} SlinkTab;

typedef enum {
    SLINK_SUBPAGE_WAVE_A,     // Main tab: Wave A controls
    SLINK_SUBPAGE_WAVE_B,     // Main tab: Wave B controls
    SLINK_SUBPAGE_BANDS       // Main tab: Band toggles
} SlinkMainSubpage;

// ============================================================
// Global State
// ============================================================
struct SlinkState {
    // Engines
    SlinkWave wave_trigger;    // Wave A - drives triggering
    SlinkWave wave_pitch;      // Wave B - drives pitch
    Band bands[SLINK_BANDS];   // 16 bands
    ActiveVoice voices[SLINK_MAX_VOICES]; // Voice pool
    
    TriggerEngine trigger_engine;
    PitchEngine pitch_engine;
    ClockEngine clock_engine;
    ScaleEngine scale_engine;
    ModEngine mod_engine;
    
    // UI state
    SlinkTab current_tab;
    SlinkMainSubpage main_subpage;
    
    // Timing
    uint32_t last_engine_tick;
    uint32_t current_time_ms;
};

// Global instance (heap-allocated)
extern SlinkState *slink_state_ptr;
#define slink_state (*slink_state_ptr)

// ============================================================
// Function Declarations
// ============================================================

// Lifecycle
void initializeSlinkMode();
void drawSlinkMode();
void handleSlinkMode();
void updateSlinkEngine();

// Engine functions
void updateWavePhase(SlinkWave* wave, float delta_time_s, float bpm);
void computeWaveNodes(SlinkWave* wave);
float computeModulatedParameter(float base_value, int param_index);
void updateModulators(float delta_time_s, float bpm);
void processBandTriggers();
void processVoiceNoteOffs();

// Trigger modes
bool checkBandTrigger(int band_index, float trigger_value);
uint8_t calculateVelocity(float trigger_value, TriggerEngine* engine);

// Pitch generation
uint8_t calculatePitch(int band_index, float pitch_value);
void quantizeToPitch(uint8_t* note, ScaleEngine* engine);

// Voice management
int allocateVoice(uint8_t note, uint8_t velocity, uint8_t channel, 
                  uint32_t off_time, uint8_t band_index);
void releaseVoice(int voice_index);
int findVoiceToSteal();

// Band operations
void toggleBand(int band_index);
void enableRandomBand();
void disableRandomBand();
void shuffleEnabledBands();
void shiftBands();
void enableAllBands();
void clearAllBands();

// UI rendering
void drawMainTab();
void drawTriggerTab();
void drawPitchTab();
void drawClockTab();
void drawScaleTab();
void drawModTab();
void drawSetupTab();

// UI handlers
void handleMainTab();
void handleTriggerTab();
void handlePitchTab();
void handleClockTab();
void handleScaleTab();
void handleModTab();
void handleSetupTab();

// UI helpers
void drawWaveVisualization(int y_start, int height, SlinkWave* wave, 
                          uint16_t color, const char* label);
void drawBandToggles(int y_pos);
void drawThresholdLine(int y_pos, float threshold);
void drawPitchGrid(int y_start, int height);
void drawMiniKeyboard(int y_pos, ScaleEngine* engine);

// Parameter controls
void drawWaveControls(SlinkWave* wave, int y_start, bool is_trigger);
void drawSliderControl(int x, int y, int w, int h, float value, 
                      const char* label, uint16_t color);
void drawKnobControl(int x, int y, int radius, float value, 
                    const char* label, uint16_t color);

// Utility
float applyForteCurve(float normalized, float forte);
float getSyncInterval(float sync_value, bool triplet, bool dotted, float bpm);
uint32_t calculateNoteLength(float trigger_val, float pitch_val, ClockEngine* engine);
int countActiveVoices();

#endif // SLINK_MODE_H
