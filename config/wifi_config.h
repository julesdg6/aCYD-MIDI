#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// Try to include a local credentials file if it exists (copy config/wifi_config.local.h.template → config/wifi_config.local.h)
#if defined(__has_include)
#  if __has_include("wifi_config.local.h")
#    include "wifi_config.local.h"
#  endif
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "dinosaur"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "global6.net"
#endif

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1  // Set to 0 to disable WiFi features
#endif

#ifndef REMOTE_DISPLAY_ENABLED
#define REMOTE_DISPLAY_ENABLED 1  // Set to 0 to disable remote display
#endif

#endif  // WIFI_CONFIG_H
