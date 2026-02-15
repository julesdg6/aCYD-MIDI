#include "module_world_music_demo_mode.h"
#include <Arduino.h>

const char *const kWMDemoModeNames[WM_DEMO_MODE_COUNT] = {
  "Raga Yaman",
  "Maqam Rast",
  "Pentatonic"
};

WorldMusicDemoState wmDemo;

// ============================================================
// Mode Creation Functions
// ============================================================

Mode createDemoRagaYaman() {
  Mode yaman;
  
  // Identity
  strcpy(yaman.id, "yaman");
  strcpy(yaman.name, "Raga Yaman");
  yaman.system = SYSTEM_RAGA;
  
  // Scale: Sa Re Ga Ma# Pa Dha Ni (C D E F# G A B)
  yaman.numDegrees = 7;
  yaman.scaleDegrees[0] = 0;   // Sa (C)
  yaman.scaleDegrees[1] = 2;   // Re (D)
  yaman.scaleDegrees[2] = 4;   // Ga (E)
  yaman.scaleDegrees[3] = 6;   // Ma# (F#) - Tivra Madhyam
  yaman.scaleDegrees[4] = 7;   // Pa (G)
  yaman.scaleDegrees[5] = 9;   // Dha (A)
  yaman.scaleDegrees[6] = 11;  // Ni (B)
  
  // Important degrees
  yaman.tonicIndex = 0;        // Sa
  yaman.dominantIndex = 3;     // Ma# (Vadi)
  yaman.cadentialIndices[0] = 0;  // Sa
  yaman.cadentialIndices[1] = 4;  // Pa (Samvadi)
  yaman.numCadential = 2;
  
  // Pakad phrase: Ni Re Ga Ma#
  yaman.numMotifs = 2;
  
  // Motif 1: Ni Re Ga Ma# (characteristic pakad)
  yaman.motifs[0].numSteps = 4;
  yaman.motifs[0].degreeSteps[0] = 6;  // Ni
  yaman.motifs[0].degreeSteps[1] = 1;  // Re
  yaman.motifs[0].degreeSteps[2] = 2;  // Ga
  yaman.motifs[0].degreeSteps[3] = 3;  // Ma#
  yaman.motifs[0].weight = 80;
  
  // Motif 2: Pa Dha Ni Sa (upper tetrachord)
  yaman.motifs[1].numSteps = 4;
  yaman.motifs[1].degreeSteps[0] = 4;  // Pa
  yaman.motifs[1].degreeSteps[1] = 5;  // Dha
  yaman.motifs[1].degreeSteps[2] = 6;  // Ni
  yaman.motifs[1].degreeSteps[3] = 0;  // Sa (next octave)
  yaman.motifs[1].weight = 60;
  
  return yaman;
}

Mode createDemoMaqamRast() {
  Mode rast;
  
  strcpy(rast.id, "rast");
  strcpy(rast.name, "Maqam Rast");
  rast.system = SYSTEM_MAQAM;
  
  // Scale: C D E F G A Bb C (Rast scale)
  rast.numDegrees = 7;
  rast.scaleDegrees[0] = 0;   // Rast (C)
  rast.scaleDegrees[1] = 2;   // Dukah (D)
  rast.scaleDegrees[2] = 4;   // Sikah (E)
  rast.scaleDegrees[3] = 5;   // Jaharka (F)
  rast.scaleDegrees[4] = 7;   // Nawa (G)
  rast.scaleDegrees[5] = 9;   // Husayni (A)
  rast.scaleDegrees[6] = 10;  // Awj (Bb)
  
  rast.tonicIndex = 0;
  rast.dominantIndex = 4;  // Nawa (dominant)
  rast.cadentialIndices[0] = 0;  // Rast
  rast.cadentialIndices[1] = 4;  // Nawa
  rast.numCadential = 2;
  
  // Lower Jins (Rast tetrachord on tonic)
  strcpy(rast.segments[0].name, "Rast Lower");
  rast.segments[0].numDegrees = 4;
  rast.segments[0].degrees[0] = 0;  // Rast
  rast.segments[0].degrees[1] = 1;  // Dukah
  rast.segments[0].degrees[2] = 2;  // Sikah
  rast.segments[0].degrees[3] = 3;  // Jaharka
  rast.segments[0].tonicIndex = 0;
  
  // Upper Jins (Rast tetrachord on dominant)
  strcpy(rast.segments[1].name, "Rast Upper");
  rast.segments[1].numDegrees = 4;
  rast.segments[1].degrees[0] = 4;  // Nawa
  rast.segments[1].degrees[1] = 5;  // Husayni
  rast.segments[1].degrees[2] = 6;  // Awj
  rast.segments[1].degrees[3] = 0;  // Rast (octave)
  rast.segments[1].tonicIndex = 0;
  
  rast.numSegments = 2;
  
  // Motif: Ascending Rast tetrachord
  rast.numMotifs = 1;
  rast.motifs[0].numSteps = 4;
  rast.motifs[0].degreeSteps[0] = 0;  // Index 0 = Rast (1st note)
  rast.motifs[0].degreeSteps[1] = 1;  // Index 1 = Dukah (2nd note)
  rast.motifs[0].degreeSteps[2] = 2;  // Index 2 = Sikah (3rd note)
  rast.motifs[0].degreeSteps[3] = 3;  // Index 3 = Jaharka (4th note)
  rast.motifs[0].weight = 70;
  
  return rast;
}

