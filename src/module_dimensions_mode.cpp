#include "module_dimensions_mode.h"
#include <algorithm>
#include <cstring>
#include <cmath>

DimensionsState dimensionsState;
static SequencerSyncState dimensionsSync;

// Default clock divisions (in MIDI clock ticks)
// Corresponds to: 1/16, 1/12, 1/8, 1/6, 1/4, 1/3, 1/2, 3/4, 1/1, custom
static const int DEFAULT_INTERVAL_DIVISIONS[DIMENSIONS_INTERVAL_DIVISIONS] = {
  CLOCK_TICKS_PER_SIXTEENTH,      // 1/16 = 6 ticks
  CLOCK_TICKS_PER_QUARTER / 3,    // 1/12 = 8 ticks  
  CLOCK_TICKS_PER_QUARTER / 2,    // 1/8 = 12 ticks
  CLOCK_TICKS_PER_QUARTER * 2 / 3,  // 1/6 = 16 ticks
  CLOCK_TICKS_PER_QUARTER,        // 1/4 = 24 ticks
  CLOCK_TICKS_PER_QUARTER * 4 / 3,  // 1/3 = 32 ticks
  CLOCK_TICKS_PER_QUARTER * 2,    // 1/2 = 48 ticks
  CLOCK_TICKS_PER_QUARTER * 3,    // 3/4 = 72 ticks
  CLOCK_TICKS_PER_QUARTER * 4,    // 1/1 = 96 ticks
  CLOCK_TICKS_PER_QUARTER         // Custom (same as 1/4 initially)
};

void initializeDimensionsMode() {
  // Reset sync state
  dimensionsSync.reset();
  
  // Default equation
  dimensionsState.fx_select = 1;
  
  // Default parameters
  dimensionsState.a = 10.0f;
  dimensionsState.b = 10.0f;
  dimensionsState.c = 1.0f;
  dimensionsState.d = 1.0f;
  dimensionsState.rnd = 0.0f;
  
  // Time settings
  dimensionsState.t = 50.0f;
  dimensionsState.value_t1 = 0.0f;
  dimensionsState.value_t2 = 100.0f;
  dimensionsState.value_gran = 0.0125f;  // Default granularity
  
  // Axis offsets
  dimensionsState.x = 64.0f;
  dimensionsState.y = 64.0f;
  
  // Initialize interval divisions
  memcpy(dimensionsState.intervalDivisions, DEFAULT_INTERVAL_DIVISIONS, 
         sizeof(DEFAULT_INTERVAL_DIVISIONS));
  
  // Initialize note subset - enable one octave (C4-B4, MIDI 60-71)
  memset(dimensionsState.enabledNotes, 0, sizeof(dimensionsState.enabledNotes));
  for (int i = 48; i < 60; i++) {  // C3-B3 (MIDI 60-71 offset by 12)
    dimensionsState.enabledNotes[i] = true;
  }
  
  // Build note subset
  dimensionsState.noteSubsetSize = 0;
  for (int i = 0; i < DIMENSIONS_MAX_NOTES; i++) {
    if (dimensionsState.enabledNotes[i]) {
      dimensionsState.noteSubset[dimensionsState.noteSubsetSize++] = i + 12;  // MIDI note number
    }
  }
  
  // MIDI settings
  dimensionsState.midiChannel = 1;
  dimensionsState.octaveTranspose = 0;
  dimensionsState.velocityMode = 1;  // Auto velocity from equation
  dimensionsState.pz_offset = 0;
  
  // Trigger settings
  dimensionsState.triggerMode = 0;
  dimensionsState.pyRest = 0.0f;
  
  // Playback state
  dimensionsState.currentClkDiv = CLOCK_TICKS_PER_SIXTEENTH;
  dimensionsState.lastNotePlayed = 0;
  dimensionsState.noteActive = false;
  
  // Visual
  dimensionsState.traceMode = 0;
  
  // Initial equation output
  dimensionsState.px = 64.0f;
  dimensionsState.py = 64.0f;
  dimensionsState.pz = 64.0f;
  
  drawDimensionsMode();
}

