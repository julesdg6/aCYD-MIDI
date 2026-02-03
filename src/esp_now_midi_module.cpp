#include "esp_now_midi_module.h"

#if ESP_NOW_ENABLED

#include <esp_now_midi.h>
#include <esp_wifi.h>
#include "hardware_midi.h"
#include "clock_manager.h"

// External declarations for BLE MIDI
extern BLECharacteristic *pCharacteristic;
extern bool deviceConnected;
extern uint8_t midiPacket[];

// Global ESP-NOW MIDI instance
esp_now_midi espNowMIDI;
EspNowMidiState espNowState;

void initEspNowMidi() {
  if (espNowState.initialized) {
    Serial.println("[ESP-NOW] Already initialized");
    return;
  }

  Serial.println("[ESP-NOW] Initializing ESP-NOW MIDI...");
  // Ensure the WiFi stack is in STA mode before touching modem power-save
  // This avoids calling esp_wifi_set_ps() before the WiFi driver is ready,
  // which can trigger an abort when BLE and WiFi are both active.
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
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
  // Note: mode is set by caller (setEspNowMode)
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
  
  // Note: ESP-NOW MIDI library doesn't provide a removePeer method
  // To remove a specific peer, use clearEspNowPeers() and re-add desired peers
  Serial.println("[ESP-NOW] Remove single peer not supported - use clearEspNowPeers() instead");
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
  
  esp_err_t result = ESP_OK;

  /* Real-time system messages (0xF8 and above) are single-byte and
   * should be handled by their raw status value. Masking with 0xF0
   * collapses these into 0xF0, so handle them first using the raw
   * status byte, then fall back to channel-based messages. */
  if (status >= 0xF8) {
    switch (status) {
      case 0xF8: // MIDI Clock
        result = espNowMIDI.sendClock();
        break;
      case 0xFA: // MIDI Start
        result = espNowMIDI.sendStart();
        break;
      case 0xFB: // MIDI Continue
        result = espNowMIDI.sendContinue();
        break;
      case 0xFC: // MIDI Stop
        result = espNowMIDI.sendStop();
        break;
      default:
        // Other system realtime messages not supported here
        return;
    }
  } else {
    // Channel voice messages: mask out channel and switch on high nibble
    uint8_t channel = status & 0x0F;
    uint8_t messageType = status & 0xF0;

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
      default:
        // Unsupported message type for ESP-NOW
        return;
    }
  }
  
  if (result == ESP_OK) {
    espNowState.messagesSent++;
  }
}

// MIDI message handlers (route to internal MIDI system)
void onEspNowNoteOn(byte channel, byte note, byte velocity) {
  espNowState.messagesReceived++;
  
  uint8_t status = 0x90 | channel;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = status;
    midiPacket[3] = note;
    midiPacket[4] = velocity;
    pCharacteristic->setValue(midiPacket, 5);
  // Route to Hardware MIDI
  sendHardwareMIDI(status, note, velocity);
  
  sendHardwareMIDI(status, note, velocity);
  
  Serial.printf("[ESP-NOW RX] Note On: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
}

void onEspNowNoteOff(byte channel, byte note, byte velocity) {
  espNowState.messagesReceived++;
  
  uint8_t status = 0x80 | channel;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = status;
    midiPacket[3] = note;
    midiPacket[4] = velocity;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  
  // Route to Hardware MIDI
  sendHardwareMIDI(status, note, velocity);
  sendHardwareMIDI(status, note, velocity);
  
  Serial.printf("[ESP-NOW RX] Note Off: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
}

void onEspNowControlChange(byte channel, byte control, byte value) {
  espNowState.messagesReceived++;
  
  uint8_t status = 0xB0 | channel;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = status;
    midiPacket[3] = control;
    midiPacket[4] = value;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  // Route to Hardware MIDI
  sendHardwareMIDI(status, control, value);
  
  Serial.printf("[ESP-NOW RX] CC: Ch=%d, CC=%d, Val=%d\n", channel, control, value);
}

void onEspNowClock() {
  espNowState.messagesReceived++;
  
  // Only process if ESP-NOW is the clock master
  if (midiClockMaster == CLOCK_ESP_NOW) {
    // Route to BLE MIDI
    if (deviceConnected && pCharacteristic) {
      midiPacket[2] = 0xF8;
      midiPacket[3] = 0;
      midiPacket[4] = 0;
      pCharacteristic->setValue(midiPacket, 5);
      pCharacteristic->notify();
    }
    
    // Route to Hardware MIDI
    sendHardwareMIDI(0xF8, 0);
    
    clockManagerExternalClock();
  }
}

void onEspNowStart() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = 0xFA;
    midiPacket[3] = 0;
    midiPacket[4] = 0;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  
  // Route to Hardware MIDI
  sendHardwareMIDI(0xFA, 0);
  if (midiClockMaster == CLOCK_ESP_NOW) {
    clockManagerExternalStart();
  }
  
  Serial.println("[ESP-NOW RX] Start");
}

void onEspNowStop() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = 0xFC;
    midiPacket[3] = 0;
    midiPacket[4] = 0;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  
  // Route to Hardware MIDI
  sendHardwareMIDI(0xFC, 0);
  if (midiClockMaster == CLOCK_ESP_NOW) {
    clockManagerExternalStop();
  }
  
  Serial.println("[ESP-NOW RX] Stop");
}

void onEspNowContinue() {
  espNowState.messagesReceived++;
  
  // Route to BLE MIDI
  if (deviceConnected && pCharacteristic) {
    midiPacket[2] = 0xFB;
    midiPacket[3] = 0;
    midiPacket[4] = 0;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  
  // Route to Hardware MIDI
  sendHardwareMIDI(0xFB, 0);
  
  if (midiClockMaster == CLOCK_ESP_NOW) {
    clockManagerExternalContinue();
  }

  Serial.println("[ESP-NOW RX] Continue");
}

#endif // ESP_NOW_ENABLED
