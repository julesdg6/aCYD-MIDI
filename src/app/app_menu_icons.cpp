#include "app/app_menu_icons.h"

#include <algorithm>
#include <cmath>

#include "common_definitions.h"

namespace {

static void fillTriangleImpl(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
  struct Pt { int x; int y; };
  Pt p[3] = {{x0, y0}, {x1, y1}, {x2, y2}};
  if (p[0].y > p[1].y) std::swap(p[0], p[1]);
  if (p[0].y > p[2].y) std::swap(p[0], p[2]);
  if (p[1].y > p[2].y) std::swap(p[1], p[2]);

  auto interp = [](Pt a, Pt b, int y) -> int {
    if (b.y == a.y) return a.x;
    return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
  };

  for (int y = p[0].y; y <= p[2].y; ++y) {
    int xa, xb;
    if (y <= p[1].y && p[1].y != p[0].y) {
      xa = interp(p[0], p[2], y);
      xb = interp(p[0], p[1], y);
    } else if (p[2].y != p[1].y) {
      xa = interp(p[0], p[2], y);
      xb = interp(p[1], p[2], y);
    } else {
      xa = interp(p[0], p[2], y);
      xb = xa;
    }
    if (xa > xb) std::swap(xa, xb);
    tft.drawFastHLine(xa, y, xb - xa + 1, color);
  }
}

static void drawKeysIcon(int cx, int cy, int size, uint16_t accent) {
  int keyHeight = std::max(24, size / 2);
  int keyWidth = std::max(6, size / 6);
  int startX = cx - (5 * keyWidth) / 2;
  int topY = cy - keyHeight / 2;
  for (int i = 0; i < 5; ++i) {
    int x = startX + i * keyWidth;
    int outerW = keyWidth;
    tft.fillRect(x, topY, outerW, keyHeight, accent);
    tft.drawRect(x, topY, outerW, keyHeight, THEME_BG);
    int innerX = x + keyWidth / 5;
    int innerW = keyWidth - keyWidth / 2;
    int innerH = keyHeight - keyHeight / 5;
    tft.fillRect(innerX, topY + SCALE_Y(2), innerW, innerH, THEME_SURFACE);
  }
}

static void drawSequencerIcon(int cx, int cy, int size, uint16_t accent) {
  int unit = std::max(5, size / 6);
  int gsize = unit * 3 + unit;
  int startX = cx - gsize / 2;
  int startY = cy - gsize / 2;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      int x = startX + col * (unit + SCALE_X(2));
      int y = startY + row * (unit + SCALE_Y(2));
      tft.fillRoundRect(x, y, unit, unit, 2, accent);
    }
  }
  tft.drawRect(startX - SCALE_X(2), startY - SCALE_Y(2), gsize + SCALE_X(4), gsize + SCALE_Y(4), THEME_BG);
}

static void drawCircleIcon(int cx, int cy, int size, uint16_t color) {
  int radius = std::max(8, size / 3);
  tft.drawCircle(cx, cy, radius, color);
  tft.drawCircle(cx, cy, radius / 2, color);
  tft.fillCircle(cx, cy, 2, color);
}

static void drawDropIcon(int cx, int cy, int size, uint16_t accent) {
  int radius = std::max(6, size / 4);
  int circleY = cy - radius / 2;
  tft.fillCircle(cx, circleY, radius, accent);
  fillTriangleImpl(cx - radius, circleY + radius, cx + radius, circleY + radius, cx,
                   circleY + radius + SCALE_Y(4), accent);
}

static void drawRngIcon(int cx, int cy, int size, uint16_t accent) {
  int step = std::max(6, size / 5);
  int x = cx - step * 2;
  int y = cy + step / 2;
  for (int i = 0; i < 4; ++i) {
    int nextX = x + step;
    int nextY = (i % 2 == 0) ? cy - step : cy + step;
    tft.drawLine(x, y, nextX, nextY, accent);
    x = nextX;
    y = nextY;
  }
  tft.drawLine(x, y, x + step, cy - step / 2, accent);
}

