# GitHub Copilot Instructions for aCYD-MIDI

## Project Overview

This is an ESP32-based Bluetooth MIDI controller for the ESP32-2432S028R "Cheap Yellow Display" (CYD). It provides 10+ interactive music modes including keyboard, sequencer, arpeggiator, XY pad, and generative music modes, all with a touchscreen interface.

**Key Technologies:**
- ESP32 microcontroller (Arduino framework)
- PlatformIO build system
- LVGL v9.2.2 graphics library (via esp32_smartdisplay)
- TFT_eSPI-compatible API (compatibility layer over LVGL)
- BLE MIDI for wireless connectivity
- Optional Hardware MIDI via UART (DIN-5 connector)

## Build System

**Primary build tool:** PlatformIO
- Configuration file: `platformio.ini`
- Default environment: `esp32-2432S028Rv2`
- Monitor speed: 115200 baud

**Build commands:**
```bash
pio run                    # Build default environment
pio run -t upload          # Build and upload
pio device monitor         # Monitor serial output
```

**Important build flags:**
- All hardware configuration must be in `platformio.ini` build_flags
- Never edit library files like `lib/TFT_eSPI/User_Setup.h`

## Code Structure

### Main Components

- **`src/main.cpp`**: Main entry point, BLE setup, menu system, main loop
- **`include/common_definitions.h`**: Global types, color scheme, extern declarations
- **`include/*_mode.h`**: Interactive mode implementations (keyboard, sequencer, etc.)
- **`include/hardware_midi.h`**: Hardware MIDI output via UART
- **`include/ui_elements.h`**: Reusable UI components and scaling macros
- **`include/midi_utils.h`**: MIDI message helpers and music theory utilities
- **`include/remote_display.h`**: WiFi streaming of display to web browser
- **`include/screenshot.h`**: SD card screenshot capture

### Architecture Patterns

1. **Mode-based architecture**: Each interactive mode is a self-contained header file with:
   - `initialize<Mode>Mode()` - Setup/reset function
   - `draw<Mode>Mode()` - Full screen redraw
   - `handle<Mode>Mode()` - Touch input and updates

2. **Header-only implementations**: Most mode code lives in header files included by `main.cpp`

3. **Global state**: Shared objects declared in `common_definitions.h`:
   - `tft` - Display driver
   - `ts` - Touch controller
   - `pCharacteristic` - BLE MIDI characteristic
   - `deviceConnected` - BLE connection status
   - `touch` - Touch state tracking
   - `currentMode` - Active mode

4. **Dual MIDI output**: All MIDI messages go to both:
   - BLE MIDI via `sendMIDI()` 
   - Hardware MIDI via `sendHardwareMIDI()` (if configured)

## Display Scaling System

The UI automatically adapts to different display sizes:

- **Reference resolution**: 320x240 (CYD default)
- **Scaling macros**: `SCALE_X()`, `SCALE_Y()` for coordinates and dimensions
- **Display detection**: Automatic via LVGL at startup
- **Global config**: `displayConfig` struct holds dimensions and scale factors

**Always use scaling macros** for UI element positioning and sizing to maintain compatibility across hardware variants.

## Hardware Configuration Rules

⚠️ **CRITICAL**: All hardware configuration belongs in `platformio.ini` build_flags ONLY.

**Never edit:**
- `lib/TFT_eSPI/User_Setup.h`
- Any library configuration files

**Why**: Library file edits create duplication, conflicts, and make version updates impossible.

**Correct approach**: Add `-D FLAG_NAME=VALUE` to `build_flags` in `platformio.ini`

See `CONFIG_RULES.md` for full details.

## UART Configuration

Two UART options for hardware MIDI:

1. **UART0 (GPIO1/3)**: Uses serial breakout pins
   - Define: `HARDWARE_MIDI_UART=0`
   - Disables USB serial debugging
   - Best for production

2. **UART2 (GPIO16/17)**: Uses expansion GPIOs
   - Define: `HARDWARE_MIDI_UART=2` (default)
   - Keeps USB debugging available
   - Best for development

**Debug output**: Use `MIDI_DEBUG` macro for conditional debug printing that respects UART configuration.

## Color Scheme

Consistent theme colors defined in `common_definitions.h`:
- `THEME_BG` - Background (black)
- `THEME_SURFACE` - Surface elements
- `THEME_PRIMARY` - Primary actions (cyan)
- `THEME_SECONDARY` - Secondary actions (orange)
- `THEME_ACCENT` - Accents (bright cyan)
- `THEME_SUCCESS` - Positive actions (green)
- `THEME_WARNING` - Warning actions (yellow)
- `THEME_ERROR` - Error states (red)
- `THEME_TEXT` - Primary text (white)
- `THEME_TEXT_DIM` - Secondary text (gray)

## Code Style Guidelines

1. **Include guards**: Use `#ifndef HEADER_NAME_H` pattern
2. **Global variables**: Minimize globals; prefer mode-local state
3. **Naming**: 
   - Functions: `camelCase` (e.g., `drawKeyboardMode`)
   - Variables: `camelCase` (e.g., `keyboardOctave`)
   - Constants: `UPPER_SNAKE_CASE` (e.g., `NUM_KEYS`)
   - Macros: `UPPER_SNAKE_CASE` (e.g., `THEME_BG`)
