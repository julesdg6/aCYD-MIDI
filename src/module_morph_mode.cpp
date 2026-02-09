#include "module_morph_mode.h"

#include <algorithm>
#include <pgmspace.h>

#include "color_utils.h"

MorphState morphState;

namespace {

// Sampling/playback timing (ms)
constexpr uint32_t kMorphRecordIntervalMs = 25;
constexpr uint32_t kMorphPlayIntervalMs = 25;
constexpr uint32_t kMorphNoteIntervalMs = 120;
constexpr uint32_t kMorphNoteIdleMs = 250;
constexpr uint32_t kMorphSlotHoldClearMs = 650;

constexpr uint8_t kNoNote = 0xFF;
uint8_t heldNote = kNoNote;
uint32_t lastNoteStepMs = 0;
uint32_t lastOutputActivityMs = 0;
int slotHoldIndex = -1;
uint32_t slotHoldStartMs = 0;
bool slotHoldTriggered = false;

constexpr uint16_t kMorphQuantizeLengths[] = {16, 24, 32, 48, 64, 96};
constexpr uint8_t kMorphSmoothingPasses = 2;

inline bool timeReached(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

inline uint8_t clampU8(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return static_cast<uint8_t>(value);
}

inline uint8_t lerpU8(uint8_t a, uint8_t b, uint8_t amount) {
  return a + (((static_cast<int>(b) - static_cast<int>(a)) * amount) >> 8);
}

inline uint8_t bilinearInterpolateU8(uint8_t v00, uint8_t v10, uint8_t v01, uint8_t v11,
                                     uint8_t x, uint8_t y) {
  uint8_t v0 = lerpU8(v00, v10, x);
  uint8_t v1 = lerpU8(v01, v11, x);
  return lerpU8(v0, v1, y);
}

void morphStopHeldNote() {
  if (heldNote != kNoNote) {
    sendMIDI(0x80, heldNote, 0);
    heldNote = kNoNote;
  }
}

void morphUpdateHeldNoteIdleOff(uint32_t now) {
  if (heldNote == kNoNote) {
    return;
  }
  if (morphState.playing || morphState.recording) {
    return;
  }
  if (!timeReached(now, lastOutputActivityMs + kMorphNoteIdleMs)) {
    return;
  }
  morphStopHeldNote();
}

uint8_t morphNoteFromPoint(uint8_t x, uint8_t y) {
  // Keep the existing "X adds ~1 octave, Y adds ~1/2 octave" feel, but do it with integer math.
  uint8_t note = 60;
  note = static_cast<uint8_t>(note + (static_cast<uint16_t>(x) * 12) / 255);
  note = static_cast<uint8_t>(note + (static_cast<uint16_t>(y) * 6) / 255);
  return note;
}

void morphUpdateNoteFromOutput(uint32_t now, bool force = false) {
  if (!force && lastNoteStepMs != 0 && !timeReached(now, lastNoteStepMs + kMorphNoteIntervalMs)) {
    return;
  }

  lastNoteStepMs = now;
  uint8_t note = morphNoteFromPoint(morphState.outputX, morphState.outputY);
  if (note == heldNote) {
    return;
  }

  morphStopHeldNote();
  sendMIDI(0x90, note, 100);
  heldNote = note;
}

bool morphHasAnyRecording() {
  for (int i = 0; i < MORPH_SLOTS; ++i) {
    if (morphState.slots[i].hasData && morphState.slots[i].length > 0) {
      return true;
    }
  }
  return false;
}

uint16_t morphPlaybackLength() {
  uint16_t maxLen = 0;
  for (int i = 0; i < MORPH_SLOTS; ++i) {
    if (morphState.slots[i].hasData) {
      maxLen = std::max(maxLen, morphState.slots[i].length);
    }
  }
  return maxLen;
}

MorphPoint morphSlotPointAt(const MorphSlot &slot, uint8_t t) {
  if (!slot.hasData || slot.length == 0) {
    return MorphPoint{128, 128};
  }
  if (slot.length == 1) {
    return slot.points[0];
  }
  uint16_t idx = (static_cast<uint32_t>(t) * (slot.length - 1)) / 255;
  return slot.points[idx];
}

MorphPoint morphComputeOutputPoint(uint16_t playIndex, uint16_t playLen, uint8_t mixX, uint8_t mixY) {
  const uint8_t t = (playLen > 1) ? static_cast<uint8_t>((static_cast<uint32_t>(playIndex) * 255) / (playLen - 1))
                                  : 0;

  const MorphSlot &slot00 = morphState.slots[0];
  const MorphSlot &slot10 = morphState.slots[1];
  const MorphSlot &slot01 = morphState.slots[2];
  const MorphSlot &slot11 = morphState.slots[3];

  const MorphSlot &fallback =
      (morphState.slots[morphState.activeSlot].hasData && morphState.slots[morphState.activeSlot].length > 0)
          ? morphState.slots[morphState.activeSlot]
          : slot00;

  const MorphPoint p00 = slot00.hasData ? morphSlotPointAt(slot00, t) : morphSlotPointAt(fallback, t);
  const MorphPoint p10 = slot10.hasData ? morphSlotPointAt(slot10, t) : morphSlotPointAt(fallback, t);
  const MorphPoint p01 = slot01.hasData ? morphSlotPointAt(slot01, t) : morphSlotPointAt(fallback, t);
  const MorphPoint p11 = slot11.hasData ? morphSlotPointAt(slot11, t) : morphSlotPointAt(fallback, t);

  return MorphPoint{
      bilinearInterpolateU8(p00.x, p10.x, p01.x, p11.x, mixX, mixY),
      bilinearInterpolateU8(p00.y, p10.y, p01.y, p11.y, mixX, mixY),
  };
}

void morphClearSlot(uint8_t index) {
  if (index >= MORPH_SLOTS) {
    return;
  }
  morphState.slots[index].hasData = false;
  morphState.slots[index].length = 0;

  if (!morphHasAnyRecording()) {
    morphState.playing = false;
    morphState.playIndex = 0;
    morphStopHeldNote();
  }
}

uint16_t morphQuantizeLength(uint16_t length) {
  if (length <= 1) {
    return length;
  }

  uint16_t best = kMorphQuantizeLengths[0];
  uint16_t bestDelta = static_cast<uint16_t>(abs(static_cast<int>(length) - static_cast<int>(best)));
  for (size_t i = 1; i < (sizeof(kMorphQuantizeLengths) / sizeof(kMorphQuantizeLengths[0])); ++i) {
    uint16_t candidate = kMorphQuantizeLengths[i];
    uint16_t delta = static_cast<uint16_t>(abs(static_cast<int>(length) - static_cast<int>(candidate)));
    if (delta < bestDelta) {
      best = candidate;
      bestDelta = delta;
    }
  }
  return std::min<uint16_t>(best, MORPH_MAX_POINTS);
}

void morphResampleSlot(MorphSlot &slot, uint16_t newLen) {
  if (!slot.hasData || slot.length == 0 || newLen == 0) {
    slot.hasData = false;
    slot.length = 0;
    return;
  }
  if (newLen == slot.length) {
    return;
  }
  newLen = std::max<uint16_t>(1, std::min<uint16_t>(newLen, MORPH_MAX_POINTS));

  MorphPoint tmp[MORPH_MAX_POINTS];
  const uint16_t oldLen = slot.length;

  if (oldLen == 1) {
    for (uint16_t i = 0; i < newLen; ++i) {
      tmp[i] = slot.points[0];
    }
  } else if (newLen == 1) {
    tmp[0] = slot.points[0];
  } else {
    for (uint16_t i = 0; i < newLen; ++i) {
      uint32_t scaled = (static_cast<uint32_t>(i) * (oldLen - 1) * 256) / (newLen - 1);
      uint16_t idx = static_cast<uint16_t>(scaled >> 8);
      uint8_t frac = static_cast<uint8_t>(scaled & 0xFF);
      const MorphPoint a = slot.points[idx];
      const MorphPoint b = slot.points[std::min<uint16_t>(static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(oldLen - 1))];

      auto interp = [&](uint8_t va, uint8_t vb) -> uint8_t {
        int delta = static_cast<int>(vb) - static_cast<int>(va);
        int out = static_cast<int>(va) + ((delta * frac) >> 8);
        return clampU8(out);
      };

      tmp[i] = MorphPoint{interp(a.x, b.x), interp(a.y, b.y)};
    }
  }

  for (uint16_t i = 0; i < newLen; ++i) {
    slot.points[i] = tmp[i];
  }
  slot.length = newLen;
  slot.hasData = (slot.length > 0);
}

void morphSmoothSlot(MorphSlot &slot, uint8_t passes) {
  if (!slot.hasData || slot.length < 3 || passes == 0) {
    return;
  }

  MorphPoint tmp[MORPH_MAX_POINTS];
  for (uint8_t pass = 0; pass < passes; ++pass) {
    tmp[0] = slot.points[0];
    for (uint16_t i = 1; i < (slot.length - 1); ++i) {
      const MorphPoint prev = slot.points[i - 1];
      const MorphPoint curr = slot.points[i];
      const MorphPoint next = slot.points[i + 1];

      auto smooth = [&](uint8_t a, uint8_t b, uint8_t c) -> uint8_t {
        int value = static_cast<int>(a) + 2 * static_cast<int>(b) + static_cast<int>(c);
        return clampU8(value / 4);
      };

      tmp[i] = MorphPoint{smooth(prev.x, curr.x, next.x), smooth(prev.y, curr.y, next.y)};
    }
    tmp[slot.length - 1] = slot.points[slot.length - 1];

    for (uint16_t i = 0; i < slot.length; ++i) {
      slot.points[i] = tmp[i];
    }
  }
}

void morphFinalizeRecording(uint8_t slotIndex) {
  if (slotIndex >= MORPH_SLOTS) {
    return;
  }

  MorphSlot &slot = morphState.slots[slotIndex];
  if (!slot.hasData || slot.length == 0) {
    morphClearSlot(slotIndex);
    return;
  }

  const uint16_t quantizedLen = morphQuantizeLength(slot.length);
  morphResampleSlot(slot, quantizedLen);
  morphSmoothSlot(slot, kMorphSmoothingPasses);
}

}  // namespace

static const uint16_t PROGMEM slotColors[MORPH_SLOTS] = {
    THEME_ERROR,
    THEME_WARNING,
    THEME_SUCCESS,
    THEME_ACCENT,
};

static void drawSlot(int index, int x, int y, int w, int h) {
  const uint16_t slotColor = pgm_read_word(&slotColors[index]);
  const bool isActive = (morphState.activeSlot == index);
  const bool hasData = morphState.slots[index].hasData && morphState.slots[index].length > 0;
  const bool isRecording = morphState.recording && isActive;
  uint16_t fill = hasData ? slotColor : THEME_BG;
  uint16_t border = hasData ? THEME_TEXT_DIM : slotColor;
  
  // Active slot gets brighter border
  if (isActive) {
    if (hasData) {
      border = THEME_TEXT;
      fill = blendColor(fill, THEME_TEXT, 200);
    }
  }

  if (isRecording) {
    border = morphState.recordGestureActive ? THEME_ERROR : THEME_WARNING;
  }
  
  tft.fillRoundRect(x, y, w, h, 4, fill);
  tft.drawRoundRect(x, y, w, h, 4, border);
  if (isActive && !hasData) {
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 3, THEME_TEXT);
  }
  
