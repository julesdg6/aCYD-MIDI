#include "world_music_core.h"
#include <Arduino.h>

// ============================================================
// System Type Names
// ============================================================

static const char* const kSystemTypeNames[SYSTEM_COUNT] = {
  "maqam",
  "raga",
  "east_asian_pentatonic",
  "african_modal",
  "gamelan",
  "other"
};

const char* getSystemTypeName(SystemType type) {
  if (type >= 0 && type < SYSTEM_COUNT) {
    return kSystemTypeNames[type];
  }
  return "unknown";
}

SystemType parseSystemType(const char* name) {
  for (int i = 0; i < SYSTEM_COUNT; i++) {
    if (strcmp(name, kSystemTypeNames[i]) == 0) {
      return (SystemType)i;
    }
  }
  return SYSTEM_OTHER;
}

// ============================================================
// Validation Functions
// ============================================================

ValidationResult validateMode(const Mode& mode) {
  ValidationResult result;
  
  // Check basic constraints
  if (mode.numDegrees == 0 || mode.numDegrees > WM_MAX_SCALE_DEGREES) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage), 
             "Invalid numDegrees: %d", mode.numDegrees);
    return result;
  }
  
  if (mode.tonicIndex >= mode.numDegrees) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage),
             "Tonic index %d out of range (max %d)", 
             mode.tonicIndex, mode.numDegrees - 1);
    return result;
  }
  
  if (mode.dominantIndex >= mode.numDegrees) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage),
             "Dominant index %d out of range (max %d)", 
             mode.dominantIndex, mode.numDegrees - 1);
    return result;
  }
  
  // Validate cadential degrees
  if (!validateCadentialDegrees(mode)) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage),
             "Cadential degrees contain invalid indices");
    return result;
  }
  
  // Validate motif degrees
  if (!validateMotifDegrees(mode)) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage),
             "Motif contains degree outside scale range");
    return result;
  }
  
  // Validate tuning alignment
  if (!validateTuningAlignment(mode)) {
    result.isValid = false;
    snprintf(result.errorMessage, sizeof(result.errorMessage),
             "Tuning degrees mismatch with scale degrees");
    return result;
  }
  
  return result;
}

bool validateCadentialDegrees(const Mode& mode) {
  for (uint8_t i = 0; i < mode.numCadential; i++) {
    if (mode.cadentialIndices[i] >= mode.numDegrees) {
      return false;
    }
  }
  return true;
}

bool validateMotifDegrees(const Mode& mode) {
  for (uint8_t m = 0; m < mode.numMotifs; m++) {
    const Motif& motif = mode.motifs[m];
    for (uint8_t s = 0; s < motif.numSteps; s++) {
      // degreeSteps are indices into the scale array
      // Check they are within valid range [0, numDegrees-1]
      if (motif.degreeSteps[s] < 0 || motif.degreeSteps[s] >= (int)mode.numDegrees) {
        return false;
      }
    }
  }
  return true;
}

bool validateTuningAlignment(const Mode& mode) {
  // If tuning is defined, it should match the number of scale degrees
  if (mode.tuning.numDegrees > 0) {
    if (mode.tuning.numDegrees != mode.numDegrees) {
      return false;
    }
  }
  return true;
}

// ============================================================
// Generator Functions
// ============================================================

void generatePhrase(const Mode& mode, const GeneratorParams& params, 
                   int8_t* outNotes, uint8_t* outOctaves, uint8_t maxNotes) {
  if (maxNotes == 0 || mode.numDegrees == 0) {
    return;
  }
  
  uint8_t noteCount = min(params.phraseLength, maxNotes);
  uint8_t currentOctave = params.baseOctave;
  int8_t currentDegreeIndex = mode.tonicIndex;  // Start at tonic (index, not semitone)
  
  for (uint8_t i = 0; i < noteCount; i++) {
    bool isLastNote = (i == noteCount - 1);
    
    // Use motif or random walk
    if (params.useMotifs && mode.numMotifs > 0 && random(100) < params.motifDensity) {
      // Select and apply motif
      Motif motif;
      selectMotif(mode, motif);
      
      // Apply first step of motif (could be extended to apply full motif)
      if (motif.numSteps > 0) {
        currentDegreeIndex = motif.degreeSteps[0];  // This is an index
      }
    } else {
      // Random walk through scale
      int8_t step = random(-2, 3);  // -2 to +2 step size
      int newDegreeIndex = currentDegreeIndex + step;
      
      // Wrap to stay within scale
      while (newDegreeIndex < 0) newDegreeIndex += mode.numDegrees;
      while (newDegreeIndex >= (int)mode.numDegrees) newDegreeIndex -= mode.numDegrees;
      
      currentDegreeIndex = newDegreeIndex;
    }
    
    // On last note, use cadential note if requested
    if (isLastNote && params.useCadence) {
      int8_t cadentialIndex = selectCadentialNote(mode);
      if (cadentialIndex >= 0) {
        currentDegreeIndex = cadentialIndex;
      }
    }
    
    // Occasional octave shift
    if (params.registerRange > 0 && random(100) < 15) {
      int8_t octaveShift = random(-params.registerRange, params.registerRange + 1);
      int newOctave = currentOctave + octaveShift;
      if (newOctave >= 2 && newOctave <= 6) {
        currentOctave = newOctave;
      }
    }
    
    outNotes[i] = currentDegreeIndex;  // Store index, not semitone value
    outOctaves[i] = currentOctave;
  }
}