// Core parametric equation evaluator
// Ported from ParametricFunctions.ino
void dimensionsEvaluateEquation(int equationNum, float t, float a, float b, float c, float d, float rnd,
                                float x, float y, float &px_out, float &py_out, float &pz_out) {
  // Avoid division by zero
  if (a == 0) a = 0.001f;
  if (b == 0) b = 0.001f;
  if (c == 0) c = 0.001f;
  if (d == 0) d = 0.001f;
  
  float px_fl, py_fl, pz_fl;
  
  switch (equationNum) {
    case 1: // Lissajous
      px_fl = x + t * sin(a * t + PI/2.0f);
      py_fl = y + t * sin(b * t);
      pz_fl = t * cos(a * t / PI * c);
      break;
      
    case 2:
      px_fl = x + t * sin(a * t + PI/2.0f) + cos(sq(a+b)/PI) * tan(a);
      py_fl = y + t * sin(b * t) * (tan((t * a)/(a-b)) / (b * 4.0f));
      pz_fl = t * cos(a * t / PI);
      break;
      
    case 3:
      px_fl = x + t * sin(a * t + PI/(c + 1.0f)) + cos(sq(a+b)/(c + 0.5f));
      py_fl = y + t * sin(c * t) * (tan((t * a)/(c-b + 0.12f)) / ((b * 4.0f) + 0.5f));
      pz_fl = t * sin(a * t / PI);
      break;
      
    case 4:
      px_fl = x + t * sin(a * t + PI/c) + cos(sq(PI/2.0f)/c);
      py_fl = y + t * sin(b * t/2.0f) * cos(t/PI * a);
      pz_fl = t * sin(c * t / PI);
      break;
      
    case 5: // With randomness
      px_fl = x + t * sin(a * t/2.0f) + (rnd * (PI/random(2,30) * 1.75f));
      py_fl = y + t * sin(b * t) + (rnd * (PI/random(5,25) * c));
      pz_fl = t * cos(a * t / PI * c);
      break;
      
    case 6: // Pure random
      px_fl = x + random(-64,64);
      py_fl = y + random(-64,64);
      pz_fl = t * random(0,127);
      break;
      
    case 7:
      px_fl = x + t * sin(a * t + PI/3.0f) * (PI / pow(b, 2.0f));
      py_fl = y + t * sin(b * t);
      pz_fl = t * cos(a * t / PI * c * t);
      break;
      
    case 8:
      px_fl = x + t * sin(a * t + PI/2.0f) - (t * cos(a * t + PI/4.0f));
      py_fl = y + t * sin(b * t) + (y - t * cos(0.25f * b * t));
      pz_fl = t * cos(a * t / PI * c);
      break;
      
    case 9:
      px_fl = x + t * sin(a * t + PI/25.0f) * (d * 0.5f);
      py_fl = y + t * cos(-b * t + PI/130.0f) * (d * 0.5f) + t * sin(a * t + PI/25.0f);
      pz_fl = t * sin(t / PI * c);
      break;
      
    case 10:
      px_fl = x + t * sin(a * t + PI/2.0f) * log(16.0f)/b * 2.0f;
      py_fl = y + t * sin(b * t) + sin(d * t * 100.0f) + tan(PI * t);
      pz_fl = t * 0.5f * sin(t / PI * c + 0.25f);
      break;
      
    case 11:
      px_fl = x + t * sin(a * t + PI/3.0f * 0.025f);
      py_fl = y + t * sin(b * 0.05f * t * cos(b * t * 0.05f)) + rnd;
      pz_fl = t * tan(a * t / PI * c * 0.025f) + rnd * 1.0f + d;
      break;
      
    case 12:
      px_fl = x + t * sin(a * t + PI/2.0f);
      py_fl = y + random(-64,64) * rnd * 1.0f/64.0f;
      pz_fl = t * cos(d * t / PI * c) * 1.0f + b * sin(t * 2.25f);
      break;
      
    case 13:
      if (b == 0) b = 1.0f;
      px_fl = x + t * sin(((int)a % (int)b) * t) + sin(PI/3.0f * c);
      py_fl = y + t * sin(t * c) + cos((int)a % (int)b) * 20.25f;
      pz_fl = t * cos(a * t / PI);
      break;
      
    case 14:
      px_fl = x + random(-64,64) * rnd * 1.0f/64.0f;
      py_fl = y + t * sin(a * t + PI/2.0f);
      pz_fl = t * sin(b * t / PI) * 2.0f + b * cos(t * 20.0f);
      break;
      
    case 15:
      px_fl = x + t * tan(a * t + PI * sin(b * t)) * 0.75f;
      py_fl = y + t * tan(b * t) - cos(a * t * 0.5f) * 0.75f;
      pz_fl = t * cos(2.0f * a * t / PI * c);
      break;
      
    case 16:
      if (c == 0) c = 1.0f;
      px_fl = x + t * sin(a * t + PI/3.0f) - (1.75f * t) * sin(1.5f * a * t + PI/3.0f) * 1.0f/c * 0.5f;
      py_fl = y + t * sin(b * t + PI/3.0f) - (1.75f * t) * sin(1.5f * b * t + PI/3.0f) * 1.0f/c * 0.5f;
      pz_fl = y + t * sin(d * t + PI/3.0f) - (1.75f * t) * cos(1.5f * d * t + PI/2.0f) * 1.0f/c * 0.5f;
      break;
      
    case 17:
      px_fl = x + t * sin(a * t + PI);
      py_fl = y + t * tan(b * t - PI);
      pz_fl = t * cos(a * t / PI * c);
      break;
      
    case 18: // Random with rnd control
      px_fl = x + random(-abs((int)rnd), abs((int)rnd));
      py_fl = y + random(-abs((int)rnd), abs((int)rnd));
      pz_fl = t * random(0, abs((int)rnd));
      break;
      
    case 19:
      if (c == 0) c = 1.0f;
      px_fl = x + t * sin(a * t + PI/2.0f) + cos(sq(a+b)/PI) * tan(a) + rnd;
      py_fl = y + t * sin(b/c * t) * (tan((t * a)/(a+b)) / (b * 4.0f)) + d/2.25f;
      pz_fl = t * tan(c * t / PI);
      break;
      
    case 20:
      px_fl = x + t * sin(a * t + (PI / pow(c, 3.0f)));
      py_fl = y + (t * sin(b * t/2.0f) * (tan(t/PI * a) * 0.25f)) - d;
      pz_fl = t/2.25f * tan(c * t/4.0f) * rnd;
      break;
      
    default:
      // Default: dot in middle
      px_fl = x;
      py_fl = y;
      pz_fl = 64.0f;
      break;
  }
  
  // Guard boundaries (0-127)
  px_out = constrain(px_fl, 0.0f, 127.0f);
  py_out = constrain(py_fl, 0.0f, 127.0f);
  pz_out = constrain(pz_fl, 0.0f, 127.0f);
}

