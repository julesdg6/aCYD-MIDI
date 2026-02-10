#include "drivers/m5_8encoder.h"

#ifdef ENABLE_M5_8ENCODER

// Global encoder instance
M5_8Encoder encoder8;

M5_8Encoder::M5_8Encoder() 
  : _wire(nullptr), _addr(M5_8ENCODER_ADDR), _initialized(false), _lastButtonStates(0), _currentButtonStates(0) {
  memset(_lastEncoderValues, 0, sizeof(_lastEncoderValues));
}

bool M5_8Encoder::begin(TwoWire &wire, uint8_t sda, uint8_t scl, uint8_t addr) {
  _wire = &wire;
  _addr = addr;
  
  // Initialize I2C
  _wire->begin(sda, scl);
  _wire->setClock(100000);  // 100kHz I2C speed
  
  // Check if device is present
  _wire->beginTransmission(_addr);
  if (_wire->endTransmission() == 0) {
    _initialized = true;
    
    // Reset all encoders to zero
    resetAllEncoders();
    
    // Read initial button states
    readButtons();
    _lastButtonStates = _currentButtonStates;
    
    return true;
  }
  
  _initialized = false;
  return false;
}

bool M5_8Encoder::isConnected() {
  if (!_initialized || !_wire) {
    return false;
  }
  
  _wire->beginTransmission(_addr);
  return (_wire->endTransmission() == 0);
}

bool M5_8Encoder::poll() {
  if (!_initialized || !_wire) {
    return false;
  }
  
  bool changed = false;
  
  // Read all encoder values
  readEncoders();
  
  // Read button states
  uint8_t oldButtonStates = _currentButtonStates;
  readButtons();
  
  // Check for changes in encoders or buttons
  for (int i = 0; i < 8; i++) {
    if (_lastEncoderValues[i] != 0) {
      changed = true;
    }
  }
  
  if (oldButtonStates != _currentButtonStates) {
    changed = true;
  }
  
  return changed;
}

EncoderEvent M5_8Encoder::getEvent(uint8_t index) {
  EncoderEvent event;
  event.encoderIndex = index;
  event.delta = 0;
  event.buttonPressed = false;
  event.buttonJustPressed = false;
  event.buttonJustReleased = false;
  
  if (index >= 8) {
    return event;
  }
  
  // Get encoder delta
  event.delta = _lastEncoderValues[index];
  
  // Get button states
  uint8_t buttonMask = (1 << index);
  event.buttonPressed = (_currentButtonStates & buttonMask) != 0;
  bool wasPressed = (_lastButtonStates & buttonMask) != 0;
  
  event.buttonJustPressed = event.buttonPressed && !wasPressed;
  event.buttonJustReleased = !event.buttonPressed && wasPressed;
  
  return event;
}

int8_t M5_8Encoder::getEncoderValue(uint8_t index) {
  if (index >= 8) {
    return 0;
  }
  return _lastEncoderValues[index];
}

bool M5_8Encoder::getButtonState(uint8_t index) {
  if (index >= 8) {
    return false;
  }
  return (_currentButtonStates & (1 << index)) != 0;
}

void M5_8Encoder::resetEncoder(uint8_t index) {
  if (index >= 8 || !_initialized) {
    return;
  }
  
  // Write to the encoder reset register
  writeRegister(ENCODER_RESET_REG + index, 0x00);
  _lastEncoderValues[index] = 0;
}

void M5_8Encoder::resetAllEncoders() {
  if (!_initialized) {
    return;
  }
  
  for (int i = 0; i < 8; i++) {
    writeRegister(ENCODER_RESET_REG + i, 0x00);
    _lastEncoderValues[i] = 0;
  }
}

uint8_t M5_8Encoder::readRegister(uint8_t reg) {
  if (!_wire) {
    return 0;
  }
  
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->endTransmission(false);
  
  _wire->requestFrom(_addr, (uint8_t)1);
  if (_wire->available()) {
    return _wire->read();
  }
  return 0;
}

void M5_8Encoder::writeRegister(uint8_t reg, uint8_t value) {
  if (!_wire) {
    return;
  }
  
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(value);
  _wire->endTransmission();
}

void M5_8Encoder::readEncoders() {
  if (!_wire) {
    return;
  }
  
  // Read all 8 encoder values (8 bytes starting at ENCODER_REG_BASE)
  _wire->beginTransmission(_addr);
  _wire->write(ENCODER_REG_BASE);
  _wire->endTransmission(false);
  
  _wire->requestFrom(_addr, (uint8_t)8);
  for (int i = 0; i < 8; i++) {
    if (_wire->available()) {
      _lastEncoderValues[i] = (int8_t)_wire->read();
    } else {
      _lastEncoderValues[i] = 0;
    }
  }
}

void M5_8Encoder::readButtons() {
  // Update last state before reading new state
  _lastButtonStates = _currentButtonStates;
  
  // Read button register (1 byte, 8 bits for 8 buttons)
  _currentButtonStates = readRegister(BUTTON_REG);
}

#endif // ENABLE_M5_8ENCODER
