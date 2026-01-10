#ifndef HARDWARE_MIDI_H
#define HARDWARE_MIDI_H

#include <Arduino.h>

// Hardware MIDI Configuration
// Set to 0 for UART0 (GPIO1/3 - Serial Breakout, no USB debug)
// Set to 2 for UART2 (GPIO16/17 - Expansion GPIOs, keeps USB debug)
#ifndef HARDWARE_MIDI_UART
#define HARDWARE_MIDI_UART 2  // Default to UART2 for development
#endif

// Enable/disable hardware MIDI output
#ifndef HARDWARE_MIDI_ENABLED
#define HARDWARE_MIDI_ENABLED true
#endif

// MIDI baud rate (standard MIDI 1.0)
#define MIDI_BAUD_RATE 31250

// Pin definitions
#if HARDWARE_MIDI_UART == 0
  // UART0 - Uses Serial breakout pins on CYD
  #define MIDI_RX_PIN 3   // GPIO3 (RX0)
  #define MIDI_TX_PIN 1   // GPIO1 (TX0)
  #define MIDI_SERIAL Serial
  // Disable debug output when using UART0 for MIDI
  #define DEBUG_ENABLED false
#elif HARDWARE_MIDI_UART == 2
  // UART2 - Uses expansion GPIOs
  #define MIDI_RX_PIN 16  // GPIO16 (RX2)
  #define MIDI_TX_PIN 17  // GPIO17 (TX2)
  // Keep debug output available when using UART2
  #define DEBUG_ENABLED true
#else
  #error "HARDWARE_MIDI_UART must be 0 or 2"
#endif

// UART2 instance (only used when HARDWARE_MIDI_UART == 2)
#if HARDWARE_MIDI_UART == 2
extern HardwareSerial MIDISerial;
#endif

// Function declarations
void initHardwareMIDI();
void sendHardwareMIDI(uint8_t byte1, uint8_t byte2, uint8_t byte3);
void sendHardwareMIDI(uint8_t byte1, uint8_t byte2);

// Debug macro - only active when DEBUG_ENABLED is true
#if DEBUG_ENABLED
  #define MIDI_DEBUG(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define MIDI_DEBUG(fmt, ...) ((void)0)
#endif

// Initialize hardware MIDI output
inline void initHardwareMIDI() {
  if (!HARDWARE_MIDI_ENABLED) return;
  
#if HARDWARE_MIDI_UART == 0
  // UART0 - reconfigure Serial for MIDI
  // Note: This must be called AFTER any Serial.begin(115200) has been skipped
  Serial.begin(MIDI_BAUD_RATE, SERIAL_8N1, MIDI_RX_PIN, MIDI_TX_PIN);
#elif HARDWARE_MIDI_UART == 2
  // UART2 - initialize separate MIDI serial
  MIDISerial.begin(MIDI_BAUD_RATE, SERIAL_8N1, MIDI_RX_PIN, MIDI_TX_PIN);
#endif
}

// Send 3-byte MIDI message to hardware output
inline void sendHardwareMIDI(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  if (!HARDWARE_MIDI_ENABLED) return;
  
#if HARDWARE_MIDI_UART == 0
  Serial.write(byte1);
  Serial.write(byte2);
  Serial.write(byte3);
#elif HARDWARE_MIDI_UART == 2
  MIDISerial.write(byte1);
  MIDISerial.write(byte2);
  MIDISerial.write(byte3);
#endif
}

// Send 2-byte MIDI message to hardware output (e.g., program change)
inline void sendHardwareMIDI(uint8_t byte1, uint8_t byte2) {
  if (!HARDWARE_MIDI_ENABLED) return;
  
#if HARDWARE_MIDI_UART == 0
  Serial.write(byte1);
  Serial.write(byte2);
#elif HARDWARE_MIDI_UART == 2
  MIDISerial.write(byte1);
  MIDISerial.write(byte2);
#endif
}

#endif // HARDWARE_MIDI_H
