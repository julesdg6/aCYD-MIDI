#ifndef MODULE_SETTINGS_MODE_H
#define MODULE_SETTINGS_MODE_H

#include "common_definitions.h"

void initializeSettingsMode();
void drawSettingsMode();
void handleSettingsMode();

// Screenshot support - get/set scroll position
int getSettingsScrollOffset();
void setSettingsScrollOffset(int offset);
int getSettingsMaxScroll();

#endif // MODULE_SETTINGS_MODE_H
