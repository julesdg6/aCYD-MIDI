# CYD Web Debug Console - Visual Guide

## Interface Layout

The debug console uses a clean, dark-themed interface optimized for debugging:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ CYD Web Debug Console    [Connect BLE MIDI] [Connect Serial] [Settings]    │
│ Status: ● BLE: Disconnected  ● Serial: Disconnected                         │
├─────────────────────────── TOP PANEL (50%) ────────────────────────────────┤
│                                                                              │
│ ┌─ Keyboard ───────────────────────────────────────────────────────────┐   │
│ │ [C][C#][D][D#][E][F][F#][G][G#][A][A#][B]                             │   │
│ │                                                                        │   │
│ │ [Oct -] [Oct +]  Octave: 3   [Gate] [Accent] [Slide] [All Notes Off] │   │
│ └────────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│ ┌─ Controls (CC) ──────────────────────────────────────────────────────┐   │
│ │   ╭───╮  ╭───╮  ╭───╮  ╭───╮  ╭───╮  ╭───╮                           │   │
│ │   │ ◆ │  │ ◆ │  │ ◆ │  │ ◆ │  │ ◆ │  │ ◆ │                           │   │
│ │   ╰───╯  ╰───╯  ╰───╯  ╰───╯  ╰───╯  ╰───╯                           │   │
│ │  Cutoff   Res  EnvMod  Decay  Accent Volume                          │   │
│ │    64     64     64     64     64     64                              │   │
│ └────────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│ ┌─ Debug ──────────────────────────────────────────────────────────────┐   │
│ │ [Send Test Burst]                                                     │   │
│ └────────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
├─────────────────────────── BOTTOM PANEL (50%) ─────────────────────────────┤
│                                                                              │
│ [Search...] ☑MIDI In ☑MIDI Out ☑Serial ☑Auto-scroll ☑Δt                   │
│ [Pause] [Clear] [Export JSON] [Export CSV]                                 │
│ ┌─ Log Viewer ─────────────────────────────────────────────────────────┐   │
│ │ 18:42:01.123 +000ms  UI→MIDI  NOTE_ON  ch1 note=60 (C4) vel=100      │   │
│ │ 18:42:01.126 +003ms  CYD←BLE  NOTE_ON  ch1 note=60 (C4) vel=100      │   │
│ │ 18:42:01.130 +004ms  CYD→SER  "gate on, step=5, accent=1"            │   │
│ │ 18:42:01.280 +150ms  UI→MIDI  CC       ch1 cc=74 val=92 (cutoff)     │   │
│ │ 18:42:01.284 +004ms  CYD←BLE  CC       ch1 cc=74 val=92              │   │
│ │ 18:42:02.015 +731ms  CYD→SER  "filter cutoff updated: 92"            │   │
│ │ ...                                                                    │   │
│ │                                                                        │   │
│ │                                                                        │   │
│ └────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Color Coding

The interface uses consistent color coding for easy identification:

### Header
- **Background**: Dark gray (#2a2a2a)
- **Title**: Bright cyan (#00d4ff)
- **Status dots**: 
  - Disconnected: Gray (#666)
  - Connected: Green (#00ff88) with glow effect

### Top Panel (Controls)
- **Background**: Medium dark (#242424)
- **Section boxes**: Dark gray (#2a2a2a)
- **Keyboard keys**:
  - White keys: White background
  - Black keys: Near-black (#1a1a1a)
  - Active: Bright cyan (#00d4ff)
- **Knobs**: Dark background with cyan indicator
- **Buttons**: Gray with white text

### Bottom Panel (Logging)
- **Background**: Darkest (#1a1a1a)
- **Log entries color-coded by source**:
  - UI→MIDI: Cyan (#00d4ff)
  - CYD←BLE: Green (#00ff88)
  - CYD→SER: Orange (#ffaa00)
- **Timestamps**: Dimmed gray (#888)

## Interactive Elements

### Keyboard
```
White keys: Click to trigger notes
Black keys: Click for sharps/flats
Visual feedback: Key highlights in cyan when pressed
```

### Knobs
```
Click and drag vertically:
  - Up = Increase value (0-127)
  - Down = Decrease value
Visual indicator rotates -135° to +135°
Sends CC messages in real-time
```

### Toggles
```
[Gate]   - Click to toggle (gray ↔ cyan)
[Accent] - Click to toggle (gray ↔ cyan)
[Slide]  - Click to toggle (gray ↔ cyan)
```

### Log Controls
```
[Search box]     - Type to filter entries
☑ Checkboxes    - Toggle source visibility
[Pause/Resume]  - Freeze/unfreeze updates
[Clear]         - Empty log buffer
[Export]        - Download JSON or CSV
```

## Event Flow Visualization

```
┌─────────────┐
│ User Action │ (Click keyboard key)
└──────┬──────┘
       │
       ▼
┌──────────────────┐
│  Event Bus       │ Timestamp: t0 = performance.now()
│  Logs: UI→MIDI   │
└──────┬───────────┘
       │
       ├──────────────────────┐
       │                      │
       ▼                      ▼
┌──────────────┐      ┌──────────────┐
│  BLE MIDI    │      │  Logger      │
│  Send packet │      │  Add entry   │
└──────┬───────┘      └──────┬───────┘
       │                      │
       ▼                      ▼
┌──────────────┐      ┌──────────────┐
│  CYD Device  │      │  UI Render   │
│  Receives    │      │  (RAF batch) │
└──────┬───────┘      └──────────────┘
       │
       ▼
┌──────────────┐
│ BLE Notify   │ (Device echo or response)
│ Logs: CYD←BLE│
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Event Bus   │ Timestamp: t1 = performance.now()
│  Δt = t1-t0  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Logger      │
│  Add entry   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  UI Render   │
│  Show both   │
│  events      │
└──────────────┘
```

## Typical Usage Workflow

1. **Open the page**
   - Visit https://julesdg6.github.io/aCYD-MIDI/debug-console/
   - Chrome/Edge/Opera required

2. **Connect to CYD**
   ```
   Click [Connect BLE MIDI]
   → Browser shows device picker
   → Select "CYD MIDI"
   → Status: ● BLE: Connected (green)
   ```

3. **Optionally connect Serial**
   ```
   Click [Connect Serial]
   → Browser shows port picker
   → Select USB Serial port
   → Status: ● Serial: Connected (green)
   ```

4. **Test MIDI**
   ```
   Click keyboard key
   → Log shows: UI→MIDI NOTE_ON
   → Log shows: CYD←BLE NOTE_ON (echo)
   → DAW/synth plays sound
   ```

5. **Adjust parameters**
   ```
   Drag Cutoff knob
   → Log shows: UI→MIDI CC ch1 cc=74 val=92
   → Log shows: CYD←BLE CC (echo)
   → Sound changes
   ```

6. **Monitor firmware**
   ```
   Serial connection active
   → Log shows: CYD→SER "debug message"
   → Interleaved with MIDI events
   → All share same timeline
   ```

7. **Debug issues**
   ```
   Use filters to focus on problem
   Use search to find specific messages
   Pause to inspect timing
   Export to share with others
   ```

## Performance Indicators

### Normal Operation
```
Events/sec: ~10-50
CPU: Low (< 10%)
Memory: Stable (~50MB)
UI: Responsive
```

### High Traffic (Test Burst)
```
Events/sec: 500+
CPU: Moderate (~30%)
Memory: Stable (ring buffer)
UI: Responsive (RAF batching)
```

### Sustained Heavy Load
```
Events/sec: 200+
CPU: Moderate
Memory: Capped at 10k entries
UI: Smooth scrolling
Recommendation: Use filters or pause
```

## Keyboard Shortcuts

Currently not implemented. All interactions are mouse-based.

Future v2 may add:
- Space: Pause/Resume
- C: Clear log
- F: Focus search
- Arrow keys: Octave up/down
- 1-7: Play notes

## Mobile Support

Not optimized for mobile devices due to:
- Web Bluetooth limited support
- Web Serial not available
- Touch interaction not optimized
- Screen size constraints

Desktop Chrome/Edge/Opera required.

---

## Summary

The CYD Web Debug Console provides a **professional-grade debugging interface** with:
- Immediate visual feedback
- Precise timing information
- Comprehensive event logging
- Intuitive controls
- Export capabilities

Perfect for:
- Testing new firmware
- Debugging MIDI issues
- Demonstrating CYD capabilities
- Sharing debug sessions
- Learning MIDI protocol

**Access now**: https://julesdg6.github.io/aCYD-MIDI/debug-console/ (after deployment)
