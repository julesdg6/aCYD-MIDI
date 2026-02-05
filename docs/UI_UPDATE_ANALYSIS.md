# UI Update Analysis for aCYD-MIDI Modules

This document provides a detailed analysis of UI updates needed for each module based on the screenshots provided in the issue and the user's detailed UI/UX review.

**General Themes Across All Screens:**
- Too many controls visible at once
- Inconsistent layout and control semantics
- Important information clipped, obscured, or visually de-emphasised
- Poor hierarchy: the eye has nowhere obvious to rest
- Controls lack affordance (unclear what is editable, draggable, or stateful)
- Visual density exceeds what the screen size can comfortably support

**Cross-Screen Design Principles:**
- Reduce per-screen control count
- Prefer **progressive disclosure** over "everything at once"
- Design for **one primary interaction per screen**
- Enforce consistent visual hierarchy
- Clarify intent and affordances

---

## Module 1: SLINK – Wave Engine

**File:** `include/module_slink_mode.h` / `src/module_slink_mode.cpp`  
**Header:** "SLINK" (multiple sub-pages: Wave Engine, Trigger Engine, Pitch Engine, Clock & Length, Scale & Arp, Modulators, Presets)

### Current Problems

1. **Wave A / Wave B visualization is crammed vertically**
   - Wave B is barely visible
   - Insufficient space for meaningful waveform display
   
2. **Menu buttons (MAIN / TRIG / PITCH / CLOCK / SCAL / MOD / SETP) are visually identical**
   - Should be on one line
   - Visually de-prioritised despite being important navigation
   - No clear indication of current page

### Recommended Improvements

- [ ] Split Wave A and Wave B into **separate pages or toggle views**
- [ ] Group tabs by function (Sound / Modulation / Timing / Settings)
- [ ] Consolidate navigation buttons into a single horizontal row
- [ ] Add visual indicator for current active page/tab
- [ ] Increase waveform visualization area
- [ ] Use consistent button styling for tab navigation
- [ ] Consider icon-based navigation to save space

### Implementation Priority: MEDIUM
**Rationale:** Complex multi-page interface needs clear navigation and visual hierarchy

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - All pages render correctly, waveforms visible
3. **Interaction Test** - Tab navigation works smoothly, touch targets adequate
4. **Functional Test** - All parameters function correctly
5. **Performance Test** - Waveform updates don't cause lag

---

## Module 2: MORPH – Gesture Morphing

**File:** `include/module_morph_mode.h` / `src/module_morph_mode.cpp`  
**Header:** "MORPH - Gesture morphing"

### Current Problems

1. **Large empty space with minimal information**
   - XY pad dominates but lacks context
   - Poor use of available screen space

2. **Slot controls are cramped and confusing**
   - Slot colours are unclear
   - Slot states (armed / active / recorded) are not obvious
   - Controls scattered spatially

3. **PLAY and RECORD buttons are spatially disconnected**
   - Their relationship is unclear
   - Difficult to understand recording workflow

4. **X/Y percentage readouts are tiny and low contrast**
   - Hard to read during performance
   
5. **No visual indication of recorded gesture length or playback state**
   - Can't see what's been recorded
   - No feedback during playback

### Recommended Improvements

- [ ] Add **visual trails or envelopes** to show recorded morph data
- [ ] Group Slot controls into a single, clearly labelled panel (move to bottom row, left)
- [ ] Standardise colour meaning (e.g. armed = yellow, active = green, recording = red)
- [ ] Place PLAY / RECORD buttons together on the right side
- [ ] Add timeline or loop-length indicator
- [ ] Increase size and contrast of X/Y value readouts
- [ ] Show recording status visually (recording indicator, timer)
- [ ] Add visual feedback during gesture playback (path overlay)
- [ ] Label slot states clearly (Empty / Armed / Recorded / Playing)

### Implementation Priority: HIGH
**Rationale:** Confusing workflow needs immediate UX improvements for usability

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - All controls properly positioned, readable
3. **Interaction Test** - Recording workflow is intuitive, playback clear
4. **Functional Test** - Gesture capture and playback work correctly
5. **Performance Test** - Visual trails don't impact responsiveness

