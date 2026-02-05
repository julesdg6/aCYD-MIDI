#include "module_tb3po_mode.h"
#include "clock_manager.h"
#include <uClock.h>

TB3POState tb3po;
static SequencerSyncState tb3poSync;

// ISR-safe step counter from uClock step extension
static volatile uint32_t tb3poStepCount = 0;
// runtime-assigned base track
static volatile uint8_t tb3poAssignedTrack = 0xFF;
static const uint8_t tb3poRequestedTracks = 1;
static volatile bool tb3poAssignedFlag = false;

static void onTb3poStepISR(uint32_t step, uint8_t track) {
  (void)step;
  if (tb3poAssignedTrack == 0xFF) {
    tb3poAssignedTrack = track;
    tb3poAssignedFlag = true;
    tb3poStepCount++;
    return;
  }
  if (track >= tb3poAssignedTrack && track < tb3poAssignedTrack + tb3poRequestedTracks) {
    tb3poStepCount++;
  }
}

static bool randBit(int probability) {
  return (random(100) < probability);
}

static void reseed() {
  if (!tb3po.lockSeed) {
    randomSeed(millis());
    tb3po.seed = random(1, 65536);
  }
}

static int getOnOffDensity() {
  int noteDens = (int)tb3po.density - 7;
  return abs(noteDens);
}

static int getPitchChangeDensity() {
  return constrain(tb3po.density, 0, 8);
}

static void regeneratePitches() {
  randomSeed(tb3po.seed);
  const Scale &scale = scales[tb3po.scaleIndex];
  int pitchChangeDens = getPitchChangeDensity();
  int availablePitches = 0;
  if (scale.numNotes > 0) {
    if (pitchChangeDens > 7) {
      availablePitches = scale.numNotes - 1;
    } else if (pitchChangeDens < 2) {
      availablePitches = pitchChangeDens;
    } else {
      int rangeFromScale = scale.numNotes - 3;
      if (rangeFromScale < 4) {
        rangeFromScale = 4;
      }
      availablePitches = 3 + ((pitchChangeDens - 3) * rangeFromScale / 4);
      availablePitches = constrain(availablePitches, 1, scale.numNotes - 1);
    }
  }

  tb3po.oct_ups = 0;
  tb3po.oct_downs = 0;

  for (int s = 0; s < TB3PO_MAX_STEPS; ++s) {
    int forceRepeatProb = 50 - (getPitchChangeDensity() * 6);
    if (s > 0 && randBit(forceRepeatProb)) {
      tb3po.notes[s] = tb3po.notes[s - 1];
    } else {
      tb3po.notes[s] = random(0, availablePitches + 1);
      tb3po.oct_ups <<= 1;
      tb3po.oct_downs <<= 1;
      if (randBit(40)) {
        if (randBit(50)) {
          tb3po.oct_ups |= 0x1;
        } else {
          tb3po.oct_downs |= 0x1;
        }
      }
    }
  }
}

static void applyDensity() {
  randomSeed(tb3po.seed + 1000);
  int onOffDens = getOnOffDensity();
  int densProb = 10 + onOffDens * 6;
  int latestSlide = 0;
  int latestAccent = 0;
  tb3po.gates = 0;
  tb3po.slides = 0;
  tb3po.accents = 0;

  for (int i = 0; i < TB3PO_MAX_STEPS; ++i) {
    tb3po.gates <<= 1;
    if (randBit(densProb)) {
      tb3po.gates |= 1;
    }
    tb3po.slides <<= 1;
    latestSlide = randBit(latestSlide ? 10 : 18);
    if (latestSlide) {
      tb3po.slides |= 1;
    }
    tb3po.accents <<= 1;
    latestAccent = randBit(latestAccent ? 7 : 16);
    if (latestAccent) {
      tb3po.accents |= 1;
    }
  }
}

static void regenerateAll() {
  reseed();
  regeneratePitches();
  applyDensity();
}

