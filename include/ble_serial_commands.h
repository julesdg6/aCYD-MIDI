#ifndef BLE_SERIAL_COMMANDS_H
#define BLE_SERIAL_COMMANDS_H

#include "common_definitions.h"

#if ENABLE_BLE_SERIAL
#include "ble_serial.h"

// Mode names for status reporting
static const char* kModeNames[] = {
  "Menu", "Settings", "Keyboard", "Sequencer", "Bouncing Ball",
  "Physics Drop", "Random Generator", "XY Pad", "Arpeggiator",
  "Grid Piano", "Auto Chord", "LFO", "SLINK", "TB3PO", "Grids",
  "Raga", "Euclidean", "Morph"
};

/**
 * Process BLE Serial commands
 * Call this from the main loop to handle incoming BLE Serial commands
 */
inline void processBLESerialCommands() {
  if (!bleSerial.isConnected() || bleSerial.available() == 0) {
    return;
  }
  
  char command[128];
  size_t len = bleSerial.readLine(command, sizeof(command));
  
  if (len == 0) {
    return;
  }
  
  // Convert to lowercase for case-insensitive matching
  for (size_t i = 0; i < len; i++) {
    command[i] = tolower(command[i]);
  }
  
  // Status command
  if (strcmp(command, "status") == 0) {
    char response[256];
    snprintf(response, sizeof(response),
             "Mode: %s\nBPM: %d\nBLE MIDI: %s\nVersion: %s",
             kModeNames[currentMode],
             sharedBPM,
             deviceConnected ? "Connected" : "Disconnected",
             ACYD_MIDI_VERSION);
    bleSerial.println(response);
  }
  
  // Get BPM
  else if (strcmp(command, "get bpm") == 0) {
    char response[32];
    snprintf(response, sizeof(response), "%d", sharedBPM);
    bleSerial.println(response);
  }
  
  // Set BPM
  else if (strncmp(command, "set bpm ", 8) == 0) {
    int bpm = atoi(command + 8);
    if (bpm >= 40 && bpm <= 300) {
      setSharedBPM(bpm);
      bleSerial.println("OK");
    } else {
      bleSerial.println("ERROR: BPM must be 40-300");
    }
  }
  
  // Get current mode
  else if (strcmp(command, "get mode") == 0) {
    char response[32];
    snprintf(response, sizeof(response), "%d", (int)currentMode);
    bleSerial.println(response);
  }
  
  // List available modes
  else if (strcmp(command, "list modes") == 0) {
    bleSerial.println("0: Menu");
    bleSerial.println("1: Settings");
    bleSerial.println("2: Keyboard");
    bleSerial.println("3: Sequencer");
    bleSerial.println("4: Bouncing Ball");
    bleSerial.println("5: Physics Drop");
    bleSerial.println("6: Random Generator");
    bleSerial.println("7: XY Pad");
    bleSerial.println("8: Arpeggiator");
    bleSerial.println("9: Grid Piano");
    bleSerial.println("10: Auto Chord");
    bleSerial.println("11: LFO");
    bleSerial.println("12: SLINK");
    bleSerial.println("13: TB3PO");
    bleSerial.println("14: Grids");
    bleSerial.println("15: Raga");
    bleSerial.println("16: Euclidean");
    bleSerial.println("17: Morph");
  }
  
  // Help command
  else if (strcmp(command, "help") == 0) {
    bleSerial.println("Available commands:");
    bleSerial.println("  status - Get device status");
    bleSerial.println("  get bpm - Get current BPM");
    bleSerial.println("  set bpm <value> - Set BPM (40-300)");
    bleSerial.println("  get mode - Get current mode number");
    bleSerial.println("  list modes - List all available modes");
    bleSerial.println("  help - Show this help");
    bleSerial.println("  version - Show firmware version");
  }
  
  // Version command
  else if (strcmp(command, "version") == 0) {
    bleSerial.println(ACYD_MIDI_VERSION);
  }
  
  // Unknown command
  else {
    bleSerial.print("ERROR: Unknown command: ");
    bleSerial.println(command);
    bleSerial.println("Type 'help' for available commands");
  }
}

/**
 * Send periodic status updates via BLE Serial (optional)
 * Call this from main loop if you want automatic status updates
 */
inline void sendBLESerialStatus() {
  static unsigned long lastStatusTime = 0;
  static const unsigned long STATUS_INTERVAL = 5000; // Every 5 seconds
  
  if (!bleSerial.isConnected()) {
    return;
  }
  
  unsigned long now = millis();
  if (now - lastStatusTime >= STATUS_INTERVAL) {
    char status[128];
    snprintf(status, sizeof(status),
             "[AUTO] BPM: %d, Mode: %d, MIDI: %s",
             sharedBPM,
             (int)currentMode,
             deviceConnected ? "Connected" : "Disconnected");
    bleSerial.println(status);
    lastStatusTime = now;
  }
}

#endif // ENABLE_BLE_SERIAL

#endif // BLE_SERIAL_COMMANDS_H
