#ifndef ESP_NOW_MIDI_MODULE_H
#define ESP_NOW_MIDI_MODULE_H

#include "common_definitions.h"

#if ESP_NOW_ENABLED

#include <esp_now_midi.h>

// ESP-NOW MIDI operating modes
enum EspNowMode {
  ESP_NOW_OFF = 0,
  ESP_NOW_BROADCAST,  // Auto-discovery, sends to all discovered peers
  ESP_NOW_PEER        // Manual peer management
};

// ESP-NOW MIDI module state
struct EspNowMidiState {
  bool initialized = false;
  EspNowMode mode = ESP_NOW_OFF;
  uint32_t messagesSent = 0;
  uint32_t messagesReceived = 0;
  uint8_t peerMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default broadcast MAC
};

// Global ESP-NOW MIDI instance
extern esp_now_midi espNowMIDI;
extern EspNowMidiState espNowState;

// Initialize ESP-NOW MIDI system
void initEspNowMidi();

// Deinitialize ESP-NOW MIDI system
void deinitEspNowMidi();

// Set ESP-NOW mode
void setEspNowMode(EspNowMode mode);

// Add a peer by MAC address (format: "AA:BB:CC:DD:EE:FF")
bool addEspNowPeer(const char* macAddress);

// Add a peer by MAC address bytes
bool addEspNowPeer(const uint8_t mac[6]);

// Remove a peer by MAC address
bool removeEspNowPeer(const uint8_t mac[6]);

// Clear all peers
void clearEspNowPeers();

// Get number of connected peers
int getEspNowPeerCount();

// Send MIDI message via ESP-NOW
void sendEspNowMidi(uint8_t status, uint8_t data1, uint8_t data2);

// MIDI message handlers (called when ESP-NOW MIDI received)
void onEspNowNoteOn(byte channel, byte note, byte velocity);
void onEspNowNoteOff(byte channel, byte note, byte velocity);
void onEspNowControlChange(byte channel, byte control, byte value);
void onEspNowClock();
void onEspNowStart();
void onEspNowStop();
void onEspNowContinue();

#endif // ESP_NOW_ENABLED

#endif // ESP_NOW_MIDI_MODULE_H