static int getMIDINoteForStep(int stepNum) {
  const Scale &scale = scales[tb3po.scaleIndex];
  int noteIndex = 60 + tb3po.notes[stepNum] + tb3po.rootNote;
  noteIndex += (tb3po.octaveOffset * scale.numNotes);
  if (tb3po.oct_ups & (1 << stepNum)) {
    noteIndex += scale.numNotes;
  } else if (tb3po.oct_downs & (1 << stepNum)) {
    noteIndex -= scale.numNotes;
  }
  noteIndex = constrain(noteIndex, 0, 127);
  return getNoteInScale(tb3po.scaleIndex, noteIndex - 60, 3); // Changed from 4 to 3 to lower by 1 octave
}

static bool stepIsGated(int stepNum) {
  return (tb3po.gates & (1 << stepNum)) != 0;
}

static bool stepIsSlid(int stepNum) {
  return (tb3po.slides & (1 << stepNum)) != 0;
}

static bool stepIsAccent(int stepNum) {
  return (tb3po.accents & (1 << stepNum)) != 0;
}

void updateTB3POPlayback() {
  bool wasPlaying = tb3poSync.playing;
  bool justStarted = tb3poSync.tryStartIfReady(!instantStartMode) && !wasPlaying;
  if (justStarted) {
    uint32_t tick = clockManagerGetTickCount();
    uint32_t elapsedMs = (tb3poSync.startRequestMs == UINT32_MAX)
                             ? 0
                             : millis() - tb3poSync.startRequestMs;
    Serial.printf("[TB3PO] justStarted tick=%u stepBeforeReset=%u startDelayMs=%u\n", tick,
                  tb3po.step, elapsedMs);
    tb3po.step = 0;
    tb3po.currentNote = -1;
    // Clear any accumulated steps to prevent catch-up notes
    if (tb3po.useInternalClock) {
      noInterrupts();
      tb3poStepCount = 0;
      interrupts();
    }
  }
  if (!tb3poSync.playing) {
    return;
  }
  uint32_t readySteps = 0;
  if (tb3po.useInternalClock) {
    if (tb3poAssignedFlag) {
      Serial.printf("[TB3PO] assignedTrack=%u requested=%u\n", (unsigned)tb3poAssignedTrack, (unsigned)tb3poRequestedTracks);
      tb3poAssignedFlag = false;
    }
    noInterrupts();
    readySteps = tb3poStepCount;
    tb3poStepCount = 0;
    interrupts();
  } else {
    readySteps = tb3poSync.consumeReadySteps();
  }
  if (readySteps == 0) {
    return;
  }

  for (uint32_t i = 0; i < readySteps; ++i) {
    uint32_t currentTick = clockManagerGetTickCount();
    bool gated = stepIsGated(tb3po.step);
    bool slid = stepIsSlid(tb3po.step);
    Serial.printf("[TB3PO] tick=%u step=%u gate=%d slide=%d playing=%d\n", currentTick, tb3po.step, gated, slid,
                  tb3poSync.playing);
    
    // Only send note-off if not sliding
    if (tb3po.currentNote >= 0 && !slid) {
      sendMIDI(0x80, tb3po.currentNote, 0);
      tb3po.currentNote = -1;
    }
    
    if (gated) {
      int note = getMIDINoteForStep(tb3po.step);
      int velocity = stepIsAccent(tb3po.step) ? 127 : 100;
      
      // If sliding and note is different, send note-off for old note first
      if (slid && tb3po.currentNote >= 0 && tb3po.currentNote != note) {
        sendMIDI(0x80, tb3po.currentNote, 0);
      }
      
      sendMIDI(0x90, note, velocity);
      tb3po.currentNote = note;
    }
    
    tb3po.step++;
    if (tb3po.step >= tb3po.numSteps) {
      tb3po.step = 0;
    }
    Serial.printf("[TB3PO] tick=%u numSteps=%u stepAfterWrap=%u\n", currentTick, tb3po.numSteps,
                  tb3po.step);
  }
  requestRedraw();  // Request redraw to show step progress and pattern
}

