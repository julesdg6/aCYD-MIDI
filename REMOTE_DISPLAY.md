# Remote Display Feature

This document describes the remote display capability added to the aCYD MIDI Controller.

## Overview

The remote display feature allows you to view the CYD display on any device with a web browser over WiFi. This is useful for:
- Monitoring the display remotely
- Recording or documenting the UI
- Debugging and development
- Demonstrations

## How It Works

The system uses:
- **WiFi**: ESP32 connects to your local WiFi network
- **WebSocket**: Real-time streaming of display frames
- **Web Interface**: HTML5 canvas-based viewer accessible from any browser

## Setup

### 1. Configure WiFi Credentials

Copy the example configuration file and update with your credentials:

```bash
cp include/wifi_config.h.example include/wifi_config.h
```

Then edit `include/wifi_config.h` and update:

```cpp
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
```

Alternatively, you can set these in `platformio.ini` build flags:

```ini
build_flags = 
    -D WIFI_SSID=\"YourWiFiSSID\"
    -D WIFI_PASSWORD=\"YourWiFiPassword\"
```

**Note:** The `wifi_config.h` file is ignored by git to prevent accidentally committing credentials.

### 2. Build and Upload

Build and upload the firmware to your CYD:

```bash
pio run --target upload
```

### 3. Find the IP Address

After uploading, open the Serial Monitor (115200 baud). You'll see:

```
WiFi connected!
IP address: 192.168.1.XXX
Remote Display URL: http://192.168.1.XXX
```

### 4. Access the Remote Display

Open a web browser and navigate to the IP address shown. You'll see the remote display viewer showing the CYD screen in real-time.

## Features

- **Real-time Updates**: Display updates at 20 FPS (configurable)
- **Auto-reconnect**: Browser automatically reconnects if connection is lost
- **Status Indicator**: Shows connection status
- **Responsive Design**: Works on desktop and mobile browsers

## Configuration

You can adjust these settings in `include/remote_display.h`:

```cpp
#define REMOTE_DISPLAY_PORT 80          // Web server port
#define FRAME_UPDATE_INTERVAL 50        // Update interval in ms (50ms = 20 FPS)
```

## Disabling Remote Display

To disable the remote display feature:

1. Edit `include/wifi_config.h`:
   ```cpp
   #define REMOTE_DISPLAY_ENABLED 0
   ```

2. Or comment out the remote display calls in `main.cpp`

## Troubleshooting

### WiFi Won't Connect
- Verify SSID and password are correct
- Ensure your WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check that your network allows new device connections

### Can't Access Web Interface
- Verify the IP address in Serial Monitor
- Ensure your computer/phone is on the same WiFi network
- Try accessing from different browsers
- Check firewall settings

### Display Shows Black Screen
- The current implementation sends placeholder black frames
- Full framebuffer capture requires deeper LVGL integration
- This is a known limitation in the current version
- Future updates will implement proper screen capture
- Check Serial Monitor for error messages
- Ensure sufficient memory is available

## Implementation Notes

### Current Limitations

The current implementation includes a basic WebSocket-based remote display framework but has the following limitations:

1. **Framebuffer Capture**: The screen capture is currently a placeholder. Full implementation requires:
   - Hooking into the LVGL display driver's flush callback
   - Using LVGL's snapshot API (if available in your version)
   - Direct access to the display driver buffer

2. **Performance**: With full framebuffer capture enabled:
   - Each frame is ~150KB (320x240 RGB565)
   - At 20 FPS, this requires ~3MB/s bandwidth
   - May impact main application performance

### Future Enhancements

To complete the implementation, developers can:

1. **Add Framebuffer Capture Hook**:
   ```cpp
   // In display driver flush callback
   void my_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *color_p) {
       // Copy buffer data for remote display
       // Then call normal flush
   }
   ```

2. **Use LVGL Snapshot API** (LVGL 9.x):
   ```cpp
   lv_draw_buf_t *buf = lv_snapshot_take_to_buf(scr, LV_COLOR_FORMAT_RGB565);
   // Send buf->data via WebSocket
   ```

3. **Optimize Transmission**:
   - Add frame compression (JPEG, PNG)
   - Implement delta encoding
   - Reduce frame rate dynamically based on changes

## Technical Details

### Frame Format
- Frames are sent as binary data over WebSocket
- Format: RGB565 (2 bytes per pixel)
- Resolution: 320x240 pixels
- Size: ~150KB per frame

### Performance
- Frame rate: ~20 FPS (configurable)
- Latency: <100ms on local network
- Memory usage: ~150KB static buffer for frame capture (allocated at compile time)

**Note on Memory**: The remote display uses a 150KB static buffer to avoid memory fragmentation from repeated allocations. If you're experiencing memory constraints, you can:
- Reduce the update rate by increasing `FRAME_UPDATE_INTERVAL`
- Disable remote display when not needed by setting `REMOTE_DISPLAY_ENABLED 0`
- Modify the code to use dynamic allocation if your use case benefits from it

## Future Enhancements

Possible improvements:
- Frame compression to reduce bandwidth
- Touch input forwarding from browser to device
- Multiple client support
- Recording/screenshot capability
- Adjustable quality/FPS settings via web UI