---

## Module 3: RAGA – Indian Classical Scale

**File:** `include/module_raga_mode.h` / `src/module_raga_mode.cpp`  
**Header:** "RAGA - Indian Classical Scales"

### Current Problems

1. **Information density is too high for the available space**
   - Screen feels cramped and overwhelming
   
2. **Raga and Tala selectors feel crammed vertically**
   - Difficult to navigate
   - Minus buttons are tiny and easy to miss
   
3. **Scale name appears multiple times without clear hierarchy**
   - Redundant information
   - Visual confusion

4. **Drone control is visually disconnected from scale selection**
   - Unclear relationship between controls

### Recommended Improvements

- [ ] Separate **Scale Selection** and **Playback Control** into distinct sections
- [ ] Replace +/- buttons with scrollable lists or rotary-style selectors
- [ ] Increase spacing between Raga and Tala rows
- [ ] Make Drone control part of a "Sound Output" group
- [ ] Remove duplicate scale name displays
- [ ] Add visual grouping (boxes/panels) for related controls
- [ ] Increase touch target size for all buttons
- [ ] Consider multi-page layout if space insufficient

### Implementation Priority: MEDIUM
**Rationale:** Dense interface needs breathing room and better organization

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Clear visual hierarchy, adequate spacing
3. **Interaction Test** - Easy selection of Raga/Tala, drone control clear
4. **Functional Test** - Correct scale playback, drone functions properly
5. **Performance Test** - Responsive interaction with all controls

---

## Module 4: EUCLID – Euclidean Rhythm

**File:** `include/module_euclidean_mode.h` / `src/module_euclidean_mode.cpp`  
**Header:** "EUCLID - Euclidean Rhythm"

### Current Problems

1. **Central circular visual is visually dense and hard to decode**
   - Too much information compressed into small space
   - Unclear which ring represents what
   
2. **V1 / V2 / V3 controls are cramped and inconsistent**
   - Difficult to adjust parameters
   - No clear relationship to the rings
   
3. **BPM controls duplicated unnecessarily**
   - Should use global BPM
   
4. **Play button competes visually with BPM buttons**
   - Poor visual hierarchy
   
5. **The rhythm division (e.g. 2/4) is small and visually weak**
   - Important timing information is de-emphasized

### Recommended Improvements

- [ ] Move circular visualization to the left to create space
- [ ] Allow solo/mute per ring instead of tiny labels
- [ ] Consolidate BPM control into a single area (or remove if global)
- [ ] Increase size and contrast of the time division indicator
- [ ] Allow zoom or focus mode per rhythm layer
- [ ] Add clear labels for each ring (V1, V2, V3)
- [ ] Increase spacing between parameter controls
- [ ] Make Play/Stop button more prominent
- [ ] Add visual feedback when rhythm is playing (animated ring highlight)

### Implementation Priority: MEDIUM
**Rationale:** Powerful rhythmic tool needs clearer visual organization

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Rings clearly distinguishable, controls well-organized
3. **Interaction Test** - Easy parameter adjustment, clear feedback
4. **Functional Test** - Correct euclidean rhythm generation, sync works
5. **Performance Test** - Smooth animation during playback

---

## Module 5: LFO MOD – CC Modulation

**File:** `include/module_lfo_mode.h` / `src/module_lfo_mode.cpp`  
**Header:** "LFO MOD - Pitchwheel" or "LFO MOD - CC X"

### Current Problems

1. **Control cluster is dense and intimidating**
   - Too many +/- buttons
   - Difficult to understand what each parameter does
   
2. **+/- buttons everywhere create visual noise**
   - Inefficient use of space
   - Tedious to adjust values
   
3. **Waveform preview is too small relative to its importance**
   - Core visual feedback is minimized
   
4. **Numeric values lack units and context**
   - Unclear what values represent

### Recommended Improvements

