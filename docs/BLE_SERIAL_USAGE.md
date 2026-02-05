# BLE Serial Service Usage Guide

## Overview

The BLE Serial service provides a UART-like serial interface over BLE for control, debug, and configuration purposes. It runs alongside the BLE MIDI service over a single BLE connection.

## Enabling BLE Serial

### Compile-Time Configuration

BLE Serial is disabled by default to save memory. To enable it:

1. **Using a pre-configured environment:**
   ```bash
   pio run -e esp32-2432S028Rv2-ble-serial
   ```

2. **Custom configuration in platformio.ini:**
   ```ini
   build_flags = 
       ${common.build_flags}
       -D ENABLE_BLE_SERIAL=1
   ```

## Service UUIDs

- **Service UUID:** `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic:** `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (Write)
- **TX Characteristic:** `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (Notify)

These are standard Nordic UART Service (NUS) UUIDs for maximum compatibility with existing apps and tools.

## Using BLE Serial in Your Code

### Basic API

```cpp
#include "ble_serial.h"

// Check if BLE Serial is connected
if (bleSerial.isConnected()) {
    // Send a message
    bleSerial.println("Hello from aCYD-MIDI!");
    
    // Send formatted data
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "BPM: %d", sharedBPM);
    bleSerial.println(buffer);
}

// Check for incoming data
if (bleSerial.available() > 0) {
    char command[128];
    size_t len = bleSerial.readLine(command, sizeof(command));
    
    // Process command
    if (strcmp(command, "status") == 0) {
        bleSerial.println("Status: Running");
    }
}
```

### Stream-Like Interface

```cpp
// Write methods
bleSerial.write('A');                    // Single byte
bleSerial.write(buffer, length);         // Buffer
bleSerial.print("Text");                 // String (no newline)
bleSerial.println("Text");               // String with newline

// Read methods
int byte = bleSerial.read();             // Read single byte (-1 if empty)
int byte = bleSerial.peek();             // Peek without removing
size_t n = bleSerial.readBytes(buf, len); // Read multiple bytes
size_t n = bleSerial.readLine(buf, len);  // Read until newline

// Utility
int count = bleSerial.available();       // Bytes available to read
bool connected = bleSerial.isConnected(); // Connection status
bleSerial.flush();                       // Force send buffered data
bleSerial.clear();                       // Clear all buffers
```

## Example Use Cases

### 1. Live Parameter Monitoring

```cpp
void loop() {
    // Periodically send status updates
    static unsigned long lastUpdate = 0;
    if (bleSerial.isConnected() && millis() - lastUpdate > 1000) {
        char status[128];
        snprintf(status, sizeof(status), 
                 "Mode: %d, BPM: %d, Connected: %s",
                 currentMode, sharedBPM, deviceConnected ? "Yes" : "No");
        bleSerial.println(status);
        lastUpdate = millis();
    }
}
```

### 2. Remote Configuration

```cpp
void processSerialCommands() {
    if (bleSerial.available() > 0) {
        char cmd[128];
        bleSerial.readLine(cmd, sizeof(cmd));
        
        if (strncmp(cmd, "set bpm ", 8) == 0) {
            int bpm = atoi(cmd + 8);
            setSharedBPM(bpm);
            bleSerial.println("OK");
        } 
        else if (strcmp(cmd, "get bpm") == 0) {
            char response[32];
            snprintf(response, sizeof(response), "BPM: %d", sharedBPM);
            bleSerial.println(response);
        }
        else if (strcmp(cmd, "mode list") == 0) {
            bleSerial.println("Available modes:");
            bleSerial.println("0: Menu");
            bleSerial.println("1: Settings");
            bleSerial.println("2: Keyboard");
            // ... etc
        }
    }
}
```

### 3. Debug Logging

```cpp
void debugLog(const char* message) {
#if ENABLE_BLE_SERIAL
    if (bleSerial.isConnected()) {
        bleSerial.print("[DEBUG] ");
        bleSerial.println(message);
    }
#endif
    // Also log to serial
    Serial.println(message);
}
```

## Client Applications

### Mobile Apps

**iOS:**
- **nRF Connect** - Nordic's BLE scanner and UART tool
- **BLE Terminal** - Simple terminal app
- **LightBlue** - BLE explorer with UART support

**Android:**
- **nRF Connect** - Nordic's official app
- **Serial Bluetooth Terminal** - UART terminal
- **BLE Scanner** - Generic BLE tools

### Desktop Tools

**Cross-Platform:**
- **Web BLE Terminal** - Browser-based (Chrome/Edge)
- **Adafruit Bluefruit LE Connect** - Python-based

**macOS/Linux/Windows:**
- Custom scripts using `bleak` (Python)
- Node.js apps using `noble` or `@abandonware/noble`

### Example: Python Client with bleak

```python
import asyncio
from bleak import BleakClient, BleakScanner

SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Device -> Client
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Client -> Device

async def main():
    # Find aCYD MIDI device
    devices = await BleakScanner.discover()
    acyd = next((d for d in devices if d.name and "aCYD MIDI" in d.name), None)
    
    if not acyd:
        print("aCYD MIDI device not found")
        return
    
    # Connect
    async with BleakClient(acyd.address) as client:
        # Subscribe to notifications
        def notification_handler(sender, data):
            print(f"Received: {data.decode('utf-8', errors='ignore')}")
        
        await client.start_notify(TX_CHAR_UUID, notification_handler)
        
        # Send command
        cmd = b"status\n"
        await client.write_gatt_char(RX_CHAR_UUID, cmd)
        
        # Wait for response
        await asyncio.sleep(2)

asyncio.run(main())
```

## Protocol Considerations

### Message Format

- **Text-based, newline-delimited** (recommended)
  - Commands/responses end with `\n` or `\r\n`
  - Easy to parse, human-readable
  - Compatible with terminal apps

- **Binary/Framed** (advanced)
  - For structured data (JSON, MessagePack, etc.)
  - Requires custom client implementation

### Rate Limiting

- TX buffer auto-flushes every 20ms
- Maximum 20 bytes per notification (BLE MTU consideration)
- Large messages automatically chunked

### Buffer Sizes

- **RX Buffer:** 256 bytes (incoming data)
- **TX Buffer:** 256 bytes (outgoing data)
- Buffers protect against overflow but drop data if full

## Best Practices

1. **Keep messages small** - Under 100 bytes per logical message
2. **Use newlines** - Makes debugging easier and improves compatibility
3. **Don't block** - Check `isConnected()` before writing to avoid delays
4. **Rate limit** - Don't flood the BLE Serial (MIDI has priority)
5. **Error handling** - Check return values from read/write operations

## Architecture Notes

- Both BLE MIDI and BLE Serial share the same BLE connection
- Services are advertised together but operate independently
- BLE MIDI is prioritized (real-time safe)
- BLE Serial is buffered and rate-limited to avoid starving MIDI
- No performance impact when BLE Serial is disabled (compile-time flag)

## Troubleshooting

### BLE Serial not appearing

- Verify `ENABLE_BLE_SERIAL=1` in build flags
- Check serial output for "BLE Serial service started successfully"
- Use nRF Connect to verify service UUID is advertised

### Data not being received

- Check `bleSerial.isConnected()` returns true
- Verify client has subscribed to TX characteristic notifications
- Try calling `bleSerial.flush()` to force transmission

### Buffer overflow warnings

- Reduce message size
- Increase read frequency
- Add flow control in your protocol

## Future Enhancements

Potential additions (not currently implemented):

- Flow control (CTS/RTS emulation)
- Adjustable MTU negotiation
- Binary frame support
- Command parser utilities
- Statistics/diagnostics
