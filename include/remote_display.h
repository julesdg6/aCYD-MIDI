#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifi_config.h"

// Remote Display Settings
#define REMOTE_DISPLAY_PORT 80
#define WEBSOCKET_PATH "/ws"
#define FRAME_UPDATE_INTERVAL 50  // Update every 50ms (20 FPS)

// Function declarations
void initRemoteDisplay();
void handleRemoteDisplay();
void sendFrameUpdate();
bool isRemoteDisplayConnected();
String getRemoteDisplayIP();

#endif // REMOTE_DISPLAY_H
