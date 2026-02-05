# UI Update Sub-Issues

This document breaks down the UI updates into individual sub-issues that can be tracked and implemented independently. Each sub-issue corresponds to one module/screen.

> **Note:** See `UI_UPDATE_ANALYSIS.md` for detailed recommendations for each module.

---

## Sub-Issue #1: Main Menu UI Update
**Screenshot:** ![Screenshot 1](https://github.com/user-attachments/assets/e28c46cf-c3d5-484a-940f-584092b5d501)  
**File:** `src/main.cpp`  
**Priority:** HIGH

**Key Updates Needed:**
- [ ] Standardize tile sizes and spacing in 4x4 grid
- [ ] Improve icon design for better differentiation
- [ ] Apply consistent theme colors to tiles
- [ ] Add subtle shadows/borders for depth
- [ ] Implement touch feedback
- [ ] Ensure label readability

---

## Sub-Issue #2: Keyboard Mode UI Update
**Screenshot:** ![Screenshot 2](https://github.com/user-attachments/assets/7e4941bc-f68d-4327-b782-1883b57b26f0)  
**File:** `include/module_keyboard_mode.h`  
**Priority:** HIGH

**Key Updates Needed:**
- [ ] Optimize key sizing for touch (minimum SCALE_X(30))
- [ ] Add clear pressed state for keys
- [ ] Improve black key styling and positioning
- [ ] Organize control panel (octave, settings)
- [ ] Add optional note labels
- [ ] Consider scale highlighting feature

---

## Sub-Issue #3: Sequencer Mode (BEATS) UI Update
**Screenshot:** ![Screenshot 3](https://github.com/user-attachments/assets/f9439e22-d7b2-4512-b351-fc23d8e03ca9)  
**File:** `include/module_sequencer_mode.h`  
**Priority:** HIGH

**Key Updates Needed:**
- [ ] Increase step button size for easier touch
- [ ] Use bright color (THEME_ACCENT) for active step
- [ ] Clear distinction between enabled/disabled steps
- [ ] Prominent transport controls (play/pause/stop)
- [ ] Display current BPM prominently
- [ ] Add pattern management UI
- [ ] Ensure even step spacing and alignment

---

## Sub-Issue #4: Bouncing Ball Mode (ZEN) UI Update
**Screenshot:** ![Screenshot 4](https://github.com/user-attachments/assets/e1f2e9f5-10e7-439e-a44d-c05bdc9613d5)  
**File:** `include/module_bouncing_ball_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Use gradient/glow effect for ball visibility
- [ ] Add motion trail to show ball path
- [ ] Visual feedback for boundary collisions
- [ ] Display physics parameters
- [ ] Clear visual cues for interaction zones
- [ ] Use color changes to indicate pitch/velocity
- [ ] Add prominent reset button

---

## Sub-Issue #5: Physics Drop Mode UI Update
**Screenshot:** ![Screenshot 5](https://github.com/user-attachments/assets/30fc69ef-d91f-4d47-a259-06f4c25bbb44)  
**File:** `include/module_physics_drop_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Make dropped objects larger and more visible
- [ ] Clearly mark collision/trigger zones
- [ ] Flash/pulse effect on trigger hits
- [ ] Add gravity direction indicator
- [ ] Brief trail effect for motion
- [ ] Add clear all objects button
- [ ] UI for physics parameter adjustments

---

## Sub-Issue #6: Random Generator Mode (RNG) UI Update
**Screenshot:** ![Screenshot 6](https://github.com/user-attachments/assets/02493ecf-4ba1-4265-8677-226bae10a0e4)  
**File:** `include/module_random_generator_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Clear labels for all parameters
- [ ] Visual sliders for ranges (note, velocity, timing)
- [ ] Large trigger button
- [ ] Display generated pattern/sequence
- [ ] Show random seed if applicable
- [ ] Add parameter lock checkboxes
- [ ] Probability visualization
- [ ] Preset save/load buttons

---

## Sub-Issue #7: XY Pad Mode UI Update
**Screenshot:** ![Screenshot 7](https://github.com/user-attachments/assets/3a2cf3bf-1d1e-4729-9635-b4df0f463524)  
**File:** `include/module_xy_pad_mode.h`  
**Priority:** HIGH

**Key Updates Needed:**
- [ ] Maximize pad area (use full screen)
- [ ] Large, clear cursor for touch position
- [ ] Label X/Y axes with CC numbers
- [ ] Optional subtle grid overlay
- [ ] Display current X/Y values numerically
- [ ] Clear CC mapping display
- [ ] Optional snap-to-grid feature
- [ ] Touch trail for recent movement
- [ ] Mark center point clearly

---

## Sub-Issue #8: Arpeggiator Mode UI Update
**Screenshot:** ![Screenshot 8](https://github.com/user-attachments/assets/32a96455-a019-4cdd-8471-3e0962c8cd83)  
**File:** `include/module_arpeggiator_mode.h`  
**Priority:** HIGH

**Key Updates Needed:**
- [ ] Clear note input area
- [ ] Visual pattern selector (up, down, up/down, random)
- [ ] Display active notes in arpeggio
- [ ] Playback position indicator
- [ ] Rate/speed control slider
- [ ] Octave range control
- [ ] Gate length control
- [ ] Graphical pattern visualization
- [ ] Latch mode toggle

---

## Sub-Issue #9: Grid Piano Mode UI Update
**Screenshot:** ![Screenshot 9](https://github.com/user-attachments/assets/1a79cd3b-e90b-4c01-93cf-e7d4240bc870)  
**File:** `include/module_grid_piano_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Optimize cell size (minimum SCALE_X(25) x SCALE_Y(25))
- [ ] Add optional note labels in cells
- [ ] Highlight scale notes in different color
- [ ] Clear root note marking
- [ ] Visual octave separation
- [ ] Clear pressed state for cells
- [ ] Scale/key selector UI
- [ ] Layout options toggle
- [ ] Color-coded intervals

---

## Sub-Issue #10: Auto Chord Mode UI Update
**Screenshot:** ![Screenshot 10](https://github.com/user-attachments/assets/8267c91e-28f1-47e9-a995-61306a5cd8f6)  
**File:** `include/module_auto_chord_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Large chord buttons with names (C, Dm, G7)
- [ ] Key selector dropdown/buttons
- [ ] Scale selector UI
- [ ] Visual chord quality indication
- [ ] Voicing/inversion controls
- [ ] Chord progression builder UI
- [ ] Active chord highlight
- [ ] Strum/arpeggio toggle
- [ ] Optional theory helper display

---

## Sub-Issue #11: LFO Mode UI Update
**Screenshot:** ![Screenshot 11](https://github.com/user-attachments/assets/d8419650-4fbf-449e-9743-62636e798015)  
**File:** `include/module_lfo_mode.h`  
**Priority:** MEDIUM

**Key Updates Needed:**
- [ ] Large waveform display
- [ ] Animated current position indicator
- [ ] Clear waveform type selector
- [ ] Large rate/frequency control
- [ ] Depth control slider
- [ ] Phase control
- [ ] CC assignment display
- [ ] Tempo sync toggle
- [ ] Min/Max range controls
- [ ] Multiple LFO management (if supported)

---

## Sub-Issue #12: TB3PO Mode UI Update
**File:** `include/module_tb3po_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Ensure consistent UI with other modes
- [ ] Clear parameter labeling
- [ ] Visual feedback for active elements
- [ ] Consistent theme color application

---

## Sub-Issue #13: Grids Mode UI Update
**File:** `include/module_grids_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Clear grid visualization
- [ ] Pattern selection UI
- [ ] Density/fill controls
- [ ] Euclidean rhythm parameter display

---

## Sub-Issue #14: Raga Mode UI Update
**File:** `include/module_raga_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Raga selection interface
- [ ] Note visualization for selected raga
- [ ] Drone controls
- [ ] Tanpura simulation UI (if applicable)

---

## Sub-Issue #15: Euclidean Mode UI Update
**File:** `include/module_euclidean_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Steps and pulses controls
- [ ] Visual euclidean pattern representation
- [ ] Rotation control
- [ ] Multiple rhythm tracks (if supported)

---

## Sub-Issue #16: Morph Mode UI Update
**File:** `include/module_morph_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Preset A and B selectors
- [ ] Morph position slider/indicator
- [ ] Visual parameter morphing display
- [ ] Save/load preset UI

---

## Sub-Issue #17: Slink Mode UI Update
**File:** `include/module_slink_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Clear parameter labeling
- [ ] Visual feedback for active state
- [ ] Consistent theme application
- [ ] Intuitive control layout

---

## Sub-Issue #18: Settings Mode UI Update
**File:** `include/module_settings_mode.h`  
**Priority:** MEDIUM  
**Note:** No screenshot provided

**Key Updates Needed:**
- [ ] Organized settings categories
- [ ] Clear labels and descriptions
- [ ] Toggle switches for boolean options
- [ ] Slider controls for numeric values
- [ ] Color inversion preview
- [ ] Display rotation control
- [ ] BLE connection status
- [ ] MIDI channel selection
- [ ] Version information display

---

## Global UI Improvements (Apply to All Sub-Issues)

These improvements should be applied across all modules:

### Header Consistency
- [ ] Use `drawHeader()` from `ui_elements.h` consistently
- [ ] Standard back button placement
- [ ] Clear mode name display
- [ ] Connection status indicators

### Color Theme
- [ ] Use THEME_BG for backgrounds
- [ ] Use THEME_PRIMARY for primary actions
- [ ] Use THEME_SECONDARY for secondary actions
- [ ] Use THEME_SUCCESS/ERROR for feedback
- [ ] Use THEME_TEXT with good contrast

### Touch Feedback
- [ ] Visual feedback for all interactive elements
- [ ] Pressed state using darker shade or THEME_ACCENT
- [ ] Proper use of `touch.justPressed`/`justReleased`

### Scaling
- [ ] All coordinates use SCALE_X()/SCALE_Y()
- [ ] All dimensions use SCALE_W()/SCALE_H()
- [ ] Minimum touch target: SCALE_X(30) x SCALE_Y(30)
- [ ] Use scaled spacing constants

### Typography & Layout
- [ ] Consistent font sizes
- [ ] High contrast text
- [ ] Proper text alignment
- [ ] Consistent margins and spacing
- [ ] Grid-based alignment

---

## Implementation Phases

### Phase 1: Core Modes (Weeks 1-2)
- Sub-Issue #1: Main Menu
- Sub-Issue #2: Keyboard Mode
- Sub-Issue #3: Sequencer Mode
- Sub-Issue #7: XY Pad Mode
- Sub-Issue #8: Arpeggiator Mode

### Phase 2: Interactive Modes (Weeks 3-4)
- Sub-Issue #4: Bouncing Ball Mode
- Sub-Issue #5: Physics Drop Mode
- Sub-Issue #6: Random Generator Mode
- Sub-Issue #9: Grid Piano Mode
- Sub-Issue #10: Auto Chord Mode
- Sub-Issue #11: LFO Mode

### Phase 3: Specialized Modes (Week 5)
- Sub-Issue #12: TB3PO Mode
- Sub-Issue #13: Grids Mode
- Sub-Issue #14: Raga Mode
- Sub-Issue #15: Euclidean Mode
- Sub-Issue #16: Morph Mode
- Sub-Issue #17: Slink Mode

### Phase 4: System UI (Week 6)
- Sub-Issue #18: Settings Mode
- Global improvements polish
- Integration testing

---

## Testing Protocol for Each Sub-Issue

Before marking a sub-issue complete:

1. **Build Test**
   - [ ] Code compiles without errors
   - [ ] No new warnings introduced

2. **Visual Test**
   - [ ] UI renders correctly
   - [ ] All elements visible within screen bounds
   - [ ] Colors match theme
   - [ ] Text is readable

3. **Interaction Test**
   - [ ] All buttons/controls respond to touch
   - [ ] Visual feedback works correctly
   - [ ] Touch targets are adequate size
   - [ ] No unintended touches

4. **Functional Test**
   - [ ] Mode functionality unchanged
   - [ ] MIDI output still works
   - [ ] Back button returns to menu
   - [ ] No crashes or freezes

5. **Performance Test**
   - [ ] UI remains responsive
   - [ ] No noticeable lag
   - [ ] Frame rate acceptable

---

## Notes

- Each sub-issue can be implemented independently
- User should review and edit recommendations before implementation
- All changes should follow the "minimal changes" principle
- Test thoroughly after each module update
- Document any deviations from recommendations

---

**Created:** 2026-02-05  
**Based on Issue:** "All of these screens require UI updates"  
**Status:** Awaiting user review and approval