void drawTB3POMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("TB-3PO", "Acid Pattern Generator", 3);
  int y = HEADER_HEIGHT + SCALE_Y(4);
  
  // Status line with playing state and seed info grouped
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String status = tb3poSync.playing ? "PLAYING" : (tb3poSync.startPending ? "WAITING" : "STOPPED");
  tft.drawString(status, MARGIN_SMALL, y, 2);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Seed: " + String(tb3po.seed), DISPLAY_WIDTH - SCALE_X(90), y, 1);
  y += SCALE_Y(16);

  // Draw 4-row pattern visualization (like TB-303)
  int stepW = SCALE_X(18);
  int rowH = SCALE_Y(20);  // Increased from 15 to 20 for more space
  int spacing = SCALE_X(1);
  int startX = SCALE_X(10);
  int patternY = y + SCALE_Y(2);
  
  // Row headers
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Note", SCALE_X(2), patternY, 1);
  tft.drawString("Up/Dn", SCALE_X(2), patternY + rowH, 1);
  tft.drawString("A/S", SCALE_X(2), patternY + rowH * 2, 1);
  tft.drawString("Gate", SCALE_X(2), patternY + rowH * 3, 1);
  
  for (int step = 0; step < TB3PO_MAX_STEPS; step++) {
    int x = startX + step * (stepW + spacing);
    bool isGated = stepIsGated(step);
    bool isCurrent = (tb3poSync.playing && step == tb3po.step);
    bool isAccent = stepIsAccent(step);
    bool isSlide = stepIsSlid(step);
    bool hasOctUp = (tb3po.oct_ups & (1 << step)) != 0;
    bool hasOctDown = (tb3po.oct_downs & (1 << step)) != 0;
    
    // Determine colors
    uint16_t noteBgColor = isCurrent ? THEME_ACCENT : THEME_SURFACE;
    uint16_t noteTextColor = isCurrent ? THEME_BG : THEME_TEXT;
    uint16_t transposeColor = hasOctUp ? THEME_SUCCESS : (hasOctDown ? THEME_WARNING : THEME_SURFACE);
    uint16_t accentSlideColor = isAccent ? THEME_WARNING : (isSlide ? THEME_PRIMARY : THEME_SURFACE);
    uint16_t gateColor = isGated ? THEME_PRIMARY : THEME_SURFACE;
    
    // Row 1: Note name
    tft.fillRect(x, patternY, stepW, rowH, noteBgColor);
    tft.drawRect(x, patternY, stepW, rowH, THEME_TEXT_DIM);
    int midiNote = getMIDINoteForStep(step);
    String noteName = getNoteNameFromMIDI(midiNote);
    // Remove octave number from display to keep it concise
    if (noteName.length() > 0) {
      int octavePos = noteName.length() - 1;
      if (noteName[octavePos] >= '0' && noteName[octavePos] <= '9') {
        noteName = noteName.substring(0, octavePos);
      }
    }
    tft.setTextColor(noteTextColor, noteBgColor);
    tft.drawCentreString(noteName, x + stepW / 2, patternY + SCALE_Y(3), 1);
    
    // Row 2: Transpose up/down
    tft.fillRect(x, patternY + rowH, stepW, rowH, transposeColor);
    tft.drawRect(x, patternY + rowH, stepW, rowH, THEME_TEXT_DIM);
    if (hasOctUp || hasOctDown) {
      tft.setTextColor(THEME_BG, transposeColor);
      tft.drawCentreString(hasOctUp ? "+" : "-", x + stepW / 2, patternY + rowH + SCALE_Y(3), 1);
    }
    
    // Row 3: Accent/Slide
    tft.fillRect(x, patternY + rowH * 2, stepW, rowH, accentSlideColor);
    tft.drawRect(x, patternY + rowH * 2, stepW, rowH, THEME_TEXT_DIM);
    if (isAccent || isSlide) {
      tft.setTextColor(THEME_BG, accentSlideColor);
      String accentSlideText = "";
      if (isAccent && isSlide) accentSlideText = "A/S";
      else if (isAccent) accentSlideText = "A";
      else if (isSlide) accentSlideText = "S";
      tft.drawCentreString(accentSlideText, x + stepW / 2, patternY + rowH * 2 + SCALE_Y(3), 1);
    }
    
    // Row 4: Gate/timing (filled circle)
    tft.fillRect(x, patternY + rowH * 3, stepW, rowH, THEME_BG);
    tft.drawRect(x, patternY + rowH * 3, stepW, rowH, THEME_TEXT_DIM);
    if (isGated) {
      int circleRadius = SCALE_X(4);
      tft.fillCircle(x + stepW / 2, patternY + rowH * 3 + rowH / 2, circleRadius, gateColor);
    }
  }
  
  // Position controls at bottom - group SEED and REGEN together
  int btnH = SCALE_Y(44);
  int btnSpacing = SCALE_X(6);
  int btnY = DISPLAY_HEIGHT - btnH - SCALE_Y(6);
  
  // Left side: Play control
  int playBtnW = SCALE_X(60);
  drawRoundButton(MARGIN_SMALL, btnY, playBtnW, btnH,
                  tb3poSync.playing || tb3poSync.startPending ? "STOP" : "PLAY",
                  THEME_PRIMARY, false, 2);
  
  // Center: Density controls (grouped)
  int densStartX = MARGIN_SMALL + playBtnW + btnSpacing + SCALE_X(4);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("DENSITY", densStartX, btnY - SCALE_Y(14), 1);
  int densBtnW = SCALE_X(50);
  drawRoundButton(densStartX, btnY, densBtnW, btnH, "-", THEME_WARNING, false, 4);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawCentreString(String(tb3po.density), densStartX + densBtnW + SCALE_X(18), btnY + SCALE_Y(14), 4);
  drawRoundButton(densStartX + densBtnW + SCALE_X(36), btnY, densBtnW, btnH, "+", THEME_ACCENT, false, 4);
  
  // Right side: SEED and REGEN grouped
  int seedRegenX = DISPLAY_WIDTH - SCALE_X(66);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("SEED", seedRegenX, btnY - SCALE_Y(14), 1);
  drawRoundButton(seedRegenX, btnY, SCALE_X(60), btnH, "REGEN", THEME_SECONDARY, false, 2);
}

