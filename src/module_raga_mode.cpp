#include "module_raga_mode.h"
#include "clock_manager.h"
#include <uClock.h>

#include <Arduino.h>
#include <algorithm>

const char *const kRagaNames[RAGA_COUNT] = {
    "Bhairavi",
    "Lalit",
    "Bhupali",
    "Todi",
    "Madhuvanti",
    "Meghmalhar",
    "Yaman",
    "Malkauns",
};
const char *const kTalaNames[TALA_COUNT] = {
    "Teental",
    "Rupak",
    "Jhaptal",
};

RagaState raga;

static constexpr int kRagaScaleNotes = 7;
static constexpr int kRagaBars = 8;
static constexpr int kRagaNotesPerBar = 4;
static constexpr int kRagaMaxPhrase = 128;

struct TalaPattern {
  int beats;
  const uint8_t *accents;
};

static const uint8_t kTeentalAccents[16] = {
    2, 0, 0, 0, 1, 0, 0, 0,
    1, 0, 0, 0, 1, 0, 0, 0,
};
static const uint8_t kRupakAccents[7] = {
    2, 0, 0, 1, 0, 0, 0,
};
static const uint8_t kJhaptalAccents[10] = {
    2, 0, 0, 1, 0, 0, 0, 1, 0, 0,
};

static const TalaPattern kTalaPatterns[TALA_COUNT] = {
    {16, kTeentalAccents},
    {7, kRupakAccents},
    {10, kJhaptalAccents},
};

static inline const TalaPattern &getCurrentTalaPattern() {
  return kTalaPatterns[static_cast<int>(raga.currentTala)];
}

static inline int selectorRowSpacing() {
  return SCALE_Y(10);
}

struct SelectorLayout {
  int y;
  int height;
  int minusX;
  int valueX;
  int plusX;
  int minusW;
  int valueW;
  int plusW;
};

static SelectorLayout computeSelectorLayout(int topY) {
  SelectorLayout layout;
  layout.y = topY;
  layout.height = SCALE_Y(40);
  layout.minusW = SCALE_X(48);
  layout.plusW = layout.minusW;
  int buttonSpacing = SCALE_X(8);
  int rowWidth = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  layout.valueW = rowWidth - layout.minusW - layout.plusW - 2 * buttonSpacing;
  layout.minusX = MARGIN_SMALL;
  layout.valueX = layout.minusX + layout.minusW + buttonSpacing;
  layout.plusX = layout.valueX + layout.valueW + buttonSpacing;
  return layout;
}

static const uint8_t kRagaIntervals[RAGA_COUNT][kRagaScaleNotes] = {
    {0, 1, 3, 5, 7, 8, 10},
    {0, 1, 4, 5, 6, 9, 11},
    {0, 2, 4, 7, 9, 0, 0},
    {0, 1, 3, 6, 7, 8, 11},
    {0, 2, 3, 6, 7, 9, 11},
    {0, 2, 5, 7, 10, 0, 0},
    {0, 2, 4, 6, 7, 9, 11},
    {0, 3, 5, 8, 10, 0, 0},
};

static const uint8_t kRagaScaleLengths[RAGA_COUNT] = {7, 7, 5, 7, 7, 5, 7, 5};

static int8_t g_ragaPhrase[kRagaMaxPhrase];
static int g_phraseLength = 0;
static int g_phraseIndex = 0;
static uint32_t g_noteIntervalMs = 0;
static uint32_t g_noteDurationMs = 0;
static uint32_t g_noteOffTime = 0;
static bool g_noteActive = false;
static uint8_t g_currentNote = 0;
static bool g_droneActive = false;
static uint8_t g_droneNote = 0;
static uint16_t g_activeTempo = 0;
static int g_talaBeatIndex = 0;
static int g_lastTalaBeatIndex = -1;
static SequencerSyncState ragaSync;

// ISR-safe step counter from uClock step extension
static volatile uint32_t ragaStepCount = 0;
// Assigned at runtime by uClock when the ISR is first invoked
static volatile uint8_t ragaAssignedTrack = 0xFF;
static const uint8_t ragaRequestedTracks = 1;
static volatile bool ragaAssignedFlag = false;

// ISR callback for uClock step sequencer extension
static void onRagaStepISR(uint32_t step, uint8_t track) {
  (void)step;
  // If we don't yet know the assigned base track, capture it on first call
  if (ragaAssignedTrack == 0xFF) {
    ragaAssignedTrack = track;
    ragaAssignedFlag = true; // main loop will print this once
    ragaStepCount++;
    return;
  }
  if (track >= ragaAssignedTrack && track < ragaAssignedTrack + ragaRequestedTracks) {
    ragaStepCount++;
  }
}

