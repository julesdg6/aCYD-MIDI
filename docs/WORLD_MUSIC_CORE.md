# World Music Generator Core

## Overview

The World Music Generator Core is a shared infrastructure for generating music in various world music systems including Maqam, Raga, Gamelan, and other modal traditions. It provides a flexible data model, validation, phrase generation, and serialization capabilities for defining and using musical modes from different cultural traditions.

## Architecture

### Design Principles

1. **ESP32-Optimized**: Designed for microcontroller constraints (limited RAM, no dynamic allocation)
2. **Header-Only Integration**: Follows the aCYD-MIDI pattern of header-only mode implementations
3. **Validation-First**: All mode definitions are validated before use
4. **Extensible**: Easy to add new systems, modes, and generator strategies
5. **Microtonal-Ready**: Supports arbitrary tuning systems with cents-based detuning

### Core Components

- **Data Model**: Structs for Tuning, Motif, Segment, Mode
- **Validation**: Ensures musical consistency and prevents runtime errors
- **Generator**: Phrase generation with motif-based and random-walk strategies
- **Serialization**: Simple text-based format for mode bundles

## Data Model

### SystemType (enum)

Identifies the music system/tradition:

```cpp
enum SystemType {
  SYSTEM_MAQAM,                    // Middle Eastern maqam system
  SYSTEM_RAGA,                     // Indian classical raga system
  SYSTEM_EAST_ASIAN_PENTATONIC,    // Chinese/Japanese pentatonic systems
  SYSTEM_AFRICAN_MODAL,            // Various African modal systems
  SYSTEM_GAMELAN,                  // Indonesian gamelan tuning
  SYSTEM_OTHER                     // Other/custom systems
};
```

### Tuning

Defines microtonal tuning with cents offset per scale degree.

```cpp
struct Tuning {
  char name[WM_MAX_NAME_LENGTH];                  // "Just Intonation", "22-Shruti", etc.
  float centsOffsets[WM_MAX_SCALE_DEGREES];       // Cents deviation from equal temperament
  uint8_t numDegrees;                             // Number of degrees in tuning
};
```

**Example** (22-Shruti approximation for raga):
```cpp
Tuning shruti22;
strcpy(shruti22.name, "22-Shruti");
shruti22.numDegrees = 7;
shruti22.centsOffsets[0] = 0;      // Sa (Tonic)
shruti22.centsOffsets[1] = -22;    // Komal Re (slightly flat)
shruti22.centsOffsets[2] = -14;    // Komal Ga
// ... etc.
```

### Motif

A melodic pattern fragment with scale degree steps, optional rhythm, and sampling weight.

```cpp
struct Motif {
  int8_t degreeSteps[WM_MAX_MOTIF_STEPS];        // Scale degree INDICES (0-based, e.g., 0=first note, 1=second note)
  uint8_t rhythmPattern[WM_MAX_MOTIF_STEPS];     // Duration in ticks (0 = default)
  uint8_t numSteps;                               // Number of steps in motif
  uint8_t weight;                                 // Sampling probability (1-100)
};
```

**Example** (Raga Yaman pakad phrase):
```cpp
// Raga Yaman scale: Sa Re Ga Ma# Pa Dha Ni (indices 0-6)
Motif yamanPakad;
yamanPakad.numSteps = 4;
yamanPakad.degreeSteps[0] = 6;  // Index 6 = Ni (7th note of scale)
yamanPakad.degreeSteps[1] = 1;  // Index 1 = Re (2nd note of scale)
yamanPakad.degreeSteps[2] = 2;  // Index 2 = Ga (3rd note of scale)
yamanPakad.degreeSteps[3] = 3;  // Index 3 = Ma# (4th note of scale)
yamanPakad.weight = 50;  // Higher weight = more likely to be selected
```

