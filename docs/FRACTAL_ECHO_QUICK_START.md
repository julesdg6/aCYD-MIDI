# Fractal Note Echo - Quick Start Guide

## What is Fractal Note Echo?

Fractal Note Echo is a MIDI effect that generates complex, evolving echo patterns from single notes. Inspired by Zack Steinkamp's [Fractal Note Echo](https://maxforlive.com/library/device/8173/fractal-note-echo) Max for Live device, it creates fractal-like delay structures where each echo iteration can have different timing, pitch, velocity, and duration characteristics.

## How to Access

1. Open **Settings** (gear icon from main menu)
2. Find "Main Menu Mode" setting
3. Toggle from "Original" to **"Experimental"**
4. Return to main menu
5. Select **"ECHO"** tile (bottom-right corner)

## Quick Start

### Try the Defaults
1. Enter the ECHO mode
2. Press **"TEST NOTE"** button (top-left)
3. Listen to the fractal echo pattern on C4

### Understand the Pages

The mode has 3 pages of parameters:

**Page 1 - TIMING** (the foundation)
- **Tap 1-4**: Base delay times for each echo tap
- **Iterations**: How many levels of echoes to generate
- **Stretch**: Time multiplier for each iteration level

**Page 2 - DYNAMICS** (the character)
- **Vel Decay**: How much quieter each iteration gets
- **Min Vel**: Stop generating echoes below this velocity
- **Base Len**: Duration of echo notes
- **Len Decay**: How much shorter each iteration's notes get
- **Max Echoes**: Safety limit to prevent overflow

**Page 3 - OFFSETS** (the harmony)
- **Iter 1-N**: Semitone offset for each iteration
  - 0 = unison
  - +7 = perfect fifth
  - +12 = octave
  - -5 = fourth below

### Basic Presets

**Simple Delay (rhythmic)**
- Page 1: Tap1=120ms, Tap2=240ms, others=0, Iter=2, Stretch=1.0
- Page 2: VelDecay=0.7, MinVel=20
- Page 3: All offsets=0

**Harmonic Shimmer**
- Page 1: Tap1=100ms, Tap2=200ms, Tap3=0, Tap4=0, Iter=3, Stretch=1.5
- Page 2: VelDecay=0.8, MinVel=15
- Page 3: Iter1=0, Iter2=+7, Iter3=+12

**Chaotic Cloud**
- Page 1: All taps active (100/150/200/250), Iter=4, Stretch=1.3
- Page 2: VelDecay=0.85, MinVel=10
- Page 3: Iter1=+7, Iter2=-5, Iter3=+12, Iter4=+3

## Tips & Tricks

### Creating Rhythmic Patterns
- Set **Stretch=1.0** for consistent timing
- Use multiples for tap times (e.g., 100, 200, 300, 400)
- Keep **Iterations** low (2-3)

### Creating Harmonic Patterns
- Use **Offsets** based on musical intervals:
  - +7 = fifth
  - +12 = octave
  - +5 = fourth
  - +3 = minor third
  - +4 = major third

### Creating Complex Textures
- Higher **Iterations** (4-6)
- **Stretch** > 1.0 for expanding delays
- **Stretch** < 1.0 for accelerating delays
- Mix tap times for irregular rhythms

### Controlling Density
- **Max Echoes**: Lower for sparse, higher for dense
- **Min Velocity**: Higher to cut off echoes sooner
- **Velocity Decay**: Higher (closer to 1.0) for sustained echoes

## Understanding the Algorithm

Each note triggers a tree of echoes:

```
Original Note (C4, vel=100)
├─ Iteration 1 (offset=0, vel=80)
│  ├─ Tap1: 100ms
│  ├─ Tap2: 200ms
│  ├─ Tap3: 300ms
│  └─ Tap4: 400ms
│
├─ Iteration 2 (offset=+7, vel=64, time×1.5)
│  ├─ Tap1: 150ms (G4)
│  ├─ Tap2: 300ms (G4)
│  ├─ Tap3: 450ms (G4)
│  └─ Tap4: 600ms (G4)
│
└─ Iteration 3 (offset=+12, vel=51, time×2.25)
   ├─ Tap1: 225ms (C5)
   ├─ Tap2: 450ms (C5)
   ├─ Tap3: 675ms (C5)
   └─ Tap4: 900ms (C5)
```

## Current Limitations

- **No persistence**: Parameters reset when you leave the mode
- **Single test note**: Only C4 available via TEST NOTE button
- **Not passthrough**: Currently doesn't process external MIDI input
- **No scale quantization**: Offsets are chromatic, not scale-aware

## Future Enhancements (Potential)

Ideas for future development:
- Save/load presets
- Process external MIDI input
- Full keyboard for testing
- Tempo sync to BPM
- Scale quantization
- Dry/wet mix
- Randomization per tap
- Visual feedback of echo pattern

## Troubleshooting

**No sound when pressing TEST NOTE:**
- Check BLE MIDI connection to your DAW/device
- Verify the mode shows "ACTIVE" (not "DISABLED")
- Try toggling ON/OFF button

**Too many echoes / overwhelming:**
- Reduce **Iterations**
- Reduce **Max Echoes**
- Increase **Min Velocity**
- Increase **Velocity Decay**

**Not enough echoes:**
- Increase **Iterations**
- Decrease **Min Velocity**
- Decrease **Velocity Decay** (closer to 1.0)

**Echoes too fast:**
- Increase tap times
- Increase **Stretch** value

**Echoes too slow:**
- Decrease tap times
- Decrease **Stretch** value (< 1.0)

## Technical Details

- **Event Queue**: 128 events max
- **Processing**: Non-blocking, runs every frame
- **Timing Resolution**: ~16ms (60 FPS)
- **MIDI Output**: Both BLE and Hardware MIDI (if configured)

## Attribution

Original concept by Zack Steinkamp
- Max for Live device: https://maxforlive.com/library/device/8173/fractal-note-echo
- Source code: https://github.com/zsteinkamp/m4l-FractalNoteEcho
- License: GPL-3.0

Implementation for aCYD-MIDI by GitHub Copilot
- License: MIT (compatible)

## Additional Documentation

- **FRACTAL_ECHO_IMPLEMENTATION.md**: Technical implementation details
- **docs/FRACTAL_ECHO_UI_MOCKUP.md**: UI layouts and interaction flow

---

*Experiment, explore, and create complex musical textures with Fractal Note Echo!*