void dimensionsPlayNote() {
  if (dimensionsState.noteSubsetSize == 0) {
    return;  // No notes enabled
  }
  
  // Release previous note if active
  if (dimensionsState.noteActive) {
    dimensionsReleaseNote();
  }
  
  // Map py (0-127) to note subset
  int noteIndex = map((int)dimensionsState.py, 0, 127, 
                      dimensionsState.noteSubsetSize - 1, 0);
  noteIndex = constrain(noteIndex, 0, dimensionsState.noteSubsetSize - 1);
  int midiNote = dimensionsState.noteSubset[noteIndex];
  
  // Apply transpose
  midiNote += dimensionsState.octaveTranspose;
  midiNote = constrain(midiNote, 0, 127);
  
  // Map pz to velocity
  int velocity;
  if (dimensionsState.velocityMode == 0) {
    velocity = 127;  // Fixed velocity
  } else {
    velocity = (int)dimensionsState.pz + dimensionsState.pz_offset;
    velocity = constrain(velocity, 1, 127);
  }
  
  // Send note on
  sendMIDI(0x90 | (dimensionsState.midiChannel - 1), midiNote, velocity);
  
  dimensionsState.lastNotePlayed = midiNote;
  dimensionsState.noteActive = true;
}

void dimensionsReleaseNote() {
  if (dimensionsState.noteActive && dimensionsState.lastNotePlayed > 0) {
    sendMIDI(0x80 | (dimensionsState.midiChannel - 1), dimensionsState.lastNotePlayed, 0);
    dimensionsState.noteActive = false;
  }
}

