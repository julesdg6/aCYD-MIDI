// main_headless.cpp - Headless USB MIDI master with ESP-NOW sync
// Build with: [env:esp32-headless-midi-master] or [env:esp32s3-headless]

#ifdef HEADLESS_BUILD

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>
#include <BLE2902.h>

// USB MIDI only available on ESP32-S3 with native USB support
#if USB_MIDI_DEVICE && defined(ARDUINO_USB_MODE)
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// USB MIDI device
Adafruit_USBD_MIDI usbMIDI;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbMIDI, MIDI_USB);
#define USB_MIDI_ENABLED 1
#else
#define USB_MIDI_ENABLED 0
#endif

#if ESP_NOW_ENABLED
#include <esp_now.h>
#include <esp_now_midi.h>

// ESP-NOW MIDI instance
esp_now_midi espNowMIDI;
#endif

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

void sendHardwareMIDI2(uint8_t byte1, uint8_t byte2) {
  if (!HARDWARE_MIDI_ENABLED) return;
  MIDISerial.write(byte1);
  MIDISerial.write(byte2);
}

void sendHardwareMIDI3(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  if (!HARDWARE_MIDI_ENABLED) return;
  MIDISerial.write(byte1);
  MIDISerial.write(byte2);
  MIDISerial.write(byte3);
}




// BLE MIDI setup (minimal, no display dependencies)
BLECharacteristic *pCharacteristic = nullptr;
volatile bool deviceConnected = false;
uint8_t midiPacket[5] = {0x80, 0x80, 0, 0, 0};
// Transport running state for reliable Start/Stop transitions
static bool transportRunning = false;

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

#if ESP_NOW_ENABLED
// ESP-NOW message handlers
void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
  // ESP-NOW MIDI library handles this
}
#endif

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 10; ++i) { delay(10); }
  Serial.println("aCYD-HEADLESS starting...");
  Serial.flush();

#if USB_MIDI_ENABLED
  Serial.println("Step 1: Initializing USB MIDI");
  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  Serial.println("USB MIDI initialized");
#endif

  Serial.println("Step 2: WiFi.mode(WIFI_STA)");
  WiFi.mode(WIFI_STA);
  
  Serial.println("Step 3: setupBLE()");
  setupBLE();
  
  Serial.println("Step 4: initializing hardware MIDI");
  initHardwareMIDI();
#if ESP_NOW_ENABLED
  Serial.println("Step 5: Initializing ESP-NOW as master");
  // Initialize ESP-NOW MIDI with auto-peer discovery enabled and low latency
  espNowMIDI.begin(false, true);  // (encryption=false, low_latency=true)

  // Register ESP-NOW MIDI handlers for routing received messages
  espNowMIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    Serial.printf("[ESP-NOW RX] Note On: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, static_cast<uint8_t>(0x90 | channel), note, velocity};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDI3(0x90 | channel, note, velocity);
#if USB_MIDI_ENABLED
    MIDI_USB.sendNoteOn(note, velocity, channel + 1);
#endif
  });

  espNowMIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    Serial.printf("[ESP-NOW RX] Note Off: Ch=%d, Note=%d, Vel=%d\n", channel, note, velocity);
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, static_cast<uint8_t>(0x80 | channel), note, velocity};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDI3(0x80 | channel, note, velocity);
#if USB_MIDI_ENABLED
    MIDI_USB.sendNoteOff(note, velocity, channel + 1);