  // Slot label
  tft.setTextColor(THEME_BG, fill);
  if (!hasData) {
    tft.setTextColor(slotColor, fill);
  }
  tft.drawCentreString(String(index + 1), x + w / 2, y + h / 2 - SCALE_Y(8), 2);
}

static void drawGestureTrail(const MorphSlot &slot,
                             int padX, int padY, int padW, int padH,
                             uint16_t color) {
  if (!slot.hasData || slot.length == 0) {
    return;
  }

  const int w = std::max(padW - 1, 1);
  const int h = std::max(padH - 1, 1);

  auto px = [&](uint8_t v) { return padX + (static_cast<int>(v) * w) / 255; };
  auto py = [&](uint8_t v) { return padY + (static_cast<int>(v) * h) / 255; };

  int lastX = px(slot.points[0].x);
  int lastY = py(slot.points[0].y);
  if (slot.length == 1) {
    tft.fillCircle(lastX, lastY, SCALE_X(2), color);
    return;
  }

  for (uint16_t i = 1; i < slot.length; ++i) {
    int x = px(slot.points[i].x);
    int y = py(slot.points[i].y);
    tft.drawLine(lastX, lastY, x, y, color);
    lastX = x;
    lastY = y;
  }
}

struct MorphLayout {
  int padX;
  int padY;
  int padW;
  int padH;
  int slotX;
  int slotY;
  int slotW;
  int slotH;
  int slotSpacing;
  int controlX;
  int controlY;
  int controlW;
  int controlH;
};

