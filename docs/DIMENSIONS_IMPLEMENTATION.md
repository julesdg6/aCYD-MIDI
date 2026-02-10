# Dimensions Parametric Sequencer - Implementation Summary

## Overview

Successfully ported the **Dimensions** parametric sequencer from Erik Oostveen's hardware implementation to aCYD-MIDI. The port maintains the mathematical core while adapting the interface and integration to work seamlessly with aCYD-MIDI's touchscreen UI, clock system, and MIDI pipeline.

## What Was Implemented

### Core Engine (✅ Complete)

**File**: `src/module_dimensions_mode.cpp` (15KB)

1. **20 Parametric Equations** - All equations ported exactly from upstream:
   - Lissajous curves (#1)
   - Asymmetric waves (#2-4)
   - Random blends (#5, 12, 14, 18)
   - Pure chaos (#6)
   - Exponential/logarithmic patterns (#7, 10)
   - Interference patterns (#8, 9)
   - Complex harmonics (#11, 19, 20)
   - Modulo patterns (#13)
   - Tangent curves (#15)
   - Multi-oscillator (#16)
   - Phase inversion (#17)

2. **Parametric Evaluation System**:
   - Function: `dimensionsEvaluateEquation()`
   - Inputs: equation#, t, a, b, c, d, rnd, x, y
   - Outputs: px, py, pz (0-127 each)
   - Boundary guarding and division-by-zero protection
   - Proper constrain() usage for output range

3. **MIDI Note Generation**:
   - Note subset system (default: C3-B3, 12 notes)
   - Linear mapping: py → note index
   - Transpose support (octaveTranspose parameter)
   - Velocity modes: fixed (127) or from equation (pz)
   - Note-on/note-off tracking to prevent stuck notes

4. **Interval/Timing System**:
   - 10 interval divisions (1/32 to whole notes)
   - Dynamic mapping: px → clock division
   - Step-by-step clock consumption via `consumeReadySteps()`
   - Smooth integration with aCYD's `SequencerSyncState`

5. **Transport Control**:
   - Start/stop with bar-boundary quantization
   - Reset to t_min
   - Play state tracking
   - Automatic note-off on stop

### UI Implementation (✅ Complete)

**Interface Layout**:

```
┌─────────────────────────────────────────────────┐
│ DIMENSIONS         Parametric Sequencer      [≡]│
├─────────────────────────────────────────────────┤
│ f(x) #5   [-] [+]                               │
│                                                  │
│ Parameters:            Output:                  │
│ A: 10.0  [-] [+]       px: 64                   │
│ B: 10.0  [-] [+]       py: 72                   │
│ C:  1.0  [-] [+]       pz: 95                   │
│ D:  1.0  [-] [+]                                │
│                        Time:                     │
│                        t: 50                     │
│                                                  │
│                        PLAY                      │
│                        N: 12                     │
├─────────────────────────────────────────────────┤
│ [START]  [RST]  [SET]  [MENU]                   │
└─────────────────────────────────────────────────┘
```

**Interactive Controls**:
- Equation selector: +/- buttons (cycles 1-20)
- Parameter controls: +/- for A, B, C, D (0-100 range)
- Transport buttons: START/STOP, RESET, SETTINGS (placeholder), MENU
- Real-time value display: px, py, pz, t, play status, note count

### Integration (✅ Complete)

1. **Mode Registration**:
   - Added `DIMENSIONS` to `AppMode` enum
   - Registered in `kModeTable` with init/draw/handle functions
   - Mode count updated correctly

2. **Menu System**:
   - Added to experimental menu (replacing FRACTAL_ECHO)
   - Custom menu icon: Lissajous curve (2:3 frequency ratio)
   - Label: "DIMS"

3. **Clock Integration**:
   - Uses `SequencerSyncState` for transport
   - Respects `clockManagerGetTickCount()` for timing
   - Proper `consumeReadySteps()` usage
   - Bar-start quantization on playback start

### Documentation (✅ Complete)

1. **User Guide** (`docs/DIMENSIONS.md`, 7.8KB):
   - How parametric equations work
   - Complete equation reference (all 20)
   - Musical application examples
   - Technical details (clock, note subset, intervals)
   - Tips and tricks
   - Comparison with original hardware
   - Future enhancement roadmap

2. **README Updates**:
   - Added Dimensions to mode list
   - Brief description with upstream credit

3. **CHANGELOG**:
   - Detailed feature list
   - Implementation highlights
   - Documentation references

## Key Design Decisions

### 1. Clock System
- **Decision**: Use `SequencerSyncState` pattern (same as Euclid mode)
- **Rationale**: Proven, maintainable, handles bar-start quantization
- **Result**: Clean integration with minimal custom timing code

### 2. UI Paradigm
- **Decision**: Touch buttons for parameters instead of rotary encoders
- **Rationale**: CYD is a touchscreen device, not a hardware panel
- **Result**: Simple +/- controls, easy to understand and use

### 3. Note Subset
- **Decision**: Fixed C3-B3 default, future UI for customization
- **Rationale**: Get core working first, defer complex UI
- **Result**: Playable immediately, room for enhancement

### 4. Equation Porting
- **Decision**: Exact 1:1 port of mathematical formulas
- **Rationale**: Preserve musical characteristics of original
- **Result**: Authentic Dimensions sound and behavior

### 5. Menu Placement
- **Decision**: Experimental menu, replace FRACTAL_ECHO
- **Rationale**: Dimensions is more comprehensive generative mode
- **Result**: Better use of limited menu slots

## Code Quality

### Structure
- ✅ Follows aCYD-MIDI naming conventions
- ✅ Uses common theme colors and scaling macros
- ✅ Proper header guards and includes
- ✅ Consistent with other mode implementations

### Safety
- ✅ Division-by-zero protection in equations
- ✅ Boundary checking on px/py/pz outputs
- ✅ Note-off tracking to prevent stuck notes
- ✅ Touch bounds validation

### Performance
- ✅ Step processing only when clock advances
- ✅ No blocking operations in handle loop
- ✅ Efficient equation evaluation (direct math)
- ✅ Minimal redraw requests

## Testing Status

### Compile Check
- ✅ Basic syntax validation passed
- ⏳ Full PlatformIO build pending (network issue)
- ⏳ Hardware testing pending

### Expected Behavior
Based on code analysis:
1. **Startup**: Initializes with equation #1, A=10, B=10, C=1, D=1
2. **Playback**: Waits for bar start, then evaluates equation at each step
3. **Note Generation**: Maps py to C3-B3, sends MIDI note-on
4. **Timing**: Uses px to vary step interval (1/16 to whole notes)
5. **Transport**: Clean start/stop with note-off on stop

### Known Limitations
- No visual trace mode yet (future enhancement)
- Note subset fixed to C3-B3 (UI needed for customization)
- Settings button placeholder (future feature)
- No preset system yet (planned)

## File Manifest

**New Files**:
- `include/module_dimensions_mode.h` (2.6KB) - Mode interface
- `src/module_dimensions_mode.cpp` (15KB) - Core implementation
- `docs/DIMENSIONS.md` (7.8KB) - User documentation

**Modified Files**:
- `include/common_definitions.h` - Added DIMENSIONS to AppMode enum
- `include/app/app_menu_icons.h` - Added Dimensions menu icon enum
- `src/app/app_modes.cpp` - Registered mode in mode table
- `src/app/app_menu.cpp` - Added to experimental menu
- `src/app/app_menu_icons.cpp` - Implemented Lissajous icon drawing
- `README.md` - Added mode description
- `CHANGELOG.md` - Documented new feature

**Total Code Added**: ~600 lines (including documentation)

## Upstream Attribution

**Original Work**: Dimensions parametric sequencer by Erik Oostveen  
**Source**: https://github.com/ErikOostveen/Dimensions  
**Version Ported**: Dimensions_December_18_2021  
**License Compliance**: Check upstream license before release

All parametric equation formulas are direct ports from the original `ParametricFunctions.ino` file with minimal adaptation for C++ syntax.

## Future Work

### Phase 1: Visual Enhancements
- Trace mode (draw equation path on screen)
- Real-time px/py/pz graph
- Equation preview visualization

### Phase 2: Advanced UI
- Note subset editor with piano roll
- Interval array customization
- Parameter sliders for fine control
- Touch-drag parameter adjustment

### Phase 3: Preset System
- Save/load presets
- Built-in example presets (5 factory)
- Preset browsing UI
- Auto-save last settings

### Phase 4: Extensions
- Random parameter mutation
- CC output mode (use px/py/pz as CC values)
- Multiple simultaneous equations (poly mode)
- Equation chaining/morphing

### Phase 5: Performance Features
- MIDI learn for parameter mapping
- External clock improvements
- Swing/humanize options
- Per-parameter LFO modulation

## Conclusion

The Dimensions parametric sequencer has been successfully ported to aCYD-MIDI with:
- ✅ Complete mathematical core (all 20 equations)
- ✅ Full clock integration
- ✅ Interactive touch UI
- ✅ Comprehensive documentation
- ✅ Clean code structure
- ⏳ Pending hardware testing

The implementation is **ready for testing** and provides a solid foundation for future enhancements.

