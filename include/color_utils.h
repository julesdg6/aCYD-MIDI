#pragma once

#include <stdint.h>

inline uint16_t blendColor(uint16_t from, uint16_t to, uint8_t ratio) {
  int rf = (from >> 11) & 0x1F;
  int gf = (from >> 5) & 0x3F;
  int bf = from & 0x1F;
  int rt = (to >> 11) & 0x1F;
  int gt = (to >> 5) & 0x3F;
  int bt = to & 0x1F;
  int r = ((rf * (255 - ratio)) + rt * ratio) / 255;
  int g = ((gf * (255 - ratio)) + gt * ratio) / 255;
  int b = ((bf * (255 - ratio)) + bt * ratio) / 255;
  return (r << 11) | (g << 5) | b;
}

