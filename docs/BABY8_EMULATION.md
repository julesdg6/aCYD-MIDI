# Baby8 Software Emulation

This document describes the Baby8 8-step sequencer software emulation feature for aCYD-MIDI.

## Overview

Baby8 is an onboard 8-step sequencer emulation that provides Baby8-style sequencing behavior without requiring external hardware. It features per-step note and gate controls, tempo and swing adjustment, pattern storage, and a virtual encoder interface for hands-on parameter control.

## Features

### Core Sequencer Features
- **8-step sequencer** - Focused, compact sequencing with 8 steps per pattern
- **Per-step controls**:
  - MIDI note value (0-127)
  - Velocity (0-127)
  - Gate on/off (enables/disables the step)
- **Pattern length** - Adjust active pattern length from 1-8 steps
- **Tempo control** - BPM range: 40-240 BPM
- **Swing** - 0-100% swing amount for groove
- **8 pattern slots** - Save and recall up to 8 different patterns
- **Pattern persistence** - Patterns are saved to flash memory

### Virtual Encoder Interface

Baby8 emulates 8 hardware encoders using a touchscreen interface:

| Encoder | Function | Range/Options |
|---------|----------|---------------|
| 1 | Step Select | Steps 1-8 |
| 2 | Note | MIDI notes 0-127 (displays note names) |
| 3 | Velocity | 0-127 |
| 4 | Gate | ON/OFF toggle |
| 5 | Pattern Length | 1-8 steps |
| 6 | BPM | 40-240 BPM |
| 7 | Swing | 0-100% |
| 8 | Pattern Select | Patterns 1-8 |

Each encoder displays:
- Label (e.g., "NOTE", "VEL", "BPM")
- Current value (numeric or note name)
- Visual bar indicator showing relative position

### UI Layout

The Baby8 mode screen consists of:

1. **Header** - Mode name, current BPM, swing percentage, and pattern name
2. **Step Grid** - 8 step buttons showing:
   - Step number
   - Note name (when gate is on)
   - Current step highlight (white when playing)
   - Selected step highlight (orange)
   - Gate status (green when on, gray when off, black when step disabled)
3. **Virtual Encoders** - 8 encoder controls in a row
4. **Transport Controls**:
   - PLAY/STOP - Start/stop sequencer playback
   - SAVE - Save current patterns to flash
   - LOAD - Reload patterns from flash
   - RESET - Reset current pattern to defaults

## Enabling Baby8 Support

### Build-Time Configuration

Baby8 support is controlled via a compile-time flag in `platformio.ini`:

1. Open `platformio.ini`
2. Find the `[common]` section's `build_flags`
3. Uncomment the line:
   ```ini
   -D ENABLE_BABY8_EMU=1
   ```
4. Build and upload the firmware

### Example Configuration

```ini
[common]
build_flags = 
    -Os
    # ... other flags ...
    # Baby8 software emulation
    -D ENABLE_BABY8_EMU=1
```

When enabled, Baby8 replaces SLINK in the main menu (unless M5Stack 8Encoder is also enabled).

## Usage

### Accessing Baby8 Mode

When enabled, Baby8 mode is accessible via:

1. Main Menu → **BABY8** tile
2. The mode shows an 8-step sequencer grid, virtual encoders, and transport controls

### Basic Operation

1. **Select a step** - Tap a step in the grid or use Encoder 1 (STEP)
2. **Edit step parameters**:
   - Tap Encoder 2 (NOTE) to cycle through note values
   - Tap Encoder 3 (VEL) to adjust velocity in increments of 10
   - Tap Encoder 4 (GATE) to toggle gate on/off
3. **Adjust sequencer settings**:
   - Encoder 5 (LEN) - Set pattern length
   - Encoder 6 (BPM) - Adjust tempo in 5 BPM increments
   - Encoder 7 (SWING) - Add swing in 10% increments
   - Encoder 8 (PTRN) - Switch between 8 pattern slots
4. **Start playback** - Tap PLAY button
5. **Save your work** - Tap SAVE to store patterns in flash memory

### Pattern Management

- **8 pattern slots** - Each pattern is independent with its own steps and length
- **Pattern naming** - Patterns are named "PAT 1" through "PAT 8" by default
- **Auto-save** - Patterns are NOT auto-saved; use the SAVE button to persist changes
- **Load from flash** - Use LOAD to reload saved patterns from flash memory
- **Reset pattern** - Use RESET to restore current pattern to default values

### MIDI Output

- Baby8 sends MIDI note on/off messages on the configured MIDI channel (channel 1 by default)
- Notes are sent to both BLE MIDI and Hardware MIDI (if enabled)
- Gate time is 50% of the step duration for consistent note articulation
- Swing is applied to odd-numbered steps (steps 2, 4, 6, 8)