**Important**: `degreeSteps` contains **indices** into the scale array, not semitone offsets. For example, if your scale is `[0, 2, 4, 7, 9]` (pentatonic), then `degreeSteps[0] = 2` means "use the third note of the scale" (which is semitone offset 4).
```

### Segment (for Maqam/Compound Modes)

Represents a modal fragment (jins/tetrachord) with modulation graph.

```cpp
struct Segment {
  char name[WM_MAX_NAME_LENGTH];                 // "Rast", "Nahawand", etc.
  int8_t degrees[WM_MAX_SCALE_DEGREES];          // Scale degrees in segment
  uint8_t numDegrees;                            // Number of degrees
  uint8_t tonicIndex;                            // Tonic within segment
  SegmentTransition transitions[WM_MAX_TRANSITIONS];  // Modulation graph
  uint8_t numTransitions;
};
```

**Example** (Maqam Rast lower jins):
```cpp
Segment rastLower;
strcpy(rastLower.name, "Rast Lower");
rastLower.numDegrees = 4;
rastLower.degrees[0] = 0;   // Rast (tonic)
rastLower.degrees[1] = 2;   // Dukah (whole step)
rastLower.degrees[2] = 4;   // Sikah (whole step)
rastLower.degrees[3] = 5;   // Jaharka (half step)
rastLower.tonicIndex = 0;

// Can transition to Nahawand upper jins
rastLower.transitions[0].targetSegmentIndex = 1;
rastLower.transitions[0].probability = 70;
rastLower.numTransitions = 1;
```

### Mode

Complete musical mode definition with scale, important degrees, motifs, and segments.

```cpp
struct Mode {
  // Identity
  char id[WM_MAX_NAME_LENGTH];                   // "yaman", "rast", etc.
  char name[WM_MAX_NAME_LENGTH];                 // "Raga Yaman", "Maqam Rast"
  SystemType system;                              // SYSTEM_RAGA, SYSTEM_MAQAM, etc.
  
  // Scale Structure
  int8_t scaleDegrees[WM_MAX_SCALE_DEGREES];     // Semitone intervals from tonic
  uint8_t numDegrees;
  
  // Important Degrees
  uint8_t tonicIndex;                            // Usually 0 (Sa/Do)
  uint8_t dominantIndex;                         // Vadi/Dominant (usually 4 or 6)
  uint8_t cadentialIndices[4];                   // Cadential/resting notes
  uint8_t numCadential;
  
  // Directional Rules (for ragas with vakra/zigzag)
  int8_t ascendOrder[WM_MAX_SCALE_DEGREES];      // Custom ascending order
  int8_t descendOrder[WM_MAX_SCALE_DEGREES];     // Custom descending order
  bool hasDirectionalRules;
  
  // Motif Bank (pakad/characteristic phrases)
  Motif motifs[WM_MAX_MOTIFS];
  uint8_t numMotifs;
  
  // Segment Bank (for maqam jins/compound modes)
  Segment segments[WM_MAX_SEGMENTS];
  uint8_t numSegments;
  
  // Tuning Reference
  Tuning tuning;
};
```

## Creating Mode Bundles

### Example 1: Raga Yaman

```cpp
#include "world_music_core.h"

