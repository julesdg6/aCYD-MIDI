# Hardware MIDI DIN-5 Output Guide

This guide explains how to add hardware MIDI output to your aCYD MIDI controller using the standard DIN-5 connector.

## Table of Contents
- [Overview](#overview)
- [Hardware Options](#hardware-options)
- [Circuit Design](#circuit-design)
- [Build Configuration](#build-configuration)
- [Wiring Instructions](#wiring-instructions)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)

---

## Overview

The aCYD MIDI controller now supports **simultaneous BLE MIDI and Hardware MIDI output**. All MIDI messages are sent to both interfaces automatically.

**Key Features:**
- ✅ Dual output: BLE MIDI + Hardware MIDI (DIN-5)
- ✅ Standard MIDI 1.0 @ 31,250 baud
- ✅ Lower latency than BLE (~1ms vs 10-30ms)
- ✅ Professional studio integration
- ✅ Compatible with vintage MIDI gear
- ✅ Two UART options: UART0 (production) or UART2 (development)

---

## Hardware Options

### Option 1: UART0 (Serial Breakout) - **RECOMMENDED**

**Pin Assignment:**
- **MIDI OUT:** GPIO1 (TX0) - Physical serial breakout
- **MIDI IN:** GPIO3 (RX0) - Physical serial breakout
- **GND:** Ground pin on serial breakout

**Advantages:**
- ✅ Uses existing serial breakout header on CYD boards
- ✅ No additional GPIO wiring needed
- ✅ Direct access to pins on board edge
- ✅ Perfect for production MIDI devices

**Disadvantages:**
- ⚠️ **Conflicts with USB Serial debugging**
- ⚠️ Cannot use `Serial.print()` for debugging
- ⚠️ Must disconnect to re-upload firmware

**When to Use:**
- Final production build
- Standalone MIDI device
- No debugging needed

---

### Option 2: UART2 (Expansion GPIOs)

**Pin Assignment:**
- **MIDI OUT:** GPIO17 (TX2)
- **MIDI IN:** GPIO16 (RX2)
- **GND:** Ground pin

**Advantages:**
- ✅ Keeps USB Serial debugging available
- ✅ No conflict with programming interface
- ✅ Can debug MIDI messages via `Serial.print()`
- ✅ Perfect for development

**Disadvantages:**
- ⚠️ Requires access to internal GPIOs (may need soldering)
- ⚠️ No physical breakout on standard CYD boards

**When to Use:**
- Development and debugging
- Need to see MIDI debug messages
- Prototyping new features

---

## Circuit Design

### MIDI OUT Circuit

**Components Required:**
- 6N138 optocoupler (1x)
- 220Ω resistor (1x) - Current limiting
- 5V power source
- Female DIN-5 connector (1x)
- Jumper wires

**Wiring:**
```
ESP32 TX Pin (GPIO1 or GPIO17)
    ↓
   220Ω resistor
    ↓
6N138 Pin 2 (Anode)
6N138 Pin 3 (Cathode) → Ground
6N138 Pin 8 (VCC) → +5V
6N138 Pin 5 (Collector) → DIN-5 Pin 5
6N138 Pin 6 (Emitter) → DIN-5 Pin 4
DIN-5 Pin 2 → Ground (Shield)
```

**Schematic:**
```
         220Ω
TX ----/\/\/\---- Pin 2 (6N138)
                  Pin 3 (6N138) ---- GND
                  Pin 8 (6N138) ---- +5V
                  Pin 5 (6N138) ---- DIN-5 Pin 5 (MIDI OUT)
                  Pin 6 (6N138) ---- DIN-5 Pin 4 (MIDI OUT)
                                     DIN-5 Pin 2 ---- GND (Shield)
```

---

### MIDI IN Circuit (Optional)

**Components Required:**
- 6N138 optocoupler (1x)
- 220Ω resistor (1x)
- 4.7kΩ resistor (1x) - Pull-up
- Female DIN-5 connector (1x)
- Jumper wires

**Wiring:**
```
DIN-5 Pin 5 → 220Ω → 6N138 Pin 2 (Anode)
DIN-5 Pin 4 → 6N138 Pin 3 (Cathode)
6N138 Pin 8 (VCC) → +5V
6N138 Pin 6 (Emitter) → Ground
6N138 Pin 5 (Collector) → 4.7kΩ → +5V
6N138 Pin 5 (Collector) → ESP32 RX Pin (GPIO3 or GPIO16)
```

**Note:** MIDI IN is optional and not currently implemented in software.

---

### DIN-5 Connector Pinout

**Looking at the female connector (solder side):**

```
      3     1
        ___
       /   \
      |  5  |
       \___/
      4     2
```

**Pin Assignments:**
- **Pin 1:** Not connected
- **Pin 2:** Ground (shield)
- **Pin 3:** Not connected
- **Pin 4:** MIDI IN/OUT (signal)
- **Pin 5:** MIDI IN/OUT (signal)

---

## Build Configuration

### Default Configuration (UART2)

By default, the firmware is configured for **UART2** to enable debugging during development.

**No changes needed** - just build and upload normally.

---

### Production Configuration (UART0)

To use **UART0** (serial breakout pins), add build flags to `platformio.ini`:

#### Edit `platformio.ini`:

```ini
[env:esp32-2432S028Rv2]
platform = espressif32@6.10.0
framework = arduino
board = esp32-2432S028Rv2
board_build.partitions = huge_app.csv
monitor_speed = 115200

build_flags = 
    -Ofast
    -D CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_INFO
    -D LV_CONF_PATH=\"${platformio.include_dir}/lv_conf.h\"
    -D HARDWARE_MIDI_UART=0          # <-- Add this line for UART0
    -D HARDWARE_MIDI_ENABLED=true    # <-- Optional: enable/disable

lib_deps =
    BLE
```

---

### Configuration Options

| Flag | Values | Description |
|------|--------|-------------|
| `HARDWARE_MIDI_UART` | `0` or `2` | Select UART0 (GPIO1/3) or UART2 (GPIO16/17) |
| `HARDWARE_MIDI_ENABLED` | `true` or `false` | Enable/disable hardware MIDI output |

**Examples:**

```ini
# Development mode (UART2, debug enabled)
-D HARDWARE_MIDI_UART=2

# Production mode (UART0, no debug)
-D HARDWARE_MIDI_UART=0

# Disable hardware MIDI completely
-D HARDWARE_MIDI_ENABLED=false
```

---

## Wiring Instructions

### For UART0 (Serial Breakout)

1. **Locate the Serial Breakout Header** on your CYD board
   - Usually labeled: `GND`, `TX`, `RX`, `5V` or `3V3`
   - TX = GPIO1
   - RX = GPIO3

2. **Connect MIDI OUT Circuit:**
   - ESP32 TX (GPIO1) → 220Ω resistor → 6N138 Pin 2
   - GND → 6N138 Pin 3
   - 5V → 6N138 Pin 8
   - 6N138 Pin 5 → DIN-5 Pin 5
   - 6N138 Pin 6 → DIN-5 Pin 4
   - GND → DIN-5 Pin 2

3. **Build and Upload:**
   - Add `-D HARDWARE_MIDI_UART=0` to `platformio.ini`
   - Build and upload firmware
   - **Disconnect USB after upload** (UART0 conflicts with USB serial)

4. **Test:**
   - Power via USB or external 5V
   - MIDI messages will output to DIN-5 connector

---

### For UART2 (Expansion GPIOs)

1. **Locate GPIO16 and GPIO17:**
   - These pins are usually on the expansion header
   - May require soldering header pins
   - Check your CYD board schematic

2. **Connect MIDI OUT Circuit:**
   - ESP32 TX2 (GPIO17) → 220Ω resistor → 6N138 Pin 2
   - GND → 6N138 Pin 3
   - 5V → 6N138 Pin 8
   - 6N138 Pin 5 → DIN-5 Pin 5
   - 6N138 Pin 6 → DIN-5 Pin 4
   - GND → DIN-5 Pin 2

3. **Build and Upload:**
   - Default configuration uses UART2
   - Build and upload normally
   - USB debugging remains available

4. **Test:**
   - Open Serial Monitor (115200 baud)
   - Debug messages will appear
   - MIDI outputs to both BLE and DIN-5

---

## Testing

### Software Test

1. **Upload the firmware** with your chosen UART configuration

2. **Check Serial Monitor** (only works with UART2):
   ```
   aCYD MIDI Controller Starting...
   Hardware MIDI: Enabled (UART2)
   Setup complete!
   ```

3. **Play notes** using the touchscreen keyboard

4. **Verify debug output** (UART2 only):
   ```
   Key R0:5: E4 ON
   Key R0:5: E4 OFF
   ```

---

### Hardware Test

**Equipment Needed:**
- MIDI-compatible synthesizer, sound module, or USB MIDI interface
- MIDI cable (DIN-5)

**Test Procedure:**

1. **Connect MIDI Cable:**
   - CYD MIDI OUT → Synthesizer MIDI IN

2. **Power On Both Devices:**
   - CYD should boot to menu screen
   - Synth should be ready to receive MIDI

3. **Select KEYS Mode:**
   - Tap "KEYS" button on CYD

4. **Play Notes:**
   - Touch keys on screen
   - Listen for sound from synthesizer

5. **Verify Latency:**
   - Should be instant (< 1ms delay)
   - No BLE pairing needed

---

### Loopback Test (Advanced)

Connect MIDI OUT to MIDI IN on same CYD:

1. Build circuit with both MIDI OUT and MIDI IN
2. Connect DIN-5 OUT → DIN-5 IN with MIDI cable
3. Enable MIDI IN parsing in future firmware updates
4. Verify echo of MIDI messages

---

## Troubleshooting

### No MIDI Output

**Problem:** Synthesizer receives no MIDI data

**Solutions:**
- Check 6N138 wiring and orientation
- Verify 5V power to optocoupler Pin 8
- Check 220Ω resistor value
- Test MIDI cable with known-working device
- Verify DIN-5 connector pins 4 and 5

---

### Cannot Upload Firmware (UART0)

**Problem:** Upload fails with UART0 configuration

**Solutions:**
- Disconnect MIDI circuit from GPIO1 during upload
- Add compile flag temporarily: `-D HARDWARE_MIDI_UART=2`
- Upload, then reconnect and rebuild with UART0

---

### No Debug Messages (UART0)

**Problem:** Serial Monitor shows nothing

**This is expected behavior** - UART0 is used for MIDI, not debugging.

**Solutions:**
- Switch to UART2 configuration for development
- Use `-D HARDWARE_MIDI_UART=2` in build flags
- Rebuild and upload

---

### BLE MIDI Still Works But Hardware Doesn't

**Problem:** Bluetooth works, hardware MIDI silent

**Solutions:**
- Verify `HARDWARE_MIDI_ENABLED=true` in build flags
- Check 6N138 power (Pin 8 = 5V)
- Test TX pin with logic analyzer or oscilloscope
- Expected output: 31,250 baud UART signal

---

### Wrong UART Pins

**Problem:** No output from expected GPIO pins

**Solutions:**
- UART0: Check GPIO1 (TX) and GPIO3 (RX)
- UART2: Check GPIO17 (TX) and GPIO16 (RX)
- Verify pin mapping in hardware_midi.h
- Check board variant in platformio.ini

---

## Bill of Materials (BOM)

| Component | Quantity | Description | Price (USD) |
|-----------|----------|-------------|-------------|
| 6N138 Optocoupler | 1-2 | DIP-8 package, high-speed | $0.50 - $1.00 |
| 220Ω Resistor | 1-2 | 1/4W, 5% tolerance | $0.05 |
| 4.7kΩ Resistor | 1 | 1/4W, 5% tolerance (MIDI IN only) | $0.05 |
| Female DIN-5 Connector | 1-2 | 180° or 90° PCB mount | $0.50 - $1.00 |
| Breadboard (optional) | 1 | For prototyping | $2.00 - $5.00 |
| Jumper Wires | 10+ | Male-to-male, male-to-female | $3.00 |
| **Total (MIDI OUT only)** | - | - | **$5 - $10** |

**Where to Buy:**
- Mouser Electronics
- Digi-Key
- SparkFun
- Adafruit
- AliExpress (cheaper, slower shipping)

---

## Compatible Hardware

### Off-the-Shelf MIDI Modules

| Product | UART | Notes |
|---------|------|-------|
| M5Stack MIDI Unit | UART2 | Requires GPIO16/17 |
| SparkFun MIDI Shield | UART0 | Uses TX/RX breakout |
| DIY Breadboard Circuit | Any | Most flexible option |

---

## Advanced Configuration

### Custom GPIO Pins

To use different GPIO pins, edit `include/hardware_midi.h`:

```cpp
#if HARDWARE_MIDI_UART == 2
  #define MIDI_RX_PIN 16  // Change to your RX pin
  #define MIDI_TX_PIN 17  // Change to your TX pin
#endif
```

**Note:** Ensure selected pins don't conflict with display, touch, or other peripherals.

---

### MIDI Channel Filtering (Future)

Future updates may include:
- Per-mode MIDI channel assignment
- MIDI THRU support
- MIDI IN parsing and forwarding
- Settings menu for hardware MIDI options

---

## References

- [MIDI 1.0 Electrical Specification](https://www.midi.org/specifications)
- [ESP32 UART Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html)
- [6N138 Optocoupler Datasheet](https://www.onsemi.com/pdf/datasheet/6n138-d.pdf)
- [CYD GPIO Pinout](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)

---

## Support

**Issues or Questions?**
- Open an issue on GitHub
- Check existing issues for solutions
- Join the community discussions

**Pull Requests Welcome!**
- MIDI IN support
- Settings menu integration
- PCB design files
- Photo documentation

---

## License

This documentation and code are part of the aCYD MIDI project.
See LICENSE file for details.