Mode createDemoPentatonic() {
  Mode penta;
  
  strcpy(penta.id, "pentatonic");
  strcpy(penta.name, "Major Pentatonic");
  penta.system = SYSTEM_EAST_ASIAN_PENTATONIC;
  
  // Scale: C D E G A (Major Pentatonic)
  penta.numDegrees = 5;
  penta.scaleDegrees[0] = 0;   // C
  penta.scaleDegrees[1] = 2;   // D
  penta.scaleDegrees[2] = 4;   // E
  penta.scaleDegrees[3] = 7;   // G
  penta.scaleDegrees[4] = 9;   // A
  
  penta.tonicIndex = 0;
  penta.dominantIndex = 3;  // G (perfect 5th)
  penta.cadentialIndices[0] = 0;  // C
  penta.cadentialIndices[1] = 3;  // G
  penta.numCadential = 2;
  
  // Simple ascending motif
  penta.numMotifs = 1;
  penta.motifs[0].numSteps = 5;
  penta.motifs[0].degreeSteps[0] = 0;  // Index 0 = C (1st note)
  penta.motifs[0].degreeSteps[1] = 1;  // Index 1 = D (2nd note)
  penta.motifs[0].degreeSteps[2] = 2;  // Index 2 = E (3rd note)
  penta.motifs[0].degreeSteps[3] = 3;  // Index 3 = G (4th note)
  penta.motifs[0].degreeSteps[4] = 4;  // Index 4 = A (5th note)
  penta.motifs[0].weight = 50;
  
  return penta;
}

// ============================================================
// Mode Functions
// ============================================================

void initializeWorldMusicDemoMode() {
  wmDemo.currentModeType = WM_DEMO_RAGA_YAMAN;
  wmDemo.currentMode = createDemoRagaYaman();
  
  // Validate mode
  ValidationResult result = validateMode(wmDemo.currentMode);
  if (!result.isValid) {
    Serial.print("ERROR: Mode validation failed: ");
    Serial.println(result.errorMessage);
  } else {
    Serial.println("Mode validated successfully");
  }
  
  // Generator parameters
  wmDemo.genParams.phraseLength = 16;
  wmDemo.genParams.baseOctave = 4;
  wmDemo.genParams.registerRange = 1;
  wmDemo.genParams.useCadence = true;
  wmDemo.genParams.useMotifs = true;
  wmDemo.genParams.motifDensity = 70;
  
  // Playback state
  wmDemo.isPlaying = false;
  wmDemo.currentStep = 0;
  wmDemo.lastNoteTime = 0;
  wmDemo.noteInterval = 250;  // 250ms = quarter note at 240 BPM
  wmDemo.phraseLength = 0;
  
  // Generate initial phrase
  generatePhrase(wmDemo.currentMode, wmDemo.genParams,
                 wmDemo.phraseNotes, wmDemo.phraseOctaves,
                 WM_DEMO_MAX_PHRASE_LENGTH);
  wmDemo.phraseLength = wmDemo.genParams.phraseLength;
}

void drawWorldMusicDemoMode() {
  tft.fillScreen(THEME_BG);
  
  // Header
  drawHeader("World Music Demo", kWMDemoModeNames[wmDemo.currentModeType]);
  
  // Mode info
  int y = HEADER_HEIGHT + SCALE_Y(10);
  tft.setTextColor(THEME_TEXT);
  tft.setTextSize(1);
  
  tft.setCursor(SCALE_X(10), y);
  tft.print("Mode: ");
  tft.println(wmDemo.currentMode.name);
  
  y += SCALE_Y(15);
  tft.setCursor(SCALE_X(10), y);
  tft.print("System: ");
  tft.println(getSystemTypeName(wmDemo.currentMode.system));
  
  y += SCALE_Y(15);
  tft.setCursor(SCALE_X(10), y);
  tft.print("Degrees: ");
  tft.println(wmDemo.currentMode.numDegrees);
  
  y += SCALE_Y(15);
  tft.setCursor(SCALE_X(10), y);
  tft.print("Motifs: ");
  tft.println(wmDemo.currentMode.numMotifs);
  
  // Control buttons
  y = DISPLAY_HEIGHT - SCALE_Y(80);
  
  // Mode select buttons
  int buttonWidth = SCALE_X(90);
  int buttonHeight = SCALE_Y(30);
  int spacing = SCALE_X(10);
  int x = spacing;
  
  for (int i = 0; i < WM_DEMO_MODE_COUNT; i++) {
    uint16_t color = (i == wmDemo.currentModeType) ? THEME_PRIMARY : THEME_SURFACE;
    drawRoundButton(x, y, buttonWidth, buttonHeight, 
                    kWMDemoModeNames[i], color, THEME_TEXT);
    x += buttonWidth + spacing;
  }
  
  // Play/Stop button
  y += buttonHeight + spacing;
  const char* playLabel = wmDemo.isPlaying ? "STOP" : "PLAY";
  uint16_t playColor = wmDemo.isPlaying ? THEME_ERROR : THEME_SUCCESS;
  drawRoundButton(spacing, y, buttonWidth, buttonHeight, 
                  playLabel, playColor, THEME_TEXT);
  
  // Generate button
  drawRoundButton(spacing * 2 + buttonWidth, y, buttonWidth, buttonHeight,
                  "GENERATE", THEME_SECONDARY, THEME_TEXT);
}

