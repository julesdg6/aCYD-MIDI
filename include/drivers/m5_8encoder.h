#ifndef M5_8ENCODER_H
#define M5_8ENCODER_H

#include <Arduino.h>

#ifdef ENABLE_M5_8ENCODER

#include <Wire.h>

// M5Stack 8Encoder I2C address (default is 0x41)
#define M5_8ENCODER_ADDR 0x41

// Register addresses for M5Stack 8Encoder
#define ENCODER_REG_BASE 0x00    // Encoder counter registers (16 encoders, 1 byte each)
#define BUTTON_REG 0x10          // Button state register (1 byte, 8 bits)
#define ENCODER_RESET_REG 0x20   // Reset encoder counters

// Encoder events
struct EncoderEvent {
  uint8_t encoderIndex;  // 0-7
  int8_t delta;          // Change in encoder value (-128 to +127)
  bool buttonPressed;    // Current button state
  bool buttonJustPressed;   // Button was just pressed
  bool buttonJustReleased;  // Button was just released
};

class M5_8Encoder {
public:
  M5_8Encoder();
  
  // Initialize the encoder unit
  // Returns true if device was detected
  bool begin(TwoWire &wire = Wire, uint8_t sda = 21, uint8_t scl = 22, uint8_t addr = M5_8ENCODER_ADDR);
  
  // Check if the encoder unit is connected and responding
  bool isConnected();
  
  // Poll encoders and buttons for changes
  // Returns true if any encoder or button changed
  bool poll();
  
  // Get the latest event (call after poll returns true)
  EncoderEvent getEvent(uint8_t index);
  
  // Get encoder value (relative position since last reset)
  int8_t getEncoderValue(uint8_t index);
  
  // Get button state (true = pressed)
  bool getButtonState(uint8_t index);
  
  // Reset encoder counter to zero
  void resetEncoder(uint8_t index);
  
  // Reset all encoder counters
  void resetAllEncoders();
  
private:
  TwoWire *_wire;
  uint8_t _addr;
  bool _initialized;
  
  // Previous states for change detection
  int8_t _lastEncoderValues[8];
  uint8_t _lastButtonStates;
  uint8_t _currentButtonStates;
  
  // Read a single byte from a register
  uint8_t readRegister(uint8_t reg);
  
  // Write a single byte to a register
  void writeRegister(uint8_t reg, uint8_t value);
  
  // Read encoder values (8 bytes)
  void readEncoders();
  
  // Read button states (1 byte, 8 bits)
  void readButtons();
};

// Global encoder instance (defined in m5_8encoder.cpp when enabled)
extern M5_8Encoder encoder8;

#endif // ENABLE_M5_8ENCODER

#endif // M5_8ENCODER_H
