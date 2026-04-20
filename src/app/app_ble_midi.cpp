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

static bool setupBLE() {
  static bool bt_mem_released = false;
  if (!bt_mem_released) {
    esp_err_t release_result = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (release_result != ESP_OK && release_result != ESP_ERR_INVALID_STATE) {
#if DEBUG_ENABLED
      Serial.printf("Failed to release BT classic memory (error %d)\n", release_result);
#endif
      return false;
    }
    bt_mem_released = true;
  }

  pCharacteristic = nullptr;
  String deviceName = getUniqueDeviceName();

  BLEDevice::init(deviceName.c_str());
#if DEBUG_ENABLED
  Serial.printf("Configuring BLE with device name: %s\n", deviceName.c_str());
#endif

  // Configure BLE security for "Just Works" pairing (no PIN/passkey) and
  // also provide a static PIN for clients that require one.
  // Use static instances for BLE-owned callback/config objects so retries
  // do not leak heap allocations after a failed initialization attempt.
  static BLESecurity security;
  BLESecurity *pSecurity = &security;
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
  static MyBLESecurityCallbacks securityCallbacks;
  BLEDevice::setSecurityCallbacks(&securityCallbacks);

  BLEServer *server = BLEDevice::createServer();
  if (server == nullptr) {
#if DEBUG_ENABLED
    Serial.println("BLE server creation failed");
#endif
    return false;
  }
  static MIDICallbacks serverCallbacks;
  server->setCallbacks(&serverCallbacks);
  BLEService *service = server->createService(SERVICE_UUID);
  if (service == nullptr) {
#if DEBUG_ENABLED
    Serial.println("BLE service creation failed");
#endif
    return false;
  }
  BLECharacteristic *characteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR |
      BLECharacteristic::PROPERTY_NOTIFY);
  if (characteristic == nullptr) {
#if DEBUG_ENABLED
    Serial.println("BLE characteristic creation failed");
#endif
    return false;
  }
  static BLE2902 descriptor;
  characteristic->addDescriptor(&descriptor);

  static MidiCharacteristicCallbacks characteristicCallbacks;
  characteristic->setCallbacks(&characteristicCallbacks);
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  if (advertising == nullptr) {
#if DEBUG_ENABLED
    Serial.println("BLE advertising object is null");
#endif
    return false;
  }
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  pCharacteristic = characteristic;

#if DEBUG_ENABLED
  Serial.printf("BLE advertising initialized for %s\n", deviceName.c_str());
#endif
  return true;
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
    if (setupBLE()) {
      ble_initialized = true;
    } else {
      // Backoff before retrying BLE initialization again.
      ble_init_start_ms = now;
#if DEBUG_ENABLED
      Serial.println("BLE initialization failed; retrying in 5s");
#endif
    }

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
