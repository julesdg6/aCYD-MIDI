# Dimensions Parametric Sequencer

## Overview

The **Dimensions** mode is a parametric sequencer that uses mathematical equations to generate MIDI patterns. Unlike traditional step sequencers, Dimensions creates evolving, non-obvious melodic and rhythmic motion through parametric functions that map to MIDI note, velocity, and timing values.

This mode is a port of Erik Oostveen's [Dimensions](https://github.com/ErikOostveen/Dimensions) hardware sequencer, adapted to work with aCYD-MIDI's touchscreen interface, clock system, and MIDI I/O pipeline.

## How It Works

### Parametric Equation System

At its core, Dimensions evaluates parametric equations at each clock step to generate three output values:

- **px** (0-127): Maps to **interval/timing** - determines how frequently notes trigger
- **py** (0-127): Maps to **pitch** - selects which MIDI note to play from your note subset
- **pz** (0-127): Maps to **velocity** - controls note dynamics

### The Math Behind the Music

Each equation is a function of:
- **t** - Time parameter that advances with each step
- **a, b, c, d** - User-controllable parameters (0-100)
- **rnd** - Random element for introducing variation
- **x, y** - Axis offsets for shifting the pattern

Example equation (f(x) #1 - Lissajous):
```
px = x + t * sin(a * t + π/2)
py = y + t * sin(b * t)
pz = t * cos(a * t / π * c)
```

This creates a Lissajous curve pattern where:
- Changing **a** affects the horizontal frequency
- Changing **b** affects the vertical frequency
- Changing **c** modulates the velocity curve

## Interface

### Main Screen Layout

**Top Section:**
- **Equation Selector**: Current equation number (1-20) with +/- buttons
- **Parameter Controls**: A, B, C, D with increment/decrement buttons

**Right Panel:**
- **Output Values**: Current px, py, pz from equation
- **Time**: Current t value
- **Transport Status**: PLAY/STOP/START
- **Note Count**: Number of enabled notes in subset

**Bottom Bar:**
- **START/STOP**: Toggle playback
- **RST**: Reset time parameter to start
- **SET**: Settings (note subset, intervals - future)
- **MENU**: Return to main menu

### Controls

**Equation Selection:**
- **-** button: Previous equation (cycles 20→1)
- **+** button: Next equation (cycles 1→20)

**Parameter Adjustment:**
- **-** buttons: Decrease parameter by 1
- **+** buttons: Increase parameter by 1
- Range: 0-100 for each parameter

**Transport:**
- **START**: Begin playback (waits for bar boundary)
- **STOP**: Stop playback and silence all notes
- **RESET**: Return time parameter to start

## The 20 Parametric Equations

Each equation produces unique musical characteristics:

1. **Lissajous** - Classic parametric curve, smooth periodic motion
2. **Asymmetric Wave** - Uneven patterns with harmonic relationships
3. **Modulated Spiral** - Evolving circular patterns with phase shifts
4. **Cosine Phase** - Gentle curves with predictable repetition
5. **Random Blend** - Combines sine waves with random elements
6. **Pure Chaos** - Fully random output (ignores parameters)
7. **Exponential Decay** - Patterns that compress over time
8. **Interference** - Wave interference patterns
9. **Orbital** - Circular motion with dual oscillators
10. **Logarithmic** - Non-linear scaling patterns
11. **Micro-variation** - Subtle parameter-driven changes
12. **Velocity Random** - Fixed pitch with random dynamics
13. **Modulo Pattern** - Integer division creates stepped patterns
14. **Interval Random** - Random timing with smooth pitch
15. **Tangent Curves** - Sharp angular transitions
16. **Triple Oscillator** - Three-wave interference
17. **Phase Inversion** - Mirrored waveforms
18. **Controlled Chaos** - Random with parameter limits
19. **Complex Harmonic** - Multi-parameter interactions
20. **Cubic Modulation** - Exponential parameter relationships

## Musical Applications

### Generative Ambient
- Use equations 1, 4, 7 with slow time advance
- Low A/B values (1-5) for gentle evolution
- Enable many notes in subset for melodic variety

### Rhythmic Patterns
- Use equations 6, 14, 18 for percussive timing
- High A/B values (20-50) for rapid changes
- Small note subset (1-4 notes) for focused patterns

### Evolving Sequences
- Use equations 3, 5, 11 with medium time advance
- Gradually adjust parameters during playback
- Medium note subset (8-12 notes) for musical phrases

### Experimental
- Use equations 13, 15, 16 for unpredictable results
- Extreme parameter values (0, 100)
- Large note subsets with wide pitch ranges

## Technical Details

### Clock Integration

Dimensions integrates with aCYD-MIDI's clock manager:
- **Internal Clock**: Uses shared BPM (40-240)
- **External MIDI Clock**: Locks to incoming 24 ppqn clock
- **Step Resolution**: Configurable via interval mapping (1/32 to whole notes)
- **Transport Sync**: Waits for bar boundary on start

### Note Subset System

By default, Dimensions enables notes C3-B3 (MIDI 60-71). The **py** output value maps linearly across your enabled notes:
- py=0 → Lowest enabled note
- py=127 → Highest enabled note

Future updates will add UI for customizing the note subset.

### Interval Divisions

The **px** output maps to 10 interval divisions:
- 0-12: 1/16 notes (fastest)
- 13-25: 1/12 notes
- 26-38: 1/8 notes
- 39-51: 1/6 notes
- 52-64: 1/4 notes
- 65-76: 1/3 notes
- 77-89: 1/2 notes
- 90-102: 3/4 notes
- 103-115: Whole notes
- 116-127: Custom (1/4 by default)

This allows the equation to dynamically vary the rhythmic density.

### Time Parameter

The **t** parameter advances by `value_gran` (default 0.0125) on each step:
- Default range: 0-100
- Can be reversed by setting t_min > t_max
- Loops back to start when reaching end of range
- Reset button returns to t_min

## Tips & Tricks

1. **Start Simple**: Begin with equation 1 (Lissajous) and adjust A/B to understand the parameter effect
2. **Live Performance**: Map MIDI controllers to parameters for real-time control (future feature)
3. **Layer Multiple Instances**: Run multiple CYD devices with different equations for polyphonic textures
4. **Record and Resample**: Capture MIDI output in your DAW and use as source material
5. **Combine with Effects**: Add reverb, delay, or modulation to enhance the generated patterns
6. **Find Sweet Spots**: Each equation has certain parameter ranges that produce musical results - experiment!

## Differences from Original Hardware

The aCYD-MIDI port makes these changes from the original Dimensions:

**Kept:**
- All 20 parametric equations (exact mathematical formulas)
- Parameter system (a, b, c, d, rnd)
- Time advancement system
- Note subset concept
- Interval/timing mapping

**Changed:**
- **UI**: Touch-based controls instead of rotary encoders
- **Display**: Adapted to CYD screen sizes (320x240+)
- **Clock**: Uses aCYD's unified clock manager
- **MIDI Output**: Both BLE and hardware MIDI
- **Presets**: Future integration with aCYD preset system

**Not Yet Ported:**
- Visual trace mode (equation path drawing)
- Note subset editing UI
- Interval array customization
- Snapshot/preset system
- Original LED animations

## Future Enhancements

Planned features for upcoming releases:

- [ ] Visual trace mode showing equation output path
- [ ] Touch-based parameter sliders for fine control
- [ ] Note subset editor with piano roll UI
- [ ] Preset save/load system
- [ ] Random parameter mutation
- [ ] MIDI clock slave mode improvements
- [ ] CC output option (use px/py/pz as CC values)
- [ ] Velocity mode toggle (fixed vs. from equation)

## References

- **Original Dimensions**: https://github.com/ErikOostveen/Dimensions
- **Upstream Source**: Dimensions_December_18_2021 folder
- **Creator**: Erik Oostveen (www.erikoostveen.co.uk)

## Credits

Dimensions parametric sequencer concept, equations, and design by **Erik Oostveen**.  
Port to aCYD-MIDI by the aCYD-MIDI contributors.