static MorphLayout calculateMorphLayout() {
  MorphLayout layout;
  
  // XY Pad - maximize size, leave room for controls at bottom
  layout.padX = MARGIN_SMALL;
  layout.padY = HEADER_HEIGHT + SCALE_Y(10);
  layout.padW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
  layout.padH = SCALE_Y(110);  // Larger pad area
  
  // Control row at bottom
  layout.controlH = SCALE_Y(35);
  layout.controlY = DISPLAY_HEIGHT - layout.controlH - SCALE_Y(8);
  
  // Slot controls - bottom left (compact horizontal row)
  layout.slotY = layout.controlY;
  layout.slotX = MARGIN_SMALL;
  layout.slotH = layout.controlH;
  layout.slotSpacing = SCALE_X(6);
  
  // PLAY/RECORD buttons - bottom right (grouped together)
  const int rowLeft = MARGIN_SMALL;
  const int rowRight = DISPLAY_WIDTH - MARGIN_SMALL;
  const int rowAvailableW = std::max(rowRight - rowLeft, 0);
  const int groupGap = GAP_MEDIUM;
  const int desiredControlW = SCALE_X(140);
  const int minControlW = SCALE_X(110);
  const int minSlotW = SCALE_X(18);
  const int minSlotGroupW = MORPH_SLOTS * minSlotW + (MORPH_SLOTS - 1) * layout.slotSpacing;
  const int maxSlotW = SCALE_X(36);

  const int maxControlW = rowAvailableW - groupGap - minSlotGroupW;
  layout.controlW = std::min(desiredControlW, std::max(maxControlW, minControlW));
  layout.controlX = rowRight - layout.controlW;

  int slotGroupW = layout.controlX - groupGap - layout.slotX;
  slotGroupW = std::max(slotGroupW, 0);
  const int computedSlotW =
      (slotGroupW - (MORPH_SLOTS - 1) * layout.slotSpacing) / MORPH_SLOTS;
  layout.slotW = std::min(maxSlotW, std::max(computedSlotW, minSlotW));
  
  return layout;
}

