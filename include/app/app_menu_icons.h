#pragma once

#include <stdint.h>

enum class MenuIcon : uint8_t {
  Keys,
  Sequencer,
  Zen,
  Drop,
  Rng,
  Xy,
  Arp,
  Grid,
  Chord,
  Lfo,
  Slink,
  Tb3po,
  Grids,
  Raga,
  Euclid,
  Morph,
};

void drawMenuIcon(int cx, int cy, int size, MenuIcon icon, uint16_t accent);

