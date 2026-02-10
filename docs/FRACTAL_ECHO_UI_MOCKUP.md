# Fractal Echo UI Mockup

## Screen Layout (320x240)

```
┌──────────────────────────────────────────────────────────┐
│ ←  FRACTAL ECHO                    ACTIVE         ⚙     │ Header (45px)
│     Fractal echo MIDI effect                             │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  [TEST NOTE]                              [ON/OFF]       │ Test & Toggle
│                                                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │                    PAGE CONTENT                     │  │
│  │                                                     │  │
│  │  === PAGE 1: TIMING ===                            │  │
│  │                                                     │  │
│  │  Tap 1          [ - ]   100 ms   [ + ]             │  │
│  │                                                     │  │
│  │  Tap 2          [ - ]   200 ms   [ + ]             │  │
│  │                                                     │  │
│  │  Tap 3          [ - ]   300 ms   [ + ]             │  │
│  │                                                     │  │
│  │  Tap 4          [ - ]   400 ms   [ + ]             │  │
│  │                                                     │  │
│  │  Iterations     [ - ]     3      [ + ]             │  │
│  │                                                     │  │
│  │  Stretch        [ - ]    1.50    [ + ]             │  │
│  │                                                     │  │
│  └────────────────────────────────────────────────────┘  │
│                                                           │
│  Page 1/3               [ < ]    [ > ]                   │ Page Nav
└──────────────────────────────────────────────────────────┘
```

## Page 2: Dynamics

```
┌──────────────────────────────────────────────────────────┐
│ ←  FRACTAL ECHO                    ACTIVE         ⚙     │
│     Fractal echo MIDI effect                             │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  [TEST NOTE]                               [ON]          │
│                                                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │                    PAGE CONTENT                     │  │
│  │                                                     │  │
│  │  === PAGE 2: DYNAMICS ===                          │  │
│  │                                                     │  │
│  │  Vel Decay      [ - ]    0.80    [ + ]             │  │
│  │                                                     │  │
│  │  Min Vel        [ - ]     10     [ + ]             │  │
│  │                                                     │  │
│  │  Base Len       [ - ]   200 ms   [ + ]             │  │
│  │                                                     │  │
│  │  Len Decay      [ - ]    0.90    [ + ]             │  │
│  │                                                     │  │
│  │  Max Echoes     [ - ]     32     [ + ]             │  │
│  │                                                     │  │
│  └────────────────────────────────────────────────────┘  │
│                                                           │
│  Page 2/3               [ < ]    [ > ]                   │
└──────────────────────────────────────────────────────────┘
```

## Page 3: Offsets

```
┌──────────────────────────────────────────────────────────┐
│ ←  FRACTAL ECHO                    ACTIVE         ⚙     │
│     Fractal echo MIDI effect                             │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  [TEST NOTE]                               [ON]          │
│                                                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │                    PAGE CONTENT                     │  │
│  │                                                     │  │
│  │  === PAGE 3: OFFSETS (semitones) ===               │  │
│  │                                                     │  │
│  │  Iter 1         [ - ]     +0     [ + ]             │  │
│  │                                                     │  │
│  │  Iter 2         [ - ]     +7     [ + ]             │  │
│  │                                                     │  │
│  │  Iter 3         [ - ]    +12     [ + ]             │  │
│  │                                                     │  │
│  │  (More rows shown based on Iterations setting)     │  │
│  │                                                     │  │
│  └────────────────────────────────────────────────────┘  │
│                                                           │
│  Page 3/3               [ < ]    [ > ]                   │
└──────────────────────────────────────────────────────────┘
```

## Color Scheme

- **Header**: Dark background (THEME_BG) with white text
- **ACTIVE/DISABLED**: Green (THEME_SUCCESS) or Red (THEME_ERROR)
- **Test Button**: Cyan accent (THEME_ACCENT)
- **Toggle Button**: Green when ON, Red when OFF
- **+/- Buttons**: Yellow for - (THEME_WARNING), Green for + (THEME_SUCCESS)
- **Navigation Buttons**: Cyan (THEME_PRIMARY)
- **Parameter Labels**: Dim white (THEME_TEXT_DIM)
- **Parameter Values**: White (THEME_TEXT)
- **Background**: Black (THEME_BG)
- **Content Panel**: Dark gray surface (THEME_SURFACE)

