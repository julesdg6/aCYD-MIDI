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

void sendMIDIClock() {
  if (deviceConnected) {
    midiPacket[2] = 0xF8;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    if (pCharacteristic) {
      pCharacteristic->setValue(midiPacket, 5);
      pCharacteristic->notify();
    }
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

void sendMIDIStart() {
  if (deviceConnected) {
    midiPacket[2] = 0xFA;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    if (pCharacteristic) {
      pCharacteristic->setValue(midiPacket, 5);
      pCharacteristic->notify();
    }
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

void sendMIDIStop() {
  if (deviceConnected) {
    midiPacket[2] = 0xFC;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    if (pCharacteristic) {
      pCharacteristic->setValue(midiPacket, 5);
      pCharacteristic->notify();
    }
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

int getNoteInScale(int scaleIndex, int degree, int octave) {
  if (scaleIndex >= NUM_SCALES) return 60;

  // If degree exceeds scale notes, wrap to next octave
  int actualDegree = degree % scales[scaleIndex].numNotes;
  int octaveOffset = degree / scales[scaleIndex].numNotes;

  int rootNote = 60; // C4
  return rootNote + scales[scaleIndex].intervals[actualDegree] + ((octave - 4 + octaveOffset) * 12);
}

String getNoteNameFromMIDI(int midiNote) {
  static const char *const kNoteNames[] = {"C", "C#", "D", "D#", "E", "F",
                                          "F#", "G", "G#", "A", "A#", "B"};
  int noteIndex = midiNote % 12;
  int octave = (midiNote / 12) - 1;
  return String(kNoteNames[noteIndex]) + String(octave);
}

void stopAllModes() {
  // Stop all MIDI notes
  for (int i = 0; i < 128; i++) {
    sendMIDI(0x80, i, 0);
  }
}