void dimensionsResetSequencer() {
  dimensionsReleaseNote();
  dimensionsState.t = dimensionsState.value_t1;
  dimensionsSync.reset();
}

void updateDimensionsSequencer() {
  // Handle start request
  if (!dimensionsSync.playing && !dimensionsSync.startPending) {
    return;  // Not playing
  }
  
  // Try to start if pending
  if (dimensionsSync.startPending) {
    if (!dimensionsSync.tryStartIfReady(true)) {
      return;  // Waiting for bar start
    }
    // Just started - reset t
    dimensionsState.t = dimensionsState.value_t1;
  }
  
  // Check if we have a step to process
  uint32_t steps = dimensionsSync.consumeReadySteps(dimensionsState.currentClkDiv);
  if (steps == 0) {
    return;  // No step yet
  }
  
  // Process each step
  for (uint32_t i = 0; i < steps; i++) {
    // Evaluate parametric equation
    dimensionsEvaluateEquation(
      dimensionsState.fx_select,
      dimensionsState.t,
      dimensionsState.a,
      dimensionsState.b,
      dimensionsState.c,
      dimensionsState.d,
      dimensionsState.rnd,
      dimensionsState.x,
      dimensionsState.y,
      dimensionsState.px,
      dimensionsState.py,
      dimensionsState.pz
    );
    
    // Map px to interval division (for next step timing)
    int intervalIndex = map((int)dimensionsState.px, 0, 127, 0, DIMENSIONS_INTERVAL_DIVISIONS - 1);
    intervalIndex = constrain(intervalIndex, 0, DIMENSIONS_INTERVAL_DIVISIONS - 1);
    dimensionsState.currentClkDiv = dimensionsState.intervalDivisions[intervalIndex];
    
    // Check trigger condition
    bool shouldTrigger = false;
    if (dimensionsState.triggerMode == 0) {
      shouldTrigger = (dimensionsState.py >= dimensionsState.pyRest);
    } else {
      shouldTrigger = (dimensionsState.py <= dimensionsState.pyRest);
    }
    
    if (shouldTrigger) {
      dimensionsPlayNote();
    }
    
    // Advance time parameter
    if (dimensionsState.value_t1 <= dimensionsState.value_t2) {
      dimensionsState.t += dimensionsState.value_gran;
      if (dimensionsState.t >= dimensionsState.value_t2) {
        dimensionsState.t = dimensionsState.value_t1;
      }
    } else {
      dimensionsState.t -= dimensionsState.value_gran;
      if (dimensionsState.t <= dimensionsState.value_t2) {
        dimensionsState.t = dimensionsState.value_t1;
      }
    }
  }
  
  requestRedraw();
}

void drawDimensionsMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("DIMENSIONS", "Parametric Sequencer", 3);
  
  // Main display area
  int mainY = HEADER_HEIGHT + SCALE_Y(5);
  
  // Current equation (larger, prominent)
  tft.setTextColor(THEME_PRIMARY, THEME_BG);
  String eqText = "f(x) #" + String(dimensionsState.fx_select);
  tft.drawString(eqText, SCALE_X(10), mainY, 2);
  
  // Equation selector buttons
  int eqBtnY = mainY - SCALE_Y(2);
  drawRoundButton(SCALE_X(80), eqBtnY, SCALE_X(25), SCALE_Y(18), "-", THEME_SECONDARY, false);
  drawRoundButton(SCALE_X(110), eqBtnY, SCALE_X(25), SCALE_Y(18), "+", THEME_SECONDARY, false);
  
  // Parameters section
  int paramY = mainY + SCALE_Y(25);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Parameters:", SCALE_X(10), paramY, 1);
  
  paramY += SCALE_Y(15);
  
  // Parameter A
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("A: " + String(dimensionsState.a, 1), SCALE_X(10), paramY, 1);
  drawRoundButton(SCALE_X(80), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "-", THEME_SURFACE, false);
  drawRoundButton(SCALE_X(110), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "+", THEME_SURFACE, false);
  
  paramY += SCALE_Y(18);
  
  // Parameter B
  tft.drawString("B: " + String(dimensionsState.b, 1), SCALE_X(10), paramY, 1);
  drawRoundButton(SCALE_X(80), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "-", THEME_SURFACE, false);
  drawRoundButton(SCALE_X(110), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "+", THEME_SURFACE, false);
  
  paramY += SCALE_Y(18);
  
  // Parameter C
  tft.drawString("C: " + String(dimensionsState.c, 1), SCALE_X(10), paramY, 1);
  drawRoundButton(SCALE_X(80), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "-", THEME_SURFACE, false);
  drawRoundButton(SCALE_X(110), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "+", THEME_SURFACE, false);
  
  paramY += SCALE_Y(18);
  
  // Parameter D
  tft.drawString("D: " + String(dimensionsState.d, 1), SCALE_X(10), paramY, 1);
  drawRoundButton(SCALE_X(80), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "-", THEME_SURFACE, false);
  drawRoundButton(SCALE_X(110), paramY - SCALE_Y(2), SCALE_X(25), SCALE_Y(15), "+", THEME_SURFACE, false);
  
  // Right side - Output values and status
  int rightX = SCALE_X(155);
  int rightY = mainY + SCALE_Y(25);
  
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Output:", rightX, rightY, 1);
  
  rightY += SCALE_Y(15);
  tft.setTextColor(THEME_ACCENT, THEME_BG);
  tft.drawString("px: " + String((int)dimensionsState.px), rightX, rightY, 1);
  
  rightY += SCALE_Y(15);
  tft.drawString("py: " + String((int)dimensionsState.py), rightX, rightY, 1);
  
  rightY += SCALE_Y(15);
  tft.drawString("pz: " + String((int)dimensionsState.pz), rightX, rightY, 1);
  
  // Time parameter
  rightY += SCALE_Y(20);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Time:", rightX, rightY, 1);
  
  rightY += SCALE_Y(15);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("t: " + String((int)dimensionsState.t), rightX, rightY, 1);
  
  // Transport status
  rightY += SCALE_Y(20);
  uint16_t statusColor = dimensionsSync.playing ? THEME_SUCCESS : THEME_TEXT_DIM;
  tft.setTextColor(statusColor, THEME_BG);
  String statusText;
  if (dimensionsSync.playing) {
    statusText = "PLAY";
  } else if (dimensionsSync.startPending) {
    statusText = "START";
  } else {
    statusText = "STOP";
  }
  tft.drawString(statusText, rightX, rightY, 1);
  
  // Note info
  rightY += SCALE_Y(15);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("N: " + String(dimensionsState.noteSubsetSize), rightX, rightY, 1);
  
  // Transport buttons at bottom
  int btnY = DISPLAY_HEIGHT - SCALE_Y(35);
  
  // Start/Stop button
  uint16_t btnColor = dimensionsSync.playing ? THEME_ERROR : THEME_SUCCESS;
  const char* btnText = dimensionsSync.playing ? "STOP" : "START";
  drawRoundButton(SCALE_X(10), btnY, SCALE_X(60), SCALE_Y(25), btnText, btnColor, false);
  
  // Reset button
  drawRoundButton(SCALE_X(80), btnY, SCALE_X(50), SCALE_Y(25), "RST", THEME_WARNING, false);
  
  // Settings button
  drawRoundButton(SCALE_X(140), btnY, SCALE_X(50), SCALE_Y(25), "SET", THEME_SURFACE, false);
  
  // Back/Menu button
  drawRoundButton(SCALE_X(200), btnY, SCALE_X(50), SCALE_Y(25), "MENU", THEME_SECONDARY, false);
}

