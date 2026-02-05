#include "module_random_generator_mode.h"
#include "clock_manager.h"

RandomGen randomGen;
static SequencerSyncState randomSync;

// Accumulator for subdivision handling (counts incoming 16th-note ticks)
static uint32_t subdivAccumulator = 0;

static bool randomModuleRunning() {
  return randomSync.playing || randomSync.startPending;
}

static uint32_t getRandomStepIntervalTicks() {
  uint32_t denom = randomGen.subdivision;
  if (denom < 4) denom = 4;
  if (denom > 16) denom = 16;
  if (denom == 4) {
    return 4 * CLOCK_TICKS_PER_SIXTEENTH;
  }
  if (denom == 8) {
    return 2 * CLOCK_TICKS_PER_SIXTEENTH;
  }
  return CLOCK_TICKS_PER_SIXTEENTH;
}

static void adjustSharedTempo(int delta) {
  int target = sharedBPM + delta;
  if (target < 40) {
    target = 40;
  }
  if (target > 240) {
    target = 240;
  }
  if (target == sharedBPM) {
    return;
  }
  setSharedBPM(target);
  requestRedraw();
}

// Implementations
void initializeRandomGeneratorMode() {
  randomGen.rootNote = 60;
  randomGen.scaleType = 0;
  randomGen.minOctave = 3;
  randomGen.maxOctave = 6;
  randomGen.probability = 50;
  randomGen.subdivision = 4;
  randomGen.currentNote = -1;
  randomSync.reset();
}

void drawRandomGeneratorMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("RNG JAMS", "Random Music", 3);
  
  drawRandomGenControls();
}

void drawRandomGenControls() {
  int y = HEADER_HEIGHT + SCALE_Y(10);
  int spacing = SCALE_Y(38);
  
  // Row 1: Play/Stop and Scale selector (most important)
  drawRoundButton(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(32), 
                  randomModuleRunning() ? "STOP" : "PLAY", 
                  randomModuleRunning() ? THEME_ERROR : THEME_SUCCESS, false, 2);
  
  int scaleW = DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(75);
  drawRoundButton(MARGIN_SMALL + SCALE_X(75), y, scaleW, SCALE_Y(32), 
                  scales[randomGen.scaleType].name, THEME_ACCENT, false, 2);
  
  y += spacing;
  
  // Row 2: Key and Octave Range (grouped together)
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Key", MARGIN_SMALL, y, 2);
  
  String rootName = getNoteNameFromMIDI(randomGen.rootNote);
  tft.drawString(rootName, MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // Key +/- buttons (small)
  drawRoundButton(MARGIN_SMALL + SCALE_X(55), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28), 
                  "-", THEME_SECONDARY, false, 1);
  drawRoundButton(MARGIN_SMALL + SCALE_X(88), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28), 
                  "+", THEME_SECONDARY, false, 1);
  
  // Octave range - using slider instead of MIN/MAX buttons
  int rangeX = DISPLAY_WIDTH / 2 + SCALE_X(10);
  tft.drawString("Octave", rangeX, y, 2);
  tft.drawString(String(randomGen.minOctave) + "-" + String(randomGen.maxOctave), 
                 rangeX, y + SCALE_Y(16), 4);
  
  y += spacing;
  
  // Row 3: Chance slider (with descriptive text)
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Note probability", MARGIN_SMALL, y, 2);
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String chanceStr = String(randomGen.probability) + "%";
  tft.drawString(chanceStr, MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // Chance slider bar (interactive)
  int sliderX = DISPLAY_WIDTH / 2 - SCALE_X(10);
  int sliderY = y + SCALE_Y(20);
  int sliderW = SCALE_X(140);
  int sliderH = SCALE_Y(18);
  tft.drawRoundRect(sliderX, sliderY, sliderW, sliderH, 3, THEME_TEXT_DIM);
  int fillW = ((sliderW - 4) * randomGen.probability) / 100;
  if (fillW > 0) {
    tft.fillRoundRect(sliderX + 2, sliderY + 2, fillW, sliderH - 4, 2, THEME_WARNING);
  }
  
  y += spacing;
  
  // Row 4: Tempo and Beat division
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Tempo", MARGIN_SMALL, y, 2);
  tft.drawString(String(sharedBPM), MARGIN_SMALL, y + SCALE_Y(16), 4);
  
  // BPM +/- (small buttons)
  drawRoundButton(MARGIN_SMALL + SCALE_X(55), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28), 
                  "-", THEME_SECONDARY, false, 1);
  drawRoundButton(MARGIN_SMALL + SCALE_X(88), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28), 
                  "+", THEME_SECONDARY, false, 1);
  
  // Beat division
  rangeX = DISPLAY_WIDTH / 2 + SCALE_X(10);
  String subdivText;
  if (randomGen.subdivision == 4) subdivText = "1/4";
  else if (randomGen.subdivision == 8) subdivText = "1/8";
  else subdivText = "1/16";
  
  tft.drawString("Division", rangeX, y, 2);
  tft.drawString(subdivText, rangeX, y + SCALE_Y(16), 4);
  
  // Division button
  drawRoundButton(rangeX + SCALE_X(80), y + SCALE_Y(10), SCALE_X(50), SCALE_Y(28), 
                  "<>", THEME_SECONDARY, false, 1);
  
  y += spacing + SCALE_Y(8);
  
  // Row 5: Activity indicator / Last played note (in box for visual emphasis)
  int boxY = y;
  int boxH = SCALE_Y(42);
  tft.drawRoundRect(MARGIN_SMALL, boxY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, boxH, 4, THEME_TEXT_DIM);
  
  if (randomGen.currentNote != -1) {
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Playing:", MARGIN_SMALL + SCALE_X(8), boxY + SCALE_Y(8), 2);
    
    String currentNoteName = getNoteNameFromMIDI(randomGen.currentNote);
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    tft.drawString(currentNoteName, MARGIN_SMALL + SCALE_X(80), boxY + SCALE_Y(12), 4);
    
    // Activity indicator (pulsing dot when playing)
    if (randomModuleRunning()) {
      tft.fillCircle(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(15), boxY + boxH / 2, 
                     SCALE_X(6), THEME_SUCCESS);
    }
  } else {
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawCentreString("No note playing", DISPLAY_WIDTH / 2, boxY + SCALE_Y(14), 2);
  }
}

void handleRandomGeneratorMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    int y = HEADER_HEIGHT + SCALE_Y(10);
    int spacing = SCALE_Y(38);
    
    // Row 1: Play/Stop
    if (isButtonPressed(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(32))) {
      if (randomModuleRunning()) {
        randomSync.stopPlayback();
        subdivAccumulator = 0;
        if (randomGen.currentNote != -1) {
          sendMIDI(0x80, randomGen.currentNote, 0);
          randomGen.currentNote = -1;
        }
      } else {
        subdivAccumulator = 0;
        randomSync.requestStart();
      }
      requestRedraw();
      return;
    }
    
    // Scale selector
    int scaleW = DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(75);
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(75), y, scaleW, SCALE_Y(32))) {
      randomGen.scaleType = (randomGen.scaleType + 1) % NUM_SCALES;
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // Row 2: Key +/-
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(55), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28))) {
      randomGen.rootNote = max(0, randomGen.rootNote - 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(88), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28))) {
      randomGen.rootNote = min(127, randomGen.rootNote + 1);
      requestRedraw();
      return;
    }
    
    // Octave range - touch on text area to adjust (simpler than separate buttons)
    int rangeX = DISPLAY_WIDTH / 2 + SCALE_X(10);
    if (isButtonPressed(rangeX, y, SCALE_X(120), SCALE_Y(40))) {
      // Cycle through common octave ranges: 3-6, 2-5, 4-7, 3-5, 4-6
      if (randomGen.minOctave == 3 && randomGen.maxOctave == 6) {
        randomGen.minOctave = 2; randomGen.maxOctave = 5;
      } else if (randomGen.minOctave == 2 && randomGen.maxOctave == 5) {
        randomGen.minOctave = 4; randomGen.maxOctave = 7;
      } else if (randomGen.minOctave == 4 && randomGen.maxOctave == 7) {
        randomGen.minOctave = 3; randomGen.maxOctave = 5;
      } else if (randomGen.minOctave == 3 && randomGen.maxOctave == 5) {
        randomGen.minOctave = 4; randomGen.maxOctave = 6;
      } else {
        randomGen.minOctave = 3; randomGen.maxOctave = 6;
      }
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // Row 3: Chance slider (interactive)
    int sliderX = DISPLAY_WIDTH / 2 - SCALE_X(10);
    int sliderY = y + SCALE_Y(20);
    int sliderW = SCALE_X(140);
    int sliderH = SCALE_Y(18);
    if (isButtonPressed(sliderX, sliderY, sliderW, sliderH)) {
      float normX = (touch.x - sliderX) / (float)sliderW;
      normX = constrain(normX, 0.0f, 1.0f);
      randomGen.probability = (int)(normX * 100);
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // Row 4: BPM +/-
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(55), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28))) {
      adjustSharedTempo(-5);
      return;
    }
    if (isButtonPressed(MARGIN_SMALL + SCALE_X(88), y + SCALE_Y(10), SCALE_X(28), SCALE_Y(28))) {
      adjustSharedTempo(+5);
      return;
    }
    
    // Division button
    rangeX = DISPLAY_WIDTH / 2 + SCALE_X(10);
    if (isButtonPressed(rangeX + SCALE_X(80), y + SCALE_Y(10), SCALE_X(50), SCALE_Y(28))) {
      if (randomGen.subdivision == 4) randomGen.subdivision = 8;
      else if (randomGen.subdivision == 8) randomGen.subdivision = 16;
      else randomGen.subdivision = 4;
      requestRedraw();
      return;
    }
  }
  
  // Update random generator
  updateRandomGenerator();
}

