#include "app/app_ble_midi.h"

#include "hardware_midi.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>
#include <esp_bt.h>
#include <esp_mac.h>

#include <string>

#include "common_definitions.h"
#include "midi_transport.h"
#include "midi_utils.h"

namespace {

static uint32_t ble_init_start_ms = 0;
static bool ble_initialized = false;
static String uniqueDeviceName;

static volatile bool ble_request_redraw = false;
static volatile bool ble_disconnect_action = false;

// Generate unique device name based on MAC address.
static String getUniqueDeviceName() {
  if (!uniqueDeviceName.isEmpty()) {
    return uniqueDeviceName;
  }

  // Get WiFi MAC address.
  uint8_t mac[6];
  esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);

  if (err != ESP_OK) {
#if DEBUG_ENABLED
    Serial.printf("Failed to read MAC address (error %d), using default name\n", err);
#endif
    uniqueDeviceName = "aCYD MIDI";
    return uniqueDeviceName;
  }

  // Format last 3 octets as hex string (e.g., "AABBCC")
  // Buffer size: 6 hex characters + null terminator = 7 bytes
  char macSuffix[7];
  snprintf(macSuffix, sizeof(macSuffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);

  uniqueDeviceName = "aCYD MIDI-" + String(macSuffix);
  return uniqueDeviceName;
}

class MIDICallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    (void)server;
    deviceConnected = true;
#if DEBUG_ENABLED
    Serial.println("BLE connected");
#endif
    if (currentMode == MENU) {
      ble_request_redraw = true;
    }
  }

  void onDisconnect(BLEServer *server) override {
    (void)server;
    deviceConnected = false;
#if DEBUG_ENABLED
    Serial.println("BLE disconnected - sending All Notes Off");
#endif
    // Defer heavy disconnect handling to main loop to avoid doing work
    // inside the BLE callback/task context.
    ble_disconnect_action = true;
  }
};

class MidiCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    std::string value = characteristic->getValue();
    if (!value.empty()) {
      midiTransportProcessIncomingBytes(reinterpret_cast<const uint8_t *>(value.data()),
                                        value.size());
    }
  }
};

static void setupBLE() {
  static bool bt_mem_released = false;
  if (!bt_mem_released) {
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    bt_mem_released = true;
  }

  String deviceName = getUniqueDeviceName();

  BLEDevice::init(deviceName.c_str());
#if DEBUG_ENABLED
  Serial.printf("Configuring BLE with device name: %s\n", deviceName.c_str());
#endif

  // Configure BLE security for "Just Works" pairing (no PIN/passkey) and
  // also provide a static PIN for clients that require one.
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setCapability(0x03); // IO_CAPS_NONE
  pSecurity->setStaticPIN(123456);
#if DEBUG_ENABLED
  Serial.println("BLESecurity: IO_CAPS_NONE, static PIN=123456 set");
#endif

  // Register security callbacks to log authentication events.
  class MyBLESecurityCallbacks : public BLESecurityCallbacks {
  public:
    uint32_t onPassKeyRequest() override {
#if DEBUG_ENABLED
      Serial.println("BLESecurityCallbacks: onPassKeyRequest()");
#endif
      return 0;
    }
    void onPassKeyNotify(uint32_t pass_key) override {
#if DEBUG_ENABLED
      Serial.printf("BLESecurityCallbacks: onPassKeyNotify: %06u\n", pass_key);
#else
      (void)pass_key;
#endif
    }
    bool onConfirmPIN(uint32_t pass_key) override {
#if DEBUG_ENABLED
      Serial.printf("BLESecurityCallbacks: onConfirmPIN: %06u\n", pass_key);
#else
      (void)pass_key;
#endif
      return true;
    }
    bool onSecurityRequest() override {
#if DEBUG_ENABLED
      Serial.println("BLESecurityCallbacks: onSecurityRequest()");
#endif
      return true;
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t) override {
#if DEBUG_ENABLED
      Serial.println("BLESecurityCallbacks: onAuthenticationComplete()");
#endif
    }
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

#if DEBUG_ENABLED
  Serial.printf("BLE advertising initialized for %s\n", deviceName.c_str());
#endif
}

}  // namespace

void bleMidiBegin() {
#if BLE_ENABLED
  ble_init_start_ms = millis();
  ble_initialized = false;
  ble_request_redraw = false;
  ble_disconnect_action = false;
#endif
}

void bleMidiLoop(uint32_t now) {
#if !BLE_ENABLED
  (void)now;
  return;
#else
  if (!ble_initialized && (now - ble_init_start_ms) > 5000) {
    setupBLE();
    ble_initialized = true;

#if ESP_NOW_ENABLED
#if DEBUG_ENABLED
    // Initialize ESP-NOW after BLE to avoid conflicts.
    Serial.println("ESP-NOW MIDI available (enable via Settings)");
#endif
#endif
  }

  // Handle deferred BLE actions set by BLE callbacks (run in main loop).
  if (ble_disconnect_action) {
    ble_disconnect_action = false;
#if DEBUG_ENABLED
    Serial.println("Handling BLE disconnect in main loop: stopping modes and restarting advertising");
#endif
    stopAllModes();
    requestRedraw();
    delay(500);
    BLEDevice::startAdvertising();
#if DEBUG_ENABLED
    Serial.println("BLE advertising restarted for reconnection");
#endif
  }
  if (ble_request_redraw) {
    ble_request_redraw = false;
    requestRedraw();
  }
#endif
}

