# M5Stack 8Encoder Support

This document describes the optional M5Stack 8Encoder expansion unit support for aCYD-MIDI.

## Overview

The M5Stack 8Encoder unit provides 8 rotary encoders with push buttons that can be mapped to MIDI CC parameters or internal settings. This expansion is completely optional and disabled by default to maintain compatibility with standard CYD builds.

## Hardware

### M5Stack 8Encoder Unit

- **Product**: M5Stack 8Encoder Unit
- **Interface**: I2C (default address: 0x41)
- **Features**:
  - 8 rotary encoders with integrated push buttons
  - I2C communication
  - Compact form factor
  - Compatible with M5Stack ecosystem

### Wiring

The 8Encoder unit connects to the CYD via I2C:

```
M5Stack 8Encoder    ESP32 CYD
-----------------   ---------
SDA                 GPIO 21
SCL                 GPIO 22
VCC (3.3V)          3.3V
GND                 GND
```

**Note**: Verify your specific CYD board's I2C pinout before connecting. Some variants may use different GPIO pins for I2C.

#### Wiring Diagram

```
     ┌─────────────────┐
     │  M5Stack        │
     │  8Encoder       │
     │                 │
     │  [E1] [E2] [E3] [E4]│
     │  [E5] [E6] [E7] [E8]│
     │                 │
     │ SDA SCL VCC GND │
     └──┬───┬───┬───┬──┘
        │   │   │   │
        │   │   │   └────────┐
        │   │   └────────┐   │
        │   └────────┐   │   │
        └────────┐   │   │   │
                 │   │   │   │
     ┌───────────┴───┴───┴───┴──┐
     │ ESP32-2432S028R (CYD)    │
     │                           │
     │ GPIO21 (SDA)              │
     │ GPIO22 (SCL)              │
     │ 3.3V                      │
     │ GND                       │
     └───────────────────────────┘
```

## Enabling M5Stack 8Encoder Support

### Build-Time Configuration

The 8Encoder support is controlled via a compile-time flag in `platformio.ini`:

1. Open `platformio.ini`
2. Find the `[common]` section's `build_flags`
3. Uncomment the line:
   ```ini
   -D ENABLE_M5_8ENCODER=1
   ```
4. Build and upload the firmware

### Example Configuration

```ini
[common]
build_flags = 
    -Os
    # ... other flags ...
    # M5Stack 8Encoder support
    -D ENABLE_M5_8ENCODER=1
```

## Usage

### Accessing Encoder Panel Mode

When enabled, the 8Encoder mode replaces SLINK in the main Audio menu (SLINK is also omitted from Video mode). The mode is accessible via:

1. Main Menu → **8ENC** tile
2. The mode shows 8 parameter controls in a 4×2 grid

### Interface Layout

```
┌─────────────────────────────────────┐
│ ENCODER          [MIDI CC 1-8 FINE] │
├─────────────────────────────────────┤
│                                     │
│  ┌──┐  ┌──┐  ┌──┐  ┌──┐            │
│  │1 │  │2 │  │3 │  │4 │            │
│  │CC1   │CC2│  │CC3│  │CC4│        │
│  │ 64│  │ 64│  │ 64│  │ 64│        │
│  └──┘  └──┘  └──┘  └──┘            │
│                                     │
│  ┌──┐  ┌──┐  ┌──┐  ┌──┐            │
│  │5 │  │6 │  │7 │  │8 │            │
│  │CC5│  │CC6│  │CC7│  │CC8│        │
│  │ 64│  │ 64│  │ 64│  │ 64│        │
│  └──┘  └──┘  └──┘  └──┘            │
│                                     │
│ [PAGE] [FINE] [SAVE] [RESET]       │
└─────────────────────────────────────┘
```

### Controls

#### On-Screen Buttons

- **PAGE**: Cycle through encoder pages (3 pages available)
- **FINE/COARSE**: Toggle between fine (step=1) and coarse (step=10) adjustment
- **SAVE**: Save current parameter values to persistent storage
- **RESET**: Reset all values to defaults

#### Hardware Encoder Controls

- **Encoder Rotation**: Adjust parameter value
  - Clockwise: Increase value
  - Counter-clockwise: Decrease value
  - Fine mode: Changes by 1 step
  - Coarse mode: Changes by 10 steps

- **Encoder 1 Button**: Cycle to next page
- **Encoder 2 Button**: Toggle Fine/Coarse mode
- **Encoders 3-8 Buttons**: Reserved for future use

### Encoder Pages

#### Page 1: MIDI CC 1-8
Default mappings for common MIDI controllers:

| Encoder | Label | MIDI CC | Range | Default |
|---------|-------|---------|-------|---------|
| 1       | CC 1  | 1       | 0-127 | 64      |
| 2       | CC 2  | 2       | 0-127 | 64      |
| 3       | CC 3  | 3       | 0-127 | 64      |
| 4       | CC 4  | 4       | 0-127 | 64      |
| 5       | CC 5  | 5       | 0-127 | 64      |
| 6       | CC 6  | 6       | 0-127 | 64      |
| 7       | CC 7  | 7       | 0-127 | 64      |
| 8       | CC 8  | 8       | 0-127 | 64      |

#### Page 2: MIDI CC 11-18
Extended MIDI CC mappings:

| Encoder | Label  | MIDI CC | Range | Default |
|---------|--------|---------|-------|---------|
| 1       | CC 11  | 11      | 0-127 | 64      |
| 2       | CC 12  | 12      | 0-127 | 64      |
| 3       | CC 13  | 13      | 0-127 | 64      |
| 4       | CC 14  | 14      | 0-127 | 64      |
| 5       | CC 15  | 15      | 0-127 | 64      |
| 6       | CC 16  | 16      | 0-127 | 64      |
| 7       | CC 17  | 17      | 0-127 | 64      |
| 8       | CC 18  | 18      | 0-127 | 64      |

