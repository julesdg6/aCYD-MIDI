#include "module_slot_performer_mode.h"

#include "app/app_modes.h"
#include "common_definitions.h"
#include "midi_out_buffer.h"
#include "midi_utils.h"
#include "ui_elements.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

namespace {

static constexpr uint8_t kMaxSlots = 12;
static constexpr uint8_t kQuantizeOptions = 5;
static constexpr uint8_t kMaxMidiChannels = 16;
static constexpr uint8_t kPatternSteps = 16;
static constexpr uint8_t kSwingTicksMaxOffset = 3;  // ~half-step offset at 16th resolution
static constexpr uint16_t kTicksPerQuarter = 24;
static constexpr uint16_t kTicksPerBar = 96;
static constexpr uint16_t kMaxLoopEvents = 384;
static constexpr uint8_t kNoSlot = 0xFF;

enum class SlotType : uint8_t {
  EMPTY = 0,
  ENGINE_EUCLID,
  ENGINE_DIMENSIONS,
  LIVE_INPUT,
  LOOPER
};

enum class RecordMode : uint8_t {
  REPLACE = 0,
  OVERDUB
};

struct LoopEvent {
  uint16_t tick = 0;
  uint8_t note = 60;
  uint8_t velocity = 100;
  bool noteOn = true;
};

struct PendingNoteOff {
  bool active = false;
  uint8_t slotIndex = 0;
  uint8_t note = 0;
  uint8_t velocity = 0;
  uint32_t offTick = 0;
};

struct SlotState {
  SlotType type = SlotType::EMPTY;
  uint8_t midiChannel = 1;  // 1-16
  bool muted = false;
  uint8_t baseNote = 60;
  uint8_t density = 5;  // 1-16 for step generators
  uint16_t engineStepIndex = 0;
  uint32_t nextEngineTick = 0;
  bool recordArmed = false;
  bool activity = false;
  uint32_t activityUntilMs = 0;

  // Looper role
  bool looperHasContent = false;
  uint16_t loopLengthTicks = 4 * kTicksPerBar;
  uint8_t loopQuantizeTicks = 6;
  LoopEvent loopEvents[kMaxLoopEvents];
  uint16_t loopEventCount = 0;
};

struct CaptureNoteState {
  bool active = false;
  uint16_t onTick = 0;
};

struct SlotPerformerState {
  SlotState slots[kMaxSlots];
  PendingNoteOff pendingNoteOffs[64];

  bool transportRunning = false;
  uint16_t bpm = 120;
  uint8_t swing = 0;  // 0..50
  uint8_t configuredSlots = 6;
  uint8_t selectedSlot = 0;
  uint8_t page = 0;

  uint32_t currentTick = 0;
  uint32_t lastTickMicros = 0;

  uint8_t bars = 4;
  uint8_t quantizeIndex = 2;  // 1/16 by default
  RecordMode recordMode = RecordMode::REPLACE;
  bool recordPending = false;
  bool recording = false;
  uint8_t recordSourceSlot = kNoSlot;
  uint8_t recordTargetSlot = kNoSlot;
  uint32_t recordStartTick = 0;
  uint32_t recordEndTick = 0;
  CaptureNoteState captureNotes[128];

