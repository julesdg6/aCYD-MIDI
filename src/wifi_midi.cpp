#include "wifi_midi.h"

#if WIFI_ENABLED

static WiFiUDP udp;
static WiFiMIDIState wifiMidiState = WIFI_MIDI_DISCONNECTED;
static bool wifiMidiEnabled = false;
static bool wifiMidiInitialized = false;
static uint32_t lastWiFiMIDICheck = 0;
static IPAddress targetIP(192, 168, 1, 100); // Default target - should be configurable

// Simple UDP-based MIDI packet structure
// Format: [0x80][0x80][status][data1][data2]
// Same format as BLE MIDI for consistency

void initWiFiMIDI() {
  if (wifiMidiInitialized) {
    return;
  }
  
  Serial.println("Initializing Wi-Fi MIDI...");
  
  // UDP will be started when WiFi connects
  wifiMidiInitialized = true;
  wifiMidiState = WIFI_MIDI_DISCONNECTED;
  
  Serial.println("Wi-Fi MIDI initialized (disabled by default)");
}

void handleWiFiMIDI() {
  if (!wifiMidiInitialized) {
    initWiFiMIDI();
    return;
  }
  
  if (!wifiMidiEnabled) {
    if (wifiMidiState != WIFI_MIDI_DISCONNECTED) {
      udp.stop();
      wifiMidiState = WIFI_MIDI_DISCONNECTED;
    }
    return;
  }
  
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiMidiState != WIFI_MIDI_DISCONNECTED) {
      udp.stop();
      wifiMidiState = WIFI_MIDI_DISCONNECTED;
    }
    return;
  }
  
  // Start UDP if not started
  if (wifiMidiState == WIFI_MIDI_DISCONNECTED) {
    if (udp.begin(WIFI_MIDI_PORT)) {
      wifiMidiState = WIFI_MIDI_CONNECTED;
      Serial.print("Wi-Fi MIDI UDP started on port ");
      Serial.println(WIFI_MIDI_PORT);
    } else {
      Serial.println("Failed to start Wi-Fi MIDI UDP");
      return;
    }
  }
  
  // Check for incoming MIDI messages
  int packetSize = udp.parsePacket();
  if (packetSize >= 5) {
    uint8_t buffer[5];
    udp.read(buffer, 5);
    
    // Verify packet format (0x80, 0x80 header)
    if (buffer[0] == 0x80 && buffer[1] == 0x80) {
      // Process incoming MIDI message
      // For now, we'll just log it
      // TODO: Route to MIDI input pipeline
      Serial.print("Wi-Fi MIDI RX: ");
      Serial.print(buffer[2], HEX);
      Serial.print(" ");
      Serial.print(buffer[3], HEX);
      Serial.print(" ");
      Serial.println(buffer[4], HEX);
    }
  }
  
  lastWiFiMIDICheck = millis();
}

void sendWiFiMIDI(uint8_t status, uint8_t data1, uint8_t data2) {
  if (!wifiMidiEnabled || wifiMidiState != WIFI_MIDI_CONNECTED) {
    return;
  }
  
  // Create MIDI packet (same format as BLE MIDI)
  uint8_t packet[5] = {0x80, 0x80, status, data1, data2};
  
  // Send to target IP
  udp.beginPacket(targetIP, WIFI_MIDI_PORT);
  udp.write(packet, 5);
  udp.endPacket();
}

bool isWiFiMIDIConnected() {
  return wifiMidiState == WIFI_MIDI_CONNECTED && wifiMidiEnabled;
}

WiFiMIDIState getWiFiMIDIState() {
  return wifiMidiState;
}

void setWiFiMIDIEnabled(bool enabled) {
  wifiMidiEnabled = enabled;
  if (!enabled && wifiMidiState != WIFI_MIDI_DISCONNECTED) {
    udp.stop();
    wifiMidiState = WIFI_MIDI_DISCONNECTED;
    Serial.println("Wi-Fi MIDI disabled");
  } else if (enabled) {
    Serial.println("Wi-Fi MIDI enabled");
  }
}

bool getWiFiMIDIEnabled() {
  return wifiMidiEnabled;
}

#else

// Stub implementations when WIFI_ENABLED is false
void initWiFiMIDI() {}
void handleWiFiMIDI() {}
void sendWiFiMIDI(uint8_t status, uint8_t data1, uint8_t data2) {
  (void)status; (void)data1; (void)data2;
}
bool isWiFiMIDIConnected() { return false; }
WiFiMIDIState getWiFiMIDIState() { return WIFI_MIDI_DISCONNECTED; }
void setWiFiMIDIEnabled(bool enabled) { (void)enabled; }
bool getWiFiMIDIEnabled() { return false; }

#endif // WIFI_ENABLED
