#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Try to include wifi_config.h, use defaults if not found
#if __has_include("wifi_config.h")
#include "wifi_config.h"
#else
// Default WiFi configuration - update these or create wifi_config.h
#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif
#ifndef REMOTE_DISPLAY_ENABLED
#define REMOTE_DISPLAY_ENABLED 1
#endif
#endif

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
