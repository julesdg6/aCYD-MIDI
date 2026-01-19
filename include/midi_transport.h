#ifndef MIDI_TRANSPORT_H
#define MIDI_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

void initMidiTransports();
void handleMidiTransports();
void midiTransportProcessIncomingBytes(const uint8_t *data, size_t length);
void sendWiFiMidiMessage(const uint8_t *data, size_t length);
bool midiTransportIsPulseActive();

#endif // MIDI_TRANSPORT_H
