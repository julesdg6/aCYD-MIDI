# aCYD MIDI Controller

Touchscreen Bluetooth MIDI controller for the ESP32-2432S028R "Cheap Yellow Display" (CYD).

[![Release](https://img.shields.io/github/v/release/julesdg6/aCYD-MIDI)](https://github.com/julesdg6/aCYD-MIDI/releases)
[![Build](https://github.com/julesdg6/aCYD-MIDI/workflows/Build%20ESP32%20firmware%20(PlatformIO)/badge.svg)](https://github.com/julesdg6/aCYD-MIDI/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Special thanks to Brian Lough for putting together the resources on this board. Check out his repo for more examples: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

## Quick Start

**Download pre-built firmware:** Check the [Releases](https://github.com/julesdg6/aCYD-MIDI/releases) page for ready-to-flash `.bin` files for your board configuration.

## Features

See [`docs/README.md`](docs/README.md) for module deep-dives, capture flow, and Wi-Fi notes.

### 16 Interactive Modes

- **KEYS** - Virtual piano keyboard with scale and key controls
- **BEATS** - 16-step sequencer with 4 tracks and tempo control
- **ZEN** - Ambient bouncing ball mode for generative music
- **DROP** - Physics-based ball drop with customizable platforms
- **RNG** - Random music generator for creative exploration
- **XY PAD** - Touch-controlled X/Y pad for real-time parameter control
- **ARP** - Arpeggiator with chord-based patterns
- **GRID** - Grid piano with 4ths layout for unique playing style
- **CHORD** - Auto-chord mode with diatonic chord progressions
- **LFO** - Low-frequency oscillator for modulation effects
- **TB3PO** - TB-3 inspired generator with density, accent, and regen controls for rapid phrase creation
- **GRIDS** - Multi-layer arpeggiator blending kick/snare/hat densities with touch-based XY tweaking
- **RAGA** - Indian raga explorer for scale selection, drone toggles, and tempo-aware dronescapes
- **EUCLID** - Euclidean rhythm sequencer with per-voice patterns, BPM, and triplet toggles
- **MORPH** - Gesture morphing surface that blends stored slots, X/Y sliders, and rec/play controls
- **SLINK** - Dual-wave generative engine with 16-band triggering, tabbed parameter panels, and MIDI scale/clock modulation for evolving textures ([docs/SLINK_IMPLEMENTATION.md](docs/SLINK_IMPLEMENTATION.md))

### Core Features

- **Bluetooth MIDI** - Wireless connection to DAWs and music software
- **Hardware MIDI** - Dual BLE + DIN-5 output with selectable UART0 (production) or UART2 (development), conditional debug, and wiring/build guidance ([docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md), [docs/HARDWARE_MIDI_CONFIG.md](docs/HARDWARE_MIDI_CONFIG.md))
- **Touchscreen Interface** - Intuitive visual controls optimized for the CYD display
- **Real-time Control** - Low-latency MIDI output
- **Visual Feedback** - Responsive graphics
- **Screenshot Capture** - LVGL snapshot → BMP writer uses incremental filenames and padding-safe rows for consistent SD output
- **Main Menu** - 4×4 grid, darker rainbow palette, and black button labels align with the updated mockup in `assets/screenshots/00_main_menu.bmp`
- **Splash Screen** - Boot artwork now pulls `assets/aCYD-MIDI.png` and prints the connected IP when Wi-Fi is up
- **Remote Display** - WebSocket viewer with 20 FPS frames, auto-reconnect, and status indicator for browser monitoring ([docs/REMOTE_DISPLAY.md](docs/REMOTE_DISPLAY.md))

## What You Need

- **ESP32-2432S028R (CYD)** - ~$15 from AliExpress/Amazon
- PlatformIO or Arduino IDE with ESP32 support
- *Optional:* Hardware MIDI circuit components (see [docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md))
- **MicroSD Card** (optional) - For screenshot capture feature
- Arduino IDE with ESP32 support or PlatformIO

## Installation

### Option A: Flash Pre-built Firmware (Recommended)

1. **Download firmware** from the [Releases](https://github.com/julesdg6/aCYD-MIDI/releases) page
   - Choose the appropriate `.bin` file for your needs:
     - `firmware-esp32-2432S028Rv2` - Default (UART2 for development with debug)
     - `firmware-esp32-2432S028Rv2-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-esp32-2432S028Rv2-uart2` - Explicit UART2 configuration

2. **Flash using ESP32 Flash Tool** or **esptool.py**:
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
   ```

3. **Connect via Bluetooth**
   - Pair "CYD MIDI" via Bluetooth
   - Select as MIDI input in your DAW

### Option B: Build from Source

#### Using PlatformIO (Recommended)

1. Install PlatformIO
2. Clone this repository
3. Build and upload:
   ```bash
   pio run -e esp32-2432S028Rv2 -t upload
   ```

#### Using Arduino IDE

### 1. Add ESP32 Board Support
1. Go to `File` → `Preferences`
2. Add to "Additional Boards Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Go to `Tools` → `Board` → `Boards Manager`
4. Search "ESP32" and install "ESP32 by Espressif Systems"

### 2. Install Libraries
In Arduino IDE Library Manager, install:
- `TFT_eSPI` by Bodmer
- `XPT2046_Touchscreen` by Paul Stoffregen

### 3. Configure TFT_eSPI
Replace the `libraries/TFT_eSPI/User_Setup.h` with the `User_Setup.h` from the repo.

### 4. Upload Code
1. Clone this repo and open `CYD_MIDI_Controller.ino`
2. Select board: `ESP32 Dev Module`
3. Connect CYD and upload
(Lower Upload Speed to `115200` if the sketch isn't uploading)

### 5. Connect
1. Pair "CYD MIDI" via Bluetooth
2. Select as MIDI input in your DAW

## Display Autoscaling

The UI automatically scales to different screen sizes and resolutions. The system:

- **Auto-detects** display dimensions at startup from LVGL
- **Scales** all UI elements proportionally using scaling macros
- **Maintains** consistent layout across different ESP32/CYD hardware
- **Supports** custom display configurations via `platformio.ini`

### Reference Display
The default layout is designed for 320x240 pixels (ESP32-2432S028R). The scaling system automatically adjusts all coordinates, sizes, and spacing for different resolutions.

### Adding Support for New Displays
1. Configure your display hardware in `platformio.ini` build flags
2. The autoscaling system will automatically adjust the UI
3. No code changes needed - scaling happens at runtime
### 6. Optional: Hardware MIDI Output
For traditional DIN-5 MIDI output with lower latency, see the complete guide:
- **[Hardware MIDI Setup Guide](docs/HARDWARE_MIDI.md)**
- Supports simultaneous BLE + Hardware MIDI
- Two UART options: UART0 (serial breakout) or UART2 (expansion GPIOs)
- Circuit components ~$5-10
## Usage

### Taking Screenshots
1. Insert a MicroSD card into the CYD's SD card slot
2. From the main menu, tap the "SCREENSHOT" button
3. Screenshots are saved as BMP files (screen000.bmp, screen001.bmp, etc.) to the SD card
4. View the serial monitor for confirmation messages
5. Hold the BACK button on the main menu for ~1.5 s to cycle through every mode and save `/<board>_<mode>_NNN.bmp` files inside `/screenshots`
### 6. Optional: Enable Remote Display
To view the CYD display in your web browser:
1. Copy `config/wifi_config.local.h.template` to `config/wifi_config.local.h` and update `WIFI_SSID` / `WIFI_PASSWORD`
2. Rebuild and upload
3. Check Serial Monitor for the IP address
4. Open the IP address in your browser

The `config/wifi_config.local.h` file is ignored by git to keep credentials private.
For full details, see [docs/REMOTE_DISPLAY.md](docs/REMOTE_DISPLAY.md)

## Troubleshooting

- **Upload Speed**: Lower it to `115200` if the sketch isn't uploading
- **Blank screen**: Check TFT_eSPI pin configuration
- **No touch**: Verify touchscreen library installation
- **No Bluetooth**: Restart device and re-pair
- **UI too small/large**: Check display resolution detection in serial output
- **Screenshot fails**: Ensure SD card is properly inserted and formatted (FAT32)
- **Remote Display not working**: Ensure WiFi credentials are correct and your network is 2.4GHz (ESP32 doesn't support 5GHz)

## New Modules & Fixes

### SLINK Mode

- Dual Slink engines with 16 bands, 7 tabbed parameter panels, and per-band trigger/pitch/clock/scale/mod settings add a deep generative mode that is fully integrated into the menu and AppMode/loop flow ([docs/SLINK_IMPLEMENTATION.md](docs/SLINK_IMPLEMENTATION.md)).
- Voice stealing, scale quantization, and auto-release handling keep the mode in sync with the rest of the MIDI output pipeline so nothing breaks when switching modes.

### Hardware MIDI Output

- All MIDI messages now stream simultaneously to BLE and a DIN-5 connector with selectable UART0 (production) or UART2 (development) pins, plus detailed wiring/circuit guidance and platformio build_flag examples to keep configuration centralized ([docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md), [docs/CIRCUIT_DIAGRAMS.md](docs/CIRCUIT_DIAGRAMS.md), [docs/HARDWARE_MIDI_CONFIG.md](docs/HARDWARE_MIDI_CONFIG.md)).
- Conditional `MIDI_DEBUG` macros disable USB prints when UART0 owns Serial while keeping them for UART2 so debugging remains reliable without disturbing hardware MIDI timing ([docs/IMPLEMENTATION_SUMMARY.md](docs/IMPLEMENTATION_SUMMARY.md)).

-### Remote Display Module

- WebSocket-based viewer streams RGB565 frames at ~20 FPS with auto-reconnect and status indications so remote monitoring stays in sync even on flaky WiFi, plus documented setup/reset guidance via `config/wifi_config.local.h.template` (copy to `config/wifi_config.local.h`) or build flags ([docs/REMOTE_DISPLAY.md](docs/REMOTE_DISPLAY.md)).
- Notes about current framebuffer limitations and future compression/touch-forwarding work keep expectations clear while giving a path for extending the feature.

### Screenshot Capture Improvements

- LVGL snapshots are converted to BMP with explicit buffer-size validation, padded rows, and sequential `/screenNNN.bmp` filenames so captures never silently corrupt the SD card, and the module logs successes/failures over Serial for easier troubleshooting (`src/screenshot.cpp`).
- The SCREENSHOT button now always writes BMP files that can be read by any computer, keeping the existing feature reliable while we explore remote display recording later.

## License

Open source - see MIT license file for details.
