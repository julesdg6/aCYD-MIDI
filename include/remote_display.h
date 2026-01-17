#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#include <Arduino.h>

// Try to include config/wifi_config.h, generate error if not found and defaults would be used
#if __has_include("../config/wifi_config.h")
#include "../config/wifi_config.h"
#else
// WiFi configuration not found - you must create config/wifi_config.local.h
// Copy config/wifi_config.local.h.template to config/wifi_config.local.h and update the credentials
#ifndef WIFI_SSID
#warning "config/wifi_config.h not found - using obviously invalid WiFi credentials"
#warning "Copy config/wifi_config.local.h.template to config/wifi_config.local.h and update with your credentials"
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

// Only include AsyncTCP and WebServer headers when remote display is enabled
#if REMOTE_DISPLAY_ENABLED && WIFI_ENABLED
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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
