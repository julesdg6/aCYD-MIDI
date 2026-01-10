#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// WiFi Configuration
// Update these values with your WiFi network credentials
// You can also override these by defining them in platformio.ini build_flags

#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif

// Remote Display Configuration
#ifndef REMOTE_DISPLAY_ENABLED
#define REMOTE_DISPLAY_ENABLED 1  // Set to 0 to disable remote display
#endif

#endif // WIFI_CONFIG_H
