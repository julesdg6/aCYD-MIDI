#include "module_tb3po_mode.h"

TB3POState tb3po;

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
  return getNoteInScale(tb3po.scaleIndex, noteIndex - 60, 4);
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
  if (!tb3po.playing) {
    return;
  }
  unsigned long now = millis();
  tb3po.stepInterval = (60000.0 / tb3po.bpm) / 4.0;
  if (now - tb3po.lastStepTime < tb3po.stepInterval) {
    return;
  }
  tb3po.lastStepTime = now;
  if (tb3po.currentNote >= 0) {
    sendMIDI(0x80, tb3po.currentNote, 0);
    tb3po.currentNote = -1;
  }
  if (stepIsGated(tb3po.step)) {
    int note = getMIDINoteForStep(tb3po.step);
    int velocity = stepIsAccent(tb3po.step) ? 127 : 100;
    sendMIDI(0x90, note, velocity);
    tb3po.currentNote = note;
  }
  tb3po.step++;
  if (tb3po.step >= tb3po.numSteps) {
    tb3po.step = 0;
  }
}

void drawTB3POMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("TB-3PO", "", 4);
  int y = HEADER_HEIGHT + SCALE_Y(8);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString(tb3po.playing ? "PLAYING" : "STOPPED", SCALE_X(10), y, 2);
  tft.drawString(tb3po.lockSeed ? "SEED LOCKED" : "SEED AUTO", SCALE_X(180), y, 2);
  y += SCALE_Y(18);

  int btnW = SCALE_X(70);
  int btnH = SCALE_Y(28);
  int spacing = SCALE_X(8);
  int startX = MARGIN_SMALL;

  drawRoundButton(startX, y, btnW, btnH, tb3po.playing ? "STOP" : "PLAY", THEME_PRIMARY);
  drawRoundButton(startX + (btnW + spacing), y, btnW, btnH, "REGEN", THEME_SECONDARY);
  drawRoundButton(startX + 2 * (btnW + spacing), y, btnW, btnH, "DENS+", THEME_ACCENT);
  drawRoundButton(startX + 3 * (btnW + spacing), y, btnW, btnH, "DENS-", THEME_WARNING);
}

void initializeTB3POMode() {
  tb3po.step = 0;
  tb3po.playing = false;
  tb3po.currentNote = -1;
  tb3po.lastStepTime = 0;
  tb3po.readyForInput = false;
  tb3po.density = 7;
  tb3po.scaleIndex = 0;
  tb3po.rootNote = 0;
  tb3po.octaveOffset = 0;
  tb3po.lockSeed = false;
  tb3po.numSteps = TB3PO_MAX_STEPS;
  tb3po.bpm = 120.0f;
  tb3po.useInternalClock = true;
  regenerateAll();
  drawTB3POMode();
}

void handleTB3POMode() {
  updateTouch();
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  updateTB3POPlayback();
  if (touch.justPressed) {
    if (isButtonPressed(MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(40), SCALE_X(70), SCALE_Y(28))) {
      tb3po.playing = !tb3po.playing;
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + (SCALE_X(70) + SCALE_X(8)), HEADER_HEIGHT + SCALE_Y(40), SCALE_X(70), SCALE_Y(28))) {
      regenerateAll();
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + 2 * (SCALE_X(70) + SCALE_X(8)), HEADER_HEIGHT + SCALE_Y(40), SCALE_X(70), SCALE_Y(28))) {
      tb3po.density = min(14, tb3po.density + 1);
      regenerateAll();
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + 3 * (SCALE_X(70) + SCALE_X(8)), HEADER_HEIGHT + SCALE_Y(40), SCALE_X(70), SCALE_Y(28))) {
      tb3po.density = max(0, tb3po.density - 1);
      regenerateAll();
      requestRedraw();
      return;
    }
  }
}