Mode createRagaYaman() {
  Mode yaman;
  
  // Identity
  strcpy(yaman.id, "yaman");
  strcpy(yaman.name, "Raga Yaman");
  yaman.system = SYSTEM_RAGA;
  
  // Scale: Sa Re Ga Ma# Pa Dha Ni (C D E F# G A B)
  yaman.numDegrees = 7;
  yaman.scaleDegrees[0] = 0;   // Sa
  yaman.scaleDegrees[1] = 2;   // Re
  yaman.scaleDegrees[2] = 4;   // Ga
  yaman.scaleDegrees[3] = 6;   // Ma# (Tivra Madhyam)
  yaman.scaleDegrees[4] = 7;   // Pa
  yaman.scaleDegrees[5] = 9;   // Dha
  yaman.scaleDegrees[6] = 11;  // Ni
  
  // Important degrees
  yaman.tonicIndex = 0;        // Sa
  yaman.dominantIndex = 3;     // Ma# (Vadi)
  yaman.cadentialIndices[0] = 0;  // Sa
  yaman.cadentialIndices[1] = 4;  // Pa (Samvadi)
  yaman.numCadential = 2;
  
  // Pakad (characteristic phrase): Ni Re Ga Ma#
  yaman.numMotifs = 1;
  yaman.motifs[0].numSteps = 4;
  yaman.motifs[0].degreeSteps[0] = 6;  // Ni
  yaman.motifs[0].degreeSteps[1] = 1;  // Re
  yaman.motifs[0].degreeSteps[2] = 2;  // Ga
  yaman.motifs[0].degreeSteps[3] = 3;  // Ma#
  yaman.motifs[0].weight = 80;
  
  // Validate
  ValidationResult result = validateMode(yaman);
  if (!result.isValid) {
    Serial.println("Validation failed:");
    Serial.println(result.errorMessage);
  }
  
  return yaman;
}
```

### Example 2: Maqam Rast (with Segments)

```cpp
Mode createMaqamRast() {
  Mode rast;
  
  strcpy(rast.id, "rast");
  strcpy(rast.name, "Maqam Rast");
  rast.system = SYSTEM_MAQAM;
  
  // Scale: C D E F G A Bb C
  rast.numDegrees = 7;
  rast.scaleDegrees[0] = 0;   // Rast
  rast.scaleDegrees[1] = 2;   // Dukah
  rast.scaleDegrees[2] = 4;   // Sikah
  rast.scaleDegrees[3] = 5;   // Jaharka
  rast.scaleDegrees[4] = 7;   // Nawa
  rast.scaleDegrees[5] = 9;   // Husayni
  rast.scaleDegrees[6] = 10;  // Awj (minor 7th)
  
  rast.tonicIndex = 0;
  rast.dominantIndex = 4;  // Nawa (dominant)
  
  // Lower Jins (Rast tetrachord on tonic)
  strcpy(rast.segments[0].name, "Rast Lower");
  rast.segments[0].numDegrees = 4;
  rast.segments[0].degrees[0] = 0;
  rast.segments[0].degrees[1] = 2;
  rast.segments[0].degrees[2] = 4;
  rast.segments[0].degrees[3] = 5;
  rast.segments[0].tonicIndex = 0;
  
  // Upper Jins (Rast tetrachord on dominant)
  strcpy(rast.segments[1].name, "Rast Upper");
  rast.segments[1].numDegrees = 4;
  rast.segments[1].degrees[0] = 7;
  rast.segments[1].degrees[1] = 9;
  rast.segments[1].degrees[2] = 10;
  rast.segments[1].degrees[3] = 12;
  rast.segments[1].tonicIndex = 0;
  
  rast.numSegments = 2;
  
  return rast;
}
```

## Using the Generator

### Basic Phrase Generation

```cpp
#include "world_music_core.h"
#include "midi_utils.h"

void playGeneratedPhrase() {
  // Create or load mode
  Mode yaman = createRagaYaman();
  
  // Configure generator
  GeneratorParams params;
  params.phraseLength = 16;      // 16 notes
  params.baseOctave = 4;          // Middle octave
  params.registerRange = 1;       // Can shift ±1 octave
  params.useCadence = true;       // End on cadential note
  params.useMotifs = true;        // Use pakad motifs
  params.motifDensity = 70;       // 70% chance of motif vs random walk
  
  // Generate phrase
  int8_t notes[16];
  uint8_t octaves[16];
  generatePhrase(yaman, params, notes, octaves, 16);
  
  // Play phrase
  for (int i = 0; i < 16; i++) {
    int8_t midiNote = scaleDegreesToMidiNote(notes[i], octaves[i], yaman);
    sendMIDI(0x90, midiNote, 100);  // Note On
    delay(250);                      // Quarter note at 240 BPM
    sendMIDI(0x80, midiNote, 0);    // Note Off
  }
}
```

### Integration with Existing Modules

The generator can be called from any module's playback loop:

```cpp
// In module_my_world_music_mode.cpp

struct MyWorldMusicState {
  Mode currentMode;
  GeneratorParams params;
  int8_t currentPhrase[32];
  uint8_t currentOctaves[32];
  uint8_t phrasePosition;
  bool isPlaying;
};

static MyWorldMusicState state;

void initializeMyWorldMusicMode() {
  state.currentMode = createRagaYaman();  // Or load from storage
  state.params.phraseLength = 16;
  state.params.useMotifs = true;
  state.isPlaying = false;
}