### Swing Implementation

- **50% swing** (default) = no swing, even timing
- **0% swing** = heavily early odd steps
- **100% swing** = heavily delayed odd steps
- Swing affects the timing of odd steps relative to even steps
- Useful for creating shuffle, triplet, or laid-back grooves

## Technical Details

### Memory Usage

- **Pattern storage**: ~2KB for 8 patterns (8 steps × 3 bytes per step × 8 patterns + metadata)
- **Flash storage**: Patterns saved using ESP32 Preferences API in "baby8" namespace
- **Code size**: ~18KB when compiled with `-Os` optimization

### Timing

- Step duration calculated as: `(60000ms / BPM) / 2` for 16th notes
- Swing modifies odd step timing by multiplying duration by swing factor (0.0 to 2.0)
- Gate time is 50% of step duration
- Notes are played using the global MIDI clock manager for synchronization

### Conditional Compilation

- All Baby8 code is wrapped in `#ifdef ENABLE_BABY8_EMU` / `#endif`
- Disabled by default (no impact on standard builds)
- Zero code size overhead when disabled
- When enabled, replaces SLINK in the menu (unless 8Encoder is also enabled)

## Comparison with Other Modes

| Feature | Baby8 | BEATS (Sequencer) | TB3PO |
|---------|-------|-------------------|-------|
| Steps | 8 | 16 | Generated |
| Tracks | 1 | 4 | 1 |
| Per-step velocity | Yes | No | Accent |
| Swing | Yes | No | No |
| Pattern storage | 8 patterns | No | No |
| Virtual encoders | Yes | No | No |
| Gate control | Per-step | Per-step | Density |

## Tips and Tricks

1. **Quick pattern creation**: Use the default ascending scale as a starting point and modify from there
2. **Rhythmic variations**: Use gate on/off to create rhythmic patterns without changing notes
3. **Velocity dynamics**: Vary velocity to create accents and dynamics
4. **Pattern chaining**: Use pattern select to switch patterns during playback for longer sequences
5. **Swing groove**: Try 60-70% swing for a subtle shuffle feel
6. **Short patterns**: Use pattern length < 8 for repeating motifs

## Troubleshooting

### Baby8 doesn't appear in menu
- Verify `ENABLE_BABY8_EMU=1` is uncommented in `platformio.ini`
- Rebuild and upload firmware
- Check that M5Stack 8Encoder is not also enabled (they compete for the same menu slot)

### Patterns don't save
- Tap the SAVE button to manually save patterns
- Verify flash storage is working (check serial output for errors)
- ESP32 Preferences API requires sufficient flash space

### Timing issues
- Baby8 uses internal clock; ensure shared BPM is set correctly
- Swing only affects odd steps (2, 4, 6, 8)
- Check that MIDI clock master is set to INTERNAL

### Notes don't play
- Verify gate is ON for the steps you want to hear
- Check pattern length includes the steps you're editing
- Ensure step is active (step number < pattern length)
- Verify MIDI connection (BLE or hardware)

## Related Issues

- [#25 - Add optional support for Baby8 sequencer module](https://github.com/julesdg6/aCYD-MIDI/issues/25)
- [#23 - External controllers / mapping](https://github.com/julesdg6/aCYD-MIDI/issues/23)

## Future Enhancements

Potential improvements for Baby8:

- **MIDI input control** - Map external MIDI CCs to virtual encoders
- **Pattern copy/paste** - Duplicate patterns across slots
- **Step copy** - Copy step parameters to other steps
- **Randomize** - Generate random patterns with constraints
- **Scale quantization** - Constrain notes to a musical scale
- **Note transpose** - Transpose entire pattern up/down
- **MIDI CC output** - Add CC sequencing alongside notes
- **Sub-step resolution** - Higher resolution timing (32nd notes)
- **Pattern naming** - Custom user-defined pattern names
- **Undo/redo** - Step-by-step edit history

## Implementation Files

- `include/module_baby8_mode.h` - Baby8 mode interface and structures
- `src/module_baby8_mode.cpp` - Baby8 mode implementation
- `include/app/app_menu_icons.h` - Menu icon enum (Baby8 icon)
- `src/app/app_menu_icons.cpp` - Baby8 icon drawing implementation
- `include/common_definitions.h` - AppMode enum (BABY8 mode)
- `src/app/app_modes.cpp` - Mode registration and callbacks
- `src/app/app_menu.cpp` - Menu tile configuration
- `platformio.ini` - Build configuration flag

## License

MIT License - see [LICENSE](LICENSE) for full terms.
