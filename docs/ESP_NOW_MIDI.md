# ESP-NOW MIDI Networking

## Overview

ESP-NOW MIDI enables low-latency wireless MIDI communication between multiple CYD devices without requiring Bluetooth pairing or Wi-Fi association. This feature allows you to create distributed MIDI setups with multiple CYD controllers working together.

## Features

- **Low Latency**: Typically <10ms round-trip time
- **No Pairing Required**: Automatic peer discovery in Broadcast mode
- **Multiple Operating Modes**: Off, Broadcast, and Peer modes
- **MIDI Clock Sync**: Synchronize tempo across multiple devices
- **Bidirectional Communication**: Send and receive MIDI messages
- **Transparent Routing**: Integrates seamlessly with BLE and Hardware MIDI

## Hardware Requirements

- ESP32-based CYD devices (ESP32 or ESP32-S3)
- All devices must be on the same ESP-NOW channel (default: Channel 6)
- Devices should be within ~100m range (typical Wi-Fi range)

## Operating Modes

### Off Mode
### Off Mode (Default)
ESP-NOW MIDI is completely disabled by default. No wireless communication occurs until you enable it in Settings.

### Broadcast Mode
- Automatic peer discovery
- Any CYD device that sends an ESP-NOW message will be automatically added as a peer
- Ideal for quick setup without manual configuration
- Messages are sent to all discovered peers

### Peer Mode
- Manual peer management
- Only communicates with explicitly added peers
- Better for controlled setups where you want specific device-to-device communication
- More secure as only known devices can communicate

## Configuration

### Enabling ESP-NOW MIDI

**ESP-NOW is disabled by default.** To enable it:

1. Open the **Settings** mode from the main menu
2. Scroll down to the **ESP-NOW MIDI** section
3. Tap the **ESP-NOW MIDI** button to toggle from "Disabled" to "Enabled"
4. When enabled, the button turns green and mode switches to Broadcast
5. Use the **ESP-NOW Mode** button to switch between modes if needed

### ESP-NOW Mode Selection

Tap the **ESP-NOW Mode** button to cycle through:
- **Off** → Disabled (gray)
- **Broadcast** → Auto-discovery enabled (cyan)
- **Peer** → Manual peer management (cyan)

### Status Display

The settings screen shows:
- **Peers**: Number of connected devices
- **TX**: Total messages sent via ESP-NOW
- **RX**: Total messages received via ESP-NOW

Example: `ESP-NOW: Peers=2 TX=1024 RX=856`

## MIDI Routing

When ESP-NOW is enabled, all MIDI messages are automatically routed to:
1. **BLE MIDI** (if connected)
2. **Hardware MIDI** (if enabled)
3. **ESP-NOW MIDI** (to all peers)

Incoming ESP-NOW MIDI messages are routed to:
1. **BLE MIDI** (if connected)
2. **Hardware MIDI** (if enabled)

This creates a transparent MIDI network where all devices see all MIDI messages.

## Clock Synchronization

ESP-NOW can act as a MIDI clock source:

1. Open **Settings** mode
2. Set **Clock Master** to **ESP-NOW MIDI**
3. The device will now sync its internal tempo to incoming ESP-NOW MIDI clock messages

**Clock Master Options:**
- **Internal Clock**: Device generates its own tempo
- **WiFi MIDI**: Sync to Wi-Fi MIDI clock
- **BLE MIDI**: Sync to Bluetooth MIDI clock
- **Hardware MIDI**: Sync to DIN MIDI clock
- **ESP-NOW MIDI**: Sync to ESP-NOW MIDI clock

When ESP-NOW is the clock master, sequencer and rhythm modes will follow the received clock.

## Use Cases

### Multi-Device Sequencer Setup
1. CYD #1: Run the sequencer in master mode
2. CYD #2, #3, #4: Set clock master to ESP-NOW MIDI
3. All devices stay in perfect sync
4. Each CYD can control different instruments via its BLE/Hardware MIDI output

### Distributed Performance
1. Enable Broadcast mode on all devices
2. Each performer controls their own CYD
3. All MIDI messages are shared across all devices
4. Create collaborative performances with multiple controllers