  bool detailOpen = false;
} gState;

static const char *slotTypeLabel(SlotType type) {
  switch (type) {
    case SlotType::EMPTY:
      return "EMPTY";
    case SlotType::ENGINE_EUCLID:
      return "EUCLID";
    case SlotType::ENGINE_DIMENSIONS:
      return "DIMS";
    case SlotType::LIVE_INPUT:
      return "LIVE";
    case SlotType::LOOPER:
      return "LOOPER";
    default:
      return "EMPTY";
  }
}

static const char *quantizeLabel(uint8_t index) {
  static const char *kLabels[] = {"1/4", "1/8", "1/16", "1/32", "1/8T"};
  return (index < kQuantizeOptions) ? kLabels[index] : kLabels[0];
}

static uint8_t quantizeTicks(uint8_t index) {
  static const uint8_t kTicks[] = {24, 12, 6, 3, 8};
  return (index < kQuantizeOptions) ? kTicks[index] : kTicks[0];
}

static uint8_t clampConfiguredSlotCount() {
  if (slotSystemSlotCount < 1) {
    slotSystemSlotCount = 1;
  } else if (slotSystemSlotCount > kMaxSlots) {
    slotSystemSlotCount = kMaxSlots;
  }
  return slotSystemSlotCount;
}

static uint32_t tickIntervalMicros() {
  uint16_t bpm = gState.bpm;
  if (bpm < 40) bpm = 40;
  return 60000000UL / (static_cast<uint32_t>(bpm) * kTicksPerQuarter);
}

static uint16_t loopLengthTicksFromBars(uint8_t bars) {
  uint8_t safeBars = bars;
  if (safeBars < 1) safeBars = 1;
  if (safeBars > 8) safeBars = 8;
  return static_cast<uint16_t>(safeBars * kTicksPerBar);
}

static uint16_t quantizeTick(uint32_t relTick, uint8_t qTicks) {
  if (qTicks == 0) return 0;
  uint32_t rounded = (relTick + (qTicks / 2)) / qTicks;
  return static_cast<uint16_t>(rounded * qTicks);
}

static uint8_t clampDensity(uint8_t value) {
  if (value < 1) return 1;
  if (value > kPatternSteps) return kPatternSteps;
  return value;
}

static void touchActivity(uint8_t slotIndex) {
  if (slotIndex >= gState.configuredSlots) {
    return;
  }
  gState.slots[slotIndex].activity = true;
  gState.slots[slotIndex].activityUntilMs = millis() + 120;
}

static void sendSlotMidi(uint8_t slotIndex, uint8_t status, uint8_t data1, uint8_t data2) {
  if (slotIndex >= gState.configuredSlots) {
    return;
  }
  const SlotState &slot = gState.slots[slotIndex];
  if (!slot.muted) {
    sendMIDI(status, data1, data2);
  }
  touchActivity(slotIndex);
}

static void queueNoteOff(uint8_t slotIndex, uint8_t note, uint8_t velocity, uint32_t offTick) {
  for (PendingNoteOff &entry : gState.pendingNoteOffs) {
    if (!entry.active) {
      entry.active = true;
      entry.slotIndex = slotIndex;
      entry.note = note;
      entry.velocity = velocity;
      entry.offTick = offTick;
      return;
    }
  }
}

static bool isRecordingSource(uint8_t slotIndex) {
  return gState.recording && gState.recordSourceSlot == slotIndex && gState.recordTargetSlot != kNoSlot;
}

static void appendLoopEvent(SlotState &target, uint16_t tick, uint8_t note, uint8_t velocity, bool noteOn) {
  if (target.loopEventCount >= kMaxLoopEvents) {
    return;
  }
  if (tick >= target.loopLengthTicks) {
    tick = static_cast<uint16_t>(tick % target.loopLengthTicks);
  }
  LoopEvent &event = target.loopEvents[target.loopEventCount++];
  event.tick = tick;
  event.note = note;
  event.velocity = velocity;
  event.noteOn = noteOn;
  target.looperHasContent = true;
}

static void captureRecordEvent(uint8_t slotIndex, uint8_t note, uint8_t velocity, bool noteOn) {
  if (!isRecordingSource(slotIndex)) {
    return;
  }
  if (gState.recordTargetSlot >= gState.configuredSlots) {
    return;
  }

  SlotState &target = gState.slots[gState.recordTargetSlot];
  if (target.type != SlotType::LOOPER || target.loopLengthTicks == 0) {
    return;
  }

  uint32_t relTick = (gState.currentTick > gState.recordStartTick)
                         ? (gState.currentTick - gState.recordStartTick)
                         : 0;
  uint16_t qt = quantizeTick(relTick, target.loopQuantizeTicks);

  if (noteOn) {
    gState.captureNotes[note].active = true;
    gState.captureNotes[note].onTick = qt;
    appendLoopEvent(target, qt, note, velocity, true);
    return;
  }

  if (gState.captureNotes[note].active) {
    uint32_t minOffRaw = static_cast<uint32_t>(gState.captureNotes[note].onTick) +
                         static_cast<uint32_t>(target.loopQuantizeTicks);
    uint16_t minOff = static_cast<uint16_t>((minOffRaw > 0xFFFFu) ? 0xFFFFu : minOffRaw);
    if (qt < minOff) {
      qt = minOff;
    }
    gState.captureNotes[note].active = false;
  }

  appendLoopEvent(target, qt, note, velocity, false);
}

static void emitNoteWithDuration(uint8_t slotIndex, uint8_t note, uint8_t velocity, uint16_t durationTicks) {
  if (slotIndex >= gState.configuredSlots || durationTicks == 0) {
    return;
  }
  SlotState &slot = gState.slots[slotIndex];
  uint8_t channel = static_cast<uint8_t>((slot.midiChannel - 1) & 0x0F);
  sendSlotMidi(slotIndex, static_cast<uint8_t>(0x90 | channel), note, velocity);
  captureRecordEvent(slotIndex, note, velocity, true);
  queueNoteOff(slotIndex, note, 0, gState.currentTick + durationTicks);
}

static void processPendingNoteOffs() {
  for (PendingNoteOff &entry : gState.pendingNoteOffs) {
    if (!entry.active || gState.currentTick < entry.offTick) {
      continue;
    }
    entry.active = false;
    if (entry.slotIndex >= gState.configuredSlots) {
      continue;
    }
    SlotState &slot = gState.slots[entry.slotIndex];
    uint8_t channel = static_cast<uint8_t>((slot.midiChannel - 1) & 0x0F);
    sendSlotMidi(entry.slotIndex, static_cast<uint8_t>(0x80 | channel), entry.note, entry.velocity);
    captureRecordEvent(entry.slotIndex, entry.note, entry.velocity, false);
  }
}

static void clearLoop(SlotState &slot) {
  slot.loopEventCount = 0;
  slot.looperHasContent = false;
}

static uint8_t findOrCreateLooperTarget() {
  for (uint8_t i = 0; i < gState.configuredSlots; ++i) {
    if (gState.slots[i].type == SlotType::LOOPER) {
      return i;
    }
  }
  for (uint8_t i = 0; i < gState.configuredSlots; ++i) {
    if (gState.slots[i].type == SlotType::EMPTY) {
      gState.slots[i].type = SlotType::LOOPER;
      gState.slots[i].loopLengthTicks = loopLengthTicksFromBars(gState.bars);
      gState.slots[i].loopQuantizeTicks = quantizeTicks(gState.quantizeIndex);
      return i;
    }
  }
  return kNoSlot;
}

static void stopTransport() {
  gState.transportRunning = false;
  gState.recordPending = false;
  gState.recording = false;
  gState.recordSourceSlot = kNoSlot;
  gState.recordTargetSlot = kNoSlot;
  memset(gState.captureNotes, 0, sizeof(gState.captureNotes));
  for (PendingNoteOff &entry : gState.pendingNoteOffs) {
    entry.active = false;
  }
  midiOutBuffer.panic();
}

static void startTransport() {
  if (!gState.transportRunning) {
    gState.transportRunning = true;
    gState.lastTickMicros = micros();
  }
}

static void armRecordingForSelectedSlot() {
  if (gState.selectedSlot >= gState.configuredSlots) {
    return;
  }
  uint8_t target = findOrCreateLooperTarget();
  if (target == kNoSlot) {
    return;
  }

  gState.recordSourceSlot = gState.selectedSlot;
  gState.recordTargetSlot = target;
  gState.recordPending = true;
  gState.recording = false;
  gState.recordStartTick = ((gState.currentTick / kTicksPerBar) + 1) * kTicksPerBar;
  SlotState &targetSlot = gState.slots[target];
  targetSlot.loopLengthTicks = loopLengthTicksFromBars(gState.bars);
  targetSlot.loopQuantizeTicks = quantizeTicks(gState.quantizeIndex);
  if (gState.recordMode == RecordMode::REPLACE) {
    clearLoop(targetSlot);
  }
  memset(gState.captureNotes, 0, sizeof(gState.captureNotes));
  startTransport();
}

static void beginRecordingNow() {
  if (!gState.recordPending || gState.recordTargetSlot >= gState.configuredSlots) {
    return;
  }
  gState.recordPending = false;
  gState.recording = true;
  gState.recordStartTick = gState.currentTick;
  const SlotState &targetSlot = gState.slots[gState.recordTargetSlot];
  gState.recordEndTick = gState.recordStartTick + targetSlot.loopLengthTicks;
  memset(gState.captureNotes, 0, sizeof(gState.captureNotes));
}

static void maybeStopRecording() {
  if (!gState.recording) {
    return;
  }
  if (gState.currentTick < gState.recordEndTick) {
    return;
  }
  gState.recording = false;
  gState.recordSourceSlot = kNoSlot;
  gState.recordTargetSlot = kNoSlot;
  memset(gState.captureNotes, 0, sizeof(gState.captureNotes));
}

static void processLooperSlot(uint8_t slotIndex) {
  SlotState &slot = gState.slots[slotIndex];
  if (slot.type != SlotType::LOOPER || !slot.looperHasContent || slot.loopLengthTicks == 0) {
    return;
  }
  uint16_t loopTick = static_cast<uint16_t>(gState.currentTick % slot.loopLengthTicks);
  uint8_t channel = static_cast<uint8_t>((slot.midiChannel - 1) & 0x0F);
  for (uint16_t i = 0; i < slot.loopEventCount; ++i) {
    const LoopEvent &event = slot.loopEvents[i];
    if (event.tick != loopTick) {
      continue;
    }
    uint8_t status = event.noteOn ? static_cast<uint8_t>(0x90 | channel)
                                  : static_cast<uint8_t>(0x80 | channel);
    sendSlotMidi(slotIndex, status, event.note, event.velocity);
  }
}

static bool engineShouldTrigger(const SlotState &slot, uint16_t stepIndex) {
  if (slot.type == SlotType::ENGINE_EUCLID) {
    uint8_t pulses = clampDensity(slot.density);
    return ((stepIndex * pulses) % kPatternSteps) < pulses;
  }
  // Dimensions engine: controlled randomness from density.
  return random(kPatternSteps) < slot.density;
}

static void processEngineSlot(uint8_t slotIndex) {
  SlotState &slot = gState.slots[slotIndex];
  if (slot.type != SlotType::ENGINE_EUCLID && slot.type != SlotType::ENGINE_DIMENSIONS) {
    return;
  }

  while (gState.currentTick >= slot.nextEngineTick) {
    uint16_t step = slot.engineStepIndex++;
    if (engineShouldTrigger(slot, step)) {
      uint8_t note = static_cast<uint8_t>(slot.baseNote + (step % 8));
      emitNoteWithDuration(slotIndex, note, 96, 4);
    }

    uint8_t swingTicks =
        static_cast<uint8_t>((gState.swing * kSwingTicksMaxOffset) / 50);
    uint8_t baseStep = 6;
    uint8_t duration = baseStep;
    if ((step & 1U) == 0U) {
      duration = static_cast<uint8_t>(baseStep + swingTicks);
    } else if (swingTicks < baseStep) {
      duration = static_cast<uint8_t>(baseStep - swingTicks);
    }
    if (duration == 0) {
      duration = 1;
    }
    slot.nextEngineTick += duration;
  }
}

static void processTick() {
  gState.currentTick++;

  if (gState.recordPending && gState.currentTick >= gState.recordStartTick) {
    beginRecordingNow();
  }

  for (uint8_t i = 0; i < gState.configuredSlots; ++i) {
    processEngineSlot(i);
    processLooperSlot(i);
  }

  processPendingNoteOffs();
  maybeStopRecording();
}

static bool transportButtonPressed() {
  return isButtonPressed(MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(6), SCALE_X(58), SCALE_Y(30));
}

static bool panicButtonPressed() {
  return isButtonPressed(DISPLAY_WIDTH - MARGIN_SMALL - SCALE_X(58), HEADER_HEIGHT + SCALE_Y(6), SCALE_X(58), SCALE_Y(30));
}

static void drawTopControls() {
  int y = HEADER_HEIGHT + SCALE_Y(6);
  int btnW = SCALE_X(58);
  int btnH = SCALE_Y(30);

  drawRoundButton(MARGIN_SMALL, y, btnW, btnH, gState.transportRunning ? "STOP" : "PLAY",
                  gState.transportRunning ? THEME_ERROR : THEME_SUCCESS, false, 1);

  int bpmX = MARGIN_SMALL + btnW + SCALE_X(6);
  int bpmW = SCALE_X(66);
  drawRoundButton(bpmX, y, bpmW, btnH, String(gState.bpm) + " BPM", THEME_PRIMARY, false, 1);

  int swingX = bpmX + bpmW + SCALE_X(6);
  int swingW = SCALE_X(72);
  drawRoundButton(swingX, y, swingW, btnH, String("SW ") + gState.swing + "%", THEME_WARNING, false, 1);

  drawRoundButton(DISPLAY_WIDTH - MARGIN_SMALL - btnW, y, btnW, btnH, "PANIC", THEME_ERROR, false, 1);
}

static void computeGrid(int &gridY, int &gridH, int &cols, int &rows, int &cardW, int &cardH, int &pageSize) {
  int topControlsY = HEADER_HEIGHT + SCALE_Y(6);
  int topControlsH = SCALE_Y(30);
  gridY = topControlsY + topControlsH + SCALE_Y(8);
  int bottomBarY = DISPLAY_HEIGHT - SCALE_Y(38);
  gridH = bottomBarY - gridY - SCALE_Y(4);
  cols = (DISPLAY_WIDTH >= SCALE_X(400)) ? 3 : 2;
  rows = 2;
  pageSize = cols * rows;
  int gapX = SCALE_X(6);
  int gapY = SCALE_Y(6);
  cardW = (DISPLAY_WIDTH - 2 * MARGIN_SMALL - ((cols - 1) * gapX)) / cols;
  cardH = (gridH - ((rows - 1) * gapY)) / rows;
}

static void drawSlotCard(uint8_t slotIndex, int x, int y, int w, int h) {
  const SlotState &slot = gState.slots[slotIndex];
  bool selected = (slotIndex == gState.selectedSlot);
  uint16_t border = selected ? THEME_ACCENT : THEME_TEXT_DIM;
  uint16_t fill = selected ? THEME_SURFACE : THEME_BG;
  tft.fillRoundRect(x, y, w, h, 8, fill);
  tft.drawRoundRect(x, y, w, h, 8, border);

  tft.setTextColor(THEME_TEXT, fill);
  tft.drawString(String("S") + (slotIndex + 1), x + SCALE_X(6), y + SCALE_Y(5), 1);
  tft.drawRightString(slotTypeLabel(slot.type), x + w - SCALE_X(6), y + SCALE_Y(5), 1);
  tft.setTextColor(THEME_TEXT_DIM, fill);
  tft.drawString(String("CH ") + slot.midiChannel, x + SCALE_X(6), y + SCALE_Y(20), 1);

  const char *muteLabel = slot.muted ? "MUTE" : "LIVE";
  uint16_t muteColor = slot.muted ? THEME_WARNING : THEME_SUCCESS;
  tft.setTextColor(muteColor, fill);
  tft.drawRightString(muteLabel, x + w - SCALE_X(6), y + SCALE_Y(20), 1);

  if (slot.activity && millis() <= slot.activityUntilMs) {
    tft.fillCircle(x + SCALE_X(10), y + h - SCALE_Y(10), SCALE_X(4), THEME_PRIMARY);
  }

  if ((gState.recordPending || gState.recording) &&
      (slotIndex == gState.recordSourceSlot || slotIndex == gState.recordTargetSlot)) {
    uint16_t recColor = gState.recording ? THEME_ERROR : THEME_WARNING;
    tft.fillCircle(x + w - SCALE_X(10), y + h - SCALE_Y(10), SCALE_X(4), recColor);
  }
}

static void drawGrid() {
  int gridY, gridH, cols, rows, cardW, cardH, pageSize;
  computeGrid(gridY, gridH, cols, rows, cardW, cardH, pageSize);

  uint8_t maxPage = static_cast<uint8_t>((gState.configuredSlots + pageSize - 1) / pageSize);
  if (gState.page >= maxPage) {
    gState.page = 0;
  }
  uint8_t start = static_cast<uint8_t>(gState.page * pageSize);
  int gapX = SCALE_X(6);
  int gapY = SCALE_Y(6);

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      uint8_t index = static_cast<uint8_t>(start + row * cols + col);
      if (index >= gState.configuredSlots) {
        continue;
      }
      int x = MARGIN_SMALL + col * (cardW + gapX);
      int y = gridY + row * (cardH + gapY);
      drawSlotCard(index, x, y, cardW, cardH);
    }
  }

  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String pageText = String(gState.page + 1) + "/" + String(maxPage);
  tft.drawCentreString(pageText, DISPLAY_CENTER_X, DISPLAY_HEIGHT - SCALE_Y(52), 1);
}