void handleWorldMusicDemoMode() {
  // Touch input
  bool pressed = touch.justPressed;
  
  if (pressed) {
    int x = touch.x;
    int y = touch.y;
    
    // Check mode select buttons
    int buttonY = DISPLAY_HEIGHT - SCALE_Y(80);
    int buttonWidth = SCALE_X(90);
    int buttonHeight = SCALE_Y(30);
    int spacing = SCALE_X(10);
    
    if (y >= buttonY && y <= buttonY + buttonHeight) {
      int buttonX = spacing;
      for (int i = 0; i < WM_DEMO_MODE_COUNT; i++) {
        if (x >= buttonX && x <= buttonX + buttonWidth) {
          // Switch mode
          wmDemo.currentModeType = (WMDemoModeType)i;
          
          switch (wmDemo.currentModeType) {
            case WM_DEMO_RAGA_YAMAN:
              wmDemo.currentMode = createDemoRagaYaman();
              break;
            case WM_DEMO_MAQAM_RAST:
              wmDemo.currentMode = createDemoMaqamRast();
              break;
            case WM_DEMO_PENTATONIC:
              wmDemo.currentMode = createDemoPentatonic();
              break;
            default:
              break;
          }
          
          // Validate and regenerate
          ValidationResult result = validateMode(wmDemo.currentMode);
          if (result.isValid) {
            generatePhrase(wmDemo.currentMode, wmDemo.genParams,
                          wmDemo.phraseNotes, wmDemo.phraseOctaves,
                          WM_DEMO_MAX_PHRASE_LENGTH);
            wmDemo.phraseLength = wmDemo.genParams.phraseLength;
          }
          
          requestRedraw();
          return;
        }
        buttonX += buttonWidth + spacing;
      }
    }
    
    // Check play/stop button
    buttonY += buttonHeight + spacing;
    if (y >= buttonY && y <= buttonY + buttonHeight) {
      if (x >= spacing && x <= spacing + buttonWidth) {
        wmDemo.isPlaying = !wmDemo.isPlaying;
        if (wmDemo.isPlaying) {
          wmDemo.currentStep = 0;
          wmDemo.lastNoteTime = millis();
        } else {
          // Stop all notes
          stopAllModes();
        }
        requestRedraw();
        return;
      }
      
      // Check generate button
      int genX = spacing * 2 + buttonWidth;
      if (x >= genX && x <= genX + buttonWidth) {
        // Generate new phrase
        generatePhrase(wmDemo.currentMode, wmDemo.genParams,
                      wmDemo.phraseNotes, wmDemo.phraseOctaves,
                      WM_DEMO_MAX_PHRASE_LENGTH);
        wmDemo.phraseLength = wmDemo.genParams.phraseLength;
        Serial.println("New phrase generated");
        return;
      }
    }
  }
  
  // Playback
  if (wmDemo.isPlaying && wmDemo.phraseLength > 0) {
    unsigned long now = millis();
    if (now - wmDemo.lastNoteTime >= wmDemo.noteInterval) {
      // Send note off for previous note
      if (wmDemo.currentStep > 0) {
        uint8_t prevStep = (wmDemo.currentStep - 1) % wmDemo.phraseLength;
        int8_t prevNote = scaleDegreesToMidiNote(
          wmDemo.phraseNotes[prevStep],
          wmDemo.phraseOctaves[prevStep],
          wmDemo.currentMode
        );
        sendMIDI(0x80, prevNote, 0);
      }
      
      // Send note on for current note
      int8_t midiNote = scaleDegreesToMidiNote(
        wmDemo.phraseNotes[wmDemo.currentStep],
        wmDemo.phraseOctaves[wmDemo.currentStep],
        wmDemo.currentMode
      );
      
      sendMIDI(0x90, midiNote, 100);
      
      wmDemo.currentStep = (wmDemo.currentStep + 1) % wmDemo.phraseLength;
      wmDemo.lastNoteTime = now;
    }
  }
}