void handleDimensionsMode() {
  updateTouch();
  
  if (touch.justPressed) {
    int mainY = HEADER_HEIGHT + SCALE_Y(5);
    int eqBtnY = mainY - SCALE_Y(2);
    
    // Equation selector buttons
    if (touch.y >= eqBtnY && touch.y <= eqBtnY + SCALE_Y(18)) {
      // Previous equation
      if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(105)) {
        dimensionsState.fx_select--;
        if (dimensionsState.fx_select < 1) dimensionsState.fx_select = DIMENSIONS_NUM_EQUATIONS;
        requestRedraw();
        return;
      }
      // Next equation
      if (touch.x >= SCALE_X(110) && touch.x <= SCALE_X(135)) {
        dimensionsState.fx_select++;
        if (dimensionsState.fx_select > DIMENSIONS_NUM_EQUATIONS) dimensionsState.fx_select = 1;
        requestRedraw();
        return;
      }
    }
    
    int paramY = mainY + SCALE_Y(40);
    
    // Parameter A controls
    if (touch.y >= paramY - SCALE_Y(2) && touch.y <= paramY + SCALE_Y(13)) {
      if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(105)) {
        dimensionsState.a = max(0.0f, dimensionsState.a - 1.0f);
        requestRedraw();
        return;
      }
      if (touch.x >= SCALE_X(110) && touch.x <= SCALE_X(135)) {
        dimensionsState.a = min(100.0f, dimensionsState.a + 1.0f);
        requestRedraw();
        return;
      }
    }
    
    paramY += SCALE_Y(18);
    
    // Parameter B controls
    if (touch.y >= paramY - SCALE_Y(2) && touch.y <= paramY + SCALE_Y(13)) {
      if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(105)) {
        dimensionsState.b = max(0.0f, dimensionsState.b - 1.0f);
        requestRedraw();
        return;
      }
      if (touch.x >= SCALE_X(110) && touch.x <= SCALE_X(135)) {
        dimensionsState.b = min(100.0f, dimensionsState.b + 1.0f);
        requestRedraw();
        return;
      }
    }
    
    paramY += SCALE_Y(18);
    
    // Parameter C controls
    if (touch.y >= paramY - SCALE_Y(2) && touch.y <= paramY + SCALE_Y(13)) {
      if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(105)) {
        dimensionsState.c = max(0.0f, dimensionsState.c - 1.0f);
        requestRedraw();
        return;
      }
      if (touch.x >= SCALE_X(110) && touch.x <= SCALE_X(135)) {
        dimensionsState.c = min(100.0f, dimensionsState.c + 1.0f);
        requestRedraw();
        return;
      }
    }
    
    paramY += SCALE_Y(18);
    
    // Parameter D controls
    if (touch.y >= paramY - SCALE_Y(2) && touch.y <= paramY + SCALE_Y(13)) {
      if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(105)) {
        dimensionsState.d = max(0.0f, dimensionsState.d - 1.0f);
        requestRedraw();
        return;
      }
      if (touch.x >= SCALE_X(110) && touch.x <= SCALE_X(135)) {
        dimensionsState.d = min(100.0f, dimensionsState.d + 1.0f);
        requestRedraw();
        return;
      }
    }
    
    int btnY = DISPLAY_HEIGHT - SCALE_Y(35);
    
    // Start/Stop button
    if (touch.x >= SCALE_X(10) && touch.x <= SCALE_X(70) &&
        touch.y >= btnY && touch.y <= btnY + SCALE_Y(25)) {
      if (dimensionsSync.playing) {
        dimensionsSync.stopPlayback();
        dimensionsReleaseNote();
      } else {
        dimensionsSync.requestStart();
      }
      requestRedraw();
      return;
    }
    
    // Reset button
    if (touch.x >= SCALE_X(80) && touch.x <= SCALE_X(130) &&
        touch.y >= btnY && touch.y <= btnY + SCALE_Y(25)) {
      dimensionsResetSequencer();
      requestRedraw();
      return;
    }
    
    // Settings button (future expansion)
    if (touch.x >= SCALE_X(140) && touch.x <= SCALE_X(190) &&
        touch.y >= btnY && touch.y <= btnY + SCALE_Y(25)) {
      // TODO: Open settings page for note subset, intervals, etc.
      requestRedraw();
      return;
    }
    
    // Menu button
    if (touch.x >= SCALE_X(200) && touch.x <= SCALE_X(250) &&
        touch.y >= btnY && touch.y <= btnY + SCALE_Y(25)) {
      dimensionsSync.stopPlayback();
      dimensionsReleaseNote();
      exitToMenu();
      return;
    }
  }
  
  // Update sequencer
  updateDimensionsSequencer();
}
