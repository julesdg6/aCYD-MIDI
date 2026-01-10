# Hardware MIDI Configuration Examples

This file contains example build configurations for different hardware MIDI setups.

## Default Configuration (UART2 - Development Mode)

**File:** `platformio.ini`

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

lib_deps =
    BLE
```

**Features:**
- Hardware MIDI on GPIO17 (TX2) and GPIO16 (RX2)
- USB Serial debugging enabled
- Can view MIDI debug messages
- Perfect for development

---

## Production Configuration (UART0 - Serial Breakout)

**File:** `platformio.ini`

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
    -D HARDWARE_MIDI_UART=0

lib_deps =
    BLE
```

**Features:**
- Hardware MIDI on GPIO1 (TX0) and GPIO3 (RX0)
- Uses physical serial breakout pins
- USB Serial debugging disabled
- Lower latency, production-ready

---

## Disabled Hardware MIDI (BLE Only)

**File:** `platformio.ini`

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
    -D HARDWARE_MIDI_ENABLED=false

lib_deps =
    BLE
```

**Features:**
- BLE MIDI only
- No hardware MIDI output
- USB Serial debugging enabled
- Minimal hardware requirements

---

## Pin Mapping Reference

| UART | TX Pin | RX Pin | Debug Output | Notes |
|------|--------|--------|--------------|-------|
| UART0 | GPIO1 | GPIO3 | ❌ Disabled | Serial breakout pins |
| UART2 | GPIO17 | GPIO16 | ✅ Enabled | Expansion GPIOs |

---

## Build Commands

### Using PlatformIO CLI

```bash
# Build with default settings (UART2)
platformio run -e esp32-2432S028Rv2

# Upload firmware
platformio run -e esp32-2432S028Rv2 --target upload

# Monitor serial output (UART2 only)
platformio device monitor -b 115200
```

### Using Arduino IDE

1. Edit `platformio.ini` with desired configuration
2. Import project: **File → Open Folder**
3. Select environment: **esp32-2432S028Rv2**
4. Build: **PlatformIO: Build**
5. Upload: **PlatformIO: Upload**

---

## Testing Your Configuration

### Verify UART2 (Development)

1. Upload firmware with default config
2. Open Serial Monitor at 115200 baud
3. You should see:
   ```
   aCYD MIDI Controller Starting...
   Hardware MIDI: Enabled (UART2)
   Setup complete!
   ```
4. Play notes and see debug output

### Verify UART0 (Production)

1. Add `-D HARDWARE_MIDI_UART=0` to build_flags
2. Upload firmware
3. Serial Monitor will be silent (expected)
4. Test MIDI output with hardware synthesizer

---

## Troubleshooting Build Issues

### "Multiple definition" errors

**Problem:** Linker errors about multiple definitions

**Solution:** Ensure `hardware_midi.h` is only included via `midi_utils.h`

---

### UART pins not working

**Problem:** No MIDI output from expected pins

**Solution:**
1. Check `include/hardware_midi.h` for correct pin assignments
2. Verify GPIO pins aren't used by display/touch
3. Check board variant matches your CYD model

---

### Debug output not appearing

**Problem:** Serial Monitor shows nothing

**Solution:**
- If using UART0: This is expected (UART0 is MIDI, not debug)
- If using UART2: Check USB cable and COM port
- Verify monitor_speed = 115200

---

## Advanced: Custom Pin Configuration

To use different GPIO pins, edit `include/hardware_midi.h`:

```cpp
#elif HARDWARE_MIDI_UART == 2
  #define MIDI_RX_PIN 25  // Your custom RX pin
  #define MIDI_TX_PIN 26  // Your custom TX pin
```

**Warning:** Ensure pins don't conflict with:
- SPI (display)
- I2C (touch)
- Other peripherals

---

## Configuration Flags Reference

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `HARDWARE_MIDI_UART` | `0` or `2` | `2` | Select UART0 or UART2 |
| `HARDWARE_MIDI_ENABLED` | `true`/`false` | `true` | Enable/disable hardware MIDI |
| `DEBUG_ENABLED` | Auto-set | Based on UART | Enable debug output |

---

## See Also

- [HARDWARE_MIDI.md](HARDWARE_MIDI.md) - Complete wiring and circuit guide
- [README.md](README.md) - Main project documentation
- [platformio.ini](platformio.ini) - Build configuration file
