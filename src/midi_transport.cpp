#include "midi_transport.h"

#include "clock_manager.h"
#include "common_definitions.h"
#include "hardware_midi.h"
#include "wifi_manager.h"

#include <Arduino.h>
#if WIFI_ENABLED
#include <WiFiUdp.h>
#endif

namespace {

#if WIFI_ENABLED
static WiFiUDP wifiMidiUdp;
static constexpr uint16_t kWifiMidiListenPort = 5004;
static constexpr uint16_t kWifiMidiRemotePort = 5004;
static const IPAddress kWifiMidiRemoteIP = IPAddress(255, 255, 255, 255);
#endif

static constexpr uint32_t kStartStopDebounceMs = 100;
static constexpr uint32_t kClockLogIntervalMs = 500;
static uint32_t lastStartMs = 0;
static uint32_t lastStopMs = 0;
static uint32_t lastClockLogMs = 0;
static bool pendingExternalStart = false;
static bool externalRunning = false;
static bool clockPulseState = false;

static void processTransportByte(uint8_t byte) {
  switch (byte) {
    case 0xFA: {
      uint32_t now = millis();
      if (now - lastStartMs >= kStartStopDebounceMs) {
        pendingExternalStart = true;
        lastStartMs = now;
        Serial.println("[MidiTransport] Received MIDI Start (0xFA)");
      }
      break;
    }
    case 0xFC: {
      uint32_t now = millis();
      if (now - lastStopMs >= kStartStopDebounceMs) {
        lastStopMs = now;
        Serial.println("[MidiTransport] Received MIDI Stop (0xFC)");
        pendingExternalStart = false;
        if (externalRunning) {
          clockManagerExternalStop();
          externalRunning = false;
        }
      }
      break;
    }
    case 0xF8: {
      uint32_t now = millis();
      if (!externalRunning && pendingExternalStart) {
        externalRunning = true;
        pendingExternalStart = false;
        Serial.println("[MidiTransport] Validated External Start – sending to ClockManager");
        clockManagerExternalStart();
      }
      if (externalRunning) {
        if (now - lastClockLogMs >= kClockLogIntervalMs) {
          Serial.println("[MidiTransport] Received MIDI Clock (0xF8)");
          lastClockLogMs = now;
        }
        clockManagerExternalClock();
        clockPulseState = !clockPulseState;
      }
      break;
    }
    default:
      break;
  }
}

}  // namespace

#if HARDWARE_MIDI_ENABLED
static void handleHardwareMidiInput() {
  if (midiClockMaster != CLOCK_HARDWARE) {
    return;
  }
#if HARDWARE_MIDI_UART == 0
  while (Serial.available() > 0) {
    uint8_t byte = static_cast<uint8_t>(Serial.read());
    processTransportByte(byte);
  }
#elif HARDWARE_MIDI_UART == 2
  while (MIDISerial.available() > 0) {
    uint8_t byte = static_cast<uint8_t>(MIDISerial.read());
    processTransportByte(byte);
  }
#endif
}
#else
static void handleHardwareMidiInput() {}
#endif

#if WIFI_ENABLED
static void handleWiFiMidiInput() {
  if (midiClockMaster != CLOCK_WIFI || !isWiFiConnected()) {
    return;
  }
  int packetSize = wifiMidiUdp.parsePacket();
  while (packetSize > 0) {
    static uint8_t buffer[256];
    int len = wifiMidiUdp.read(buffer, sizeof(buffer));
    if (len > 0) {
      for (int i = 0; i < len; ++i) {
        processTransportByte(buffer[i]);
      }
    }
    packetSize = wifiMidiUdp.parsePacket();
  }
}
#else
static void handleWiFiMidiInput() {}
#endif

void initMidiTransports() {
#if WIFI_ENABLED
  wifiMidiUdp.begin(kWifiMidiListenPort);
#endif
}

void handleMidiTransports() {
  handleHardwareMidiInput();
  handleWiFiMidiInput();
}

void midiTransportProcessIncomingBytes(const uint8_t *data, size_t length) {
  if (midiClockMaster != CLOCK_BLE) {
    return;
  }
  for (size_t i = 0; i < length; ++i) {
    processTransportByte(data[i]);
  }
}

void sendWiFiMidiMessage(const uint8_t *data, size_t length) {
#if WIFI_ENABLED
  if (!isWiFiConnected() || length == 0) {
    return;
  }
  wifiMidiUdp.beginPacket(kWifiMidiRemoteIP, kWifiMidiRemotePort);
  wifiMidiUdp.write(data, static_cast<int>(length));
  wifiMidiUdp.endPacket();
#else
  (void)data;
  (void)length;
#endif
}

bool midiTransportIsPulseActive() {
  return clockPulseState;
}