void handleMyWorldMusicMode() {
  // When play button pressed
  if (state.isPlaying && needsNewPhrase()) {
    generatePhrase(state.currentMode, state.params, 
                   state.currentPhrase, state.currentOctaves, 32);
    state.phrasePosition = 0;
  }
  
  // Play notes on clock tick
  if (isClockTick()) {
    int8_t note = scaleDegreesToMidiNote(
      state.currentPhrase[state.phrasePosition],
      state.currentOctaves[state.phrasePosition],
      state.currentMode
    );
    sendMIDI(0x90, note, 100);
    state.phrasePosition = (state.phrasePosition + 1) % state.params.phraseLength;
  }
}
```

## Validation

All modes should be validated before use:

```cpp
Mode myMode = createMyCustomMode();

ValidationResult result = validateMode(myMode);
if (!result.isValid) {
  Serial.print("Mode validation failed: ");
  Serial.println(result.errorMessage);
  return;
}

// Safe to use mode
generatePhrase(myMode, params, notes, octaves, 32);
```

### Validation Checks

1. **Scale Constraints**: 
   - `numDegrees` must be 1-12
   - All degree indices must be valid (0-11)

2. **Important Degree Indices**:
   - `tonicIndex` < `numDegrees`
   - `dominantIndex` < `numDegrees`
   - All `cadentialIndices` < `numDegrees`

3. **Motif Degrees**:
   - All `degreeSteps` must reference valid scale degrees
   - Checks both positive and negative steps (for descending motion)

4. **Tuning Alignment**:
   - If tuning is defined, `tuning.numDegrees` must match `mode.numDegrees`

## Serialization

### Simple Text Format

Modes can be serialized to a simple key=value text format:

```cpp
Mode yaman = createRagaYaman();
char buffer[512];
serializeMode(yaman, buffer, sizeof(buffer));

// Output: "id=yaman;name=Raga Yaman;system=raga;degrees=7;tonic=0;dominant=3"
```

### Deserialization

```cpp
const char* data = "id=yaman;name=Raga Yaman;system=raga;degrees=7;tonic=0;dominant=3";
Mode loadedMode;
if (deserializeMode(data, loadedMode)) {
  Serial.println("Mode loaded successfully");
}
```

**Note**: The current implementation is minimal. For full mode storage including scale degrees, motifs, and segments, extend the serialization format or use a structured format (JSON-like text parsing).

## Advanced Features

### Directional Rules (Vakra)

Some ragas have different ascending and descending patterns:

```cpp
Mode createRagaBhupali() {
  Mode bhupali;
  // ... basic setup ...
  
  // Use custom ascending/descending order
  bhupali.hasDirectionalRules = true;
  
  // Ascending: Sa Re Ga Pa Dha Sa
  bhupali.ascendOrder[0] = 0;  // Sa
  bhupali.ascendOrder[1] = 1;  // Re
  bhupali.ascendOrder[2] = 2;  // Ga
  bhupali.ascendOrder[3] = 4;  // Pa
  bhupali.ascendOrder[4] = 5;  // Dha
  
  // Descending: Sa Dha Pa Ga Re Sa
  bhupali.descendOrder[0] = 0;  // Sa
  bhupali.descendOrder[1] = 5;  // Dha
  bhupali.descendOrder[2] = 4;  // Pa
  bhupali.descendOrder[3] = 2;  // Ga
  bhupali.descendOrder[4] = 1;  // Re
  
  return bhupali;
}
```

### Segment-Based Modulation (Maqam)

For maqam systems using jins (tetrachords), use segments with transition graphs:

```cpp
// Define segments with modulation probabilities
rastLower.transitions[0].targetSegmentIndex = 1;  // Can go to upper jins
rastLower.transitions[0].probability = 60;
rastLower.transitions[1].targetSegmentIndex = 2;  // Can go to Nahawand
rastLower.transitions[1].probability = 30;
rastLower.numTransitions = 2;
```

### Microtonal Tuning

Define custom tunings with cents offsets:

```cpp
Tuning justIntonation;
strcpy(justIntonation.name, "5-Limit Just");
justIntonation.numDegrees = 7;
justIntonation.centsOffsets[0] = 0;      // Tonic (unison)
justIntonation.centsOffsets[1] = 4;      // Major 2nd (9/8 = 204 cents, +4 from ET)
justIntonation.centsOffsets[2] = -14;    // Major 3rd (5/4 = 386 cents, -14 from ET)
justIntonation.centsOffsets[3] = -2;     // Perfect 4th (4/3 = 498 cents, -2 from ET)
justIntonation.centsOffsets[4] = 2;      // Perfect 5th (3/2 = 702 cents, +2 from ET)
justIntonation.centsOffsets[5] = -16;    // Major 6th (5/3 = 884 cents, -16 from ET)
justIntonation.centsOffsets[6] = -12;    // Major 7th (15/8 = 1088 cents, -12 from ET)

