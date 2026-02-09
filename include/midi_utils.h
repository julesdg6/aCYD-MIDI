#ifndef MIDI_UTILS_H
#define MIDI_UTILS_H

#include "common_definitions.h"
#include "hardware_midi.h"
#include "midi_transport.h"

#if ESP_NOW_ENABLED
#include "esp_now_midi_module.h"
#endif

// MIDI utility functions
void sendMIDI(byte cmd, byte note, byte vel);
void sendMIDIClock();
void sendMIDIStart();
void sendMIDIStop();
int getNoteInScale(int scaleIndex, int degree, int octave);
String getNoteNameFromMIDI(int midiNote);
void stopAllModes();

#endif