void playMorphNote() {
  uint32_t now = millis();
  lastOutputActivityMs = now;
  // Force an immediate note update even if we're mid-interval.
  morphUpdateNoteFromOutput(now, true);
}

void initializeMorphMode() {
  morphState.morphX = 0.5f;
  morphState.morphY = 0.5f;
  morphState.activeSlot = 0;
  morphState.recording = false;
  morphState.recordGestureActive = false;
  morphState.playing = false;
  morphState.playIndex = 0;
  morphState.lastRecordSampleMs = 0;
  morphState.lastPlayStepMs = 0;
  morphState.outputX = 128;
  morphState.outputY = 128;
  for (int i = 0; i < MORPH_SLOTS; ++i) {
    morphState.slots[i].hasData = false;
    morphState.slots[i].length = 0;
  }
  lastNoteStepMs = 0;
  morphStopHeldNote();
  drawMorphMode();
}

void drawMorphMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("MORPH", "REC slots | PLAY loop | XY morph", 3);

  const MorphLayout layout = calculateMorphLayout();
  
  // Draw XY Pad area
  tft.drawRect(layout.padX, layout.padY, layout.padW, layout.padH, THEME_TEXT_DIM);
  tft.drawFastHLine(layout.padX, layout.padY + layout.padH / 2, layout.padW, THEME_TEXT_DIM);
  tft.drawFastVLine(layout.padX + layout.padW / 2, layout.padY, layout.padH, THEME_TEXT_DIM);
  
  // Corner badges show how slots map to the pad (1-4)
  const int badgeR = SCALE_X(8);
  struct CornerBadge {
    int dx;
    int dy;
    int slot;
  };
  const CornerBadge badges[4] = {
      {badgeR + SCALE_X(4), badgeR + SCALE_Y(4), 0},                                     // top-left
      {layout.padW - badgeR - SCALE_X(5), badgeR + SCALE_Y(4), 1},                       // top-right
      {badgeR + SCALE_X(4), layout.padH - badgeR - SCALE_Y(5), 2},                       // bottom-left
      {layout.padW - badgeR - SCALE_X(5), layout.padH - badgeR - SCALE_Y(5), 3},         // bottom-right
  };
  for (const auto &b : badges) {
    uint16_t c = pgm_read_word(&slotColors[b.slot]);
    int cx = layout.padX + b.dx;
    int cy = layout.padY + b.dy;
    tft.fillCircle(cx, cy, badgeR, c);
    tft.drawCircle(cx, cy, badgeR, THEME_TEXT_DIM);
  }

  // Draw recorded trails (dimmed)
  for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
    if (!morphState.slots[slot].hasData) {
      continue;
    }
    const uint16_t base = pgm_read_word(&slotColors[slot]);
    const bool isActive = (morphState.activeSlot == slot);
    const uint16_t trailColor = blendColor(base, THEME_BG, isActive ? 120 : 170);
    drawGestureTrail(morphState.slots[slot], layout.padX, layout.padY, layout.padW, layout.padH, trailColor);
  }

  // Draw the morphed playback path (computed) when playing
  if (morphState.playing && morphHasAnyRecording()) {
    const uint16_t playLen = morphPlaybackLength();
    if (playLen >= 2) {
      const uint8_t mixX = clampU8(static_cast<int>(morphState.morphX * 255.0f + 0.5f));
      const uint8_t mixY = clampU8(static_cast<int>(morphState.morphY * 255.0f + 0.5f));
      const uint16_t pathColor = blendColor(THEME_PRIMARY, THEME_BG, 120);

      const int w = std::max(layout.padW - 1, 1);
      const int h = std::max(layout.padH - 1, 1);
      auto px = [&](uint8_t v) { return layout.padX + (static_cast<int>(v) * w) / 255; };
      auto py = [&](uint8_t v) { return layout.padY + (static_cast<int>(v) * h) / 255; };

      MorphPoint p = morphComputeOutputPoint(0, playLen, mixX, mixY);
      int lastX = px(p.x);
      int lastY = py(p.y);
      for (uint16_t i = 1; i < playLen; ++i) {
        p = morphComputeOutputPoint(i, playLen, mixX, mixY);
        int x = px(p.x);
        int y = py(p.y);
        tft.drawLine(lastX, lastY, x, y, pathColor);
        lastX = x;
        lastY = y;
      }
    }
  }

  // Mix handle (square) + Output cursor (circle)
  const int mixX = layout.padX + static_cast<int>(morphState.morphX * std::max(layout.padW - 1, 1));
  const int mixY = layout.padY + static_cast<int>(morphState.morphY * std::max(layout.padH - 1, 1));
  tft.drawRect(mixX - SCALE_X(5), mixY - SCALE_Y(5), SCALE_X(10), SCALE_Y(10), THEME_WARNING);

  const int outX = layout.padX + (static_cast<int>(morphState.outputX) * std::max(layout.padW - 1, 1)) / 255;
  const int outY = layout.padY + (static_cast<int>(morphState.outputY) * std::max(layout.padH - 1, 1)) / 255;
  tft.fillCircle(outX, outY, SCALE_X(6), THEME_PRIMARY);
  tft.drawCircle(outX, outY, SCALE_X(6), THEME_TEXT);
  
  // Readouts + status (adapt based on available vertical space)
  const int padBottom = layout.padY + layout.padH;
  const int gapToControls = layout.controlY - padBottom;
  const bool compactInfo = gapToControls < 60;
  const int readoutFont = compactInfo ? 2 : 4;

  int readoutY = padBottom + SCALE_Y(6);
  tft.setTextColor(THEME_TEXT, THEME_BG);
  String xStr = "X: " + String(static_cast<int>(morphState.morphX * 100)) + "%";
  String yStr = "Y: " + String(static_cast<int>(morphState.morphY * 100)) + "%";
  tft.drawString(xStr, layout.padX, readoutY, readoutFont);
  tft.drawString(yStr, layout.padX + layout.padW / 2 + SCALE_X(20), readoutY, readoutFont);

  auto statusText = [&]() -> String {
    if (morphState.recording) {
      if (!morphState.recordGestureActive) {
        return "REC ARM S" + String(morphState.activeSlot + 1) + " - touch pad";
      }
      const MorphSlot &slot = morphState.slots[morphState.activeSlot];
      return "REC S" + String(morphState.activeSlot + 1) + "  " +
             String(slot.length) + "/" + String(MORPH_MAX_POINTS);
    }
    if (morphState.playing) {
      uint16_t playLen = morphPlaybackLength();
      if (playLen > 0) {
        return "PLAY " + String(morphState.playIndex + 1) + "/" + String(playLen);
      }
      return "PLAY (no data)";
    }
    if (!morphHasAnyRecording()) {
      return "Tap REC, touch pad to record";
    }
    return "Move square; hold slot clears";
  };

  const String status = statusText();
  if (compactInfo) {
    const int statusH = SCALE_Y(16);
    const int statusY = layout.padY + layout.padH - statusH - SCALE_Y(2);
    tft.fillRect(layout.padX + 1, statusY, layout.padW - 2, statusH, THEME_BG);
    if (morphState.recording) {
      tft.setTextColor(morphState.recordGestureActive ? THEME_ERROR : THEME_WARNING, THEME_BG);
    } else if (morphState.playing) {
      tft.setTextColor(THEME_SUCCESS, THEME_BG);
    } else {
      tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    }
    tft.drawString(status, layout.padX + SCALE_X(4), statusY + SCALE_Y(2), 2);
  } else {
    int statusY = readoutY + (readoutFont == 4 ? SCALE_Y(28) : SCALE_Y(18));
    if (statusY < layout.controlY - SCALE_Y(14)) {
      if (morphState.recording) {
        tft.setTextColor(morphState.recordGestureActive ? THEME_ERROR : THEME_WARNING, THEME_BG);
      } else if (morphState.playing) {
        tft.setTextColor(THEME_SUCCESS, THEME_BG);
      } else {
        tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
      }
      tft.drawString(status, layout.padX, statusY, 2);
    }
  }
  
  // Slot controls - bottom left panel (horizontal row)
  for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
    int x = layout.slotX + slot * (layout.slotW + layout.slotSpacing);
    drawSlot(slot, x, layout.slotY, layout.slotW, layout.slotH);
  }
  
  // PLAY and RECORD buttons - grouped together on right
  int playBtnW = SCALE_X(65);
  int recordBtnW = layout.controlW - playBtnW - SCALE_X(6);
  
  // PLAY button
  drawRoundButton(layout.controlX, layout.controlY, playBtnW, layout.controlH,
                  morphState.playing ? "STOP" : "PLAY",
                  morphState.playing ? THEME_ERROR : THEME_SUCCESS, false, 2);
  
  // RECORD button (visual state indicator)
  uint16_t recordColor = THEME_SECONDARY;
  String recordText = "REC";
  bool recordPressed = false;
  if (morphState.recording) {
    recordPressed = true;
    if (morphState.recordGestureActive) {
      recordColor = THEME_ERROR;
      recordText = "STOP";
    } else {
      recordColor = THEME_WARNING;
      recordText = "ARM";
    }
  }
  drawRoundButton(layout.controlX + playBtnW + SCALE_X(6), layout.controlY, 
                  recordBtnW, layout.controlH,
                  recordText, recordColor, recordPressed, 2);
}