static void drawArpIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int width = std::max(22, size - 12);
  int height = std::max(12, size / 3);
  int baseY = cy + height / 3;
  int startX = cx - width / 2;
  int steps = 4;
  tft.drawFastHLine(startX, baseY, width, accent);
  for (int i = 0; i <= steps; ++i) {
    int x = startX + (width * i) / steps;
    int y = baseY - (height * i) / steps;
    tft.drawLine(x, baseY, x, y, fg);
    tft.fillCircle(x, y, SCALE_X(2), fg);
  }
  fillTriangleImpl(startX + width, baseY, startX + width + SCALE_X(4), baseY - SCALE_Y(3),
                   startX + width + SCALE_X(4), baseY + SCALE_Y(3), accent);
}

static void drawGridIcon(int cx, int cy, int size, uint16_t accent) {
  int side = std::max(12, size - 10);
  int start = cx - side / 2;
  tft.drawRect(start, cy - side / 2, side, side, accent);
  tft.drawLine(start + side / 2, cy - side / 2, start + side / 2, cy + side / 2, accent);
  tft.drawLine(start, cy, start + side, cy, accent);
}

static void drawChordIcon(int cx, int cy, int size, uint16_t accent) {
  int height = std::max(14, size - 8);
  int width = std::max(12, size);
  int startX = cx - width / 3;
  for (int i = 0; i < 3; ++i) {
    int x = startX + i * (width / 3);
    tft.drawLine(x, cy - height / 2, x, cy + height / 2, accent);
    tft.fillCircle(x, cy - height / 2 + SCALE_Y(3), SCALE_X(3), accent);
  }
}

static void drawLfoIcon(int cx, int cy, int size, uint16_t accent) {
  int width = std::max(14, size - 10);
  int startX = cx - width / 2;
  int offsetY = size / 4;
  for (int i = 0; i < 5; ++i) {
    int x0 = startX + (width * i) / 4;
    int y0 = cy + ((i % 2 == 0) ? -offsetY : offsetY);
    int x1 = startX + (width * (i + 1)) / 4;
    int y1 = cy + (((i + 1) % 2 == 0) ? -offsetY : offsetY);
    tft.drawLine(x0, y0, x1, y1, accent);
  }
}

static void drawSlinkIcon(int cx, int cy, int size, uint16_t accent) {
  int amplitude = std::max(3, size / 6);
  int width = std::max(20, size);
  int startX = cx - width / 2;
  int prevX = startX;
  int prevY = cy;
  for (int i = 1; i <= 4; ++i) {
    int nextX = startX + (width * i) / 4;
    int nextY = cy + ((i % 2 == 0) ? -amplitude : amplitude);
    tft.drawLine(prevX, prevY, nextX, nextY, accent);
    prevX = nextX;
    prevY = nextY;
  }
}

static void drawTb3poIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int knobRadius = std::max(3, size / 12);
  int knobGap = knobRadius * 4;
  int startX = cx - (knobGap * 3) / 2;
  int startY = cy - size / 4;
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      int x = startX + col * knobGap;
      int y = startY + row * (knobGap / 2);
      tft.fillCircle(x, y, knobRadius, fg);
      tft.drawCircle(x, y, knobRadius, accent);
      tft.drawLine(x - knobRadius, y + knobRadius, x + knobRadius, y + knobRadius, accent);
    }
  }
  int waveY = cy + size / 4;
  int waveStart = cx - size / 2 + SCALE_X(4);
  int waveEnd = cx + size / 2 - SCALE_X(4);
  int segments = 4;
  int segmentWidth = (waveEnd - waveStart) / segments;
  int px = waveStart;
  int py = waveY;
  for (int i = 0; i <= segments; ++i) {
    int nextX = waveStart + i * segmentWidth;
    int nextY = waveY + ((i % 2 == 0) ? -SCALE_Y(4) : SCALE_Y(4));
    tft.drawLine(px, py, nextX, nextY, fg);
    px = nextX;
    py = nextY;
  }
  tft.fillCircle(waveStart, waveY, SCALE_X(2), fg);
  tft.fillCircle(waveEnd, py, SCALE_X(2), accent);
}

