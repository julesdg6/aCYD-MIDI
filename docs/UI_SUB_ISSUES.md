# UI Update Sub-Issues for aCYD-MIDI

This document breaks down the UI update initiative into trackable sub-issues for each module. Each sub-issue can be implemented and tested independently as part of the comprehensive UI update.

**Based on User's detailed UI/UX review feedback**

---

## Implementation Timeline

### Phase 1: HIGH Priority Modules (Weeks 1-2)
Critical usability improvements for most problematic interfaces
- Sub-Issue #2: MORPH – Gesture Morphing
- Sub-Issue #5: LFO MOD – CC Modulation
- Sub-Issue #7: RNG JAMS – Random Music
- Sub-Issue #9: ARPEGGIATOR – Piano Chord Arp

### Phase 2: MEDIUM Priority Modules (Weeks 3-4)
Improve remaining dense interfaces
- Sub-Issue #1: SLINK – Wave Engine
- Sub-Issue #3: RAGA – Indian Classical Scale
- Sub-Issue #4: EUCLID – Euclidean Rhythm
- Sub-Issue #6: GRIDS – XY Rhythm Generator
- Sub-Issue #8: TB-3PO – Pattern Generator

### Phase 3: Global Polish (Week 5)
Apply global improvements to all modules
- Header standardization
- Color theme enforcement
- Scaling verification
- Touch feedback consistency

### Phase 4: Testing & Refinement (Week 6)
- Comprehensive integration testing
- Bug fixes and performance optimization
- Final polish

---

## Sub-Issue #1: SLINK – Wave Engine

**Priority:** MEDIUM  
**File:** `include/module_slink_mode.h`, `src/module_slink_mode.cpp`  
**Estimated Effort:** 3-4 hours

### Problems to Fix
- Wave A / Wave B visualization is crammed vertically, Wave B barely visible
- Menu buttons (MAIN / TRIG / PITCH / CLOCK / SCAL / MOD / SETP) visually identical and not on one line
- Navigation unclear

### Implementation Checklist
- [ ] Split Wave A and Wave B into separate pages or toggle views
- [ ] Group tabs by function (Sound / Modulation / Timing / Settings)
- [ ] Consolidate navigation buttons into single horizontal row
- [ ] Add visual indicator for current active page/tab
- [ ] Increase waveform visualization area
- [ ] Use consistent button styling for tab navigation
- [ ] Consider icon-based navigation to save space

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - All pages render correctly, waveforms clearly visible, tabs organized
3. **Interaction** - Tab navigation smooth, adequate touch targets, current page obvious
4. **Functional** - All parameters work, waveform updates correctly
5. **Performance** - No lag during waveform updates

---

## Sub-Issue #2: MORPH – Gesture Morphing

**Priority:** HIGH  
**File:** `include/module_morph_mode.h`, `src/module_morph_mode.cpp`  
**Estimated Effort:** 4-5 hours

### Problems to Fix
- Large empty space with minimal information
- Slot controls cramped and confusing (unclear states, scattered layout)
- PLAY and RECORD buttons spatially disconnected
- X/Y readouts tiny and low contrast
- No visual indication of recorded gesture or playback state

### Implementation Checklist
- [ ] Add visual trails or envelopes to show recorded morph data
- [ ] Move Slot controls to bottom row, left side as single panel
- [ ] Standardize color meaning (armed = yellow, active = green, recording = red)
- [ ] Move PLAY / RECORD buttons together on right side
- [ ] Add timeline or loop-length indicator
- [ ] Increase size and contrast of X/Y value readouts
- [ ] Show recording status visually (indicator, timer)
- [ ] Add visual feedback during playback (path overlay)
- [ ] Label slot states clearly (Empty / Armed / Recorded / Playing)

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Controls properly positioned, XY readouts readable, slot states clear
3. **Interaction** - Recording workflow intuitive, playback obvious, slot selection easy
4. **Functional** - Gesture capture and playback work correctly, all slots function
5. **Performance** - Visual trails don't impact responsiveness

---

## Sub-Issue #3: RAGA – Indian Classical Scale

**Priority:** MEDIUM  
**File:** `include/module_raga_mode.h`, `src/module_raga_mode.cpp`  
**Estimated Effort:** 3-4 hours

