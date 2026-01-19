#include "esp_now_midi_module.h"

#if ESP_NOW_ENABLED

#include "midi_utils.h"
#include "hardware_midi.h"

// Global ESP-NOW MIDI instance
esp_now_midi espNowMIDI;
EspNowMidiState espNowState;

// Forward declarations for internal routing
extern void sendMIDI(uint8_t status, uint8_t data1, uint8_t data2);

void initEspNowMidi() {
  if (espNowState.initialized) {
    Serial.println("[ESP-NOW] Already initialized");
    return;
  }

  Serial.println("[ESP-NOW] Initializing ESP-NOW MIDI...");
  
  // Initialize with auto-peer discovery enabled and low latency
  espNowMIDI.begin(false, true);
  
  // Register MIDI handlers
  espNowMIDI.setHandleNoteOn(onEspNowNoteOn);
  espNowMIDI.setHandleNoteOff(onEspNowNoteOff);
  espNowMIDI.setHandleControlChange(onEspNowControlChange);
  espNowMIDI.setHandleClock(onEspNowClock);
  espNowMIDI.setHandleStart(onEspNowStart);
  espNowMIDI.setHandleStop(onEspNowStop);
  espNowMIDI.setHandleContinue(onEspNowContinue);
  
  espNowState.initialized = true;
  espNowState.mode = ESP_NOW_BROADCAST;
  espNowState.messagesSent = 0;
  espNowState.messagesReceived = 0;
  
  Serial.println("[ESP-NOW] Initialization complete");
  Serial.print("[ESP-NOW] MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void deinitEspNowMidi() {
  if (!espNowState.initialized) {
    return;
  }
  
  Serial.println("[ESP-NOW] Deinitializing ESP-NOW MIDI...");
  espNowMIDI.clearPeers();
  espNowState.initialized = false;
  espNowState.mode = ESP_NOW_OFF;
}

void setEspNowMode(EspNowMode mode) {
  espNowState.mode = mode;
  
  if (mode == ESP_NOW_OFF) {
    deinitEspNowMidi();
  } else if (!espNowState.initialized) {
    initEspNowMidi();
  }
  
  Serial.print("[ESP-NOW] Mode changed to: ");
  Serial.println(mode == ESP_NOW_OFF ? "OFF" : (mode == ESP_NOW_BROADCAST ? "BROADCAST" : "PEER"));
}

bool addEspNowPeer(const char* macAddress) {
  if (!espNowState.initialized) {
    Serial.println("[ESP-NOW] Cannot add peer: Not initialized");
    return false;
  }
  
  // Parse MAC address string (format: "AA:BB:CC:DD:EE:FF")
  uint8_t mac[6];
  int values[6];
  
  if (sscanf(macAddress, "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) {
      mac[i] = (uint8_t)values[i];
    }
    return espNowMIDI.addPeer(mac);
  }
  
  Serial.println("[ESP-NOW] Invalid MAC address format");
  return false;
}

bool addEspNowPeer(const uint8_t mac[6]) {
  if (!espNowState.initialized) {
    Serial.println("[ESP-NOW] Cannot add peer: Not initialized");
    return false;
  }
  
  return espNowMIDI.addPeer(mac);
}

bool removeEspNowPeer(const uint8_t mac[6]) {
  if (!espNowState.initialized) {
    return false;
  }
  
  // Note: ESP-NOW MIDI library doesn't have a removePeer method
  // We'd need to clear all and re-add the ones we want to keep
  // For now, just log this limitation
  Serial.println("[ESP-NOW] Remove peer not implemented - use clearPeers instead");
  return false;
}

void clearEspNowPeers() {
  if (!espNowState.initialized) {
    return;
  }
  
  espNowMIDI.clearPeers();
  Serial.println("[ESP-NOW] All peers cleared");
}

int getEspNowPeerCount() {
  if (!espNowState.initialized) {
    return 0;
  }
  
  return espNowMIDI.getPeersCount();
}

void sendEspNowMidi(uint8_t status, uint8_t data1, uint8_t data2) {
  if (!espNowState.initialized || espNowState.mode == ESP_NOW_OFF) {
    return;
  }
  
  // Extract channel and message type
  uint8_t channel = status & 0x0F;
  uint8_t messageType = status & 0xF0;
  
  esp_err_t result = ESP_OK;
  
  switch (messageType) {
    case 0x90: // Note On
      result = espNowMIDI.sendNoteOn(data1, data2, channel);
      break;
    case 0x80: // Note Off
      result = espNowMIDI.sendNoteOff(data1, data2, channel);
      break;
    case 0xB0: // Control Change
      result = espNowMIDI.sendControlChange(data1, data2, channel);
      break;
    case 0xC0: // Program Change
      result = espNowMIDI.sendProgramChange(data1, channel);
      break;
    case 0xE0: // Pitch Bend
      {
        int16_t pitchValue = ((data2 << 7) | data1) - 8192;
        result = espNowMIDI.sendPitchBend(pitchValue, channel);
      }
      break;
    case 0xF8: // MIDI Clock
      result = espNowMIDI.sendClock();
      break;
    case 0xFA: // MIDI Start
      result = espNowMIDI.sendStart();
      break;
    case 0xFC: // MIDI Stop
      result = espNowMIDI.sendStop();
      break;
    case 0xFB: // MIDI Continue
      result = espNowMIDI.sendContinue();
      break;
    default:
      // Unsupported message type for ESP-NOW
      return;
  }
  
  if (result == ESP_OK) {
    espNowState.messagesSent++;
  }
}

// MIDI message handlers (route to internal MIDI system)
void onEspNowNoteOn(byte channel, byte note, byte velocity) {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  uint8_t status = 0x90 | channel;
  sendMIDI(status, note, velocity);
  
  // Route to Hardware MIDI
  sendHardwareMIDI3(status, note, velocity);
  
  Serial.printf("[ESP-NOW RX] Note On: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
}

void onEspNowNoteOff(byte channel, byte note, byte velocity) {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  uint8_t status = 0x80 | channel;
  sendMIDI(status, note, velocity);
  
  // Route to Hardware MIDI
  sendHardwareMIDI3(status, note, velocity);
  
  Serial.printf("[ESP-NOW RX] Note Off: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
}

void onEspNowControlChange(byte channel, byte control, byte value) {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  uint8_t status = 0xB0 | channel;
  sendMIDI(status, control, value);
  
  // Route to Hardware MIDI
  sendHardwareMIDI3(status, control, value);
  
  Serial.printf("[ESP-NOW RX] CC: Ch=%d, CC=%d, Val=%d\n", channel, control, value);
}

void onEspNowClock() {
  espNowState.messagesReceived++;
  
  // Only process if ESP-NOW is the clock master
  if (midiClockMaster == CLOCK_ESP_NOW) {
    // Route to BLE MIDI
    sendMIDI(0xF8, 0, 0);
    
    // Route to Hardware MIDI
    sendHardwareMIDI2(0xF8, 0);
  }
}

void onEspNowStart() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  sendMIDI(0xFA, 0, 0);
  
  // Route to Hardware MIDI
  sendHardwareMIDI2(0xFA, 0);
  
  Serial.println("[ESP-NOW RX] Start");
}

void onEspNowStop() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  sendMIDI(0xFC, 0, 0);
  
  // Route to Hardware MIDI
  sendHardwareMIDI2(0xFC, 0);
  
  Serial.println("[ESP-NOW RX] Stop");
}

void onEspNowContinue() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  sendMIDI(0xFB, 0, 0);
  
  // Route to Hardware MIDI
  sendHardwareMIDI2(0xFB, 0);
  
  Serial.println("[ESP-NOW RX] Continue");
}

#endif // ESP_NOW_ENABLED