void initializeTB3POMode() {
  tb3po.step = 0;
  tb3po.currentNote = -1;
  tb3poSync.reset();
  tb3po.readyForInput = false;
  tb3po.density = 16;
  tb3po.scaleIndex = 0;
  tb3po.rootNote = 0;
  tb3po.octaveOffset = 0;
  tb3po.lockSeed = false;
  tb3po.numSteps = TB3PO_MAX_STEPS;
  tb3po.bpm = 120.0f;
  regenerateAll();
  // Step callback registration is done at startup via registerAllStepCallbacks().
  drawTB3POMode();
}

void registerTB3POStepCallback() {
  uClock.setOnStep(onTb3poStepISR, 1);
}

void handleTB3POMode() {
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  updateTB3POPlayback();
  if (touch.justPressed) {
    // Calculate button positions (must match drawTB3POMode)
    int btnH = SCALE_Y(44);
    int btnSpacing = SCALE_X(6);
    int btnY = DISPLAY_HEIGHT - btnH - SCALE_Y(6);
    
    // Play button (left)
    int playBtnW = SCALE_X(60);
    if (isButtonPressed(MARGIN_SMALL, btnY, playBtnW, btnH)) {
      if (tb3poSync.playing || tb3poSync.startPending) {
        tb3poSync.stopPlayback();
      } else {
        tb3poSync.requestStart();
      }
      requestRedraw();
      return;
    }
    
    // Density controls (center)
    int densStartX = MARGIN_SMALL + playBtnW + btnSpacing + SCALE_X(4);
    int densBtnW = SCALE_X(50);
    if (isButtonPressed(densStartX, btnY, densBtnW, btnH)) {
      tb3po.density = max(0, tb3po.density - 1);
      regenerateAll();
      requestRedraw();
      return;
    }
    if (isButtonPressed(densStartX + densBtnW + SCALE_X(36), btnY, densBtnW, btnH)) {
      tb3po.density = min(16, tb3po.density + 1);
      regenerateAll();
      requestRedraw();
      return;
    }
    
    // REGEN button (right)
    int seedRegenX = DISPLAY_WIDTH - SCALE_X(66);
    if (isButtonPressed(seedRegenX, btnY, SCALE_X(60), btnH)) {
      regenerateAll();
      requestRedraw();
      return;
    }
  }
}