4. **Comments**: Add comments for complex logic; keep brief for obvious code
5. **Touch handling**: Use `TouchState` struct and check `justPressed`/`justReleased`

## MIDI Implementation

**BLE MIDI packet format:**
```cpp
uint8_t midiPacket[] = {0x80, 0x80, status, data1, data2};
```

**Common MIDI messages:**
- Note On: `0x90 | channel` (status), note (data1), velocity (data2)
- Note Off: `0x80 | channel` (status), note (data1), velocity (data2)
- Control Change: `0xB0 | channel` (status), CC# (data1), value (data2)

**Helper functions:**
- `sendMIDI(status, data1, data2)` - Send to BLE MIDI
- `sendHardwareMIDI2(status, data1)` - Send 2-byte to hardware MIDI
- `sendHardwareMIDI3(status, data1, data2)` - Send 3-byte to hardware MIDI

## Testing

**No automated tests** - manual testing workflow:

1. **Build**: `pio run`
2. **Upload**: `pio run -t upload`
3. **Monitor**: `pio device monitor`
4. **Test**: Interact with physical device
5. **Verify**: Check MIDI output in DAW/software

**Test each mode** after changes to ensure touch interaction and MIDI output work correctly.

## Common Tasks

### Adding a new mode:
1. Create `include/new_mode.h`
2. Implement init, draw, and handle functions
3. Add to `AppMode` enum in `common_definitions.h`
4. Add menu item in `main.cpp` `kMenuItems` array
5. Include header in `main.cpp`
6. Add switch cases in `setup()` and `loop()`

### Modifying UI elements:
1. Use scaling macros: `SCALE_X()`, `SCALE_Y()`
2. Use theme colors from `common_definitions.h`
3. Use UI helpers from `ui_elements.h` (`drawRoundButton`, `drawHeader`, etc.)
4. Test on physical hardware if possible

### Debugging:
1. Use `Serial.println()` wrapped in `MIDI_DEBUG` checks
2. Check `pio device monitor` for output
3. Verify BLE connection status via `deviceConnected` flag
4. Use remote display feature for visual debugging without physical access

## Documentation

Key documentation files:
- **`README.md`**: User-facing setup and usage guide
- **`HARDWARE_MIDI.md`**: Hardware MIDI wiring and configuration
- **`CIRCUIT_DIAGRAMS.md`**: ASCII circuit diagrams
- **`REMOTE_DISPLAY.md`**: WiFi display streaming setup
- **`CONFIG_RULES.md`**: Critical hardware configuration rules
- **`IMPLEMENTATION_SUMMARY.md`**: Hardware MIDI implementation details

Keep documentation in sync with code changes.

## Dependencies

**PlatformIO libraries** (in `platformio.ini` lib_deps):
- `BLE` - Bluetooth Low Energy (ESP32-specific library)
- `me-no-dev/AsyncTCP` - Async networking (optional, for remote display)
- `me-no-dev/ESP Async WebServer` - Web server for remote display (optional)

**Local libraries** (in `lib/` directory):
- `esp32_smartdisplay` - Display abstraction library for CYD boards
  - Includes LVGL v9.2.2 graphics library as a dependency
  - Provides hardware-specific display and touch drivers
  - Located in `lib/esp32_smartdisplay/`

**Compatibility Layer**:
- The project includes `smartdisplay_compat.h` which provides a TFT_eSPI-compatible API on top of LVGL
- This allows existing TFT_eSPI code to work with the LVGL-based display system
- Touch input uses XPT2046_Touchscreen interface (implementation provided at build time)

**Note**: While the README mentions Arduino IDE installation with TFT_eSPI and XPT2046_Touchscreen libraries, the PlatformIO build uses the `esp32_smartdisplay` library with compatibility wrappers instead.

## Security & Best Practices

1. **WiFi credentials**: Never commit real credentials
   - Use `include/wifi_config.h.example` template
   - User creates `include/wifi_config.h` locally
   - `.gitignore` excludes `wifi_config.h`

2. **Serial debugging**: Respect UART configuration
   - Use `MIDI_DEBUG` macro for debug output
   - Don't assume Serial is always available

3. **Touch input**: Always validate touch coordinates
   - Check bounds before accessing arrays
   - Use `touch.justPressed` to avoid repeats

4. **MIDI timing**: Keep loop fast
   - Avoid blocking operations
   - Use non-blocking delays where needed

## Common Pitfalls

❌ **Don't**:
- Edit library configuration files
- Use hardcoded coordinates without scaling
- Assume Serial is available when using UART0 for MIDI
- Block the main loop with delays
- Forget to send MIDI to both BLE and hardware outputs

✅ **Do**:
- Put all hardware config in `platformio.ini`
- Use scaling macros for all UI coordinates
- Wrap debug output in `MIDI_DEBUG` checks
- Keep main loop responsive
- Use helper functions for MIDI output

## Getting Help

- Check existing mode implementations for examples
- Review documentation files for detailed information
- Test on actual hardware when possible
- Monitor serial output during development