void selectMotif(const Mode& mode, Motif& outMotif) {
  if (mode.numMotifs == 0) {
    outMotif.numSteps = 0;
    return;
  }
  
  // Calculate total weight
  uint16_t totalWeight = 0;
  for (uint8_t i = 0; i < mode.numMotifs; i++) {
    totalWeight += mode.motifs[i].weight;
  }
  
  if (totalWeight == 0) {
    // Fallback to uniform selection
    uint8_t index = random(mode.numMotifs);
    outMotif = mode.motifs[index];
    return;
  }
  
  // Weighted random selection
  uint16_t randValue = random(totalWeight);
  uint16_t cumulative = 0;
  
  for (uint8_t i = 0; i < mode.numMotifs; i++) {
    cumulative += mode.motifs[i].weight;
    if (randValue < cumulative) {
      outMotif = mode.motifs[i];
      return;
    }
  }
  
  // Fallback (shouldn't reach here)
  outMotif = mode.motifs[0];
}

int8_t selectCadentialNote(const Mode& mode) {
  if (mode.numCadential == 0) {
    // Default to tonic index
    return mode.tonicIndex;
  }
  
  // Random cadential note (returns index, not semitone value)
  uint8_t index = random(mode.numCadential);
  uint8_t degreeIndex = mode.cadentialIndices[index];
  
  if (degreeIndex < mode.numDegrees) {
    return degreeIndex;
  }
  
  return mode.tonicIndex;
}

int8_t scaleDegreesToMidiNote(int8_t degree, uint8_t octave, const Mode& mode) {
  if (degree < 0 || degree >= (int)mode.numDegrees) {
    return 60;  // Middle C fallback
  }
  
  // Base MIDI note: C0 = 12, C4 = 60
  int midiNote = 12 + (octave * 12) + mode.scaleDegrees[degree];
  
  // Apply microtonal tuning if defined
  // (Note: MIDI pitch bend would be needed for actual microtonal playback)
  // This just returns the nearest semitone for now
  
  // Clamp to valid MIDI range
  if (midiNote < 0) midiNote = 0;
  if (midiNote > 127) midiNote = 127;
  
  return (int8_t)midiNote;
}

// ============================================================
// Serialization Functions (Simple Key=Value Format)
// ============================================================

void serializeMode(const Mode& mode, char* buffer, size_t bufferSize) {
  if (buffer == NULL || bufferSize == 0) {
    return;
  }
  
  // Build simple text format
  snprintf(buffer, bufferSize,
           "id=%s;name=%s;system=%s;degrees=%d;tonic=%d;dominant=%d",
           mode.id,
           mode.name,
           getSystemTypeName(mode.system),
           mode.numDegrees,
           mode.tonicIndex,
           mode.dominantIndex);
  
  // Note: Full serialization would include scale degrees, motifs, etc.
  // This is a minimal implementation showing the pattern
}

bool deserializeMode(const char* data, Mode& outMode) {
  if (data == NULL) {
    return false;
  }
  
  // Simple parser for key=value format
  // This is a minimal implementation - would need expansion for full support
  
  // Use smaller buffer to avoid stack overflow on ESP32
  char tempData[256];
  strncpy(tempData, data, sizeof(tempData) - 1);
  tempData[sizeof(tempData) - 1] = '\0';
  
  char* token = strtok(tempData, ";");
  
  while (token != NULL) {
    char* equals = strchr(token, '=');
    if (equals != NULL) {
      *equals = '\0';
      const char* key = token;
      const char* value = equals + 1;
      
      if (strcmp(key, "id") == 0) {
        strncpy(outMode.id, value, WM_MAX_NAME_LENGTH - 1);
      } else if (strcmp(key, "name") == 0) {
        strncpy(outMode.name, value, WM_MAX_NAME_LENGTH - 1);
      } else if (strcmp(key, "system") == 0) {
        outMode.system = parseSystemType(value);
      } else if (strcmp(key, "degrees") == 0) {
        outMode.numDegrees = atoi(value);
      } else if (strcmp(key, "tonic") == 0) {
        outMode.tonicIndex = atoi(value);
      } else if (strcmp(key, "dominant") == 0) {
        outMode.dominantIndex = atoi(value);
      }
    }
    
    token = strtok(NULL, ";");
  }
  
  return true;
}
