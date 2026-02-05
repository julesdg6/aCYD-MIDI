#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

  #if __has_include("../config/wifi_config.h")
  #include "../config/wifi_config.h"
  #elif __has_include("config/wifi_config.h")
  #include "config/wifi_config.h"
  #elif __has_include("config/wifi_config.local.h")
  #include "config/wifi_config.local.h"
  #else
  // WiFi configuration not found - fall back to safe defaults
  #define WIFI_SSID "INVALID_SSID_CONFIGURE_ME"
  #define WIFI_PASSWORD "INVALID_PASSWORD_CONFIGURE_ME"
  #endif

#ifndef WIFI_SSID
#warning "WIFI_SSID not defined - using obviously invalid WiFi credentials"
#define WIFI_SSID "INVALID_SSID_CONFIGURE_ME"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "INVALID_PASSWORD_CONFIGURE_ME"
#endif

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

#if WIFI_ENABLED
#include <WiFi.h>
#endif

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;

void initWiFi();
void handleWiFi();
bool isWiFiConnected();
String getWiFiIPAddress();

#endif // WIFI_MANAGER_H