## Interaction Flow

1. **Enter Mode**: From main menu → Settings → Toggle to "Experimental" → Main Menu → Select "ECHO"
2. **Navigate Pages**: Use < and > buttons at bottom to cycle through 3 pages
3. **Adjust Parameters**: Tap +/- buttons next to each parameter
4. **Test Effect**: Tap "TEST NOTE" to hear C4 with current settings
5. **Enable/Disable**: Tap ON/OFF toggle (top-right) to enable/disable effect
6. **Exit**: Tap back arrow (←) to return to menu

## Menu Icon

The Fractal Echo icon appears in the Experimental menu (position 16, bottom-right):

```
    Original Menu (MENU_ORIGINAL)           Experimental Menu (MENU_EXPERIMENTAL)
┌───────────────────────────────┐       ┌───────────────────────────────┐
│ KEYS  BEATS   ZEN    DROP     │       │ WAAAVE KEYS   BEATS   ZEN     │
│ RNG   XY PAD  ARP    GRID     │       │ DROP   RNG    XY PAD  ARP     │
│ CHORD LFO     TB3PO  GRIDS    │       │ GRID   CHORD  LFO     TB3PO   │
│ RAGA  EUCLID  MORPH  SLINK    │       │ GRIDS  RAGA   EUCLID [ECHO]   │ ← New!
└───────────────────────────────┘       └───────────────────────────────┘
```

## Default Parameter Values

### Timing (Page 1)
- Tap 1: 100ms
- Tap 2: 200ms
- Tap 3: 300ms
- Tap 4: 400ms
- Iterations: 3
- Stretch: 1.5

### Dynamics (Page 2)
- Velocity Decay: 0.8
- Min Velocity: 10
- Base Length: 200ms
- Length Decay: 0.9
- Max Echoes: 32

### Offsets (Page 3)
- Iteration 1: 0 semitones
- Iteration 2: +7 semitones (perfect fifth)
- Iteration 3: +12 semitones (octave)
- Iteration 4: -5 semitones
- Iteration 5: +5 semitones
- Iteration 6: 0 semitones

## Example: Default Settings Behavior

When TEST NOTE is pressed with default settings:

**Original Note:**
- Time: 0ms
- Note: C4 (MIDI 60)
- Velocity: 100
- Duration: 300ms

**Iteration 1 Echoes:** (offset +0, velocity ×0.8, length ×0.9)
- Tap1: Time=100ms, Note=C4, Vel=80, Duration=180ms
- Tap2: Time=200ms, Note=C4, Vel=80, Duration=180ms
- Tap3: Time=300ms, Note=C4, Vel=80, Duration=180ms
- Tap4: Time=400ms, Note=C4, Vel=80, Duration=180ms

**Iteration 2 Echoes:** (offset +7, velocity ×0.64, length ×0.81, time ×1.5)
- Tap1: Time=150ms, Note=G4, Vel=64, Duration=162ms
- Tap2: Time=300ms, Note=G4, Vel=64, Duration=162ms
- Tap3: Time=450ms, Note=G4, Vel=64, Duration=162ms
- Tap4: Time=600ms, Note=G4, Vel=64, Duration=162ms

**Iteration 3 Echoes:** (offset +12, velocity ×0.51, length ×0.73, time ×2.25)
- Tap1: Time=225ms, Note=C5, Vel=51, Duration=146ms
- Tap2: Time=450ms, Note=C5, Vel=51, Duration=146ms
- Tap3: Time=675ms, Note=C5, Vel=51, Duration=146ms
- Tap4: Time=900ms, Note=C5, Vel=51, Duration=146ms

Total: 1 original + 12 echoes = 13 notes
