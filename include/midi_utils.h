#ifndef MIDI_UTILS_H
#define MIDI_UTILS_H

#include "common_definitions.h"
#include "hardware_midi.h"
#include "midi_transport.h"

#if ESP_NOW_ENABLED
#include "esp_now_midi_module.h"
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
  
  // Send via ESP-NOW MIDI (only if enabled and mode is not OFF)
#if ESP_NOW_ENABLED
  if (espNowState.initialized && espNowState.mode != ESP_NOW_OFF) {
    sendEspNowMidi(cmd, note, vel);
  }
#endif
  uint8_t wifiBuffer[3] = {cmd, note, vel};
  sendWiFiMidiMessage(wifiBuffer, sizeof(wifiBuffer));
}

inline void sendMIDIClock() {
  if (deviceConnected) {
    midiPacket[2] = 0xF8;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xF8);
#if ESP_NOW_ENABLED
  if (espNowState.initialized && espNowState.mode != ESP_NOW_OFF) {
    sendEspNowMidi(0xF8, 0x00, 0x00);
  }
#endif
  uint8_t wifiClock = 0xF8;
  sendWiFiMidiMessage(&wifiClock, 1);
}

inline void sendMIDIStart() {
  if (deviceConnected) {
    midiPacket[2] = 0xFA;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFA);
#if ESP_NOW_ENABLED
  if (espNowState.initialized && espNowState.mode != ESP_NOW_OFF) {
    sendEspNowMidi(0xFA, 0x00, 0x00);
  }
#endif
  uint8_t wifiStart = 0xFA;
  sendWiFiMidiMessage(&wifiStart, 1);
}

inline void sendMIDIStop() {
  if (deviceConnected) {
    midiPacket[2] = 0xFC;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFC);
#if ESP_NOW_ENABLED
  if (espNowState.initialized && espNowState.mode != ESP_NOW_OFF) {
    sendEspNowMidi(0xFC, 0x00, 0x00);
  }
#endif
  uint8_t wifiStop = 0xFC;
  sendWiFiMidiMessage(&wifiStop, 1);
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