static bool updateRagaTempo();
static void generateRagaPhrase();
static void scheduleNextNote(unsigned long now);
static void stopCurrentNote();
static void updateDroneNote();
static void resetPhraseState();
static void updateRagaPlayback();
static void cycleRaga(int delta);
static void cycleTala(int delta);

void initializeRagaMode() {
  raga.currentRaga = RAGA_BHAIRAVI;
  raga.currentTala = TALA_TEENTAL;
  raga.rootNote = 60;
  raga.playing = false;
  raga.droneEnabled = false;
  g_phraseLength = 0;
  g_phraseIndex = 0;
  g_noteActive = false;
  g_droneActive = false;
  g_noteOffTime = 0;
  g_activeTempo = 0;
  ragaSync.reset();
  updateRagaTempo();
  
  // Step callback registration is done at startup via registerAllStepCallbacks().
}

void registerRagaStepCallback() {
  // Request one track slot; registration order in registerAllStepCallbacks()
  // determines the assigned track index (expected to be 4 for Raga).
  uClock.setOnStep(onRagaStepISR, 1);
  Serial.println("[RAGA] registerRagaStepCallback() called (requestedTracks=1)");
}

void drawRagaMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("RAGA", "Indian Classical Scales", 3);

  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Root: " + getNoteNameFromMIDI(raga.rootNote), MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(10), 1);
  tft.drawString(raga.playing ? "Playing" : "Idle", DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(80), HEADER_HEIGHT + SCALE_Y(10), 1);
  const TalaPattern &activePattern = getCurrentTalaPattern();
  int beatDisplay = (g_lastTalaBeatIndex >= 0) ? g_lastTalaBeatIndex + 1 : g_talaBeatIndex + 1;
  beatDisplay = ((beatDisplay - 1) % activePattern.beats) + 1;
  tft.drawString("Tala: " + String(kTalaNames[static_cast<int>(raga.currentTala)]) + " " +
                    String(beatDisplay) + "/" + String(activePattern.beats),
                 MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(24), 1);

  const int selectorSpacing = selectorRowSpacing();
  const int ragaRowTop = HEADER_HEIGHT + SCALE_Y(30);
  const SelectorLayout ragaLayout = computeSelectorLayout(ragaRowTop);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Raga", MARGIN_SMALL, ragaLayout.y - SCALE_Y(18), 2);
  drawRoundButton(ragaLayout.minusX, ragaLayout.y, ragaLayout.minusW, ragaLayout.height, "-", THEME_ERROR, false, 5);
  drawRoundButton(ragaLayout.valueX, ragaLayout.y, ragaLayout.valueW, ragaLayout.height,
                  String(kRagaNames[static_cast<int>(raga.currentRaga)]), THEME_PRIMARY, false, 4);
  drawRoundButton(ragaLayout.plusX, ragaLayout.y, ragaLayout.plusW, ragaLayout.height, "+", THEME_SUCCESS, false, 5);

  const int talaRowTop = ragaLayout.y + ragaLayout.height + selectorSpacing;
  const SelectorLayout talaLayout = computeSelectorLayout(talaRowTop);
  tft.drawString("Tala", MARGIN_SMALL, talaLayout.y - SCALE_Y(18), 2);
  drawRoundButton(talaLayout.minusX, talaLayout.y, talaLayout.minusW, talaLayout.height, "-", THEME_ERROR, false, 5);
  drawRoundButton(talaLayout.valueX, talaLayout.y, talaLayout.valueW, talaLayout.height,
                  String(kTalaNames[static_cast<int>(raga.currentTala)]), THEME_ACCENT, false, 4);
  drawRoundButton(talaLayout.plusX, talaLayout.y, talaLayout.plusW, talaLayout.height, "+", THEME_SUCCESS, false, 5);

  const int controlY = DISPLAY_HEIGHT - SCALE_Y(45);
  // Play button: show STOP when playing, PENDING (orange) when start is pending,
  // otherwise show PLAY (green).
  const char *playLabel;
  uint16_t playColor;
  if (ragaSync.playing) {
    playLabel = "STOP";
    playColor = THEME_ERROR;
  } else if (ragaSync.startPending) {
    playLabel = "PENDING";
    playColor = THEME_SECONDARY;
  } else {
    playLabel = "PLAY";
    playColor = THEME_SUCCESS;
  }
  drawRoundButton(MARGIN_SMALL, controlY, SCALE_X(70), SCALE_Y(35), playLabel, playColor);
  drawRoundButton(MARGIN_SMALL + SCALE_X(78), controlY, SCALE_X(80), SCALE_Y(35),
                  raga.droneEnabled ? "DRONE ON" : "DRONE OFF",
                  raga.droneEnabled ? THEME_SUCCESS : THEME_SECONDARY);
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Scale: " + String(kRagaNames[static_cast<int>(raga.currentRaga)]),
                 MARGIN_SMALL, controlY - SCALE_Y(15), 1);
}

