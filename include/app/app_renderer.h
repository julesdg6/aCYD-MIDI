#pragma once

#include <stdint.h>

void appRendererInit();
void appRendererLoopTick(uint32_t now);
void appRendererProcessRedraw();

