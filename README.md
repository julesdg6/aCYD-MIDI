# aCYD MIDI Controller

Touchscreen Bluetooth MIDI controller for ESP32-based displays including the ESP32-2432S028R "Cheap Yellow Display" (CYD) and ESP32-4832S035 series.

[![Release](https://img.shields.io/github/v/release/julesdg6/aCYD-MIDI)](https://github.com/julesdg6/aCYD-MIDI/releases)
[![Build](https://github.com/julesdg6/aCYD-MIDI/workflows/Build%20ESP32%20firmware%20(PlatformIO)/badge.svg)](https://github.com/julesdg6/aCYD-MIDI/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Special thanks to Brian Lough for putting together the resources on this board. Check out his repo for more examples: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

This project was based on and heavily inspired by NickCulbertson/CYD-MIDI-Controller — thank you to Nick Culbertson for the original implementation and design decisions that informed this work. See the original repository: https://github.com/NickCulbertson/CYD-MIDI-Controller

## Quick Start

**Download pre-built firmware:** Check the [Releases](https://github.com/julesdg6/aCYD-MIDI/releases) page for ready-to-flash `.bin` files for your board configuration.

**GitHub Pages:** Visit the [web installer](https://julesdg6.github.io/aCYD-MIDI/) for easy firmware flashing, or use the [debug console](https://julesdg6.github.io/aCYD-MIDI/debug-console/) for testing and troubleshooting.

**Web Debug Console:** Try the [online debugging tool](https://julesdg6.github.io/aCYD-MIDI/debug-console/) to test MIDI and monitor device logs via Web Bluetooth and Web Serial (Chrome/Edge required).

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
- **USB MIDI** - Native USB MIDI device for ESP32-S3 headless builds (PC/Mac/Android/iOS compatible, no drivers needed) ([docs/HEADLESS_USB_MIDI.md](docs/HEADLESS_USB_MIDI.md))
- **ESP-NOW MIDI** - Low-latency wireless MIDI networking between multiple CYD devices without pairing (<10ms latency, auto-discovery, MIDI clock sync) ([docs/ESP_NOW_MIDI.md](docs/ESP_NOW_MIDI.md))
- **Hardware MIDI** - Dual BLE + DIN-5 output with selectable UART0 (production) or UART2 (development), conditional debug, and wiring/build guidance ([docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md), [docs/HARDWARE_MIDI_CONFIG.md](docs/HARDWARE_MIDI_CONFIG.md))
- **Touchscreen Interface** - Intuitive visual controls optimized for the CYD display
- **Real-time Control** - Low-latency MIDI output
- **Visual Feedback** - Responsive graphics
- **Screenshot Capture** - LVGL snapshot → BMP writer uses incremental filenames and padding-safe rows for consistent SD output
- **Main Menu** - 4×4 grid, darker rainbow palette, and black button labels align with the updated mockup in `assets/screenshots/00_main_menu.bmp`
- **Splash Screen** - Boot artwork now pulls `assets/aCYD-MIDI.png` and prints the connected IP when Wi-Fi is up
- **Remote Display** - WebSocket viewer with 20 FPS frames, auto-reconnect, and status indicator for browser monitoring ([docs/REMOTE_DISPLAY.md](docs/REMOTE_DISPLAY.md))
- **M5Stack 8Encoder Support** (Optional) - Hardware encoder expansion for hands-on parameter control with 8 rotary encoders and buttons, MIDI CC mapping, and persistent storage ([docs/M5STACK_8ENCODER.md](docs/M5STACK_8ENCODER.md))

## What You Need

### Supported Hardware

- **ESP32-2432S028R (CYD)** - 2.8" 320x240 display - ~$15 from AliExpress/Amazon
- **ESP32-4832S035C** - 3.5" 480x320 display with capacitive touch
- **ESP32-4832S035R** - 3.5" 480x320 display with resistive touch
- **ESP32-4832S040R** - 4.0" 480x320 display with resistive touch
- **Elecrow ESP32 7.0"** - 7.0" 800x480 display with capacitive touch ([product page](https://www.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html))
- **ESP32-S3 Dongles** - Headless USB MIDI mode (no display) with native USB MIDI support ([docs/HEADLESS_USB_MIDI.md](docs/HEADLESS_USB_MIDI.md))
- PlatformIO or Arduino IDE with ESP32 support
- *Optional:* Hardware MIDI circuit components (see [docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md))
- *Optional:* M5Stack 8Encoder Unit for hardware parameter control (see [docs/M5STACK_8ENCODER.md](docs/M5STACK_8ENCODER.md))
- **MicroSD Card** (optional) - For screenshot capture feature
- Arduino IDE with ESP32 support or PlatformIO

## Installation

### Option A: Web Installer (Easiest - No Software Required!)

Flash firmware directly from your browser using ESP Web Tools:

1. **Visit the Web Installer**: [https://julesdg6.github.io/aCYD-MIDI/](https://julesdg6.github.io/aCYD-MIDI/)

2. **Requirements:**
   - Chrome, Edge, or Opera browser (Web Serial API support)
   - USB cable to connect your ESP32 to your computer
   - ESP32 USB drivers (usually auto-installed)

3. **Flash Process:**
   - Connect your ESP32 board via USB
   - Click the "Install" button for your board model
   - Select your device's USB port when prompted
   - Wait for flashing to complete (1-2 minutes)

4. **Connect via Bluetooth**
   - Device will restart automatically
   - Pair "CYD MIDI" via Bluetooth
   - Select as MIDI input in your DAW
   - **Windows 11 users:** See [Platform-Specific Setup](#platform-specific-setup) for required software

**Supported boards:** All ESP32-2432S028R, ESP32-4832S035C/R, ESP32-4832S040R variants, and headless builds.

### Option B: Flash Pre-built Firmware (Command Line)

1. **Download firmware** from the [Releases](https://github.com/julesdg6/aCYD-MIDI/releases) page
   - Choose the appropriate `.bin` file for your board:
     
     **ESP32-2432S028R (CYD):**
     - `firmware-esp32-2432S028Rv2` - Default (UART2 for development with debug)
     - `firmware-esp32-2432S028Rv2-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-esp32-2432S028Rv2-uart2` - Explicit UART2 configuration
     
     **ESP32-4832S035C (Capacitive Touch):**
     - `firmware-esp32-4832S035C` - Default (UART2 for development with debug)
     - `firmware-esp32-4832S035C-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-esp32-4832S035C-uart2` - Explicit UART2 configuration
     
     **ESP32-4832S035R (Resistive Touch):**
     - `firmware-esp32-4832S035R` - Default (UART2 for development with debug)
     - `firmware-esp32-4832S035R-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-esp32-4832S035R-uart2` - Explicit UART2 configuration
     
     **ESP32-4832S040R (4.0" Resistive Touch):**
     - `firmware-esp32-4832S040R` - Default (UART2 for development with debug)
     - `firmware-esp32-4832S040R-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-esp32-4832S040R-uart2` - Explicit UART2 configuration
     
     **Elecrow ESP32 7.0" (7.0" Capacitive Touch):**
     - `firmware-elecrow-esp32-7_0` - Default (UART2 for development with debug)
     - `firmware-elecrow-esp32-7_0-uart0` - Production (UART0 for hardware MIDI, no debug)
     - `firmware-elecrow-esp32-7_0-uart2` - Explicit UART2 configuration
     
     **Headless USB MIDI:**
     - `firmware-esp32-headless-midi-master` - ESP32 headless (BLE + Hardware MIDI + ESP-NOW master, **no USB MIDI**)
     - `firmware-esp32s3-headless` - ESP32-S3 headless USB MIDI dongle (native USB MIDI + BLE + Hardware MIDI + ESP-NOW master)

2. **Flash using ESP32 Flash Tool** or **esptool.py**:
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
   ```

3. **Connect via Bluetooth**
   - Pair "CYD MIDI" via Bluetooth
   - Select as MIDI input in your DAW
   - **Windows 11 users:** See [Platform-Specific Setup](#platform-specific-setup) for required software

### Option C: Build from Source

#### Using PlatformIO (Recommended)

1. Install PlatformIO
2. Clone this repository
3. Build and upload for your board:
   ```bash
   # For ESP32-2432S028R (CYD)
   pio run -e esp32-2432S028Rv2 -t upload
   
   # For ESP32-4832S035C (Capacitive Touch)
   pio run -e esp32-4832S035C -t upload
   
   # For ESP32-4832S035R (Resistive Touch)
   pio run -e esp32-4832S035R -t upload
   
   # For ESP32-4832S040R (4.0" Resistive Touch)
   pio run -e esp32-4832S040R -t upload
   
   # For Elecrow ESP32 7.0" (7.0" Capacitive Touch)
   pio run -e elecrow-esp32-7_0 -t upload
   
   # For Headless ESP32 USB MIDI
   pio run -e esp32-headless-midi-master -t upload
   
   # For Headless ESP32-S3 USB MIDI dongle
   pio run -e esp32s3-headless -t upload
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
3. **Windows 11 users:** See [Platform-Specific Setup](#platform-specific-setup) for required software

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

## Platform-Specific Setup

### Windows 11 BLE-MIDI Connection

Windows 11 requires additional software to connect to BLE-MIDI devices. Follow these steps to use your aCYD-MIDI controller with Windows:

**Requirements:**
- [BLE-MIDI Connect](https://apps.microsoft.com/detail/9NVMLZTTWWVL) (Microsoft Store)
- [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) by Tobias Erichsen

**Setup Steps:**

1. **Install and configure loopMIDI**
   - Download and install loopMIDI from [https://www.tobias-erichsen.de/software/loopmidi.html](https://www.tobias-erichsen.de/software/loopmidi.html)
   - Open the loopMIDI application
   - Create a new virtual MIDI port by clicking the `+` button
   - Name it something like "aCYD MIDI In"
   - This virtual port will act as the MIDI interface your DAW can see

2. **Install and configure BLE-MIDI Connect**
   - Install BLE-MIDI Connect from the [Microsoft Store](https://apps.microsoft.com/detail/9NVMLZTTWWVL)
   - Launch the app
   - Scan for your BLE-MIDI device (it should appear as "CYD MIDI")
   - Connect to the device
   - In the app's routing settings, select your loopMIDI port ("aCYD MIDI In") as the output
   - BLE-MIDI Connect now translates the Bluetooth MIDI stream into a standard Windows MIDI port

3. **Use the virtual port in your DAW**
   - Open your DAW or MIDI-capable software
   - Look for the loopMIDI port you created ("aCYD MIDI In")
   - Enable it as a MIDI input device

**Troubleshooting:**
- Make sure both loopMIDI and BLE-MIDI Connect are running before opening your DAW
- If the device doesn't appear, try restarting the BLE-MIDI Connect app
- Ensure your aCYD-MIDI device is powered on and in Bluetooth pairing mode

### macOS BLE-MIDI Connection

macOS has native BLE-MIDI support:

1. Open **Audio MIDI Setup** (Applications → Utilities → Audio MIDI Setup)
2. Click the Bluetooth MIDI icon in the toolbar
3. Select "CYD MIDI" from the list of available devices
4. The device will now appear as a MIDI input in your DAW

### Linux BLE-MIDI Connection

Linux users can use [bluez-alsa](https://github.com/Arkq/bluez-alsa) or [BLEMidi](https://github.com/oxesoft/blemidi) for BLE-MIDI connectivity. Specific setup varies by distribution.

## Usage

### Basic Operation
1. Power on the CYD - you'll see the splash screen
2. Pair "CYD MIDI" via Bluetooth on your device
3. Select as MIDI input in your DAW
4. Tap a mode from the main menu to start playing

### Taking Screenshots
1. Insert a MicroSD card into the CYD's SD card slot (optional feature)
2. From the main menu, tap the "SCREENSHOT" button
3. Screenshots are saved as BMP files (screen000.bmp, screen001.bmp, etc.) to the SD card
4. View the serial monitor for confirmation messages
5. Hold the BACK button on the main menu for ~1.5 s to cycle through every mode and save `/<board>_<mode>_NNN.bmp` files inside `/screenshots`

## Experimental & Optional Features

Some features are **disabled by default** to reduce complexity and resource usage. You can enable them by modifying `platformio.ini` build flags:

### WiFi & Remote Display (Disabled by Default)

The remote display feature allows viewing the CYD screen in a web browser over WiFi. This is useful for debugging, demonstrations, and documentation.

**Status:** ⚠️ Disabled by default (`WIFI_ENABLED=0`, `REMOTE_DISPLAY_ENABLED=0`)

**To enable:**
1. Copy `config/wifi_config.local.h.template` to `config/wifi_config.local.h`
2. Update WiFi credentials in the file
3. In `platformio.ini`, change build flags:
   ```ini
   -D WIFI_ENABLED=1
   -D REMOTE_DISPLAY_ENABLED=1
   ```
4. Rebuild and upload

**Note:** WiFi adds ~50KB to firmware size and requires 2.4GHz network. See [docs/REMOTE_DISPLAY.md](docs/REMOTE_DISPLAY.md) for details.

### ESP-NOW MIDI (Disabled by Default)

ESP-NOW enables low-latency wireless MIDI networking between multiple CYD devices without pairing (<10ms latency).

**Status:** ⚠️ Disabled by default (`ESP_NOW_ENABLED=0`)

**To enable:**
1. In `platformio.ini`, change build flags:
   ```ini
   -D ESP_NOW_ENABLED=1
   ```
2. Rebuild and upload
3. Enable in Settings menu on each device

**Note:** Can be enabled at runtime via Settings menu if compiled with support. See [docs/ESP_NOW_MIDI.md](docs/ESP_NOW_MIDI.md) for setup.

### USB MIDI (ESP32-S3 Only)

Native USB MIDI support for ESP32-S3 headless builds. Works on PC/Mac/Android/iOS without drivers.

**Status:** ✅ Enabled for `esp32s3-headless` builds only

**Note:** Requires ESP32-S3 hardware. Not available on ESP32 (non-S3) boards. See [docs/HEADLESS_USB_MIDI.md](docs/HEADLESS_USB_MIDI.md) for details.

## Troubleshooting

### Device Issues

- **Upload Speed**: Lower it to `115200` if the sketch isn't uploading
- **Blank screen**: Check TFT_eSPI pin configuration
- **No touch**: Verify touchscreen library installation
- **No Bluetooth**: Restart device and re-pair
- **UI too small/large**: Check display resolution detection in serial output
- **Screenshot fails**: Ensure SD card is properly inserted and formatted (FAT32)
- **Remote Display not working**: Ensure WiFi credentials are correct and your network is 2.4GHz (ESP32 doesn't support 5GHz)

### Web Pages Issues

If you're having trouble with the web installer or debug console:

- **Web installer not loading**: See [GitHub Pages Troubleshooting Guide](docs/GITHUB_PAGES_TROUBLESHOOTING.md)
- **Debug console 404 errors**: Clear browser cache and verify GitHub Pages is enabled
- **"Installation failed" errors**: Make sure your ESP32 device is connected via USB
- **Browser compatibility**: Use Chrome, Edge, or Opera (Web Serial API required)

## New Modules & Fixes

### SLINK Mode

- Dual Slink engines with 16 bands, 7 tabbed parameter panels, and per-band trigger/pitch/clock/scale/mod settings add a deep generative mode that is fully integrated into the menu and AppMode/loop flow ([docs/SLINK_IMPLEMENTATION.md](docs/SLINK_IMPLEMENTATION.md)).
- Voice stealing, scale quantization, and auto-release handling keep the mode in sync with the rest of the MIDI output pipeline so nothing breaks when switching modes.

### Hardware MIDI Output

- All MIDI messages now stream simultaneously to BLE and a DIN-5 connector with selectable UART0 (production) or UART2 (development) pins, plus detailed wiring/circuit guidance and platformio build_flag examples to keep configuration centralized ([docs/HARDWARE_MIDI.md](docs/HARDWARE_MIDI.md), [docs/CIRCUIT_DIAGRAMS.md](docs/CIRCUIT_DIAGRAMS.md), [docs/HARDWARE_MIDI_CONFIG.md](docs/HARDWARE_MIDI_CONFIG.md)).
- Conditional `MIDI_DEBUG` macros disable USB prints when UART0 owns Serial while keeping them for UART2 so debugging remains reliable without disturbing hardware MIDI timing ([docs/IMPLEMENTATION_SUMMARY.md](docs/IMPLEMENTATION_SUMMARY.md)).

## Documentation

For comprehensive documentation, see:
- **[docs/README.md](docs/README.md)** - Complete documentation index
- **[CONTRIBUTING.md](docs/CONTRIBUTING.md)** - How to contribute to the project
- **[CHANGELOG.md](docs/CHANGELOG.md)** - Version history and release notes

Key documentation files:
- **Hardware Setup:** [Hardware MIDI](docs/HARDWARE_MIDI.md), [Circuit Diagrams](docs/CIRCUIT_DIAGRAMS.md), [Headless USB MIDI](docs/HEADLESS_USB_MIDI.md)
- **Networking:** [ESP-NOW MIDI](docs/ESP_NOW_MIDI.md), [Remote Display](docs/REMOTE_DISPLAY.md)
- **Implementation:** [SLINK Mode](docs/SLINK_IMPLEMENTATION.md), [Performance Optimizations](docs/PERFORMANCE_OPTIMIZATIONS.md)
- **Build & Release:** [Build Verification](docs/BUILD_VERIFICATION.md), [Release Process](docs/RELEASE.md)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](docs/CONTRIBUTING.md) for:
- Development setup instructions
- Coding standards and guidelines
- Pull request process
- How to report bugs and suggest features

## Web Debug Console

Use the [CYD Web Debug Console](https://julesdg6.github.io/aCYD-MIDI/debug-console/) to test and debug your CYD device directly from your browser:

- **BLE MIDI Connection**: Connect wirelessly to send/receive MIDI messages
- **Web Serial Monitoring**: View firmware logs via USB in real-time
- **Live Control Panel**: TB-303-style keyboard and knobs for testing
- **Time-Synced Logging**: Unified timeline showing all MIDI and Serial events
- **Export Logs**: Download debug sessions as JSON or CSV

**Requirements**: Chrome, Edge, or Opera browser (Web Bluetooth + Web Serial API support)

See [`debug-console/README.md`](debug-console/README.md) for detailed usage instructions.

## License

MIT License - see [LICENSE](LICENSE) for full terms.