void toggleRagaPlayback() {
  if (ragaSync.playing || ragaSync.startPending) {
    ragaSync.stopPlayback();
    stopCurrentNote();
    g_noteActive = false;
    g_noteOffTime = 0;
    raga.playing = false;
  } else {
    updateRagaTempo();
    resetPhraseState();
    ragaSync.requestStart();
    raga.playing = true;
  }
  requestRedraw();
}

void handleRagaMode() {
  updateRagaPlayback();

  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    ragaSync.stopPlayback();
    stopCurrentNote();
    raga.playing = false;
    exitToMenu();
    return;
  }

  const int controlY = DISPLAY_HEIGHT - SCALE_Y(45);
  if (touch.justPressed &&
      isButtonPressed(MARGIN_SMALL, controlY, SCALE_X(70), SCALE_Y(35) )) {
    toggleRagaPlayback();
    return;
  }

  if (touch.justPressed &&
      isButtonPressed(MARGIN_SMALL + SCALE_X(78), controlY, SCALE_X(80), SCALE_Y(35))) {
    raga.droneEnabled = !raga.droneEnabled;
    requestRedraw();
    return;
  }

  if (!touch.justPressed) {
    return;
  }

  const int ragaRowTop = HEADER_HEIGHT + SCALE_Y(30);
  SelectorLayout ragaLayout = computeSelectorLayout(ragaRowTop);
  if (touch.justPressed && isButtonPressed(ragaLayout.minusX, ragaLayout.y, ragaLayout.minusW, ragaLayout.height)) {
    cycleRaga(-1);
    return;
  }
  if (touch.justPressed && isButtonPressed(ragaLayout.plusX, ragaLayout.y, ragaLayout.plusW, ragaLayout.height)) {
    cycleRaga(1);
    return;
  }

  const int talaRowTop = ragaLayout.y + ragaLayout.height + selectorRowSpacing();
  SelectorLayout talaLayout = computeSelectorLayout(talaRowTop);
  if (touch.justPressed && isButtonPressed(talaLayout.minusX, talaLayout.y, talaLayout.minusW, talaLayout.height)) {
    cycleTala(-1);
    return;
  }
  if (touch.justPressed && isButtonPressed(talaLayout.plusX, talaLayout.y, talaLayout.plusW, talaLayout.height)) {
    cycleTala(1);
    return;
  }
}

static bool updateRagaTempo() {
  uint16_t tempo = sharedBPM;
  if (tempo < 40) {
    tempo = 40;
  } else if (tempo > 240) {
    tempo = 240;
  }
  if (tempo == g_activeTempo) {
    return false;
  }
  g_activeTempo = tempo;
  uint32_t interval = 60000UL / g_activeTempo;
    g_noteIntervalMs = std::max<uint32_t>(40, interval / 4);
    g_noteDurationMs = std::max<uint32_t>(30, (g_noteIntervalMs * 8) / 10);
  return true;
}

static void generateRagaPhrase() {
  const uint8_t *scale = kRagaIntervals[static_cast<int>(raga.currentRaga)];
  int scaleLength = kRagaScaleLengths[static_cast<int>(raga.currentRaga)];
  int totalNotes = kRagaBars * kRagaNotesPerBar;
  g_phraseLength = std::min(totalNotes, kRagaMaxPhrase);
  int degree = random(0, scaleLength);
  int octave = 0;

  for (int i = 0; i < g_phraseLength; ++i) {
    int step = random(3) - 1;
    degree = std::max(0, std::min(scaleLength - 1, degree + step));
    if (random(100) < 10) {
      int octaveShift = random(3) - 1;
      octave = std::max(-1, std::min(1, octave + octaveShift));
    }
    g_ragaPhrase[i] = scale[degree] + octave * 12;
  }
  g_phraseIndex = 0;
}