- [ ] Replace +/- buttons with sliders or rotary controls
- [ ] Group controls into logical sections: Rate / Shape / Amount / Target
- [ ] Enlarge waveform preview and make it interactive (drag to shape)
- [ ] Visually highlight the currently modulated parameter
- [ ] Add units to all numeric displays (Hz, %, ms, etc.)
- [ ] Use progressive disclosure for advanced parameters
- [ ] Add preset waveform selection (Sine, Triangle, Square, Saw, Random)
- [ ] Show real-time modulation output value
- [ ] Add visual feedback for modulation intensity

### Implementation Priority: HIGH
**Rationale:** Powerful modulation tool is hampered by cluttered interface

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Waveform preview prominent, controls organized
3. **Interaction Test** - Easy parameter adjustment, interactive waveform
4. **Functional Test** - Correct LFO generation, MIDI output accurate
5. **Performance Test** - Real-time waveform display doesn't lag

---

## Module 6: GRIDS – XY Rhythm Generator

**File:** `include/module_grids_mode.h` / `src/module_grids_mode.cpp`  
**Header:** "GRIDS"

### Current Problems

1. **XY pad is visually dominant but semantically unclear**
   - Not obvious what X and Y control
   - No visual feedback of current position
   
2. **K / S / H sliders are difficult to use and badly spaced**
   - Too small for precise control
   - Unclear what K, S, H represent
   
3. **BPM controls repeated yet again**
   - Should use global BPM
   
4. **Bottom bar is cramped and partially obscured**
   - Important controls are difficult to access

### Recommended Improvements

- [ ] Add labels/legends to XY pad (what X and Y control)
- [ ] Show visual crosshair or cursor on XY pad
- [ ] Increase size of K / S / H sliders
- [ ] Add full text labels for K / S / H (Kick / Snare / Hihat or similar)
- [ ] Make RNDM a submenu with selectable targets
- [ ] Increase padding around bottom controls
- [ ] Remove local BPM and use global transport
- [ ] Add visual rhythm pattern preview
- [ ] Show current parameter values numerically

### Implementation Priority: MEDIUM
**Rationale:** Generative rhythm engine needs clearer parameter mapping

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - XY pad clear, sliders usable, labels visible
3. **Interaction Test** - Precise XY control, easy slider adjustment
4. **Functional Test** - Rhythm generation works, randomization functions
5. **Performance Test** - Responsive to XY pad movements

---

## Module 7: RNG JAMS – Random Music

**File:** `include/module_random_generator_mode.h` / `src/module_random_generator_mode.cpp`  
**Header:** "RNG JAMS - Random Music"

### Current Problems

1. **Too many buttons for too few pixels**
   - Overwhelming number of controls
   - Difficult to find specific parameters
   
2. **Key, scale, octave, chance, and range all compete visually**
   - No clear hierarchy
   - Everything looks equally important
   
3. **MIN/MAX buttons are ambiguous (range of what?)**
   - Unclear what parameter they affect
   
4. **Chance slider is visually detached from its meaning**
   - No explanation of what "chance" affects
   
5. **No preview or explanation of the generated output**
   - Can't see what will be generated

### Recommended Improvements

- [ ] Collapse parameters into expandable sections (Scale / Range / Probability / Timing)
- [ ] Replace MIN/MAX buttons with bounded sliders
- [ ] Visually group randomness controls together
- [ ] Add short descriptive text ("Chance affects note probability")
- [ ] Provide a small preview or activity indicator
- [ ] Use progressive disclosure (show basic controls, hide advanced)
- [ ] Add visual representation of note range (piano keyboard highlighting)
- [ ] Show last generated pattern or note sequence
- [ ] Add "Generate" button that's visually prominent
- [ ] Label all controls clearly with full words

### Implementation Priority: HIGH
**Rationale:** Complex generative tool needs clear organization to be usable

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Clear hierarchy, grouped controls, readable labels
3. **Interaction Test** - Intuitive parameter adjustment, clear workflow
4. **Functional Test** - Random generation works, parameters affect output correctly
5. **Performance Test** - Real-time generation doesn't lag