static void drawBottomBar() {
  int y = DISPLAY_HEIGHT - SCALE_Y(38);
  int h = SCALE_Y(30);
  int gap = SCALE_X(6);
  int x = MARGIN_SMALL;
  int w = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - (5 * gap)) / 6;

  drawRoundButton(x, y, w, h, "REC", THEME_ERROR, gState.recordPending || gState.recording, 1);
  x += w + gap;
  drawRoundButton(x, y, w, h, gState.recordMode == RecordMode::OVERDUB ? "OVR" : "RPL",
                  THEME_SECONDARY, false, 1);
  x += w + gap;
  drawRoundButton(x, y, w, h, String(gState.bars) + "B", THEME_PRIMARY, false, 1);
  x += w + gap;
  drawRoundButton(x, y, w, h, quantizeLabel(gState.quantizeIndex), THEME_ACCENT, false, 1);
  x += w + gap;
  drawRoundButton(x, y, w, h, "<", THEME_TEXT_DIM, false, 1);
  x += w + gap;
  drawRoundButton(x, y, w, h, ">", THEME_TEXT_DIM, false, 1);
}

static void drawDetailModal() {
  const SlotState &slot = gState.slots[gState.selectedSlot];
  int w = DISPLAY_WIDTH - SCALE_X(26);
  int h = DISPLAY_HEIGHT - HEADER_HEIGHT - SCALE_Y(20);
  int x = SCALE_X(13);
  int y = HEADER_HEIGHT + SCALE_Y(10);

  tft.fillRoundRect(x, y, w, h, 10, THEME_SURFACE);
  tft.drawRoundRect(x, y, w, h, 10, THEME_ACCENT);

  tft.setTextColor(THEME_TEXT, THEME_SURFACE);
  tft.drawString(String("SLOT S") + (gState.selectedSlot + 1), x + SCALE_X(10), y + SCALE_Y(8), 2);
  drawRoundButton(x + w - SCALE_X(52), y + SCALE_Y(6), SCALE_X(42), SCALE_Y(24), "X", THEME_ERROR, false, 1);

  int rowY = y + SCALE_Y(38);
  int rowH = SCALE_Y(28);
  drawRoundButton(x + SCALE_X(10), rowY, w - SCALE_X(20), rowH, String("TYPE ") + slotTypeLabel(slot.type), THEME_PRIMARY, false, 1);
  rowY += rowH + SCALE_Y(8);

  int halfW = (w - SCALE_X(30)) / 2;
  drawRoundButton(x + SCALE_X(10), rowY, halfW, rowH, String("CH- ") + slot.midiChannel, THEME_ACCENT, false, 1);
  drawRoundButton(x + SCALE_X(20) + halfW, rowY, halfW, rowH, String("CH+ ") + slot.midiChannel, THEME_ACCENT, false, 1);
  rowY += rowH + SCALE_Y(8);

  drawRoundButton(x + SCALE_X(10), rowY, halfW, rowH, slot.muted ? "UNMUTE" : "MUTE", THEME_WARNING, false, 1);
  drawRoundButton(x + SCALE_X(20) + halfW, rowY, halfW, rowH,
                  slot.type == SlotType::LOOPER ? "CLEAR" : "ARM REC", THEME_SECONDARY, false, 1);
  rowY += rowH + SCALE_Y(8);

  if (slot.type == SlotType::LIVE_INPUT) {
    drawRoundButton(x + SCALE_X(10), rowY, w - SCALE_X(20), rowH, "TRIG NOTE", THEME_SUCCESS, false, 1);
    rowY += rowH + SCALE_Y(8);
  }

  tft.setTextColor(THEME_TEXT_DIM, THEME_SURFACE);
  tft.drawString("Reassign wipes slot state", x + SCALE_X(10), rowY + SCALE_Y(4), 1);
}