### Problems to Fix
- Information density too high
- Raga and Tala selectors crammed vertically, minus buttons tiny
- Scale name redundantly displayed
- Drone control visually disconnected

### Implementation Checklist
- [ ] Separate Scale Selection and Playback Control into distinct sections
- [ ] Replace +/- buttons with scrollable lists or rotary-style selectors
- [ ] Increase spacing between Raga and Tala rows
- [ ] Make Drone control part of "Sound Output" group
- [ ] Remove duplicate scale name displays
- [ ] Add visual grouping (boxes/panels) for related controls
- [ ] Increase touch target size for all buttons
- [ ] Consider multi-page layout if space insufficient

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Clear visual hierarchy, adequate spacing, grouped controls
3. **Interaction** - Easy Raga/Tala selection, drone control clear, buttons accessible
4. **Functional** - Correct scale playback, drone functions properly
5. **Performance** - Responsive interaction with all controls

---

## Sub-Issue #4: EUCLID – Euclidean Rhythm

**Priority:** MEDIUM  
**File:** `include/module_euclidean_mode.h`, `src/module_euclidean_mode.cpp`  
**Estimated Effort:** 4-5 hours

### Problems to Fix
- Central circular visual visually dense and hard to decode
- V1 / V2 / V3 controls cramped and inconsistent
- BPM controls duplicated unnecessarily
- Play button competes visually with BPM buttons
- Rhythm division (e.g. 2/4) small and visually weak

### Implementation Checklist
- [ ] Move circular visualization to left to create space
- [ ] Allow solo/mute per ring instead of tiny labels
- [ ] Consolidate BPM control into single area (or remove if global)
- [ ] Increase size and contrast of time division indicator
- [ ] Allow zoom or focus mode per rhythm layer
- [ ] Add clear labels for each ring (V1, V2, V3)
- [ ] Increase spacing between parameter controls
- [ ] Make Play/Stop button more prominent
- [ ] Add visual feedback when rhythm is playing (animated ring highlight)

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Rings clearly distinguishable, controls well-organized, division visible
3. **Interaction** - Easy parameter adjustment, clear feedback, solo/mute works
4. **Functional** - Correct euclidean rhythm generation, sync works
5. **Performance** - Smooth animation during playback

---

## Sub-Issue #5: LFO MOD – CC Modulation

**Priority:** HIGH  
**File:** `include/module_lfo_mode.h`, `src/module_lfo_mode.cpp`  
**Estimated Effort:** 4-5 hours

### Problems to Fix
- Control cluster dense and intimidating
- +/- buttons everywhere create visual noise
- Waveform preview too small
- Numeric values lack units and context

### Implementation Checklist
- [ ] Replace +/- buttons with sliders or rotary controls
- [ ] Group controls into: Rate / Shape / Amount / Target
- [ ] Enlarge waveform preview and make it interactive (drag to shape)
- [ ] Visually highlight the currently modulated parameter
- [ ] Add units to all numeric displays (Hz, %, ms, etc.)
- [ ] Use progressive disclosure for advanced parameters
- [ ] Add preset waveform selection (Sine, Triangle, Square, Saw, Random)
- [ ] Show real-time modulation output value
- [ ] Add visual feedback for modulation intensity

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Waveform preview prominent, controls organized, units shown
3. **Interaction** - Easy parameter adjustment, interactive waveform works
4. **Functional** - Correct LFO generation, MIDI output accurate
5. **Performance** - Real-time waveform display doesn't lag

---

## Sub-Issue #6: GRIDS – XY Rhythm Generator

**Priority:** MEDIUM  
**File:** `include/module_grids_mode.h`, `src/module_grids_mode.cpp`  
**Estimated Effort:** 3-4 hours

### Problems to Fix
- XY pad visually dominant but semantically unclear
- K / S / H sliders difficult to use and badly spaced
- BPM controls repeated again
- Bottom bar cramped and partially obscured

