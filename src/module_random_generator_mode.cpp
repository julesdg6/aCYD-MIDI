#include "module_random_generator_mode.h"

RandomGen randomGen;

// Implementations
void initializeRandomGeneratorMode() {
  randomGen.rootNote = 60;
  randomGen.scaleType = 0;
  randomGen.minOctave = 3;
  randomGen.maxOctave = 6;
  randomGen.probability = 50;
  randomGen.bpm = 120;
  randomGen.subdivision = 4;
  randomGen.isPlaying = false;
  randomGen.currentNote = -1;
  calculateNoteInterval();
  randomGen.nextNoteTime = millis() + randomGen.noteInterval;
}

void drawRandomGeneratorMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("RNG JAMS", "Random Music", 3);
  
  drawRandomGenControls();
}

void drawRandomGenControls() {
  int y = HEADER_HEIGHT + SCALE_Y(10);
  int spacing = SCALE_Y(35);  // Increased spacing for larger buttons
  
  // Play/Stop and Root note on same line
  drawRoundButton(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(35), randomGen.isPlaying ? "STOP" : "PLAY", 
                 randomGen.isPlaying ? THEME_ERROR : THEME_SUCCESS);
  
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Key:", SCALE_X(90), y + SCALE_Y(10), 2);
  String rootName = getNoteNameFromMIDI(randomGen.rootNote);
  drawRoundButton(SCALE_X(130), y, SCALE_X(50), SCALE_Y(35), rootName, THEME_PRIMARY);
  drawRoundButton(SCALE_X(190), y, SCALE_X(40), SCALE_Y(35), "+", THEME_SECONDARY);
  drawRoundButton(SCALE_X(240), y, SCALE_X(40), SCALE_Y(35), "-", THEME_SECONDARY);
  
  y += spacing;
  
  // Scale selector - full width
  drawRoundButton(MARGIN_SMALL, y, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(35), 
                 scales[randomGen.scaleType].name, THEME_ACCENT);
  
  y += spacing;
  
  // Octave range - larger buttons
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Octave:", MARGIN_SMALL, y + SCALE_Y(10), 2);
  tft.drawString(String(randomGen.minOctave) + "-" + String(randomGen.maxOctave), SCALE_X(70), y + SCALE_Y(10), 2);
  drawRoundButton(SCALE_X(120), y, SCALE_X(50), SCALE_Y(35), "MIN-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(180), y, SCALE_X(50), SCALE_Y(35), "MIN+", THEME_SECONDARY);
  drawRoundButton(MARGIN_SMALL, y + SCALE_Y(40), SCALE_X(50), SCALE_Y(35), "MAX-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(60), y + SCALE_Y(40), SCALE_X(50), SCALE_Y(35), "MAX+", THEME_SECONDARY);
  
  y += spacing + SCALE_Y(40);
  
  // Probability with visual bar
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("Chance: " + String(randomGen.probability) + "%", MARGIN_SMALL, y + SCALE_Y(5), 2);
  drawRoundButton(SCALE_X(100), y, SCALE_X(50), SCALE_Y(35), "-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(160), y, SCALE_X(50), SCALE_Y(35), "+", THEME_SECONDARY);
  
  // Probability bar
  int barW = SCALE_X(110);
  int barX = SCALE_X(220);
  int barY = y + SCALE_Y(10);
  tft.fillRect(barX, barY, barW, SCALE_Y(15), THEME_BG);
  tft.drawRect(barX, barY, barW, SCALE_Y(15), THEME_TEXT_DIM);
  int fillW = (barW * randomGen.probability) / 100;
  if (fillW > 0) {
    tft.fillRect(barX + 1, barY + 1, fillW, SCALE_Y(13), THEME_PRIMARY);
  }
  
  y += spacing;
  
  // BPM and subdivision controls
  tft.setTextColor(THEME_TEXT, THEME_BG);
  tft.drawString("BPM: " + String(randomGen.bpm), MARGIN_SMALL, y + SCALE_Y(5), 2);
  drawRoundButton(SCALE_X(80), y, SCALE_X(50), SCALE_Y(35), "-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(140), y, SCALE_X(50), SCALE_Y(35), "+", THEME_SECONDARY);
  
  String subdivText;
  if (randomGen.subdivision == 4) subdivText = "1/4";
  else if (randomGen.subdivision == 8) subdivText = "1/8";
  else if (randomGen.subdivision == 16) subdivText = "1/16";
  tft.drawString("Beat: " + subdivText, SCALE_X(200), y + SCALE_Y(5), 2);
  drawRoundButton(SCALE_X(270), y, SCALE_X(40), SCALE_Y(35), "<>", THEME_SECONDARY);
  
  y += spacing + SCALE_Y(5);
  
  // Current note indicator
  if (randomGen.currentNote != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString("Now Playing: ", MARGIN_SMALL, y, 2);
    String currentNoteName = getNoteNameFromMIDI(randomGen.currentNote);
    tft.setTextColor(THEME_ACCENT, THEME_BG);
    tft.drawString(currentNoteName, SCALE_X(140), y, 4);
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
    int spacing = SCALE_Y(35);
    
    // Play/Stop and Root note controls
    if (isButtonPressed(MARGIN_SMALL, y, SCALE_X(70), SCALE_Y(35))) {
      randomGen.isPlaying = !randomGen.isPlaying;
      if (randomGen.isPlaying) {
        randomGen.nextNoteTime = millis() + randomGen.noteInterval;
      } else if (randomGen.currentNote != -1) {
        sendMIDI(0x80, randomGen.currentNote, 0);
        randomGen.currentNote = -1;
      }
      requestRedraw();
      return;
    }
    
    if (isButtonPressed(SCALE_X(190), y, SCALE_X(40), SCALE_Y(35))) {
      randomGen.rootNote = min(127, randomGen.rootNote + 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(240), y, SCALE_X(40), SCALE_Y(35))) {
      randomGen.rootNote = max(0, randomGen.rootNote - 1);
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // Scale selector
    if (isButtonPressed(MARGIN_SMALL, y, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(35))) {
      randomGen.scaleType = (randomGen.scaleType + 1) % NUM_SCALES;
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // Octave controls
    if (isButtonPressed(SCALE_X(120), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.minOctave = max(1, randomGen.minOctave - 1);
      if (randomGen.minOctave >= randomGen.maxOctave) {
        randomGen.maxOctave = randomGen.minOctave + 1;
      }
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(180), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.minOctave = min(8, randomGen.minOctave + 1);
      if (randomGen.minOctave >= randomGen.maxOctave) {
        randomGen.maxOctave = randomGen.minOctave + 1;
      }
      requestRedraw();
      return;
    }
    if (isButtonPressed(MARGIN_SMALL, y + SCALE_Y(40), SCALE_X(50), SCALE_Y(35))) {
      randomGen.maxOctave = max(randomGen.minOctave + 1, randomGen.maxOctave - 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(60), y + SCALE_Y(40), SCALE_X(50), SCALE_Y(35))) {
      randomGen.maxOctave = min(9, randomGen.maxOctave + 1);
      requestRedraw();
      return;
    }
    
    y += spacing + SCALE_Y(40);
    
    // Probability controls
    if (isButtonPressed(SCALE_X(100), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.probability = max(0, randomGen.probability - 5);
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(160), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.probability = min(100, randomGen.probability + 5);
      requestRedraw();
      return;
    }
    
    y += spacing;
    
    // BPM controls
    if (isButtonPressed(SCALE_X(80), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.bpm = max(60, randomGen.bpm - 5);
      calculateNoteInterval();
      requestRedraw();
      return;
    }
    if (isButtonPressed(SCALE_X(140), y, SCALE_X(50), SCALE_Y(35))) {
      randomGen.bpm = min(200, randomGen.bpm + 5);
      calculateNoteInterval();
      requestRedraw();
      return;
    }
    
    // Subdivision controls
    if (isButtonPressed(SCALE_X(270), y, SCALE_X(40), SCALE_Y(35))) {
      if (randomGen.subdivision == 4) randomGen.subdivision = 8;
      else if (randomGen.subdivision == 8) randomGen.subdivision = 16;
      else randomGen.subdivision = 4;
      calculateNoteInterval();
      requestRedraw();
      return;
    }
  }
  
  // Update random generator
  updateRandomGenerator();
}

void updateRandomGenerator() {
  if (!randomGen.isPlaying || !deviceConnected) return;
  
  unsigned long now = millis();
  
  if (now >= randomGen.nextNoteTime) {
    playRandomNote();
    randomGen.nextNoteTime = now + randomGen.noteInterval;
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

void calculateNoteInterval() {
  // Calculate note interval from BPM and subdivision
  float beatsPerSecond = randomGen.bpm / 60.0;
  float notesPerSecond = beatsPerSecond * (randomGen.subdivision / 4.0);
  randomGen.noteInterval = 1000.0 / notesPerSecond;
}
