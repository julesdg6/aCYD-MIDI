# Dimensions Mode - UI Reference

## Screen Layout (320x240 reference)

```
┌─────────────────────────────────────────────────────────────────────┐
│ ← DIMENSIONS                    Parametric Sequencer           [≡] │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  f(x) #1        [-]  [+]                                            │
│                                      ┌──────────────────┐           │
│                                      │ Output:          │           │
│  Parameters:                         │                  │           │
│                                      │ px: 64           │           │
│  A: 10.0      [-]  [+]               │ py: 72           │           │
│                                      │ pz: 95           │           │
│  B: 10.0      [-]  [+]               │                  │           │
│                                      │ Time:            │           │
│  C:  1.0      [-]  [+]               │ t: 50            │           │
│                                      │                  │           │
│  D:  1.0      [-]  [+]               │ ● PLAY           │           │
│                                      │ N: 12            │           │
│                                      └──────────────────┘           │
│                                                                      │
│                                                                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  [  START  ]    [  RST  ]    [  SET  ]    [  MENU  ]               │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Control Reference

### Equation Selector (Top)
- **Display**: "f(x) #1" through "f(x) #20"
- **[-] Button**: Previous equation (wraps 1→20)
- **[+] Button**: Next equation (wraps 20→1)
- **Action**: Immediately changes active equation

### Parameter Controls (Left Side)
For each parameter (A, B, C, D):
- **Display**: "A: 10.0" (current value with 1 decimal)
- **[-] Button**: Decrease by 1.0
- **[+] Button**: Increase by 1.0
- **Range**: 0.0 - 100.0
- **Effect**: Updates in real-time during playback

### Output Panel (Right Side)
Read-only values showing equation outputs:
- **px**: 0-127, maps to interval/timing
- **py**: 0-127, maps to MIDI note
- **pz**: 0-127, maps to velocity
- **t**: Current time parameter value
- **Status**: ● PLAY (green) or ○ STOP (gray)
- **N**: Note subset size (how many notes enabled)

### Transport Controls (Bottom)
- **[START]**: 
  - When stopped: Request start (waits for bar)
  - Shows green when playing
- **[STOP]**:
  - When playing: Stop playback + release notes
  - Shows red when playing
- **[RST]**: Reset time parameter to start
- **[SET]**: Settings (future: note subset, intervals)
- **[MENU]**: Return to main menu

## Color Coding

Based on aCYD-MIDI theme:
- **Primary** (Cyan): Equation number display
- **Success** (Green): START button (when playing), status
- **Error** (Red): STOP indication (when playing)
- **Warning** (Yellow): RST button
- **Surface** (Dark Gray): Parameter buttons, SET button
- **Secondary** (Orange): MENU button
- **Accent** (Bright Cyan): Output values
- **Text** (White): Labels and values
- **Text Dim** (Gray): Secondary labels

## Touch Zones

All buttons sized for finger-friendly interaction:
- **Small Buttons**: 25×15px (parameter +/-)
- **Medium Buttons**: 25×18px (equation +/-)
- **Large Buttons**: 50-60×25px (transport)

Hit areas include padding for easier activation.

## Interactive Behavior

### During Playback
- Parameter changes take effect immediately
- Equation changes apply to next step
- Output panel updates in real-time
- Status indicator shows "PLAY"

### When Stopped
- All controls active
- Can change equation and parameters
- Output shows last values
- Status indicator shows "STOP"

### Start Pending
- Waiting for bar boundary
- Status shows "START"
- Can cancel by pressing STOP

## Responsive Design

Uses aCYD-MIDI scaling macros:
- `SCALE_X()` for horizontal positioning
- `SCALE_Y()` for vertical positioning
- `SCALE_W()` for button widths
- `SCALE_H()` for button heights

Adapts automatically to different display sizes:
- 320×240 (ESP32-2432S028R)
- 480×320 (ESP32-4832S035)
- 800×480 (Elecrow 7")

## Typical User Flow

1. **Enter Mode**: Select "DIMS" from experimental menu
2. **Choose Equation**: Use +/- to browse (try #1 first)
3. **Adjust Parameters**: Fine-tune A/B to taste
4. **Start Playing**: Press START button
5. **Live Tweaking**: Adjust params while playing
6. **Experiment**: Try different equations
7. **Exit**: Press MENU when done

## Pro Tips

- **Start Simple**: Equation #1 with A=10, B=10 is musical
- **Watch Output**: px/py/pz show what the equation is doing
- **Parameter Ranges**: Most equations sound good with 1-20
- **Equation Browse**: Each equation has unique character
- **Reset Often**: RST button helps find the "start" again
- **Note Count**: More notes = more melodic variety