#### Page 3: Custom (Synthesizer Controls)
Pre-configured for typical synthesizer parameters:

| Encoder | Label     | MIDI CC | Range | Default | Description           |
|---------|-----------|---------|-------|---------|-----------------------|
| 1       | Volume    | 7       | 0-127 | 100     | Channel Volume        |
| 2       | Pan       | 10      | 0-127 | 64      | Pan (0=L, 127=R)     |
| 3       | Cutoff    | 74      | 0-127 | 64      | Filter Cutoff         |
| 4       | Resonance | 71      | 0-127 | 64      | Filter Resonance      |
| 5       | Attack    | 73      | 0-127 | 0       | Envelope Attack       |
| 6       | Release   | 72      | 0-127 | 64      | Envelope Release      |
| 7       | Reverb    | 91      | 0-127 | 40      | Reverb Send Level     |
| 8       | Delay     | 92      | 0-127 | 0       | Delay Send Level      |

## Persistent Storage

Parameter values are automatically saved to ESP32 Preferences when you press the **SAVE** button. Values are restored when entering Encoder Panel mode.

**Storage key format**: `encoder8:p{page}e{encoder}v`

Example: Page 0, Encoder 3 value is stored as `encoder8:p0e3v`

## Example Mappings

### DAW Control (Ableton Live, FL Studio, etc.)

Map encoders to common DAW parameters:

- **Encoder 1-4**: Track volumes (CC 7 on channels 1-4)
- **Encoder 5-8**: Track pan (CC 10 on channels 1-4)

### Synthesizer Control

Use Page 3 (Custom) for typical synth controls:

- **Encoder 1**: Master volume
- **Encoder 2**: Pan
- **Encoder 3-4**: Filter cutoff and resonance
- **Encoder 5-6**: Envelope attack and release
- **Encoder 7-8**: Effect sends

### Creating Custom Mappings

While the current implementation uses fixed page definitions, future versions may support user-configurable mappings via:

1. JSON configuration files on SD card
2. Web-based configuration interface
3. Touch-based parameter assignment

## Troubleshooting

### 8Encoder Not Detected

If the display shows "8ENCODER NOT CONNECTED":

1. **Check wiring**: Verify SDA/SCL connections
2. **Check power**: Ensure 3.3V and GND are properly connected
3. **Check I2C address**: Default is 0x41 (verify with your unit)
4. **Check for conflicts**: Ensure no other I2C devices use the same address
5. **Try re-seating connections**: Loose wires can cause intermittent issues

### Encoders Not Responding

1. **Check mode**: Ensure you're in Encoder Panel mode (8ENC)
2. **Reset encoders**: Press RESET button to reinitialize
3. **Check MIDI output**: Verify MIDI is being sent (check with DAW)
4. **Firmware issue**: Reflash firmware with 8Encoder support enabled

### Values Jumping

If encoder values jump erratically:

1. **Electrical noise**: Add pull-up resistors (4.7kΩ) on SDA/SCL lines
2. **Wire length**: Keep I2C wires short (<30cm recommended)
3. **Power supply**: Ensure stable 3.3V power
4. **Grounding**: Verify good ground connection

## Technical Details

### I2C Communication

- **Bus Speed**: 100 kHz (standard mode)
- **Address**: 0x41 (default)
- **Protocol**: Register-based read/write

### Register Map

| Register | Function            | Size   | Access |
|----------|---------------------|--------|--------|
| 0x00-0x07| Encoder values      | 8 bytes| R      |
| 0x10     | Button states       | 1 byte | R      |
| 0x20-0x27| Encoder reset       | 8 bytes| W      |

### Encoder Value Format

Encoder values are signed 8-bit integers (-128 to +127):
- Clockwise rotation: Positive values
- Counter-clockwise rotation: Negative values
- Values accumulate until read, then auto-reset

### Button State Format

Button states are packed into a single byte (bits 0-7 = encoders 1-8):
- Bit = 1: Button pressed
- Bit = 0: Button released

## Backward Compatibility

When `ENABLE_M5_8ENCODER` is **not** defined:

- Encoder mode is completely excluded from compilation
- No impact on code size or memory usage
- SLINK mode remains in the Audio menu
- All other modes function normally

This ensures the feature is truly optional and doesn't affect users without the hardware.

## Future Enhancements

Potential improvements for future versions:

1. **User-configurable mappings**: Edit labels, CC numbers, ranges via UI
2. **More parameter types**: Internal params (BPM, octave, scale, etc.)
3. **LED feedback**: Visual indication of encoder activity (if hardware supports)
4. **Preset system**: Save/load complete encoder page configurations
5. **Learn mode**: Assign parameters by rotating encoder while in learn mode
6. **Multi-channel support**: Route different encoders to different MIDI channels
7. **Velocity curves**: Non-linear response for certain parameters

## Related Issues

- [#25 - Add optional support for Baby8 sequencer module](https://github.com/julesdg6/aCYD-MIDI/issues/25)

## References

- [M5Stack 8Encoder Unit Product Page](https://shop.m5stack.com/products/8-encoder-unit)
- [M5Stack 8Encoder Unit Datasheet](https://static-cdn.m5stack.com/resource/docs/datasheet/unit/8encoder/8encoder_sch.pdf)
- [MIDI CC List](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2)

## License

This feature is part of aCYD-MIDI and follows the same MIT license as the main project.
