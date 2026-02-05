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
  if (!pTxCharacteristic) {
    Serial.println("BLE Serial: Failed to create TX characteristic");
    return false;
  }
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // Create RX characteristic (client -> device, write)
  pRxCharacteristic = pService->createCharacteristic(
    BLE_SERIAL_CHARACTERISTIC_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  if (!pRxCharacteristic) {
    Serial.println("BLE Serial: Failed to create RX characteristic");
    return false;
  }
  pRxCharacteristic->setCallbacks(new BLESerialCallbacks(this));
  
  // Register server callbacks so we receive connect/disconnect events
  server->setCallbacks(new BLESerialServerCallbacks(this));
  
  // Start the service
  pService->start();
  
  Serial.println("BLE Serial service initialized");
  return true;
}

void BLESerial::onRxWrite(const uint8_t *data, size_t length) {
  // Add received data to RX buffer with mutex protection
  std::lock_guard<std::mutex> g(rxMutex);
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
  std::lock_guard<std::mutex> g(rxMutex);
  return (int)rxBuffer.size();
}

int BLESerial::read() {
  std::lock_guard<std::mutex> g(rxMutex);
  if (rxBuffer.empty()) {
    return -1;
  }
  uint8_t byte = rxBuffer.front();
  rxBuffer.erase(rxBuffer.begin());
  return byte;
}

size_t BLESerial::readBytes(uint8_t *buffer, size_t length) {
  std::lock_guard<std::mutex> g(rxMutex);
  size_t count = 0;
  while (count < length && !rxBuffer.empty()) {
    buffer[count++] = rxBuffer.front();
    rxBuffer.erase(rxBuffer.begin());
  }
  return count;
}

size_t BLESerial::readLine(char *buffer, size_t maxLen) {
  std::lock_guard<std::mutex> g(rxMutex);
  size_t count = 0;
  while (count < maxLen - 1 && !rxBuffer.empty()) {
    uint8_t byte = rxBuffer.front();
    rxBuffer.erase(rxBuffer.begin());
    if (byte == '\n' || byte == '\r') {
      // Consume trailing \n after \r (handle \r\n)
      if (byte == '\r' && !rxBuffer.empty() && rxBuffer.front() == '\n') {
        rxBuffer.erase(rxBuffer.begin());
      }
      break;
    }
    buffer[count++] = byte;
  }
  buffer[count] = '\0';
  return count;
}

int BLESerial::peek() {
  std::lock_guard<std::mutex> g(rxMutex);
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
  std::lock_guard<std::mutex> g(txMutex);
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
    std::lock_guard<std::mutex> g(txMutex);
    txBuffer.clear();
    return false;
  }
  sendTxData();
  return true;
}

void BLESerial::sendTxData() {
  if (!clientConnected || !pTxCharacteristic) {
    return;
  }
  // Send at most one chunk per invocation to avoid overflowing the BLE queue
  std::lock_guard<std::mutex> g(txMutex);
  if (txBuffer.empty()) return;
  size_t chunkSize = min((size_t)BLE_SERIAL_TX_MAX_CHUNK, txBuffer.size());
  uint8_t chunk[BLE_SERIAL_TX_MAX_CHUNK];
  for (size_t i = 0; i < chunkSize; i++) {
    chunk[i] = txBuffer[i];
  }
  // Attempt to notify; if client disconnected mid-way, leave buffer intact
  if (clientConnected) {
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
  std::lock_guard<std::mutex> g1(rxMutex);
  std::lock_guard<std::mutex> g2(txMutex);
  rxBuffer.clear();
  txBuffer.clear();
}