---

## Module 8: TB-3PO – Pattern Generator

**File:** `include/module_tb3po_mode.h` / `src/module_tb3po_mode.cpp`  
**Header:** "TB-3PO"

### Current Problems

1. **Note names, steps, accents, and gates overlap visually**
   - Difficult to distinguish different elements
   - Visual chaos
   
2. **Colour meaning is not self-evident**
   - No legend or clear mapping
   - Difficult to understand current pattern state
   
3. **Density controls are visually disconnected from the grid**
   - Unclear how they affect the pattern

### Recommended Improvements

- [ ] Group SEED and REGEN controls together
- [ ] Add clear visual separation between note grid and controls
- [ ] Provide a colour legend (what each colour represents)
- [ ] Increase spacing between grid elements
- [ ] Make accent and gate states more distinguishable
- [ ] Add clear labels for all parameters
- [ ] Consider step sequencer-style vertical layout
- [ ] Add pattern preview/playback indicator
- [ ] Group density controls near pattern display
- [ ] Add visual feedback during pattern playback (highlight current step)

### Implementation Priority: MEDIUM
**Rationale:** Pattern generator needs clearer visual organization and feedback

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Grid elements distinguishable, controls organized
3. **Interaction Test** - Easy pattern editing, clear feedback
4. **Functional Test** - Pattern generation/playback correct
5. **Performance Test** - Smooth playback animation

---

## Module 9: ARPEGGIATOR – Piano Chord Arp

**File:** `include/module_arpeggiator_mode.h` / `src/module_arpeggiator_mode.cpp`  
**Header:** "ARPEGGIATOR - Piano Chord Arps"

### Current Problems

1. **Control cluster is scattered**
   - Pattern, speed, octave, BPM, and type are not grouped
   - Difficult to find related controls
   
2. **Piano keyboard is small and hard to interact with**
   - Keys too small for accurate touch
   
3. **Active notes are hard to distinguish**
   - Unclear which notes are part of the chord
   
4. **BPM repeated again despite being global elsewhere**
   - Should use global BPM
   
5. **No visual feedback for arpeggio playback**
   - Can't see which note is currently playing

### Recommended Improvements

- [ ] Group controls into logical sections: Timing / Pattern / Range
- [ ] Enlarge keyboard and improve contrast for active notes
- [ ] Allow chord selection as blocks rather than individual keys (e.g. "C Major", "D Minor")
- [ ] Remove local BPM and rely on global transport
- [ ] Add visual step indicator for arpeggio playback (highlight current note)
- [ ] Show arpeggio pattern visually (up, down, up-down, random, etc.)
- [ ] Increase spacing between controls
- [ ] Add preset chord selection
- [ ] **Question:** Does it use the scale functions at all? If not, integrate scale system for diatonic arpeggios
- [ ] Show chord name when notes are selected

### Implementation Priority: HIGH
**Rationale:** Popular performance tool needs better organization and feedback

### Testing Requirements
1. **Build Test** - Compile without errors
2. **Visual Test** - Keyboard usable, controls organized, playback clear
3. **Interaction Test** - Easy chord input, pattern selection intuitive
4. **Functional Test** - Arpeggiation works correctly, MIDI output accurate
5. **Performance Test** - Smooth playback, no timing issues

---

## Global UI Improvements (Apply to All Modules)

### 1. Header Consistency
- [ ] Use `drawHeader()` consistently across all modules
- [ ] Standard back button placement (always top-left)
- [ ] Connection status indicators (BLE/MIDI)
- [ ] Consistent title font and size

### 2. Color Theme Standardization
- [ ] Strict adherence to `THEME_*` colors from `common_definitions.h`
- [ ] Consistent visual language across modules
- [ ] High contrast for readability
- [ ] Semantic color use (green=good, red=warning, blue=info, etc.)

