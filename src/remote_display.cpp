#include "remote_display.h"
#include <lvgl.h>
#include <esp32_smartdisplay.h>

// Web server and WebSocket
AsyncWebServer server(REMOTE_DISPLAY_PORT);
AsyncWebSocket ws(WEBSOCKET_PATH);

// State tracking
static bool wifiConnected = false;
static bool clientConnected = false;
static uint32_t lastFrameUpdate = 0;

#if REMOTE_DISPLAY_ENABLED
// Frame buffer - static allocation to avoid fragmentation
// For 320x240 RGB565 display = 153,600 bytes
// Only allocated when REMOTE_DISPLAY_ENABLED is set to 1
static uint8_t frameBuffer[320 * 240 * 2];
#endif

// HTML page for remote display viewer
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>aCYD MIDI Remote Display</title>
    <style>
        body {
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            color: #fff;
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
        }
        h1 {
            margin-bottom: 10px;
        }
        #status {
            margin-bottom: 20px;
            padding: 10px 20px;
            border-radius: 5px;
            background: #333;
        }
        #canvas {
            border: 2px solid #444;
            border-radius: 5px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
            background: #000;
        }
        .connected { background: #2d5016 !important; }
        .disconnected { background: #501616 !important; }
    </style>
</head>
<body>
    <h1>aCYD MIDI Remote Display</h1>
    <div id="status" class="disconnected">Disconnected</div>
    <canvas id="canvas" width="320" height="240"></canvas>
    <script>
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        const status = document.getElementById('status');
        
        let ws = null;
        let reconnectTimer = null;
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.host + '/ws');
            
            ws.onopen = () => {
                console.log('Connected to aCYD MIDI');
                status.textContent = 'Connected';
                status.className = 'connected';
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
            };
            
            ws.onclose = () => {
                console.log('Disconnected from aCYD MIDI');
                status.textContent = 'Disconnected';
                status.className = 'disconnected';
                // Try to reconnect after 2 seconds
                reconnectTimer = setTimeout(connect, 2000);
            };
            
            ws.onerror = (error) => {
                console.error('WebSocket error:', error);
            };
            
            ws.onmessage = (event) => {
                if (event.data instanceof Blob) {
                    event.data.arrayBuffer().then(buffer => {
                        const imageData = ctx.createImageData(320, 240);
                        const data = new Uint8Array(buffer);
                        
                        // Convert RGB565 to RGBA
                        for (let i = 0, j = 0; i < data.length; i += 2, j += 4) {
                            const rgb565 = (data[i + 1] << 8) | data[i];
                            const r = ((rgb565 >> 11) & 0x1F) << 3;
                            const g = ((rgb565 >> 5) & 0x3F) << 2;
                            const b = (rgb565 & 0x1F) << 3;
                            
                            imageData.data[j] = r;
                            imageData.data[j + 1] = g;
                            imageData.data[j + 2] = b;
                            imageData.data[j + 3] = 255;
                        }
                        
                        ctx.putImageData(imageData, 0, 0);
                    });
                }
            };
        }
        
        connect();
    </script>
</body>
</html>
)rawliteral";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            clientConnected = true;
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            clientConnected = false;
            break;
        case WS_EVT_DATA:
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initRemoteDisplay() {
    Serial.println("Initializing Remote Display...");
    
    // Connect to WiFi (blocking approach during initialization)
    // Note: This uses a simple blocking approach during initialization.
    // yield() is called in the loop to prevent watchdog timer issues.
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    const int maxAttempts = 20;  // 10 second timeout
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        yield();  // Prevent watchdog timer reset
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Remote Display URL: http://");
        Serial.println(WiFi.localIP());
        
        // Setup WebSocket
        ws.onEvent(onWsEvent);
        server.addHandler(&ws);
        
        // Setup HTTP server
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html);
        });
        
        // Start server
        server.begin();
        Serial.println("Remote Display server started!");
    } else {
        wifiConnected = false;
        Serial.println("\nWiFi connection failed!");
        Serial.println("Remote Display disabled.");
    }
}

void sendFrameUpdate() {
#if !REMOTE_DISPLAY_ENABLED
    return;  // Do nothing if remote display is disabled
#else
    if (!wifiConnected || !clientConnected || ws.count() == 0) {
        return;
    }
    
    // Get LVGL display
    lv_display_t *display = lv_display_get_default();
    if (!display) {
        return;
    }
    
    // Get display dimensions
    uint32_t width = lv_display_get_horizontal_resolution(display);
    uint32_t height = lv_display_get_vertical_resolution(display);
    
    // Calculate buffer size for RGB565 (2 bytes per pixel)
    size_t bufferSize = width * height * 2;
    
    // Verify buffer size matches our static allocation
    if (bufferSize > sizeof(frameBuffer)) {
        Serial.printf("Error: Display size %ux%u exceeds frame buffer capacity\n", width, height);
        return;
    }
    
    // IMPLEMENTATION NOTE: Framebuffer Capture
    // The current implementation sends a black placeholder frame.
    // To capture the actual display content, implement one of these approaches:
    //
    // Option 1: Use LVGL snapshot API (LVGL 9.x):
    //   lv_draw_buf_t *snapshot = lv_snapshot_take_to_buf(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
    //   if (snapshot) {
    //       memcpy(frameBuffer, snapshot->data, bufferSize);
    //       lv_draw_buf_destroy(snapshot);
    //   }
    //
    // Option 2: Hook display flush callback to copy buffer:
    //   Store a pointer to the last flushed buffer and copy from it
    //
    // Option 3: Read from display driver internal buffer:
    //   Access esp32_smartdisplay internals to get current framebuffer
    
    // Placeholder: Send black frame (intentional for development/testing)
    memset(frameBuffer, 0, bufferSize);
    
    // Send frame data to all connected clients via WebSocket
    // Using static buffer avoids malloc/free on every frame (20 FPS)
    ws.binaryAll(frameBuffer, bufferSize);
#endif
}

void handleRemoteDisplay() {
    if (!wifiConnected) {
        return;
    }
    
    // Cleanup WebSocket clients
    ws.cleanupClients();
    
    // Send frame updates at specified interval
    uint32_t now = millis();
    if (clientConnected && (now - lastFrameUpdate) >= FRAME_UPDATE_INTERVAL) {
        sendFrameUpdate();
        lastFrameUpdate = now;
    }
}

bool isRemoteDisplayConnected() {
    return wifiConnected && clientConnected;
}

String getRemoteDisplayIP() {
    if (wifiConnected) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}
