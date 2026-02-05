#ifndef BLE_SERIAL_H
#define BLE_SERIAL_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <mutex>
#include <vector>
#include <atomic>

// BLE Serial Service UUID (custom service)
// Generated UUIDs for BLE Serial service
#define BLE_SERIAL_SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_SERIAL_CHARACTERISTIC_RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_SERIAL_CHARACTERISTIC_TX_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// BLE Serial Configuration
#define BLE_SERIAL_RX_BUFFER_SIZE 256
#define BLE_SERIAL_TX_BUFFER_SIZE 256
#define BLE_SERIAL_TX_MAX_CHUNK 20  // Maximum bytes per notification (BLE MTU consideration)

/**
 * BLE Serial Service
 * Provides a UART-like serial interface over BLE for control/debug/config.
 * Implements a Stream-like interface for reading and writing.
 */
class BLESerial {
public:
  BLESerial();
  
  /**
   * Initialize the BLE Serial service
   * Must be called after BLE device is initialized but before advertising starts
   * @param server Pointer to the BLE server instance
   * @return true if initialization successful
   */
  bool begin(BLEServer *server);
  
  /**
   * Check if data is available to read
   * @return Number of bytes available
   */
  int available();
  
  /**
   * Read a single byte from the RX buffer
   * @return Byte read, or -1 if no data available
   */
  int read();
  
  /**
   * Read multiple bytes from the RX buffer
   * @param buffer Destination buffer
   * @param length Maximum bytes to read
   * @return Number of bytes actually read
   */
  size_t readBytes(uint8_t *buffer, size_t length);
  
  /**
   * Read a line (until newline or buffer full)
   * @param buffer Destination buffer
   * @param maxLen Maximum buffer size
   * @return Number of bytes read
   */
  size_t readLine(char *buffer, size_t maxLen);
  
  /**
   * Peek at the next byte without removing it
   * @return Next byte, or -1 if no data available
   */
  int peek();
  
  /**
   * Write a single byte to the TX buffer
   * @param byte Byte to write
   * @return 1 if successful, 0 otherwise
   */
  size_t write(uint8_t byte);
  
  /**
   * Write multiple bytes to the TX buffer
   * @param buffer Source buffer
   * @param length Number of bytes to write
   * @return Number of bytes actually written
   */
  size_t write(const uint8_t *buffer, size_t length);
  
  /**
   * Write a null-terminated string
   * @param str String to write
   * @return Number of bytes written
   */
  size_t print(const char *str);
  
  /**
   * Write a string with newline
   * @param str String to write
   * @return Number of bytes written
   */
  size_t println(const char *str);
  
  /**
   * Flush the TX buffer (send all pending data)
   * @return true if successful
   */
  bool flush();
  
  /**
   * Check if a BLE client is connected to the serial service
   * @return true if connected
   */
  bool isConnected();
  
  /**
   * Main loop processing - call periodically from main loop
   * Handles automatic flushing and rate limiting
   */
  void loop();
  
  /**
   * Clear all buffers
   */
  void clear();

private:
  BLECharacteristic *pTxCharacteristic;
  BLECharacteristic *pRxCharacteristic;
  std::atomic<bool> clientConnected{false};
  
  // RX buffer (data received from BLE client)
  std::vector<uint8_t> rxBuffer;
  std::mutex rxMutex;
  
  // TX buffer (data to send to BLE client)
  std::vector<uint8_t> txBuffer;
  std::mutex txMutex;
  
  // Rate limiting
  unsigned long lastFlushTime;
  static const unsigned long FLUSH_INTERVAL_MS = 20; // Flush every 20ms max
  
  // Friend class for callbacks
  friend class BLESerialCallbacks;
  // Allow server connection/disconnection callbacks to update connection state
  friend class BLESerialServerCallbacks;
  
  // Internal methods
  void onRxWrite(const uint8_t *data, size_t length);
  void sendTxData();
};

// Global BLE Serial instance
extern BLESerial bleSerial;

#endif // BLE_SERIAL_H