static void resetSlotState(uint8_t slotIndex, SlotType newType) {
  if (slotIndex >= gState.configuredSlots) {
    return;
  }
  SlotState replacement;
  replacement.type = newType;
  replacement.midiChannel = static_cast<uint8_t>((slotIndex % kMaxMidiChannels) + 1);
  replacement.baseNote = static_cast<uint8_t>(48 + (slotIndex * 2));
  replacement.loopLengthTicks = loopLengthTicksFromBars(gState.bars);
  replacement.loopQuantizeTicks = quantizeTicks(gState.quantizeIndex);
  gState.slots[slotIndex] = replacement;
  midiOutBuffer.allNotesOff(static_cast<uint8_t>((replacement.midiChannel - 1) & 0x0F));
}

static SlotType nextSlotType(SlotType type) {
  switch (type) {
    case SlotType::EMPTY:
      return SlotType::ENGINE_EUCLID;
    case SlotType::ENGINE_EUCLID:
      return SlotType::ENGINE_DIMENSIONS;
    case SlotType::ENGINE_DIMENSIONS:
      return SlotType::LIVE_INPUT;
    case SlotType::LIVE_INPUT:
      return SlotType::LOOPER;
    case SlotType::LOOPER:
      return SlotType::EMPTY;
    default:
      return SlotType::EMPTY;
  }
}

}  // namespace

