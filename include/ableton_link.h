#ifndef ABLETON_LINK_H
#define ABLETON_LINK_H

#include <Arduino.h>

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 0
#endif

// Ableton Link support requires WiFi
#if WIFI_ENABLED
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

// Link configuration
constexpr uint16_t LINK_PORT = 20808; // Standard Ableton Link port
constexpr uint32_t LINK_DISCOVERY_INTERVAL_MS = 2000;
constexpr uint32_t LINK_TIMEOUT_MS = 5000;

// Link session state
enum LinkState {
  LINK_DISCONNECTED = 0,
  LINK_DISCOVERING,
  LINK_CONNECTED
};

// Link session info
struct LinkSessionInfo {
  float tempo;           // Current session tempo in BPM
  uint8_t peerCount;     // Number of peers in session
  double beatPhase;      // Current beat phase (0.0 - 1.0)
  uint64_t beatTime;     // Current beat time in microseconds
  bool isValid;          // Whether session data is valid
};

// Ableton Link functions
void initAbletonLink();
void handleAbletonLink();
void setLinkEnabled(bool enabled);
bool getLinkEnabled();
LinkState getLinkState();
LinkSessionInfo getLinkSessionInfo();
void setLinkTempo(float bpm);
float getLinkTempo();

#endif // ABLETON_LINK_H