static void scheduleNextNote(unsigned long now) {
  if (g_phraseLength == 0 || g_phraseIndex >= g_phraseLength) {
    generateRagaPhrase();
  }
  stopCurrentNote();
  uint8_t note = raga.rootNote + g_ragaPhrase[g_phraseIndex++];
  const TalaPattern &pattern = getCurrentTalaPattern();
  int beat = g_talaBeatIndex;
  uint8_t accent = pattern.accents[beat];
  uint8_t velocity = std::min<uint8_t>(127, static_cast<uint8_t>(100 + accent * 8));
  sendMIDI(0x90, note, velocity);
  Serial.printf("[RAGA] NoteOn note=%u vel=%u beat=%d\n", note, velocity, beat);
  g_currentNote = note;
  g_noteActive = true;
  g_noteOffTime = now + g_noteDurationMs;
  g_lastTalaBeatIndex = beat;
  g_talaBeatIndex = (beat + 1) % pattern.beats;
}

static void stopCurrentNote() {
  if (g_noteActive) {
    sendMIDI(0x80, g_currentNote, 0);
    Serial.printf("[RAGA] NoteOff note=%u\n", g_currentNote);
    g_noteActive = false;
  }
}

static void updateDroneNote() {
  if (raga.droneEnabled) {
    if (!g_droneActive || g_droneNote != raga.rootNote) {
      if (g_droneActive) {
        sendMIDI(0x80, g_droneNote, 0);
        Serial.printf("[RAGA] DroneOff note=%u\n", g_droneNote);
      }
      g_droneNote = raga.rootNote;
      sendMIDI(0x90, g_droneNote, 80);
      Serial.printf("[RAGA] DroneOn note=%u vel=%u\n", g_droneNote, 80);
      g_droneActive = true;
    }
  } else if (g_droneActive) {
    sendMIDI(0x80, g_droneNote, 0);
    Serial.printf("[RAGA] DroneOff note=%u\n", g_droneNote);
    g_droneActive = false;
  }
}

static void resetPhraseState() {
  stopCurrentNote();
  generateRagaPhrase();
  g_phraseIndex = 0;
  g_noteActive = false;
  g_noteOffTime = 0;
  g_talaBeatIndex = 0;
  g_lastTalaBeatIndex = -1;
}

static void updateRagaPlayback() {
  updateRagaTempo();
  updateDroneNote();

  bool wasPlaying = ragaSync.playing;
  ragaSync.tryStartIfReady(!instantStartMode);
  bool justStarted = ragaSync.playing && !wasPlaying;
  raga.playing = ragaSync.playing;

  // Use time-based note scheduling instead of step-based
  static unsigned long lastNoteTime = 0;
  
  if (justStarted) {
    lastNoteTime = 0;  // Reset timer on start
    resetPhraseState();
  }

  if (!raga.playing) {
    return;
  }

  // Get steps from uClock step extension (ISR-safe)
  uint32_t readySteps = 0;
  noInterrupts();
  readySteps = ragaStepCount;
  ragaStepCount = 0;
  interrupts();
  
  if (readySteps == 0) {
    return;
  }

  // Print assigned track once when observed
  if (ragaAssignedFlag) {
    Serial.printf("[RAGA] assignedTrack=%u requested=%u\n", (unsigned)ragaAssignedTrack, (unsigned)ragaRequestedTracks);
    ragaAssignedFlag = false;
  }
  Serial.printf("[RAGA] readySteps=%u g_noteActive=%d g_phraseIndex=%d\n", readySteps, (int)g_noteActive, g_phraseIndex);

  unsigned long now = millis();
  
  // Check if it's time to play the next note
  if (lastNoteTime == 0 || now - lastNoteTime >= g_noteIntervalMs) {
    if (g_noteActive && now >= g_noteOffTime) {
      stopCurrentNote();
    }
    scheduleNextNote(now);
    lastNoteTime = now;
  } else if (g_noteActive && now >= g_noteOffTime) {
    // Still turn off notes even if not scheduling new ones
    stopCurrentNote();
  }
}

static void cycleRaga(int delta) {
  int next = static_cast<int>(raga.currentRaga) + delta;
  if (next < 0) {
    next = RAGA_COUNT - 1;
  } else if (next >= RAGA_COUNT) {
    next = 0;
  }
  if (next == static_cast<int>(raga.currentRaga)) {
    return;
  }
  raga.currentRaga = static_cast<RagaType>(next);
  resetPhraseState();
  requestRedraw();
}

static void cycleTala(int delta) {
  int next = static_cast<int>(raga.currentTala) + delta;
  if (next < 0) {
    next = TALA_COUNT - 1;
  } else if (next >= TALA_COUNT) {
    next = 0;
  }
  if (next == static_cast<int>(raga.currentTala)) {
    return;
  }
  raga.currentTala = static_cast<TalaType>(next);
  resetPhraseState();
  requestRedraw();
}
