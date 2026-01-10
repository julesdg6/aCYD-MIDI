# CYD MIDI Controller

Touchscreen Bluetooth MIDI controller for the ESP32-2432S028R "Cheap Yellow Display" (CYD).

Special thanks to Brian Lough for putting together the resources on this board. Check out his repo for more examples: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

## Features

### 10 Interactive Modes

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

### Core Features

- **Bluetooth MIDI** - Wireless connection to DAWs and music software
- **Hardware MIDI** - DIN-5 connector output via UART (optional)
- **Dual MIDI Output** - Send to both BLE and hardware simultaneously
- **Touchscreen Interface** - Intuitive visual controls optimized for the CYD display
- **Real-time Control** - Low-latency MIDI output
- **Visual Feedback** - Responsive graphics
- **Screenshot Capture** - Save screenshots to SD card as BMP files (from main menu)
- **Remote Display** - View the display over WiFi in your web browser (see [REMOTE_DISPLAY.md](REMOTE_DISPLAY.md))

## What You Need

- **ESP32-2432S028R (CYD)** - ~$15 from AliExpress/Amazon
- PlatformIO or Arduino IDE with ESP32 support
- *Optional:* Hardware MIDI circuit components (see [HARDWARE_MIDI.md](HARDWARE_MIDI.md))
- **MicroSD Card** (optional) - For screenshot capture feature
- Arduino IDE with ESP32 support or PlatformIO

## Installation

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
- **[Hardware MIDI Setup Guide](HARDWARE_MIDI.md)**
- Supports simultaneous BLE + Hardware MIDI
- Two UART options: UART0 (serial breakout) or UART2 (expansion GPIOs)
- Circuit components ~$5-10
## Usage

### Taking Screenshots
1. Insert a MicroSD card into the CYD's SD card slot
2. From the main menu, tap the "SCREENSHOT" button
3. Screenshots are saved as BMP files (screen000.bmp, screen001.bmp, etc.) to the SD card
4. View the serial monitor for confirmation messages
### 6. Optional: Enable Remote Display
To view the CYD display in your web browser:
1. Copy `include/wifi_config.h.example` to `include/wifi_config.h`
2. Edit the file and enter your WiFi credentials
3. Rebuild and upload
4. Check Serial Monitor for the IP address
5. Open the IP address in your browser

For full details, see [REMOTE_DISPLAY.md](REMOTE_DISPLAY.md)

## Troubleshooting

- **Upload Speed**: Lower it to `115200` if the sketch isn't uploading
- **Blank screen**: Check TFT_eSPI pin configuration
- **No touch**: Verify touchscreen library installation
- **No Bluetooth**: Restart device and re-pair
- **UI too small/large**: Check display resolution detection in serial output
- **Screenshot fails**: Ensure SD card is properly inserted and formatted (FAT32)
- **Remote Display not working**: Ensure WiFi credentials are correct and your network is 2.4GHz (ESP32 doesn't support 5GHz)

## License

Open source - see MIT license file for details.
