#include "ble_serial.h"

// Callback class for BLE Serial characteristics
class BLESerialCallbacks : public BLECharacteristicCallbacks {
public:
  BLESerialCallbacks(BLESerial *serial) : pSerial(serial) {}
  
  void onWrite(BLECharacteristic *characteristic) override {
    std::string value = characteristic->getValue();
    if (!value.empty()) {
      pSerial->onRxWrite(reinterpret_cast<const uint8_t *>(value.data()), value.size());
    }
  }

private:
  BLESerial *pSerial;
};

// Callback class for BLE Serial connection/disconnection
class BLESerialServerCallbacks : public BLEServerCallbacks {
public:
  BLESerialServerCallbacks(BLESerial *serial) : pSerial(serial) {}
  
  void onConnect(BLEServer *server) override {
    pSerial->clientConnected = true;
    Serial.println("BLE Serial client connected");
  }
  
  void onDisconnect(BLEServer *server) override {
    pSerial->clientConnected = false;
    Serial.println("BLE Serial client disconnected");
  }

private:
  BLESerial *pSerial;
};

// Global instance
BLESerial bleSerial;

BLESerial::BLESerial()
  : pTxCharacteristic(nullptr),
    pRxCharacteristic(nullptr),
    clientConnected(false),
    lastFlushTime(0) {
  rxBuffer.reserve(BLE_SERIAL_RX_BUFFER_SIZE);
  txBuffer.reserve(BLE_SERIAL_TX_BUFFER_SIZE);
}

bool BLESerial::begin(BLEServer *server) {
  if (!server) {
    Serial.println("BLE Serial: Invalid server pointer");
    return false;
  }
  
  // Create the BLE Serial service
  BLEService *pService = server->createService(BLE_SERIAL_SERVICE_UUID);
  if (!pService) {
    Serial.println("BLE Serial: Failed to create service");
    return false;
  }
  
  // Create TX characteristic (device -> client, notify)
  pTxCharacteristic = pService->createCharacteristic(
    BLE_SERIAL_CHARACTERISTIC_TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // Create RX characteristic (client -> device, write)
  pRxCharacteristic = pService->createCharacteristic(
    BLE_SERIAL_CHARACTERISTIC_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new BLESerialCallbacks(this));
  
  // Start the service
  pService->start();
  
  Serial.println("BLE Serial service initialized");
  return true;
}

void BLESerial::onRxWrite(const uint8_t *data, size_t length) {
  // Add received data to RX buffer
  // Apply buffer overflow protection
  size_t available_space = BLE_SERIAL_RX_BUFFER_SIZE - rxBuffer.size();
  size_t bytes_to_add = min(length, available_space);
  
  if (bytes_to_add < length) {
    Serial.println("BLE Serial: RX buffer overflow, dropping data");
  }
  
  for (size_t i = 0; i < bytes_to_add; i++) {
    rxBuffer.push_back(data[i]);
  }
}

int BLESerial::available() {
  return rxBuffer.size();
}

int BLESerial::read() {
  if (rxBuffer.empty()) {
    return -1;
  }
  
  uint8_t byte = rxBuffer.front();
  rxBuffer.erase(rxBuffer.begin());
  return byte;
}

size_t BLESerial::readBytes(uint8_t *buffer, size_t length) {
  size_t count = 0;
  while (count < length && !rxBuffer.empty()) {
    buffer[count++] = rxBuffer.front();
    rxBuffer.erase(rxBuffer.begin());
  }
  return count;
}

size_t BLESerial::readLine(char *buffer, size_t maxLen) {
  size_t count = 0;
  while (count < maxLen - 1 && !rxBuffer.empty()) {
    uint8_t byte = rxBuffer.front();
    rxBuffer.erase(rxBuffer.begin());
    
    if (byte == '\n' || byte == '\r') {
      break;
    }
    
    buffer[count++] = byte;
  }
  buffer[count] = '\0';
  return count;
}

int BLESerial::peek() {
  if (rxBuffer.empty()) {
    return -1;
  }
  return rxBuffer.front();
}

size_t BLESerial::write(uint8_t byte) {
  return write(&byte, 1);
}

size_t BLESerial::write(const uint8_t *buffer, size_t length) {
  if (!buffer || length == 0) {
    return 0;
  }
  
  // Add data to TX buffer with overflow protection
  size_t available_space = BLE_SERIAL_TX_BUFFER_SIZE - txBuffer.size();
  size_t bytes_to_add = min(length, available_space);
  
  if (bytes_to_add < length) {
    Serial.println("BLE Serial: TX buffer overflow, dropping data");
  }
  
  for (size_t i = 0; i < bytes_to_add; i++) {
    txBuffer.push_back(buffer[i]);
  }
  
  return bytes_to_add;
}

size_t BLESerial::print(const char *str) {
  return write(reinterpret_cast<const uint8_t *>(str), strlen(str));
}

size_t BLESerial::println(const char *str) {
  size_t n = print(str);
  n += write('\n');
  return n;
}

bool BLESerial::flush() {
  if (!clientConnected || !pTxCharacteristic) {
    // Clear buffer if not connected
    txBuffer.clear();
    return false;
  }
  
  sendTxData();
  return true;
}

void BLESerial::sendTxData() {
  if (!clientConnected || !pTxCharacteristic || txBuffer.empty()) {
    return;
  }
  
  // Send data in chunks to respect BLE MTU
  // Note: We send all pending data in one loop iteration to maintain
  // responsiveness. The BLE stack handles internal buffering.
  while (!txBuffer.empty()) {
    size_t chunkSize = min((size_t)BLE_SERIAL_TX_MAX_CHUNK, txBuffer.size());
    
    // Copy chunk to temporary buffer
    uint8_t chunk[BLE_SERIAL_TX_MAX_CHUNK];
    for (size_t i = 0; i < chunkSize; i++) {
      chunk[i] = txBuffer[i];
    }
    
    // Send via BLE notification
    pTxCharacteristic->setValue(chunk, chunkSize);
    pTxCharacteristic->notify();
    
    // Remove sent data from buffer
    txBuffer.erase(txBuffer.begin(), txBuffer.begin() + chunkSize);
  }
}

bool BLESerial::isConnected() {
  return clientConnected;
}

void BLESerial::loop() {
  // Auto-flush TX buffer periodically
  unsigned long now = millis();
  if (!txBuffer.empty() && (now - lastFlushTime) >= FLUSH_INTERVAL_MS) {
    flush();
    lastFlushTime = now;
  }
}

void BLESerial::clear() {
  rxBuffer.clear();
  txBuffer.clear();
}
