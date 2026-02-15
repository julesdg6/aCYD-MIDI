# World Music Generator Core - Integration Guide

## Quick Start

The world music generator core is now available for use in any module. Here's how to integrate it:

### 1. Include the Header

```cpp
#include "world_music_core.h"
```

### 2. Create a Mode Definition

```cpp
Mode myMode;
strcpy(myMode.id, "my_mode");
strcpy(myMode.name, "My Custom Mode");
myMode.system = SYSTEM_RAGA;  // or SYSTEM_MAQAM, etc.

// Define scale (semitone offsets from tonic)
myMode.numDegrees = 7;
myMode.scaleDegrees[0] = 0;   // Tonic
myMode.scaleDegrees[1] = 2;   // Major 2nd
myMode.scaleDegrees[2] = 4;   // Major 3rd
// ... etc.

// Important degrees
myMode.tonicIndex = 0;
myMode.dominantIndex = 4;  // Perfect 5th

// Validate
ValidationResult result = validateMode(myMode);
if (!result.isValid) {
  Serial.println(result.errorMessage);
  return;
}
```

### 3. Generate Phrases

```cpp
GeneratorParams params;
params.phraseLength = 16;
params.useMotifs = true;
params.useCadence = true;

int8_t notes[16];
uint8_t octaves[16];
generatePhrase(myMode, params, notes, octaves, 16);
```

### 4. Play the Phrase

```cpp
for (int i = 0; i < 16; i++) {
  int8_t midiNote = scaleDegreesToMidiNote(notes[i], octaves[i], myMode);
  sendMIDI(0x90, midiNote, 100);
  delay(250);
  sendMIDI(0x80, midiNote, 0);
}
```

## Example Modes Included

The demo module (`module_world_music_demo_mode`) includes three ready-to-use modes:

1. **Raga Yaman** - Indian classical with characteristic pakad phrases
2. **Maqam Rast** - Middle Eastern with segment-based structure
3. **Major Pentatonic** - East Asian pentatonic scale

See `src/module_world_music_demo_mode.cpp` for complete examples.

## Features

### Supported Music Systems

- **Maqam** (Middle Eastern)
- **Raga** (Indian Classical)
- **East Asian Pentatonic** (Chinese/Japanese)
- **African Modal**
- **Gamelan** (Indonesian)
- **Other** (custom systems)

### Generator Capabilities

- **Motif-based generation**: Define characteristic phrases (pakad, jins) and the generator will use them
- **Random walk**: Stochastic exploration of the scale
- **Cadence support**: Automatic phrase endings on important notes
- **Octave variation**: Configurable register range
- **Microtonal support**: Define custom tunings with cents offsets

### Validation

All modes are validated before use:
- Cadential degrees exist in scale
- Motifs reference valid scale indices
- Tuning matches scale structure

## Advanced Features

### Motifs (Characteristic Phrases)

```cpp
// Add a pakad/characteristic phrase
myMode.motifs[0].numSteps = 4;
myMode.motifs[0].degreeSteps[0] = 0;  // Indices into scale
myMode.motifs[0].degreeSteps[1] = 2;
myMode.motifs[0].degreeSteps[2] = 4;
myMode.motifs[0].degreeSteps[3] = 5;
myMode.motifs[0].weight = 80;  // Higher = more likely
myMode.numMotifs = 1;
```

### Segments (Jins/Tetrachords)

For compound modes like maqam:

```cpp
// Lower jins
strcpy(myMode.segments[0].name, "Lower");
myMode.segments[0].numDegrees = 4;
myMode.segments[0].degrees[0] = 0;
myMode.segments[0].degrees[1] = 1;
myMode.segments[0].degrees[2] = 2;
myMode.segments[0].degrees[3] = 3;

// Add transition to upper jins
myMode.segments[0].transitions[0].targetSegmentIndex = 1;
myMode.segments[0].transitions[0].probability = 70;
myMode.segments[0].numTransitions = 1;
```

### Microtonal Tuning

```cpp
// Define custom tuning
Tuning justIntonation;
strcpy(justIntonation.name, "Just Intonation");
justIntonation.numDegrees = 7;
justIntonation.centsOffsets[0] = 0;      // Tonic
justIntonation.centsOffsets[1] = 4;      // +4 cents
justIntonation.centsOffsets[2] = -14;    // -14 cents
// ... etc.

myMode.tuning = justIntonation;
```

## API Reference

See `docs/WORLD_MUSIC_CORE.md` for complete documentation.

## Memory Usage

- `Mode` struct: ~2-3 KB when fully populated
- Generator stack usage: ~200 bytes
- Safe for ESP32 with typical configurations

## Integration with Existing Modes

The world music core is designed to complement existing modes:

- **Raga Mode**: Can use this core for additional ragas with different generation strategies
- **TB3PO/Grids**: Can use Mode definitions for scale constraints
- **Sequencer**: Can quantize to Mode scales
- **Arpeggiator**: Can arpeggiate Mode chords

## Future Extensions

1. **SPIFFS Storage**: Save/load Mode bundles from flash
2. **Markov Chains**: Hook for probabilistic transitions (like existing Raga Bhairavi)
3. **Pitch Bend**: True microtonal MIDI output
4. **Rhythm Patterns**: Integrate with tala/usul/iqa rhythm systems
5. **Real-time Modulation**: Segment transitions during playback

## Contributing

To add new modes:

1. Create mode definition function (see `createDemoRagaYaman()` for template)
2. Validate with `validateMode()`
3. Document the mode (name, system, characteristic phrases)
4. Optional: Add to a dedicated mode selection module

## License

Same as parent project (MIT License)