void initializeSlotPerformerMode() {
  randomSeed(micros());
  gState.bpm = sharedBPM;
  gState.swing = 0;
  gState.configuredSlots = clampConfiguredSlotCount();
  gState.selectedSlot = 0;
  gState.page = 0;
  gState.currentTick = 0;
  gState.transportRunning = false;
  gState.bars = 4;
  gState.quantizeIndex = 2;
  gState.recordMode = RecordMode::REPLACE;
  gState.recordPending = false;
  gState.recording = false;
  gState.recordSourceSlot = kNoSlot;
  gState.recordTargetSlot = kNoSlot;
  gState.detailOpen = false;
  memset(gState.pendingNoteOffs, 0, sizeof(gState.pendingNoteOffs));
  memset(gState.captureNotes, 0, sizeof(gState.captureNotes));

  for (uint8_t i = 0; i < kMaxSlots; ++i) {
    resetSlotState(i, SlotType::EMPTY);
  }

  if (gState.configuredSlots > 0) resetSlotState(0, SlotType::ENGINE_EUCLID);
  if (gState.configuredSlots > 1) resetSlotState(1, SlotType::ENGINE_DIMENSIONS);
  if (gState.configuredSlots > 2) resetSlotState(2, SlotType::LIVE_INPUT);
  if (gState.configuredSlots > 3) resetSlotState(3, SlotType::LOOPER);
}

void drawSlotPerformerMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("SLOTS", "Multi Sequencer", 3);
  drawTopControls();
  drawGrid();
  drawBottomBar();
  if (gState.detailOpen) {
    drawDetailModal();
  }
}

void handleSlotPerformerMode() {
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    stopTransport();
    exitToMenu();
    return;
  }

  gState.configuredSlots = clampConfiguredSlotCount();
  if (gState.selectedSlot >= gState.configuredSlots) {
    gState.selectedSlot = 0;
  }

  if (gState.transportRunning) {
    uint32_t nowMicros = micros();
    uint32_t interval = tickIntervalMicros();
    while (nowMicros - gState.lastTickMicros >= interval) {
      gState.lastTickMicros += interval;
      processTick();
    }
  }

  if (!touch.justPressed) {
    return;
  }

  if (gState.detailOpen) {
    int w = DISPLAY_WIDTH - SCALE_X(26);
    int h = DISPLAY_HEIGHT - HEADER_HEIGHT - SCALE_Y(20);
    int x = SCALE_X(13);
    int y = HEADER_HEIGHT + SCALE_Y(10);
    int rowY = y + SCALE_Y(38);
    int rowH = SCALE_Y(28);
    int halfW = (w - SCALE_X(30)) / 2;

    if (isButtonPressed(x + w - SCALE_X(52), y + SCALE_Y(6), SCALE_X(42), SCALE_Y(24))) {
      gState.detailOpen = false;
      requestRedraw();
      return;
    }

    if (isButtonPressed(x + SCALE_X(10), rowY, w - SCALE_X(20), rowH)) {
      SlotType nextType = nextSlotType(gState.slots[gState.selectedSlot].type);
      resetSlotState(gState.selectedSlot, nextType);
      requestRedraw();
      return;
    }
    rowY += rowH + SCALE_Y(8);

    if (isButtonPressed(x + SCALE_X(10), rowY, halfW, rowH)) {
      SlotState &slot = gState.slots[gState.selectedSlot];
      slot.midiChannel = (slot.midiChannel <= 1) ? kMaxMidiChannels : (slot.midiChannel - 1);
      requestRedraw();
      return;
    }
    if (isButtonPressed(x + SCALE_X(20) + halfW, rowY, halfW, rowH)) {
      SlotState &slot = gState.slots[gState.selectedSlot];
      slot.midiChannel = (slot.midiChannel >= kMaxMidiChannels) ? 1 : (slot.midiChannel + 1);
      requestRedraw();
      return;
    }
    rowY += rowH + SCALE_Y(8);

    if (isButtonPressed(x + SCALE_X(10), rowY, halfW, rowH)) {
      gState.slots[gState.selectedSlot].muted = !gState.slots[gState.selectedSlot].muted;
      requestRedraw();
      return;
    }
    if (isButtonPressed(x + SCALE_X(20) + halfW, rowY, halfW, rowH)) {
      SlotState &slot = gState.slots[gState.selectedSlot];
      if (slot.type == SlotType::LOOPER) {
        clearLoop(slot);
      } else {
        armRecordingForSelectedSlot();
      }
      requestRedraw();
      return;
    }
    rowY += rowH + SCALE_Y(8);

    if (gState.slots[gState.selectedSlot].type == SlotType::LIVE_INPUT &&
        isButtonPressed(x + SCALE_X(10), rowY, w - SCALE_X(20), rowH)) {
      uint8_t note = static_cast<uint8_t>(gState.slots[gState.selectedSlot].baseNote);
      emitNoteWithDuration(gState.selectedSlot, note, 110, 6);
      requestRedraw();
      return;
    }
    return;
  }

  if (transportButtonPressed()) {
    if (gState.transportRunning) {
      stopTransport();
    } else {
      startTransport();
    }
    requestRedraw();
    return;
  }

  if (panicButtonPressed()) {
    midiOutBuffer.panic();
    requestRedraw();
    return;
  }

  int controlsY = HEADER_HEIGHT + SCALE_Y(6);
  int btnW = SCALE_X(58);
  int btnH = SCALE_Y(30);
  int bpmX = MARGIN_SMALL + btnW + SCALE_X(6);
  int bpmW = SCALE_X(66);
  int swingX = bpmX + bpmW + SCALE_X(6);
  int swingW = SCALE_X(72);
  if (isButtonPressed(bpmX, controlsY, bpmW, btnH)) {
    gState.bpm = (gState.bpm >= 240) ? 40 : static_cast<uint16_t>(gState.bpm + 5);
    setSharedBPM(gState.bpm);
    requestRedraw();
    return;
  }
  if (isButtonPressed(swingX, controlsY, swingW, btnH)) {
    gState.swing = (gState.swing >= 50) ? 0 : static_cast<uint8_t>(gState.swing + 5);
    requestRedraw();
    return;
  }

  int bottomY = DISPLAY_HEIGHT - SCALE_Y(38);
  int h = SCALE_Y(30);
  int gap = SCALE_X(6);
  int x = MARGIN_SMALL;
  int w = (DISPLAY_WIDTH - (2 * MARGIN_SMALL) - (5 * gap)) / 6;
  if (isButtonPressed(x, bottomY, w, h)) {
    if (gState.recording || gState.recordPending) {
      gState.recordPending = false;
      gState.recording = false;
      gState.recordSourceSlot = kNoSlot;
      gState.recordTargetSlot = kNoSlot;
    } else {
      armRecordingForSelectedSlot();
    }
    requestRedraw();
    return;
  }
  x += w + gap;
  if (isButtonPressed(x, bottomY, w, h)) {
    gState.recordMode = (gState.recordMode == RecordMode::REPLACE) ? RecordMode::OVERDUB : RecordMode::REPLACE;
    requestRedraw();
    return;
  }
  x += w + gap;
  if (isButtonPressed(x, bottomY, w, h)) {
    static const uint8_t kBars[] = {1, 2, 4, 8};
    static constexpr size_t kBarsCount = sizeof(kBars) / sizeof(kBars[0]);
    uint8_t idx = 0;
    for (; idx < kBarsCount; ++idx) {
      if (kBars[idx] == gState.bars) break;
    }
    gState.bars = kBars[(idx + 1) % kBarsCount];
    requestRedraw();
    return;
  }
  x += w + gap;
  if (isButtonPressed(x, bottomY, w, h)) {
    gState.quantizeIndex =
        static_cast<uint8_t>((gState.quantizeIndex + 1) % kQuantizeOptions);
    requestRedraw();
    return;
  }
  x += w + gap;
  if (isButtonPressed(x, bottomY, w, h)) {
    gState.page = (gState.page == 0) ? 0 : static_cast<uint8_t>(gState.page - 1);
    requestRedraw();
    return;
  }
  x += w + gap;
  if (isButtonPressed(x, bottomY, w, h)) {
    gState.page = static_cast<uint8_t>(gState.page + 1);
    requestRedraw();
    return;
  }

  int gridY, gridH, cols, rows, cardW, cardH, pageSize;
  computeGrid(gridY, gridH, cols, rows, cardW, cardH, pageSize);
  uint8_t start = static_cast<uint8_t>(gState.page * pageSize);
  int gapX = SCALE_X(6);
  int gapY = SCALE_Y(6);
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      uint8_t slotIndex = static_cast<uint8_t>(start + row * cols + col);
      if (slotIndex >= gState.configuredSlots) {
        continue;
      }
      int cardX = MARGIN_SMALL + col * (cardW + gapX);
      int cardY = gridY + row * (cardH + gapY);
      if (isButtonPressed(cardX, cardY, cardW, cardH)) {
        gState.selectedSlot = slotIndex;
        gState.detailOpen = true;
        requestRedraw();
        return;
      }
    }
  }
}
