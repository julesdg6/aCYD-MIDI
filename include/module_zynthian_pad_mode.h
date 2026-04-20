#ifndef MODULE_ZYNTHIAN_PAD_MODE_H
#define MODULE_ZYNTHIAN_PAD_MODE_H

#include "common_definitions.h"

enum class ZynthianPadAction : uint8_t {
  PRESET = 0,
  SCREEN_AUDIO_MIXER,
  SCREEN_ADMIN,
  SCREEN_ZYNPAD,
  SCREEN_SNAPSHOT,
  SCREEN_MIDI_RECORDER,
  TOGGLE_MIDI_PLAY,
  TOGGLE_MIDI_RECORD,
  MENU,
  ENTER_MIDI_LEARN,
  ARROW_UP,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
  ZYNSWITCH_1,
  ZYNSWITCH_2,
  ZYNSWITCH_3,
  ALT_LAYER,
  BACK_NO,
  SCREEN_ALSA_MIXER,
  NONE
};

void initializeZynthianPadMode();
void drawZynthianPadMode();
void handleZynthianPadMode();

bool triggerZynthianPadActionFromMidi(uint8_t status, uint8_t data1, uint8_t data2);

#endif // MODULE_ZYNTHIAN_PAD_MODE_H