### Implementation Checklist
- [ ] Add labels/legends to XY pad (what X and Y control)
- [ ] Show visual crosshair or cursor on XY pad
- [ ] Increase size of K / S / H sliders
- [ ] Add full text labels for K / S / H (Kick / Snare / Hihat or similar)
- [ ] Make RNDM a submenu with selectable targets
- [ ] Increase padding around bottom controls
- [ ] Remove local BPM and use global transport
- [ ] Add visual rhythm pattern preview
- [ ] Show current parameter values numerically

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - XY pad clear, sliders usable, labels visible, controls accessible
3. **Interaction** - Precise XY control, easy slider adjustment, clear feedback
4. **Functional** - Rhythm generation works, randomization functions
5. **Performance** - Responsive to XY pad movements

---

## Sub-Issue #7: RNG JAMS – Random Music

**Priority:** HIGH  
**File:** `include/module_random_generator_mode.h`, `src/module_random_generator_mode.cpp`  
**Estimated Effort:** 4-5 hours

### Problems to Fix
- Too many buttons for too few pixels
- Key, scale, octave, chance, and range compete visually
- MIN/MAX buttons ambiguous (range of what?)
- Chance slider visually detached from its meaning
- No preview or explanation of generated output

### Implementation Checklist
- [ ] Collapse parameters into expandable sections (Scale / Range / Probability / Timing)
- [ ] Replace MIN/MAX buttons with bounded sliders
- [ ] Visually group randomness controls together
- [ ] Add short descriptive text ("Chance affects note probability")
- [ ] Provide small preview or activity indicator
- [ ] Use progressive disclosure (show basic, hide advanced)
- [ ] Add visual representation of note range (piano keyboard highlighting)
- [ ] Show last generated pattern or note sequence
- [ ] Make "Generate" button visually prominent
- [ ] Label all controls clearly with full words

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Clear hierarchy, grouped controls, readable labels, preview visible
3. **Interaction** - Intuitive parameter adjustment, clear workflow, easy generation
4. **Functional** - Random generation works, parameters affect output correctly
5. **Performance** - Real-time generation doesn't lag

---

## Sub-Issue #8: TB-3PO – Pattern Generator

**Priority:** MEDIUM  
**File:** `include/module_tb3po_mode.h`, `src/module_tb3po_mode.cpp`  
**Estimated Effort:** 3-4 hours

### Problems to Fix
- Note names, steps, accents, and gates overlap visually
- Color meaning not self-evident
- Density controls visually disconnected from grid

### Implementation Checklist
- [ ] Group SEED and REGEN controls together
- [ ] Add clear visual separation between note grid and controls
- [ ] Provide color legend (what each color represents)
- [ ] Increase spacing between grid elements
- [ ] Make accent and gate states more distinguishable
- [ ] Add clear labels for all parameters
- [ ] Consider step sequencer-style vertical layout
- [ ] Add pattern preview/playback indicator
- [ ] Group density controls near pattern display
- [ ] Add visual feedback during pattern playback (highlight current step)

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Grid elements distinguishable, controls organized, legend present
3. **Interaction** - Easy pattern editing, clear feedback, controls accessible
4. **Functional** - Pattern generation/playback correct, all parameters work
5. **Performance** - Smooth playback animation

---

## Sub-Issue #9: ARPEGGIATOR – Piano Chord Arp

**Priority:** HIGH  
**File:** `include/module_arpeggiator_mode.h`, `src/module_arpeggiator_mode.cpp`  
**Estimated Effort:** 4-5 hours

### Problems to Fix
- Control cluster scattered (Pattern, speed, octave, BPM, type not grouped)
- Piano keyboard small and hard to interact with
- Active notes hard to distinguish
- BPM repeated despite being global elsewhere
- No visual feedback for arpeggio playback

### Special Question
- **Does it use scale functions at all?** If not, integrate scale system for diatonic arpeggios

### Implementation Checklist
- [ ] Group controls into: Timing / Pattern / Range
- [ ] Enlarge keyboard and improve contrast for active notes
- [ ] Allow chord selection as blocks rather than individual keys (e.g. "C Major", "D Minor")
- [ ] Remove local BPM and rely on global transport
- [ ] Add visual step indicator for arpeggio playback (highlight current note)
- [ ] Show arpeggio pattern visually (up, down, up-down, random, etc.)
- [ ] Increase spacing between controls
- [ ] Add preset chord selection
- [ ] Investigate scale function integration for diatonic arpeggios
- [ ] Show chord name when notes are selected

