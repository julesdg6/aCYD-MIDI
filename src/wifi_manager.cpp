#include "wifi_manager.h"

#if WIFI_ENABLED
static void updateWifiStatus() {
  wifiConnected = (WiFi.status() == WL_CONNECTED);
}
#endif
static bool wifiInitialized = false;
static bool wifiConnected = false;
static uint32_t lastWiFiAttempt = 0;

void initWiFi() {
#if WIFI_ENABLED
  if (wifiInitialized) {
    return;
  }
  wifiInitialized = true;
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startMs) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  updateWifiStatus();
  if (wifiConnected) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(getWiFiIPAddress());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
  lastWiFiAttempt = millis();
#else
  wifiInitialized = true;
  wifiConnected = false;
#endif
}

void handleWiFi() {
#if WIFI_ENABLED
  if (!wifiInitialized) {
    initWiFi();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    uint32_t now = millis();
    if (now - lastWiFiAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
      Serial.println("WiFi disconnected, attempting reconnect...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      lastWiFiAttempt = now;
    }
    wifiConnected = false;
  } else {
    updateWifiStatus();
  }
#else
  (void)lastWiFiAttempt;
#endif
}

bool isWiFiConnected() {
#if WIFI_ENABLED
  return wifiConnected;
#else
  return false;
#endif
}

String getWiFiIPAddress() {
#if WIFI_ENABLED
  if (!wifiConnected) {
    return "Not connected";
  }
  return WiFi.localIP().toString();
#else
  return "WiFi disabled";
#endif
}