static void drawGridsIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int blocks = 3;
  int blockSize = std::max(6, (size - (blocks - 1) * 2) / blocks);
  int totalW = blocks * blockSize + (blocks - 1) * SCALE_X(2);
  int startX = cx - totalW / 2;
  int startY = cy - totalW / 2;
  for (int row = 0; row < blocks; ++row) {
    for (int col = 0; col < blocks; ++col) {
      int x = startX + col * (blockSize + SCALE_X(2));
      int y = startY + row * (blockSize + SCALE_Y(2));
      tft.fillRect(x, y, blockSize, blockSize, ((row + col) % 2) ? accent : fg);
      tft.drawRect(x, y, blockSize, blockSize, THEME_BG);
    }
  }
}

static void drawRagaIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int stringHeight = size / 2;
  int startX = cx - size / 3;
  int spacing = size / 3;
  for (int i = 0; i < 3; ++i) {
    int x = startX + i * spacing;
    tft.drawLine(x, cy - stringHeight / 2, x, cy + stringHeight / 2, fg);
    tft.fillCircle(x, cy - stringHeight / 2, SCALE_X(2), accent);
  }
  int arcRadius = stringHeight / 3;
  tft.drawCircle(cx - spacing / 2, cy + stringHeight / 2 - arcRadius, arcRadius, fg);
  tft.drawCircle(cx + spacing / 2, cy + stringHeight / 2 - arcRadius / 2, arcRadius / 2, accent);
  tft.fillCircle(cx, cy - stringHeight / 2 - SCALE_Y(3), SCALE_X(3), fg);
  int waveLength = spacing / 2;
  tft.drawLine(cx - waveLength, cy + stringHeight / 2, cx + waveLength, cy + stringHeight / 2, accent);
  tft.drawLine(cx - waveLength, cy + stringHeight / 2 + SCALE_Y(2), cx + waveLength, cy + stringHeight / 2 + SCALE_Y(2), fg);
}

static void drawEuclidIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int radius = std::max(10, size / 3);
  int inner = radius - SCALE_X(4);
  tft.drawCircle(cx, cy, radius, accent);
  tft.drawCircle(cx, cy, inner, fg);
  const float twoPi = 6.2831853f;
  const float startAngle = -1.5707963f;
  const int steps = 8;
  for (int i = 0; i < steps; ++i) {
    float angle = startAngle + (twoPi * i) / steps;
    int markerX = cx + static_cast<int>(std::cos(angle) * radius);
    int markerY = cy + static_cast<int>(std::sin(angle) * radius);
    tft.fillCircle(markerX, markerY, 2, (i % 2 == 0) ? accent : fg);
    tft.drawLine(cx, cy, markerX, markerY, fg);
    float midAngle = angle + (twoPi / steps) / 2.0f;
    int arcX = cx + static_cast<int>(std::cos(midAngle) * inner);
    int arcY = cy + static_cast<int>(std::sin(midAngle) * inner);
    tft.drawLine(markerX, markerY, arcX, arcY, accent);
  }
  tft.fillCircle(cx, cy, SCALE_X(3), accent);
}