### Testing Protocol
1. **Build** - Code compiles without errors
2. **Visual** - Keyboard usable, controls organized, playback clear, pattern visible
3. **Interaction** - Easy chord input, pattern selection intuitive, clear feedback
4. **Functional** - Arpeggiation works correctly, MIDI output accurate, timing solid
5. **Performance** - Smooth playback, no timing issues

---

## Global Improvements (Apply During Implementation)

### Header Consistency
- [ ] Use `drawHeader()` consistently across all modules
- [ ] Standard back button placement (always top-left)
- [ ] Connection status indicators (BLE/MIDI)
- [ ] Consistent title font and size

### Color Theme Standardization
- [ ] Strict adherence to `THEME_*` colors from `common_definitions.h`
- [ ] Semantic color use (green=good, red=warning, blue=info)
- [ ] High contrast for readability
- [ ] Consistent color meaning across modules

### Touch Feedback
- [ ] Visual feedback on all interactive elements
- [ ] Use `touch.justPressed` / `touch.justReleased` patterns
- [ ] Minimum touch targets (30x30 scaled)
- [ ] Consistent button pressed/active states

### Scaling
- [ ] All coordinates use `SCALE_X()` / `SCALE_Y()`
- [ ] All dimensions use `SCALE_W()` / `SCALE_H()`
- [ ] Test on multiple display sizes

### Typography & Layout
- [ ] Consistent font sizes
- [ ] Proper alignment (left for labels, center for values)
- [ ] Grid-based layouts
- [ ] Adequate spacing (breathing room)
- [ ] Clear visual grouping

---

## Testing Protocol Template

Use this 5-step protocol for each module:

### 1. Build Test
- [ ] Compile without errors/warnings
- [ ] No memory issues
- [ ] Upload succeeds

### 2. Visual Test
- [ ] Elements within screen bounds
- [ ] No text clipping
- [ ] Colors match theme
- [ ] Proper scaling
- [ ] Balanced layout

### 3. Interaction Test
- [ ] Touch targets responsive
- [ ] Immediate feedback
- [ ] No accidental touches
- [ ] Logical navigation
- [ ] Back button works

### 4. Functional Test
- [ ] Module functions correctly
- [ ] MIDI output accurate
- [ ] Parameters work as expected
- [ ] State maintained
- [ ] No crashes

### 5. Performance Test
- [ ] UI responsive
- [ ] No lag/stuttering
- [ ] Accurate MIDI timing
- [ ] Acceptable frame rate
- [ ] Stable memory usage

---

## Notes for Implementation

### Design Principles
1. **Reduce visual density** - Less is more, use progressive disclosure
2. **Enforce hierarchy** - Make important things obvious
3. **Group related controls** - Spatial organization aids understanding
4. **Label clearly** - No abbreviations unless space-critical
5. **Provide feedback** - All interactions need immediate visual response
6. **Be consistent** - Reuse patterns across modules

### Common Patterns to Apply
- **Progressive Disclosure:** Hide advanced parameters in sub-menus
- **Visual Grouping:** Use boxes, panels, or spacing to group related controls
- **Touch Targets:** Minimum 30x30 scaled pixels for all buttons
- **Feedback:** Color change, highlight, or animation on touch
- **Labels:** Full words preferred over abbreviations
- **Spacing:** Adequate breathing room between elements

### Tools Available
- `ui_elements.h` - Reusable UI components (`drawHeader`, `drawRoundButton`, etc.)
- `common_definitions.h` - Theme colors (`THEME_*` constants)
- Scaling macros - `SCALE_X/Y/W/H` for responsive layouts
- Touch utilities - `touch.justPressed`, `touch.justReleased`

---

## Progress Tracking

Use `UI_PROGRESS_TRACKER.md` to track implementation status:
- Mark sub-issues as Started / In Progress / Testing / Complete
- Log any issues or deviations from plan
- Track quality metrics
- Note any user feedback or changes

---

**Status:** Sub-Issues Defined - Ready for Implementation  
**Created:** 2026-02-05  
**Based On:** User's detailed UI/UX review feedback  
**Total Sub-Issues:** 9 modules + global improvements  
**Estimated Total Effort:** 30-40 hours  
**Implementation Approach:** As single comprehensive fix (per user request)
