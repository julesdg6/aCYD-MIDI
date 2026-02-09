#include "app/app_serial_cli.h"

#include "hardware_midi.h"

#include <Arduino.h>

#include "app/app_modes.h"
#include "module_raga_mode.h"

// Minimal serial CLI to support automated testing. Commands (case-insensitive):
// MODE <name>         -> switch to mode (e.g., MODE RAGA)
// MODULE START RAGA   -> switch to mode and start Raga (uses toggleRagaPlayback)
// MODULE STOP RAGA    -> stop Raga
// Any unknown command is ignored.
void processSerialCommands() {
#if !DEBUG_ENABLED
  // In production UART0 builds, Serial is used for MIDI. Avoid parsing raw MIDI bytes
  // as text commands to prevent unintended behavior.
  return;
#else
  static String lineBuf;
  while (Serial.available()) {
    int c = Serial.read();
    if (c <= 0) break;
    if (c == '\r') continue;
    if (c == '\n') {
      String cmd = lineBuf;
      cmd.trim();
      lineBuf = "";
      if (cmd.length() == 0) continue;

      // Uppercase for simple compare
      for (size_t i = 0; i < cmd.length(); ++i) cmd[i] = toupper(cmd[i]);

      if (cmd.startsWith("MODE ")) {
        String arg = cmd.substring(5);
        arg.trim();
        if (arg == "RAGA") switchMode(RAGA);
        else if (arg == "TB3PO") switchMode(TB3PO);
        else if (arg == "SEQUENCER") switchMode(SEQUENCER);
        else if (arg == "RNG" || arg == "RANDOM_GENERATOR") switchMode(RANDOM_GENERATOR);
        Serial.printf("CLI: MODE %s -> %d\n", arg.c_str(), (int)currentMode);
      } else if (cmd.startsWith("MODULE ")) {
        // MODULE START RAGA
        if (cmd.indexOf("START") != -1) {
          if (cmd.indexOf("RAGA") != -1) {
            switchMode(RAGA);
            // toggleRagaPlayback starts/stops depending on current state
            toggleRagaPlayback();
            Serial.println("CLI: MODULE START RAGA");
          }
          // add more module handlers here as needed
        } else if (cmd.indexOf("STOP") != -1) {
          if (cmd.indexOf("RAGA") != -1) {
            if (raga.playing) toggleRagaPlayback();
            Serial.println("CLI: MODULE STOP RAGA");
          }
        }
      } else {
        Serial.printf("CLI: unknown command '%s'\n", cmd.c_str());
      }
    } else {
      lineBuf += (char)c;
      if (lineBuf.length() > 256) lineBuf = lineBuf.substring(lineBuf.length() - 256);
    }
  }
#endif
}

