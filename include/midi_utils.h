#ifndef MIDI_UTILS_H
#define MIDI_UTILS_H

#include "common_definitions.h"
#include "hardware_midi.h"

// Forward declaration for ESP-NOW MIDI
#if ESP_NOW_ENABLED
void sendEspNowMidi(uint8_t status, uint8_t data1, uint8_t data2);
#endif

// MIDI utility functions
inline void sendMIDI(byte cmd, byte note, byte vel) {
  // Send via BLE MIDI
  if (deviceConnected) {
    midiPacket[2] = cmd;
    midiPacket[3] = note;
    midiPacket[4] = vel;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  
  // Send via Hardware MIDI (DIN-5 connector)
  sendHardwareMIDI(cmd, note, vel);
  
  // Send via ESP-NOW MIDI
#if ESP_NOW_ENABLED
  sendEspNowMidi(cmd, note, vel);
#endif
}

inline int getNoteInScale(int scaleIndex, int degree, int octave) {
  if (scaleIndex >= NUM_SCALES) return 60;
  
  // If degree exceeds scale notes, wrap to next octave
  int actualDegree = degree % scales[scaleIndex].numNotes;
  int octaveOffset = degree / scales[scaleIndex].numNotes;
  
  int rootNote = 60; // C4
  return rootNote + scales[scaleIndex].intervals[actualDegree] + ((octave - 4 + octaveOffset) * 12);
}

inline String getNoteNameFromMIDI(int midiNote) {
  String noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  int noteIndex = midiNote % 12;
  int octave = (midiNote / 12) - 1;
  return noteNames[noteIndex] + String(octave);
}

inline void stopAllModes() {
  // Stop all MIDI notes
  for (int i = 0; i < 128; i++) {
    sendMIDI(0x80, i, 0);
  }
}

#endif
