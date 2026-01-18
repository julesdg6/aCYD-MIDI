#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#include <Arduino.h>
#include "wifi_manager.h"

#ifndef REMOTE_DISPLAY_ENABLED
#define REMOTE_DISPLAY_ENABLED 1
#endif

// Only include AsyncTCP and WebServer headers when remote display is enabled
#if REMOTE_DISPLAY_ENABLED && WIFI_ENABLED
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif

// Remote Display Settings
#define REMOTE_DISPLAY_PORT 80
#define WEBSOCKET_PATH "/ws"
#define FRAME_UPDATE_INTERVAL 50  // Update every 50ms (20 FPS)

// Display dimensions for framebuffer
#define REMOTE_DISPLAY_WIDTH 320
#define REMOTE_DISPLAY_HEIGHT 240
#define REMOTE_DISPLAY_BYTES_PER_PIXEL 2  // RGB565 format

// Function declarations
void initRemoteDisplay();
void handleRemoteDisplay();
void sendFrameUpdate();
bool isRemoteDisplayConnected();
String getRemoteDisplayIP();

#endif // REMOTE_DISPLAY_H