void updateRandomGenerator() {
  bool wasPlaying = randomSync.playing;
  randomSync.tryStartIfReady(!instantStartMode);
  bool justStarted = randomSync.playing && !wasPlaying;
  
  if (!randomSync.playing) {
    return;
  }
  
  // Use consumeReadySteps instead of ISR callbacks for reliability
  uint32_t readySteps = randomSync.consumeReadySteps(CLOCK_TICKS_PER_SIXTEENTH);
  
  // Apply subdivision: we get 16th notes from uClock, but may need to skip some
  subdivAccumulator += readySteps;

  uint32_t stepsToPlay = 0;
  // subdivision: randomGen.subdivision is one of {4,8,16}
  // compute factor as 16 / subdivision so quarter->4, eighth->2, sixteenth->1
  uint32_t subdivFactor = 16 / randomGen.subdivision;
  if (subdivFactor == 0) subdivFactor = 1;

  stepsToPlay = subdivAccumulator / subdivFactor;
  subdivAccumulator %= subdivFactor;
  
  if (stepsToPlay == 0) {
    return;
  }
  
  Serial.printf("[RNG] stepsToPlay=%u subdivision=%u\n", stepsToPlay, randomGen.subdivision);
  
  for (uint32_t i = 0; i < stepsToPlay; ++i) {
    playRandomNote();
  }
}

void playRandomNote() {
  // Stop current note if playing
  if (randomGen.currentNote != -1) {
    sendMIDI(0x80, randomGen.currentNote, 0);
    randomGen.currentNote = -1;
  }
  
  // Check probability
  if (random(100) < randomGen.probability) {
    // Generate random note in scale and octave range
    Scale& scale = scales[randomGen.scaleType];
    int degree = random(scale.numNotes);
    int octave = random(randomGen.minOctave, randomGen.maxOctave + 1);
    int note = randomGen.rootNote % 12 + scale.intervals[degree] + (octave * 12);
    
    if (note >= 0 && note <= 127) {
      sendMIDI(0x90, note, 100);
      randomGen.currentNote = note;
      
      MIDI_DEBUG("Random note: %s (prob: %d%%)\n", 
                   getNoteNameFromMIDI(note).c_str(), randomGen.probability);
      
      // Update display
      requestRedraw();  // Request redraw to trigger render event for display update
    }
  }
}
