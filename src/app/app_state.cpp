#include "hardware_midi.h"

#include "common_definitions.h"

// ============================================================
// Global application state (declared in common_definitions.h)
// ============================================================

DisplayConfig displayConfig;

TFT_eSPI tft;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t midiPacket[] = {0x80, 0x80, 0, 0, 0};
TouchState touch;
AppMode currentMode = MENU;

volatile bool needsRedraw = false;
uint16_t sharedBPM = 120;
MidiClockMaster midiClockMaster = CLOCK_INTERNAL;
bool displayColorsInverted = false;

#ifndef DEFAULT_DISPLAY_ROTATION
#define DEFAULT_DISPLAY_ROTATION 3
#endif
uint8_t displayRotationIndex = DEFAULT_DISPLAY_ROTATION;

bool instantStartMode = false;

// UART2 instance for hardware MIDI (only used when HARDWARE_MIDI_UART == 2)
// This definition matches the extern declaration in hardware_midi.h
#if HARDWARE_MIDI_UART == 2
HardwareSerial MIDISerial(2);
#endif

