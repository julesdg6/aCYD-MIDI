// main_headless.cpp - Headless USB MIDI master with ESP-NOW sync
// Build with: [env:esp32-headless-midi-master]

#ifdef HEADLESS_BUILD

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>
#include <BLE2902.h>

// BLE MIDI UUIDs (from common_definitions.h)
#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// Hardware MIDI config (from hardware_midi.h)
#define HARDWARE_MIDI_ENABLED true
#define HARDWARE_MIDI_UART 2
#define MIDI_BAUD_RATE 31250
#define MIDI_RX_PIN 16
#define MIDI_TX_PIN 17

HardwareSerial MIDISerial(2);

void initHardwareMIDI() {
  if (!HARDWARE_MIDI_ENABLED) return;
  MIDISerial.begin(MIDI_BAUD_RATE, SERIAL_8N1, MIDI_RX_PIN, MIDI_TX_PIN);
}

void sendHardwareMIDISingle(uint8_t byte1) {
  if (!HARDWARE_MIDI_ENABLED) return;
  MIDISerial.write(byte1);
}




// BLE MIDI setup (minimal, no display dependencies)
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t midiPacket[5] = {0x80, 0x80, 0, 0, 0};

class MIDICallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override { deviceConnected = true; }
  void onDisconnect(BLEServer *server) override { deviceConnected = false; BLEDevice::startAdvertising(); }
};
class MidiCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {}
};
void setupBLE() {
  BLEDevice::init("aCYD-HEADLESS");
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setCapability(0x03);
  pSecurity->setStaticPIN(123456);
  class MyBLESecurityCallbacks : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() override { return 0; }
    void onPassKeyNotify(uint32_t) override {}
    bool onConfirmPIN(uint32_t) override { return true; }
    bool onSecurityRequest() override { return true; }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t) override {}
  };
  BLEDevice::setSecurityCallbacks(new MyBLESecurityCallbacks());
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MIDICallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  pCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR |
      BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MidiCharacteristicCallbacks());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
}

#define MIDI_BPM 120
/* MIDI clock runs at 24 ticks per quarter note. Compute interval in microseconds
 * to avoid millisecond truncation and improve timing precision. */
#define MIDI_CLOCK_INTERVAL_US (60000000UL / (MIDI_BPM * 24UL))



unsigned long lastClock = 0;
uint32_t clockTick = 0;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 10; ++i) { delay(10); }
  Serial.println("aCYD-HEADLESS starting...");
  Serial.flush();

  Serial.println("Step 1: WiFi.mode(WIFI_STA)");
  WiFi.mode(WIFI_STA);
  Serial.println("Step 2: setupBLE()");
  setupBLE();
  Serial.println("Step 3: (initHardwareMIDI() skipped)");
  initHardwareMIDI();
  Serial.println("Step 4: esp_now_init()");
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  Serial.println("Step 5: Add ESP-NOW peer");
  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ESP-NOW add peer failed");
  }
  Serial.println("Step 6: Setup complete");
}





// Unified MIDI clock/start/stop for BLE, hardware, and ESP-NOW
void sendMidiClock() {
  // BLE MIDI
  if (deviceConnected) {
    midiPacket[2] = 0xF8;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  // Hardware MIDI
  sendHardwareMIDISingle(0xF8);
  // ESP-NOW
  uint8_t wifiClock = 0xF8;
  esp_now_send(NULL, &wifiClock, 1);
  // USB Serial debug
  Serial.println("MIDI Clock");
}
void sendMidiStart() {
  if (deviceConnected) {
    midiPacket[2] = 0xFA;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFA);
  uint8_t wifiStart = 0xFA;
  esp_now_send(NULL, &wifiStart, 1);
  Serial.println("MIDI Start");
}
void sendMidiStop() {
  if (deviceConnected) {
    midiPacket[2] = 0xFC;
    midiPacket[3] = 0x00;
    midiPacket[4] = 0x00;
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFC);
  uint8_t wifiStop = 0xFC;
  esp_now_send(NULL, &wifiStop, 1);
  Serial.println("MIDI Stop");
}

void loop() {
  unsigned long now = micros();
  if (now - lastClock >= MIDI_CLOCK_INTERVAL_US) {
    lastClock += MIDI_CLOCK_INTERVAL_US;
    sendMidiClock();
    clockTick++;
    // Optionally: send Start/Stop at appropriate times
  }
  delay(1); // Yield to watchdog
  // Optionally: handle USB MIDI input for BPM/transport
}

#endif // HEADLESS_BUILD