#endif
  });

  espNowMIDI.setHandleControlChange([](byte channel, byte control, byte value) {
    Serial.printf("[ESP-NOW RX] CC: Ch=%d, CC=%d, Val=%d\n", channel, control, value);
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, static_cast<uint8_t>(0xB0 | channel), control, value};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDI3(0xB0 | channel, control, value);
#if USB_MIDI_ENABLED
    MIDI_USB.sendControlChange(control, value, channel + 1);
#endif
  });

  espNowMIDI.setHandleClock([]() {
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, 0xF8, 0x00, 0x00};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDISingle(0xF8);
#if USB_MIDI_ENABLED
    MIDI_USB.sendRealTime(midi::Clock);
#endif
  });

  espNowMIDI.setHandleStart([]() {
    Serial.println("[ESP-NOW RX] Start");
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, 0xFA, 0x00, 0x00};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDISingle(0xFA);
#if USB_MIDI_ENABLED
    MIDI_USB.sendRealTime(midi::Start);
#endif
  });

  espNowMIDI.setHandleStop([]() {
    Serial.println("[ESP-NOW RX] Stop");
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, 0xFC, 0x00, 0x00};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDISingle(0xFC);
#if USB_MIDI_ENABLED
    MIDI_USB.sendRealTime(midi::Stop);
#endif
  });

  espNowMIDI.setHandleContinue([]() {
    Serial.println("[ESP-NOW RX] Continue");
    if (deviceConnected && pCharacteristic) {
      uint8_t packet[5] = {0x80, 0x80, 0xFB, 0x00, 0x00};
      pCharacteristic->setValue(packet, 5);
      pCharacteristic->notify();
    }
    sendHardwareMIDISingle(0xFB);
#if USB_MIDI_ENABLED
    MIDI_USB.sendRealTime(midi::Continue);
#endif
  });

  Serial.println("ESP-NOW MIDI master initialized");
  Serial.print("ESP-NOW MAC Address: ");
  Serial.println(WiFi.macAddress());
#else
  Serial.println("Step 5: ESP-NOW disabled in build");
#endif
  
  Serial.println("Setup complete - Headless MIDI master ready");
}





// Unified MIDI clock/start/stop for BLE, hardware, USB, and ESP-NOW
void sendMidiClock() {
  // BLE MIDI
  if (deviceConnected && pCharacteristic) {
    uint8_t packet[5] = {0x80, 0x80, 0xF8, 0x00, 0x00};
    pCharacteristic->setValue(packet, 5);
  }
  // Hardware MIDI
  sendHardwareMIDISingle(0xF8);
#if USB_MIDI_ENABLED
  // USB MIDI
  MIDI_USB.sendRealTime(midi::Clock);
#endif
#if ESP_NOW_ENABLED
  // ESP-NOW (send to all discovered peers)
  espNowMIDI.sendClock();
#endif
  // USB Serial debug (only when enabled for debugging to avoid jitter)
#ifdef DEBUG_MIDI_CLOCK
#endif
}

void sendMidiStart() {
  if (deviceConnected && pCharacteristic) {
    uint8_t packet[5] = {0x80, 0x80, 0xFA, 0x00, 0x00};
    pCharacteristic->setValue(packet, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFA);
#if USB_MIDI_ENABLED
  MIDI_USB.sendRealTime(midi::Start);
#endif
#if ESP_NOW_ENABLED
  espNowMIDI.sendStart();
#endif
  Serial.println("MIDI Start");
}

void sendMidiStop() {
  if (deviceConnected && pCharacteristic) {
    uint8_t packet[5] = {0x80, 0x80, 0xFC, 0x00, 0x00};
    pCharacteristic->setValue(packet, 5);
    pCharacteristic->notify();
  }
  sendHardwareMIDISingle(0xFC);
#if USB_MIDI_ENABLED
  MIDI_USB.sendRealTime(midi::Stop);
#endif
#if ESP_NOW_ENABLED
  espNowMIDI.sendStop();
#endif
  Serial.println("MIDI Stop");
}

void loop() {
#if USB_MIDI_ENABLED
  // Process USB MIDI input
  MIDI_USB.read();
#endif

  unsigned long now = micros();
  if (now - lastClock >= MIDI_CLOCK_INTERVAL_US) {
    lastClock += MIDI_CLOCK_INTERVAL_US;
    sendMidiClock();
    clockTick++;
    // Transport start/stop handling: send Start on transition to running
    if (clockTick == 1 && !transportRunning) {
      sendMidiStart();
      transportRunning = true;
    }
  }
  delay(1); // Yield to watchdog
}

#endif // HEADLESS_BUILD
