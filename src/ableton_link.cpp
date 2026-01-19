#include "ableton_link.h"

#if WIFI_ENABLED

// Note: This is a simplified Link implementation
// A full implementation would require the official Link library
// which is complex to integrate on ESP32. This provides basic
// tempo synchronization over UDP multicast.

static WiFiUDP linkUdp;
static LinkState linkState = LINK_DISCONNECTED;
static bool linkEnabled = false;
static bool linkInitialized = false;
static uint32_t lastLinkDiscovery = 0;
static uint32_t lastLinkUpdate = 0;
static LinkSessionInfo sessionInfo;

// Link discovery multicast group
static IPAddress linkMulticastIP(224, 76, 78, 75); // Link multicast address

void initAbletonLink() {
  if (linkInitialized) {
    return;
  }
  
  Serial.println("Initializing Ableton Link...");
  
  // Initialize session info
  sessionInfo.tempo = 120.0f;
  sessionInfo.peerCount = 0;
  sessionInfo.beatPhase = 0.0;
  sessionInfo.beatTime = 0;
  sessionInfo.isValid = false;
  
  linkInitialized = true;
  linkState = LINK_DISCONNECTED;
  
  Serial.println("Ableton Link initialized (disabled by default)");
  Serial.println("Note: This is a simplified Link implementation for basic tempo sync");
}

void handleAbletonLink() {
  if (!linkInitialized) {
    initAbletonLink();
    return;
  }
  
  if (!linkEnabled) {
    if (linkState != LINK_DISCONNECTED) {
      linkUdp.stop();
      linkState = LINK_DISCONNECTED;
      sessionInfo.isValid = false;
    }
    return;
  }
  
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    if (linkState != LINK_DISCONNECTED) {
      linkUdp.stop();
      linkState = LINK_DISCONNECTED;
      sessionInfo.isValid = false;
    }
    return;
  }
  
  // Start Link UDP if not started
  if (linkState == LINK_DISCONNECTED) {
    if (linkUdp.beginMulticast(linkMulticastIP, LINK_PORT)) {
      linkState = LINK_DISCOVERING;
      lastLinkDiscovery = millis();
      Serial.print("Ableton Link multicast started on port ");
      Serial.println(LINK_PORT);
    } else {
      Serial.println("Failed to start Ableton Link multicast");
      return;
    }
  }
  
  uint32_t now = millis();
  
  // Send discovery packets periodically
  if (linkState == LINK_DISCOVERING || linkState == LINK_CONNECTED) {
    if (now - lastLinkDiscovery >= LINK_DISCOVERY_INTERVAL_MS) {
      // Send simplified discovery packet
      // Format: "LINK" + tempo (4 bytes float)
      uint8_t packet[8] = {'L', 'I', 'N', 'K'};
      float tempo = sessionInfo.tempo;
      memcpy(&packet[4], &tempo, sizeof(float));
      
      linkUdp.beginPacket(linkMulticastIP, LINK_PORT);
      linkUdp.write(packet, 8);
      linkUdp.endPacket();
      
      lastLinkDiscovery = now;
    }
  }
  
  // Check for incoming Link messages
  int packetSize = linkUdp.parsePacket();
  if (packetSize >= 8) {
    uint8_t buffer[8];
    linkUdp.read(buffer, 8);
    
    // Check for Link packet signature
    if (buffer[0] == 'L' && buffer[1] == 'I' && buffer[2] == 'N' && buffer[3] == 'K') {
      // Extract tempo
      float receivedTempo;
      memcpy(&receivedTempo, &buffer[4], sizeof(float));
      
      // Validate tempo range
      if (receivedTempo >= 20.0f && receivedTempo <= 999.0f) {
        sessionInfo.tempo = receivedTempo;
        sessionInfo.peerCount = 1; // At least one peer found
        sessionInfo.isValid = true;
        lastLinkUpdate = now;
        
        if (linkState != LINK_CONNECTED) {
          linkState = LINK_CONNECTED;
          Serial.print("Ableton Link session joined. Tempo: ");
          Serial.println(receivedTempo);
        }
      }
    }
  }
  
  // Check for timeout
  if (linkState == LINK_CONNECTED) {
    if (now - lastLinkUpdate >= LINK_TIMEOUT_MS) {
      Serial.println("Link session timeout");
      linkState = LINK_DISCOVERING;
      sessionInfo.peerCount = 0;
      sessionInfo.isValid = false;
    }
  }
  
  // Update beat phase (simple calculation based on tempo)
  if (sessionInfo.isValid) {
    float beatsPerSecond = sessionInfo.tempo / 60.0f;
    float secondsSinceUpdate = (now - lastLinkUpdate) / 1000.0f;
    sessionInfo.beatPhase = fmodf(sessionInfo.beatPhase + (beatsPerSecond * secondsSinceUpdate), 1.0);
    sessionInfo.beatTime = (uint64_t)(now * 1000); // Convert to microseconds
  }
}

void setLinkEnabled(bool enabled) {
  linkEnabled = enabled;
  if (!enabled && linkState != LINK_DISCONNECTED) {
    linkUdp.stop();
    linkState = LINK_DISCONNECTED;
    sessionInfo.isValid = false;
    Serial.println("Ableton Link disabled");
  } else if (enabled) {
    Serial.println("Ableton Link enabled");
  }
}

bool getLinkEnabled() {
  return linkEnabled;
}

LinkState getLinkState() {
  return linkState;
}

LinkSessionInfo getLinkSessionInfo() {
  return sessionInfo;
}

void setLinkTempo(float bpm) {
  if (bpm >= 20.0f && bpm <= 999.0f) {
    sessionInfo.tempo = bpm;
    Serial.print("Link tempo set to: ");
    Serial.println(bpm);
  }
}

float getLinkTempo() {
  return sessionInfo.tempo;
}

#else

// Stub implementations when WIFI_ENABLED is false
void initAbletonLink() {}
void handleAbletonLink() {}
void setLinkEnabled(bool enabled) { (void)enabled; }
bool getLinkEnabled() { return false; }
LinkState getLinkState() { return LINK_DISCONNECTED; }
LinkSessionInfo getLinkSessionInfo() {
  LinkSessionInfo info;
  info.tempo = 120.0f;
  info.peerCount = 0;
  info.beatPhase = 0.0;
  info.beatTime = 0;
  info.isValid = false;
  return info;
}
void setLinkTempo(float bpm) { (void)bpm; }
float getLinkTempo() { return 120.0f; }

#endif // WIFI_ENABLED