### 3. Touch Feedback Patterns
- [ ] Visual feedback on all interactive elements
- [ ] Pressed/released states using `touch.justPressed` / `touch.justReleased`
- [ ] Adequate touch targets (minimum 30x30 scaled)
- [ ] Consistent button hover/active states

### 4. Scaling
- [ ] All coordinates use `SCALE_X()` / `SCALE_Y()`
- [ ] All dimensions use `SCALE_W()` / `SCALE_H()`
- [ ] Support for different display sizes
- [ ] Test on multiple hardware variants

### 5. Typography & Layout
- [ ] Consistent font sizes across modules
- [ ] Proper text alignment (left for labels, center for values)
- [ ] Grid-based layouts for consistency
- [ ] Adequate breathing room between elements
- [ ] Clear visual grouping (boxes, panels, separators)

### 6. Progressive Disclosure
- [ ] Hide advanced parameters behind sub-menus or expandable sections
- [ ] Show only essential controls by default
- [ ] Use tabs or pages for complex modules
- [ ] Clear navigation between modes/pages

### 7. Affordances
- [ ] Make interactive elements obviously touchable (button-like appearance)
- [ ] Show current state clearly (on/off, selected/unselected)
- [ ] Provide immediate visual feedback for all interactions
- [ ] Use consistent interaction patterns (e.g. all sliders work the same way)

---

## Implementation Strategy

### Phase 1: HIGH Priority Modules (Weeks 1-2)
Focus on modules with most severe usability issues:
1. MORPH – Gesture Morphing
2. LFO MOD – CC Modulation
3. RNG JAMS – Random Music
4. ARPEGGIATOR – Piano Chord Arp

### Phase 2: MEDIUM Priority Modules (Weeks 3-4)
Address remaining modules with dense interfaces:
5. SLINK – Wave Engine
6. RAGA – Indian Classical Scale
7. EUCLID – Euclidean Rhythm
8. GRIDS – XY Rhythm Generator
9. TB-3PO – Pattern Generator

### Phase 3: Global Polish (Week 5)
Apply global improvements to all modules:
- Header standardization
- Color theme enforcement
- Scaling verification
- Touch feedback consistency

### Phase 4: Testing & Refinement (Week 6)
- Comprehensive testing of all modules
- Fix any issues discovered
- Performance optimization
- Final polish and integration testing

---

## Testing Protocol (For Each Module)

### 1. Build Test
- [ ] Code compiles without errors or warnings
- [ ] No memory warnings or stack issues
- [ ] Upload succeeds to device

### 2. Visual Test
- [ ] All elements render within screen bounds
- [ ] No text clipping or overlap
- [ ] Colors match theme definitions
- [ ] Proper scaling on target hardware
- [ ] Layout is visually balanced

### 3. Interaction Test
- [ ] All touch targets are responsive
- [ ] Touch feedback is immediate and clear
- [ ] No accidental activations
- [ ] Navigation flows logically
- [ ] Back button returns to menu

### 4. Functional Test
- [ ] Module performs its intended function
- [ ] MIDI output is correct
- [ ] Parameters affect output as expected
- [ ] State is maintained correctly
- [ ] No crashes or freezes

### 5. Performance Test
- [ ] UI remains responsive during operation
- [ ] No lag or stuttering
- [ ] MIDI timing is accurate
- [ ] Frame rate is acceptable
- [ ] Memory usage is stable

---

## Design Principles Summary

1. **Consistency** - Uniform theme colors, layouts, and UI patterns across all modules
2. **Usability** - Clear feedback, adequate touch targets, intuitive workflows
3. **Aesthetics** - Polished, professional appearance with proper hierarchy
4. **Functionality** - Never sacrifice function for form, but make function clear
5. **Minimal Changes** - Surgical, precise modifications that improve UX without breaking existing functionality

---

**Status:** Analysis Complete - Ready for User Review and Editing  
**Created:** 2026-02-05  
**Issue:** "All of these screens require UI updates"  
**Based On:** User's detailed UI/UX review feedback
**Next Steps:** User reviews and edits recommendations, then implementation begins
