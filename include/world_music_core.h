#ifndef WORLD_MUSIC_CORE_H
#define WORLD_MUSIC_CORE_H

#include <Arduino.h>

// ============================================================
// World Music Generator Core
// ============================================================
// Shared infrastructure for generating music in various
// world music systems (maqam, raga, gamelan, etc.)
//
// This core provides:
// - Data models for tuning systems, scales, motifs, and modes
// - Serialization/deserialization for mode bundles
// - Generator logic for MIDI phrase generation
// - Validation for musical consistency
// ============================================================

// Maximum sizes for data structures (constrained by ESP32 memory)
#define WM_MAX_SCALE_DEGREES 12
#define WM_MAX_MOTIF_STEPS 16
#define WM_MAX_MOTIFS 32
#define WM_MAX_SEGMENTS 8
#define WM_MAX_TRANSITIONS 4
#define WM_MAX_NAME_LENGTH 32

// ============================================================
// System Types
// ============================================================

enum SystemType {
  SYSTEM_MAQAM = 0,
  SYSTEM_RAGA,
  SYSTEM_EAST_ASIAN_PENTATONIC,
  SYSTEM_AFRICAN_MODAL,
  SYSTEM_GAMELAN,
  SYSTEM_OTHER,
  SYSTEM_COUNT
};

// ============================================================
// Tuning System
// ============================================================
// Defines microtonal tuning with cents offset per scale degree
// 100 cents = 1 semitone; 0 cents = equal temperament

struct Tuning {
  char name[WM_MAX_NAME_LENGTH];
  float centsOffsets[WM_MAX_SCALE_DEGREES];  // Cents deviation from equal temperament
  uint8_t numDegrees;
  
  Tuning() : numDegrees(0) {
    name[0] = '\0';
    for (int i = 0; i < WM_MAX_SCALE_DEGREES; i++) {
      centsOffsets[i] = 0.0f;
    }
  }
};

// ============================================================
// Motif - Melodic Pattern Fragment
// ============================================================
// A motif is a sequence of scale degree steps with optional
// rhythm pattern and sampling weight

struct Motif {
  int8_t degreeSteps[WM_MAX_MOTIF_STEPS];  // Scale degree steps (e.g., 0,2,4 for S-g-P)
  uint8_t rhythmPattern[WM_MAX_MOTIF_STEPS];  // Optional: duration in ticks (0 = use default)
  uint8_t numSteps;
  uint8_t weight;  // Sampling probability weight (higher = more likely)
  
  Motif() : numSteps(0), weight(1) {
    for (int i = 0; i < WM_MAX_MOTIF_STEPS; i++) {
      degreeSteps[i] = 0;
      rhythmPattern[i] = 0;
    }
  }
};

// ============================================================
// Segment - Modal Fragment (Jins/Tetrachord)
// ============================================================
// For systems like Maqam that compose scales from segments

struct SegmentTransition {
  uint8_t targetSegmentIndex;
  uint8_t probability;  // 0-100
};

struct Segment {
  char name[WM_MAX_NAME_LENGTH];
  int8_t degrees[WM_MAX_SCALE_DEGREES];  // Scale degrees in this segment
  uint8_t numDegrees;
  uint8_t tonicIndex;  // Index of tonic within this segment
  SegmentTransition transitions[WM_MAX_TRANSITIONS];
  uint8_t numTransitions;
  
  Segment() : numDegrees(0), tonicIndex(0), numTransitions(0) {
    name[0] = '\0';
    for (int i = 0; i < WM_MAX_SCALE_DEGREES; i++) {
      degrees[i] = 0;
    }
    for (int i = 0; i < WM_MAX_TRANSITIONS; i++) {
      transitions[i].targetSegmentIndex = 0;
      transitions[i].probability = 0;
    }
  }
};

// ============================================================
// Mode - Complete Musical Mode Definition
// ============================================================

struct Mode {
  // Identity
  char id[WM_MAX_NAME_LENGTH];
  char name[WM_MAX_NAME_LENGTH];
  SystemType system;
  
  // Scale Structure
  int8_t scaleDegrees[WM_MAX_SCALE_DEGREES];  // Semitone intervals from tonic
  uint8_t numDegrees;
  
  // Important Degrees
  uint8_t tonicIndex;       // Index of tonic (usually 0)
  uint8_t dominantIndex;    // Index of dominant/vadi
  uint8_t cadentialIndices[4];  // Indices of cadential notes
  uint8_t numCadential;
  
  // Directional Rules (for ragas with different ascent/descent)
  int8_t ascendOrder[WM_MAX_SCALE_DEGREES];   // Order for ascending (0 = use scaleDegrees)
  int8_t descendOrder[WM_MAX_SCALE_DEGREES];  // Order for descending
  bool hasDirectionalRules;
  
  // Motif Bank
  Motif motifs[WM_MAX_MOTIFS];
  uint8_t numMotifs;
  
  // Segment Bank (for maqam/compound modes)
  Segment segments[WM_MAX_SEGMENTS];
  uint8_t numSegments;
  
  // Tuning Reference
  Tuning tuning;
  
  Mode() : system(SYSTEM_OTHER), numDegrees(0), tonicIndex(0), 
           dominantIndex(0), numCadential(0), hasDirectionalRules(false),
           numMotifs(0), numSegments(0) {
    id[0] = '\0';
    name[0] = '\0';
    for (int i = 0; i < WM_MAX_SCALE_DEGREES; i++) {
      scaleDegrees[i] = 0;
      ascendOrder[i] = 0;
      descendOrder[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
      cadentialIndices[i] = 0;
    }
  }
};

// ============================================================
// Generator Parameters
// ============================================================

struct GeneratorParams {
  uint8_t phraseLength;     // Number of notes to generate
  uint8_t baseOctave;       // Base octave (3-6 typical)
  uint8_t registerRange;    // Octave range variation (0-2)
  bool useCadence;          // End phrase on cadential note
  bool useMotifs;           // Use motifs vs. random walk
  uint8_t motifDensity;     // 0-100: how often to use motifs
  
  GeneratorParams() : phraseLength(8), baseOctave(4), registerRange(1),
                      useCadence(true), useMotifs(true), motifDensity(70) {}
};

// ============================================================
// Validation Results
// ============================================================

struct ValidationResult {
  bool isValid;
  char errorMessage[128];
  
  ValidationResult() : isValid(true) {
    errorMessage[0] = '\0';
  }
};

// ============================================================
// Core Functions
// ============================================================

// Validation
ValidationResult validateMode(const Mode& mode);
bool validateCadentialDegrees(const Mode& mode);
bool validateMotifDegrees(const Mode& mode);
bool validateTuningAlignment(const Mode& mode);

// Generator
void generatePhrase(const Mode& mode, const GeneratorParams& params, 
                   int8_t* outNotes, uint8_t* outOctaves, uint8_t maxNotes);
void selectMotif(const Mode& mode, Motif& outMotif);
int8_t selectCadentialNote(const Mode& mode);
int8_t scaleDegreesToMidiNote(int8_t degree, uint8_t octave, const Mode& mode);

// Serialization Helpers (simple text format for ESP32)
// Format: KEY=VALUE pairs separated by semicolons
void serializeMode(const Mode& mode, char* buffer, size_t bufferSize);
bool deserializeMode(const char* data, Mode& outMode);

// Utility
const char* getSystemTypeName(SystemType type);
SystemType parseSystemType(const char* name);

#endif // WORLD_MUSIC_CORE_H
