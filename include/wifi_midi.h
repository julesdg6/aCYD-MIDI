#ifndef WIFI_MIDI_H
#define WIFI_MIDI_H

#include <Arduino.h>

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 0
#endif

#if WIFI_ENABLED
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

// Wi-Fi MIDI configuration
constexpr uint16_t WIFI_MIDI_PORT = 5004; // Standard RTP-MIDI port
constexpr uint32_t WIFI_MIDI_TIMEOUT_MS = 5000;

// Wi-Fi MIDI session state
enum WiFiMIDIState {
  WIFI_MIDI_DISCONNECTED = 0,
  WIFI_MIDI_CONNECTING,
  WIFI_MIDI_CONNECTED
};

// Wi-Fi MIDI functions
void initWiFiMIDI();
void handleWiFiMIDI();
void sendWiFiMIDI(uint8_t status, uint8_t data1, uint8_t data2);
bool isWiFiMIDIConnected();
WiFiMIDIState getWiFiMIDIState();
void setWiFiMIDIEnabled(bool enabled);
bool getWiFiMIDIEnabled();

#endif // WIFI_MIDI_H
