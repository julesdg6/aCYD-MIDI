#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Try to include wifi_config.h, generate error if not found and defaults would be used
#if __has_include("wifi_config.h")
#include "wifi_config.h"
#else
// WiFi configuration not found - you must create wifi_config.h
// Copy include/wifi_config.h.example to include/wifi_config.h and update with your credentials
#ifndef WIFI_SSID
#warning "wifi_config.h not found - using obviously invalid WiFi credentials"
#warning "Copy include/wifi_config.h.example to include/wifi_config.h and update with your credentials"
#define WIFI_SSID "INVALID_SSID_CONFIGURE_ME"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "INVALID_PASSWORD_CONFIGURE_ME"
#endif
#endif

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

#ifndef REMOTE_DISPLAY_ENABLED
#define REMOTE_DISPLAY_ENABLED 1
#endif

#if WIFI_ENABLED
#include <WiFi.h>
#endif

// Remote Display Settings
#define REMOTE_DISPLAY_PORT 80
#define WEBSOCKET_PATH "/ws"
#define FRAME_UPDATE_INTERVAL 50  // Update every 50ms (20 FPS)
#define WIFI_CONNECT_TIMEOUT_MS 10000  // 10 second WiFi connection timeout

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
