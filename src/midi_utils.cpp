#include "midi_utils.h"

namespace {
struct BleSkipLogState {
  uint32_t last_log_ms = 0;
  uint32_t suppressed = 0;
  uint8_t last_cmd = 0;
  uint8_t last_note = 0;
  uint8_t last_vel = 0;
  bool logged_once = false;
};

BleSkipLogState ble_skip_log;
} // namespace

void sendMIDI(byte cmd, byte note, byte vel) {
#if defined(BLE_ENABLED) && BLE_ENABLED
  if (deviceConnected) {
    uint8_t localPacket[5] = {midiPacket[0], midiPacket[1], cmd, note, vel};
    if (pCharacteristic) {
      pCharacteristic->setValue(localPacket, sizeof(localPacket));
      pCharacteristic->notify();
    } else {
      MIDI_DEBUG("[MIDI] BLE notify skipped - pCharacteristic is NULL\n");
    }

    // Reset skip state once we're connected again.
    ble_skip_log.suppressed = 0;
    ble_skip_log.logged_once = false;
    ble_skip_log.last_log_ms = 0;
  } else {
    ble_skip_log.suppressed++;
    ble_skip_log.last_cmd = cmd;
    ble_skip_log.last_note = note;
    ble_skip_log.last_vel = vel;

    const uint32_t now = millis();
    constexpr uint32_t kLogIntervalMs = 2000;
    if (!ble_skip_log.logged_once || (now - ble_skip_log.last_log_ms) >= kLogIntervalMs) {
      ble_skip_log.logged_once = true;
      ble_skip_log.last_log_ms = now;
      MIDI_DEBUG("[MIDI] BLE skipped (not connected) x%u last cmd=0x%02X note=%u vel=%u\n",
                 ble_skip_log.suppressed, ble_skip_log.last_cmd, ble_skip_log.last_note, ble_skip_log.last_vel);
      ble_skip_log.suppressed = 0;
    }
  }
#endif

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