void handleMorphMode() {
  const uint32_t now = millis();
  morphUpdateHeldNoteIdleOff(now);

  if (touch.justReleased) {
    slotHoldIndex = -1;
    slotHoldStartMs = 0;
    slotHoldTriggered = false;
  }

  if (touch.justPressed &&
      isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    morphState.playing = false;
    morphState.recording = false;
    morphState.recordGestureActive = false;
    morphStopHeldNote();
    exitToMenu();
    return;
  }

  const MorphLayout layout = calculateMorphLayout();

  // Playback tick (drives output cursor + MIDI)
  if (morphState.playing) {
    const uint16_t playLen = morphPlaybackLength();
    if (playLen == 0) {
      morphState.playing = false;
      morphState.playIndex = 0;
      morphStopHeldNote();
      requestRedraw();
    } else if (morphState.lastPlayStepMs == 0 ||
               timeReached(now, morphState.lastPlayStepMs + kMorphPlayIntervalMs)) {
      const bool firstTick = (morphState.lastPlayStepMs == 0);
      if (morphState.lastPlayStepMs != 0) {
        morphState.playIndex = (morphState.playIndex + 1) % playLen;
      }
      morphState.lastPlayStepMs = now;

      const uint8_t mixX = clampU8(static_cast<int>(morphState.morphX * 255.0f + 0.5f));
      const uint8_t mixY = clampU8(static_cast<int>(morphState.morphY * 255.0f + 0.5f));
      const MorphPoint target = morphComputeOutputPoint(morphState.playIndex, playLen, mixX, mixY);
      if (firstTick) {
        morphState.outputX = target.x;
        morphState.outputY = target.y;
      } else {
        morphState.outputX = clampU8(((3 * static_cast<int>(morphState.outputX)) + target.x) / 4);
        morphState.outputY = clampU8(((3 * static_cast<int>(morphState.outputY)) + target.y) / 4);
      }
      lastOutputActivityMs = now;

      morphUpdateNoteFromOutput(now);
      requestRedraw();
    }
  }

  // Slot long-press clearing
  if (slotHoldIndex >= 0) {
    const int x = layout.slotX + slotHoldIndex * (layout.slotW + layout.slotSpacing);
    const bool stillInSlot =
        touch.isPressed &&
        touch.x >= x && touch.x < x + layout.slotW &&
        touch.y >= layout.slotY && touch.y < layout.slotY + layout.slotH;

    if (!touch.isPressed || !stillInSlot) {
      slotHoldIndex = -1;
      slotHoldStartMs = 0;
      slotHoldTriggered = false;
    } else if (!slotHoldTriggered &&
               timeReached(now, slotHoldStartMs + kMorphSlotHoldClearMs)) {
      morphClearSlot(static_cast<uint8_t>(slotHoldIndex));
      slotHoldTriggered = true;
      requestRedraw();
    }
  }

  if (touch.justPressed) {
    slotHoldIndex = -1;
    slotHoldStartMs = 0;
    slotHoldTriggered = false;

    // PLAY button
    int playBtnW = SCALE_X(65);
    if (isButtonPressed(layout.controlX, layout.controlY, playBtnW, layout.controlH)) {
      if (morphState.playing) {
        morphState.playing = false;
        morphState.playIndex = 0;
        morphStopHeldNote();
      } else if (morphHasAnyRecording()) {
        morphState.recording = false;
        morphState.recordGestureActive = false;
        morphState.playing = true;
        morphState.playIndex = 0;
        morphState.lastPlayStepMs = 0;
        lastNoteStepMs = 0;
        requestRedraw();
      } else {
        // No gestures recorded yet: keep PLAY as a quick audition.
        playMorphNote();
      }
      return;
    }
    
    // RECORD button
    int recordBtnW = layout.controlW - playBtnW - SCALE_X(6);
    int recordBtnX = layout.controlX + playBtnW + SCALE_X(6);
    if (isButtonPressed(recordBtnX, layout.controlY, recordBtnW, layout.controlH)) {
      if (morphState.recording) {
        morphState.recording = false;
        morphState.recordGestureActive = false;
      } else {
        morphState.playing = false;
        morphState.playIndex = 0;
        morphState.lastPlayStepMs = 0;
        morphStopHeldNote();
        morphState.recording = true;
        morphState.recordGestureActive = false;
        morphState.lastRecordSampleMs = 0;
      }
      requestRedraw();
      return;
    }
    
    // Slot buttons
    for (int slot = 0; slot < MORPH_SLOTS; ++slot) {
      int x = layout.slotX + slot * (layout.slotW + layout.slotSpacing);
      if (isButtonPressed(x, layout.slotY, layout.slotW, layout.slotH)) {
        slotHoldIndex = slot;
        slotHoldStartMs = now;
        slotHoldTriggered = false;

        morphState.activeSlot = slot;
        // Tap a slot to jump the mix handle to that corner (matches GRIDS corner morphing).
        switch (slot) {
          case 0:
            morphState.morphX = 0.0f;
            morphState.morphY = 0.0f;
            break;
          case 1:
            morphState.morphX = 1.0f;
            morphState.morphY = 0.0f;
            break;
          case 2:
            morphState.morphX = 0.0f;
            morphState.morphY = 1.0f;
            break;
          case 3:
            morphState.morphX = 1.0f;
            morphState.morphY = 1.0f;
            break;
        }

        if (!morphState.playing) {
          morphState.outputX = clampU8(static_cast<int>(morphState.morphX * 255.0f + 0.5f));
          morphState.outputY = clampU8(static_cast<int>(morphState.morphY * 255.0f + 0.5f));
        }
        requestRedraw();
        return;
      }
    }
  }

  // Stop recording as soon as the pad gesture ends.
  if (morphState.recording && morphState.recordGestureActive && touch.justReleased) {
    morphState.recording = false;
    morphState.recordGestureActive = false;
    morphFinalizeRecording(morphState.activeSlot);
    requestRedraw();
  }

  // XY Pad interaction
  if (touch.isPressed) {
    const bool inPad = touch.x >= layout.padX && touch.x < layout.padX + layout.padW &&
                       touch.y >= layout.padY && touch.y < layout.padY + layout.padH;
    if (!inPad) {
      return;
    }

    const int denomW = std::max(layout.padW - 1, 1);
    const int denomH = std::max(layout.padH - 1, 1);
    morphState.morphX = (float)(touch.x - layout.padX) / (float)denomW;
    morphState.morphY = (float)(touch.y - layout.padY) / (float)denomH;
    morphState.morphX = std::min(std::max(morphState.morphX, 0.0f), 1.0f);
    morphState.morphY = std::min(std::max(morphState.morphY, 0.0f), 1.0f);

    if (!morphState.playing) {
      morphState.outputX = clampU8(static_cast<int>(morphState.morphX * 255.0f + 0.5f));
      morphState.outputY = clampU8(static_cast<int>(morphState.morphY * 255.0f + 0.5f));
      lastOutputActivityMs = now;
      morphUpdateNoteFromOutput(now);
    }

    if (morphState.recording) {
      if (!morphState.recordGestureActive && touch.justPressed) {
        MorphSlot &slot = morphState.slots[morphState.activeSlot];
        slot.hasData = false;
        slot.length = 0;
        morphState.recordGestureActive = true;
        morphState.lastRecordSampleMs = 0;
      }

      if (morphState.lastRecordSampleMs == 0 ||
          timeReached(now, morphState.lastRecordSampleMs + kMorphRecordIntervalMs)) {
        if (morphState.recordGestureActive) {
          MorphSlot &slot = morphState.slots[morphState.activeSlot];
          if (slot.length < MORPH_MAX_POINTS) {
            slot.points[slot.length++] = MorphPoint{morphState.outputX, morphState.outputY};
            slot.hasData = true;
            morphState.lastRecordSampleMs = now;
          } else {
            morphState.recording = false;
            morphState.recordGestureActive = false;
            morphFinalizeRecording(morphState.activeSlot);
          }
        }
      }
    }

    requestRedraw();
    return;
  }
}
