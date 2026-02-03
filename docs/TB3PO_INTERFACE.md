# TB3PO Interface Documentation

## Overview

The TB3PO mode is a 16-step sequencer inspired by the Roland TB-303 bass synthesizer. This document describes the interface layout and visual design.

## Interface Layout

The interface features a 4-row pattern display, similar to the classic TB-303, with each row showing different aspects of the sequence:

```
┌──────────────────────────────────────────────────────────────┐
│ TB-3PO                                          [Status Info] │
├──────────────────────────────────────────────────────────────┤
│                                                                │
│ Note   │ C │ C │ C │ C │   │ C │ G │F# │ C │A# │D# │ C │A# │ C │C# │ C │
│ Up/Dn  │ + │ - │ - │ - │ + │ - │   │   │ + │ - │ + │ + │ - │   │   │ + │
│ A/S    │ A │   │   │   │   │   │   │ A │   │   │   │   │   │   │A/S│   │
│ Gate   │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │ ● │
│                                                                │
│        [PLAY/STOP] [REGEN] [DENS+] [DENS-]                    │
│        Density: 7                                              │
└──────────────────────────────────────────────────────────────┘
```

## Row Details

### Row 1: Note
- **Purpose**: Shows the musical note for each step
- **Display**: Note name without octave number (e.g., C, C#, D, etc.)
- **Color Coding**:
  - **Cyan background**: Current playing step
  - **Dark gray background**: Inactive steps
  - **White text** on dark background / **Black text** on cyan background

### Row 2: Up/Down (Transposition)
- **Purpose**: Shows pitch transposition for each step
- **Display**: 
  - `+` for octave up
  - `-` for octave down
  - Empty if no transposition
- **Color Coding**:
  - **Green background** (THEME_SUCCESS): Octave up (+)
  - **Yellow background** (THEME_WARNING): Octave down (-)
  - **Dark gray background**: No transposition
  - **Black text** on colored backgrounds

### Row 3: Accent/Slide
- **Purpose**: Shows accent and slide indicators
- **Display**:
  - `A` for accent only
  - `S` for slide only
  - `A/S` for both accent and slide
  - Empty if neither
- **Color Coding**:
  - **Yellow background** (THEME_WARNING): Accent
  - **Cyan background** (THEME_PRIMARY): Slide
  - **Dark gray background**: Neither
  - **Black text** on colored backgrounds

### Row 4: Gate (Timing)
- **Purpose**: Shows which steps are active (gated)
- **Display**: Filled circles for active steps
- **Color Coding**:
  - **Cyan filled circles** (THEME_PRIMARY): Gated/active steps
  - **Empty cells**: Rests/ungated steps

## Controls

### Buttons
1. **PLAY/STOP** (Cyan) - Start/stop sequence playback
2. **REGEN** (Orange) - Regenerate the pattern with current settings
3. **DENS+** (Bright Cyan) - Increase pattern density (more notes)
4. **DENS-** (Yellow) - Decrease pattern density (fewer notes)

### Display Information
- **Top Left**: Playback status (PLAYING / WAITING / STOPPED)
- **Top Right**: Seed mode (SEED LOCKED / SEED AUTO)
- **Bottom**: Current density value (0-14)

## Color Palette

The interface uses a consistent color theme:
- **THEME_BG** (Black): Main background
- **THEME_SURFACE** (Dark Gray): Inactive cells
- **THEME_PRIMARY** (Cyan): Gates, slides, buttons
- **THEME_SECONDARY** (Orange): Secondary buttons
- **THEME_ACCENT** (Bright Cyan): Current step highlight
- **THEME_SUCCESS** (Green): Octave up
- **THEME_WARNING** (Yellow): Octave down, accents
- **THEME_TEXT** (White): Primary text
- **THEME_TEXT_DIM** (Gray): Secondary text, borders

## Changes from Previous Version

### What Changed
1. **Layout**: Changed from single-row step display to 4-row TB-303 style layout
2. **Information Density**: Now shows note names, transpositions, accents, slides, and gates all at once
3. **Default Octave**: Lowered from octave 4 to octave 3 for more appropriate bass sequences
4. **Visual Clarity**: Added row labels and improved color coding

### Benefits
- **Better readability**: All pattern information visible at a glance
- **Familiar design**: Matches classic TB-303 pattern display
- **More colorful**: Better visual distinction between different pattern elements
- **Improved usability**: Easier to understand pattern structure while editing

## Technical Details

### Implementation
- File: `src/module_tb3po_mode.cpp`
- Drawing function: `drawTB3POMode()`
- Step width: 18 pixels (scaled)
- Row height: 15 pixels (scaled)
- Total pattern height: 4 rows × 15 pixels = 60 pixels
- Number of steps: 16

### Scaling
The interface uses the project's scaling system to adapt to different display sizes:
- `SCALE_X()` for horizontal dimensions
- `SCALE_Y()` for vertical dimensions
- Reference resolution: 320×240 (ESP32-2432S028R)

## Future Enhancements

Potential improvements for future versions:
- Interactive step editing via touch
- Visual feedback for slides between notes
- Step-by-step parameter editing
- Pattern save/load functionality
- Accent velocity adjustment
