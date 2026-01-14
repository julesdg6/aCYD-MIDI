#ifndef MODULE_AUTO_CHORD_MODE_H
#define MODULE_AUTO_CHORD_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Auto Chord mode variables - traditional piano chords
struct ChordType {
  String name;
  int intervals[4]; // Root, 3rd, 5th, optional 7th
  int numNotes;
};

// Traditional chord types - classic piano voicings
extern ChordType diatonicChords[];
extern int chordOctave;
extern int chordScale;
extern int activeChordNotes[8][4]; // [chord][note index]
extern bool chordPressed[8]; // 8 diatonic chords

// Function declarations
void initializeAutoChordMode();
void drawAutoChordMode();
void handleAutoChordMode();
void drawChordKeys();
void playChord(int scaleDegree, bool on);
void stopAllChords();

#endif // MODULE_AUTO_CHORD_MODE_H