### Clock Distribution
1. One CYD generates MIDI clock (set to Internal Clock)
2. Other CYDs set Clock Master to ESP-NOW MIDI
3. All devices run at the same tempo
4. Perfect for multi-CYD drum machine or arpeggiator setups

### Parameter Modulation Network
1. CYD #1: Use LFO mode to generate control changes
2. CYD #2, #3: Receive CC messages via ESP-NOW
3. All devices modulate their parameters together
4. Create evolving, synchronized soundscapes

## Technical Details

### ESP-NOW Channel
Default channel: **6**
- Can be changed in library configuration if needed
- All devices must use the same channel to communicate

### Message Types Supported
- Note On/Off
- Control Change (CC)
- Program Change
- Pitch Bend
- MIDI Clock (0xF8)
- MIDI Start (0xFA)
- MIDI Stop (0xFC)
- MIDI Continue (0xFB)

### Maximum Peers
- Broadcast mode: Up to 20 peers (library default)
- Peer mode: Up to 20 manually configured peers

### MAC Address
Each CYD has a unique MAC address for ESP-NOW identification.
To view your device's MAC address, check the serial console during initialization:
```
[ESP-NOW] Initialization complete
[ESP-NOW] MAC Address: AA:BB:CC:DD:EE:FF
```

## Troubleshooting

### ESP-NOW not working
- Verify ESP-NOW is enabled in Settings
- Check that all devices are powered on
- Ensure devices are within range (~100m)
- Try resetting by toggling ESP-NOW off and back on

### Peers not discovered in Broadcast mode
- Send any MIDI message from each device
- Peer discovery happens when messages are received
- Devices must send at least one message to be discovered

### Clock sync issues
- Verify Clock Master is set to "ESP-NOW MIDI" on receiving devices
- Ensure the clock source device is sending clock messages
- Check ESP-NOW status shows RX messages increasing
- Restart both devices if sync is lost

### Latency issues
- Keep devices closer together
- Reduce Wi-Fi interference in the area
- Ensure no other heavy Wi-Fi traffic nearby
- ESP-NOW operates on 2.4 GHz band

## Compile-Time Configuration

ESP-NOW MIDI can be disabled at compile time to save memory:

In `platformio.ini`, set:
```ini
-D ESP_NOW_ENABLED=0
```

Default is enabled (1). When disabled, all ESP-NOW code is excluded from the build.

## Compatibility with Wi-Fi and BLE

ESP-NOW uses the ESP32's Wi-Fi radio in station mode:
- **BLE MIDI**: Compatible ✓ (can run simultaneously)
- **Wi-Fi**: Compatible ✓ (ESP-NOW uses Wi-Fi hardware but doesn't require association)
- **Remote Display**: Compatible ✓ (requires Wi-Fi, can coexist with ESP-NOW)

All features can be enabled simultaneously, though this increases memory usage.

## Performance

### Latency
- Typical: <10 ms round-trip
- Measured: ~7ms average in testing
- Much lower than BLE MIDI (~20-30ms)

### Throughput
- Suitable for real-time MIDI performance
- Can handle dense note sequences
- Clock messages are lightweight

### Memory Usage
- ESP-NOW library: ~20KB flash
- Peer storage: ~200 bytes per peer
- Minimal RAM overhead during operation

## Safety and Best Practices

1. **Test Before Performance**: Always verify ESP-NOW connectivity before live use
2. **Peer Management**: In Peer mode, maintain a list of known device MAC addresses
3. **Fallback**: Keep BLE or Hardware MIDI as backup connection
4. **Range Awareness**: Stay within effective range during performance
5. **Power**: Ensure all devices have adequate power supply

## Future Enhancements

Possible future features (not yet implemented):
- Peer pairing wizard with QR codes
- Named peers for easier management
- Wireless pattern/preset sharing
- Dynamic routing configuration
- MIDI channel filtering per peer
- Encryption for secure communication

## Credits

This feature uses the **ESP-NOW-MIDI** library:
- Repository: https://github.com/grantler-instruments/ESP-NOW-MIDI
- License: LGPL-3.0
- Created by: Grantler Instruments

## See Also

- [Hardware MIDI Documentation](HARDWARE_MIDI.md)
- [Remote Display Documentation](REMOTE_DISPLAY.md)
- [Main README](README.md)
