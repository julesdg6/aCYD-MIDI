# Fractal Note Echo Implementation Summary

## Overview

This document describes the implementation of the Fractal Note Echo MIDI effect for aCYD-MIDI, inspired by Zack Steinkamp's [m4l-FractalNoteEcho](https://github.com/zsteinkamp/m4l-FractalNoteEcho).

## Changes Made

### 1. Menu System Update: Original vs Experimental

**Changed:**
- `MENU_AUDIO` → `MENU_ORIGINAL`
- `MENU_VIDEO` → `MENU_EXPERIMENTAL`

**Affected Files:**
- `include/common_definitions.h` - MenuMode enum
- `src/app/app_state.cpp` - Default menu mode
- `src/module_settings_mode.cpp` - Settings UI labels and toggle
- `src/app/app_menu.cpp` - Menu tile arrays renamed

**Rationale:** Per user request, renamed to better reflect the nature of the two menu modes - Original contains the classic stable modes, while Experimental contains newer/experimental features like the Fractal Echo.

### 2. Fractal Note Echo Mode Implementation

**New Files:**
- `include/module_fractal_echo_mode.h` - Mode interface and data structures
- `src/module_fractal_echo_mode.cpp` - Mode implementation

**Features Implemented:**

#### Core Algorithm
- **Fractal Echo Generation**: Generates complex echo patterns based on configurable tap delays, iterations, and stretch factors
- **Event Scheduler**: Non-blocking queue-based system to schedule future MIDI Note On/Off events
- **Velocity Decay**: Each echo iteration reduces velocity by a configurable decay factor
- **Pitch Offsets**: Each iteration can transpose notes by configurable semitone offsets
- **Length Decay**: Note durations decrease with each iteration

#### Parameters (3 Pages)

**Page 1 - Timing:**
- Tap 1-4: Base delay times (0-2000ms)
- Iterations: Number of echo iterations (1-6)
- Stretch: Time scaling factor per iteration (0.25-2.0)

**Page 2 - Dynamics:**
- Velocity Decay: How much velocity decreases per iteration (0.0-1.0)
- Min Velocity: Cutoff threshold to stop generating echoes (1-127)
- Base Length: Duration of echo notes (10-2000ms)
- Length Decay: How much length decreases per iteration (0.0-1.0)
- Max Echoes: Safety limit per note (1-64)

**Page 3 - Offsets:**
- Iteration 1-6: Semitone offset per iteration (-24 to +24)

#### UI Features
- 3-page interface with navigation buttons
- Enable/Disable toggle
- TEST NOTE button to trigger C4 with current echo settings
- Real-time parameter adjustment with +/- buttons
- Visual feedback showing current page and enabled state

#### Technical Details

**Event Queue:**
- Fixed-size array of 128 MIDI events maximum
- Events contain: due time, MIDI status, data1, data2, active flag
- Processed every frame via `processMidiEvents()`
- Automatic compaction removes inactive events

**Fractal Generation Logic:**
```
For each iteration k (0 to iterations-1):
  stretch_factor = stretch^k
  velocity_scale = velocityDecay^k
  length_scale = lengthDecay^k
  
  For each tap i (0 to 3):
    delay = tap[i] * stretch_factor
    note = original_note + offset[k]
    velocity = original_velocity * velocity_scale
    length = base_length * length_scale
    
    Schedule Note On at: now + delay
    Schedule Note Off at: now + delay + length
```

### 3. Menu Integration

**Updated Files:**
- `include/common_definitions.h` - Added `FRACTAL_ECHO` to AppMode enum
- `src/app/app_modes.cpp` - Added mode to mode table and includes
- `src/app/app_menu.cpp` - Added to Experimental menu (replacing MORPH)
- `include/app/app_menu_icons.h` - Added FractalEcho icon enum
- `src/app/app_menu_icons.cpp` - Implemented fractal echo icon rendering

**Menu Placement:**
- Added to **Experimental menu** in position 16 (bottom-right)
- Replaced MORPH mode in experimental menu
- MORPH still available in Original menu
- Label: "ECHO"
- Icon: Branching dot pattern representing echo iterations

## Usage

1. **Access the Mode:**
   - Open Settings
   - Toggle to "Experimental" menu mode
   - Return to main menu
   - Select "ECHO" tile (bottom-right)

2. **Configure Parameters:**
   - Use < and > buttons to navigate between pages
   - Use +/- buttons to adjust each parameter
   - Toggle ON/OFF in top-right corner

3. **Test the Effect:**
   - Press "TEST NOTE" button to hear C4 with current echo settings
   - Adjust parameters and test again
   - When satisfied, use with external MIDI input or keyboard mode

## Implementation Notes

### Current Limitations

1. **No Persistence:** Parameters reset to defaults on mode switch/reboot
2. **Not a True MIDI Effect:** Currently operates as a standalone mode rather than a passthrough effect
3. **Single Test Note:** Only provides C4 test trigger (not a full keyboard)
4. **No Scale Quantization:** Echo offsets are not quantized to a musical scale

### Future Enhancements (Not in Scope)

Per the original issue, these were explicitly out of scope:
- Scale awareness / quantization
- Telemetry / visualizations
- Multiple preset banks
- Tempo sync
- Dry/Wet routing
- Randomization parameters

### Potential Integration Points

To make this a true MIDI effect processing incoming notes:
- Hook into BLE MIDI callback in `src/app/app_ble_midi.cpp`
- Process incoming Note On messages through `addFractalEcho()`
- Allow configuration while effect runs in background
- Add to other modes as an optional effect layer

## Code Quality

- Follows existing aCYD-MIDI patterns and conventions
- Uses scaling macros for display independence
- Non-blocking event processing
- Bounded memory usage (fixed-size queue)
- Clean separation of UI and effect logic

## Testing Recommendations

1. **Functional Testing:**
   - Test each parameter page navigation
   - Verify all parameter ranges and increment/decrement
   - Test TEST NOTE button with various configurations
   - Verify echo events are sent via MIDI monitor

2. **Edge Cases:**
   - Max iterations with max taps (should not overflow queue)
   - Very high stretch values
   - Very low velocity/length decay
   - Extreme pitch offsets (should clamp to 0-127)

3. **Performance:**
   - Monitor frame rate with many scheduled events
   - Test event queue compaction
   - Verify no memory leaks on repeated triggers

## Attribution

Based on [Fractal Note Echo](https://maxforlive.com/library/device/8173/fractal-note-echo) by Zack Steinkamp
Original project: https://github.com/zsteinkamp/m4l-FractalNoteEcho
License: GPL-3.0 (compatible with aCYD-MIDI's MIT license)

## Files Modified/Created

**Modified:**
- `include/common_definitions.h`
- `src/app/app_state.cpp`
- `src/module_settings_mode.cpp`
- `src/app/app_menu.cpp`
- `src/app/app_modes.cpp`
- `include/app/app_menu_icons.h`
- `src/app/app_menu_icons.cpp`

**Created:**
- `include/module_fractal_echo_mode.h`
- `src/module_fractal_echo_mode.cpp`
- `FRACTAL_ECHO_IMPLEMENTATION.md` (this file)