mode.tuning = justIntonation;
```

**Note**: Actual microtonal playback requires MIDI pitch bend messages. The current MIDI output rounds to nearest semitone.

## Memory Considerations

The data structures are sized for ESP32 constraints:

- `WM_MAX_SCALE_DEGREES = 12`: Maximum scale size
- `WM_MAX_MOTIF_STEPS = 16`: Maximum motif length
- `WM_MAX_MOTIFS = 32`: Maximum motifs per mode
- `WM_MAX_SEGMENTS = 8`: Maximum segments (jins) per mode
- `WM_MAX_TRANSITIONS = 4`: Maximum segment transitions

A fully-populated `Mode` struct uses approximately **2-3 KB** of RAM.

## API Reference

### Core Functions

```cpp
// Validation
ValidationResult validateMode(const Mode& mode);
bool validateCadentialDegrees(const Mode& mode);
bool validateMotifDegrees(const Mode& mode);
bool validateTuningAlignment(const Mode& mode);

// Generation
void generatePhrase(const Mode& mode, const GeneratorParams& params, 
                   int8_t* outNotes, uint8_t* outOctaves, uint8_t maxNotes);
void selectMotif(const Mode& mode, Motif& outMotif);
int8_t selectCadentialNote(const Mode& mode);
int8_t scaleDegreesToMidiNote(int8_t degree, uint8_t octave, const Mode& mode);

// Serialization
void serializeMode(const Mode& mode, char* buffer, size_t bufferSize);
bool deserializeMode(const char* data, Mode& outMode);

// Utilities
const char* getSystemTypeName(SystemType type);
SystemType parseSystemType(const char* name);
```

## Demo Module

A complete working example is available in `module_world_music_demo_mode.h/cpp`:

**Features:**
- Three pre-defined modes: Raga Yaman, Maqam Rast, Major Pentatonic
- Real-time phrase generation with play/stop controls
- Mode switching with validation
- Touch-based UI for mode selection and playback control
- Demonstrates motif-based generation and cadence usage

**Integration:**
To add to your build, include the module header and register it in `app_modes.cpp`:

```cpp
#include "module_world_music_demo_mode.h"

// In mode table:
{initializeWorldMusicDemoMode, drawWorldMusicDemoMode, handleWorldMusicDemoMode}
```

**Usage Pattern:**
The demo shows the recommended workflow:
1. Create mode definitions with validation
2. Configure generator parameters
3. Generate phrases on demand
4. Play back using MIDI output
5. Handle UI interaction for mode switching

## Future Enhancements

1. **Extended Serialization**: Full JSON-like format for all mode properties
2. **SPIFFS/LittleFS Storage**: Load mode bundles from flash filesystem
3. **Markov Chain Integration**: Hook for 2nd-order Markov weights (like Raga Bhairavi)
4. **Pitch Bend Output**: Actual microtonal MIDI output using pitch bend
5. **Real-time Modulation**: Segment transition following for maqam improvisation
6. **Tala/Rhythm Integration**: Synchronized rhythm patterns with clock manager

## Related Documentation

- **Raga Mode Implementation**: `docs/RAGA_MODE_MARKOV_CHAIN.md`
- **Demo Module**: `include/module_world_music_demo_mode.h`
- **Common Definitions**: `include/common_definitions.h`
- **MIDI Utilities**: `include/midi_utils.h`
- **Clock Manager**: `include/clock_manager.h`