static void drawMorphIcon(int cx, int cy, int size, uint16_t accent, uint16_t fg) {
  int width = std::max(16, size - 8);
  int height = std::max(8, size / 4);
  int left = cx - width / 2;
  int top = cy - height / 2;
  tft.fillRoundRect(left, top, width, height, 6, fg);
  int inset = SCALE_X(5);
  tft.fillRoundRect(left + inset, top + inset / 2, width - 2 * inset, height - inset / 2, 4, accent);
  int waveAmp = size / 6;
  int steps = 3;
  for (int i = 0; i <= steps; ++i) {
    int x0 = left + (width * i) / steps;
    int y0 = cy + ((i % 2 == 0) ? -waveAmp / 2 : waveAmp / 2);
    int x1 = left + (width * (i + 1)) / steps;
    int y1 = cy + (((i + 1) % 2 == 0) ? -waveAmp / 2 : waveAmp / 2);
    if (i < steps) {
      tft.drawLine(x0, y0, x1, y1, fg);
    }
    tft.fillCircle(x0, y0, SCALE_X(2), accent);
  }
}

static void drawWaaaveIcon(int cx, int cy, int size, uint16_t accent) {
  // Draw three horizontal wave lines (representing video controller faders/waves)
  int waveHeight = std::max(3, size / 8);
  int waveWidth = std::max(12, size / 2);
  int spacing = std::max(6, size / 4);
  
  int startX = cx - waveWidth / 2;
  int startY = cy - spacing;
  
  // Draw 3 wavy lines
  for (int wave = 0; wave < 3; ++wave) {
    int y = startY + wave * spacing;
    int numSegments = 4;
    int segWidth = waveWidth / numSegments;
    
    for (int seg = 0; seg < numSegments; ++seg) {
      int x1 = startX + seg * segWidth;
      int x2 = x1 + segWidth;
      int y1 = y + ((seg % 2 == 0) ? -waveHeight : waveHeight);
      int y2 = y + ((seg % 2 == 0) ? waveHeight : -waveHeight);
      tft.drawLine(x1, y1, x2, y2, accent);
    }
  }
}

static void drawEncoder8Icon(int cx, int cy, int size, uint16_t accent) {
  // Draw 8 small circles representing encoders in 2 rows of 4
  int circleRadius = std::max(2, size / 12);
  int spacing = std::max(6, size / 4);
  
  int startX = cx - (spacing * 3) / 2;
  int startY = cy - spacing / 2;
  
  // Draw 2 rows of 4 encoders
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      int x = startX + col * spacing;
      int y = startY + row * spacing;
      tft.drawCircle(x, y, circleRadius + 1, accent);
      tft.fillCircle(x, y, circleRadius - 1, THEME_SURFACE);
    }
  }
}

}  // namespace

void drawMenuIcon(int cx, int cy, int size, MenuIcon icon, uint16_t accent) {
  const uint16_t fg = THEME_SURFACE;
  switch (icon) {
    case MenuIcon::Keys:
      drawKeysIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Sequencer:
      drawSequencerIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Zen:
      drawCircleIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Drop:
      drawDropIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Rng:
      drawRngIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Xy:
      tft.drawLine(cx - size / 2, cy, cx + size / 2, cy, fg);
      tft.drawLine(cx, cy - size / 2, cx, cy + size / 2, fg);
      tft.fillCircle(cx, cy, SCALE_X(3), fg);
      break;
    case MenuIcon::Arp:
      drawArpIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Grid:
      drawGridIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Chord:
      drawChordIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Lfo:
      drawLfoIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Slink:
      drawSlinkIcon(cx, cy, size, fg);
      break;
    case MenuIcon::Tb3po:
      drawTb3poIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Grids:
      drawGridsIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Raga:
      drawRagaIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Euclid:
      drawEuclidIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Morph:
      drawMorphIcon(cx, cy, size, accent, fg);
      break;
    case MenuIcon::Waaave:
      drawWaaaveIcon(cx, cy, size, accent);
      break;
#ifdef ENABLE_M5_8ENCODER
    case MenuIcon::Encoder8:
      drawEncoder8Icon(cx, cy, size, fg);
      break;
#endif
    default:
      tft.fillCircle(cx, cy, std::max(3, size / 4), accent);
      break;
  }
}

