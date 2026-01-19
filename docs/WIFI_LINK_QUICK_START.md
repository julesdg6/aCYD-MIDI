# Quick Start: Wi-Fi MIDI & Ableton Link

This guide will help you quickly get Wi-Fi MIDI and Ableton Link working on your aCYD-MIDI controller.

## Prerequisites

- aCYD-MIDI device with firmware version 0.0.1 or later
- Wi-Fi network (2.4GHz or 5GHz)
- Wi-Fi credentials configured in `config/wifi_config.h`
- DAW or Link-enabled app (see compatibility below)

## Quick Setup

### 1. Enable Wi-Fi

**Build Configuration** (`platformio.ini`):
```ini
build_flags = 
    -D WIFI_ENABLED=1
```

**Wi-Fi Credentials** (`config/wifi_config.h`):
```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
```

### 2. Flash Firmware

```bash
pio run -e esp32-2432S028Rv2 -t upload
```

### 3. Verify Wi-Fi Connection

On CYD device:
1. Navigate to **Settings** mode
2. Check **WiFi** status shows "Connected"
3. Note the IP address displayed

## Using Wi-Fi MIDI

### Setup on CYD

Currently, Wi-Fi MIDI requires code-level enablement:

**In `src/main.cpp` setup():**
```cpp
#if WIFI_ENABLED
  initWiFiMIDI();
  setWiFiMIDIEnabled(true);  // Add this line
#endif
```

**Configuration**: Edit `src/wifi_midi.cpp` to set target IP:
```cpp
static IPAddress targetIP(192, 168, 1, 100);  // Change to your receiver's IP
```

### Setup Receiver

#### Option A: Python MIDI Bridge (Easiest)

```python
import socket
import mido

# Listen for Wi-Fi MIDI
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 5004))

# Create virtual MIDI port
port = mido.open_output('aCYD Wi-Fi MIDI')

print("Listening for Wi-Fi MIDI on port 5004...")

while True:
    data, addr = sock.recvfrom(5)
    if len(data) == 5 and data[0] == 0x80 and data[1] == 0x80:
        status, data1, data2 = data[2], data[3], data[4]
        msg = mido.Message.from_bytes([status, data1, data2])
        port.send(msg)
        print(f"MIDI: {msg}")
```

**Install dependencies:**
```bash
pip install python-rtmidi mido
```

**Run:**
```bash
python wifi_midi_bridge.py
```

#### Option B: Network MIDI (macOS)

1. Open **Audio MIDI Setup**
2. Window → **Show MIDI Studio**
3. Click **Network** icon
4. Click **+** to add a session
5. Name it "aCYD"
6. Connect to CYD's IP address

#### Option C: rtpmidid (Linux)

```bash
# Install
sudo apt-get install rtpmidid

# Run
rtpmidid &
```

### Testing

1. **Play a note** on CYD (use KEYS mode)
2. **Verify reception** in MIDI monitor
3. **Route to DAW** via virtual MIDI port

Expected output:
```
Wi-Fi MIDI RX: 90 3C 7F  (Note On, C4, velocity 127)
Wi-Fi MIDI RX: 80 3C 00  (Note Off, C4)
```

## Using Ableton Link

### Enable Link on CYD

**Method 1: Settings UI** (Current)
1. Navigate to **Settings** mode
2. Tap **Clock Master** repeatedly
3. Select **"Ableton Link"**
4. Link automatically enables

**Method 2: Code** (Alternative)
```cpp
setLinkEnabled(true);
midiClockMaster = CLOCK_LINK;
```

### Start Link-Enabled App

#### Ableton Live
1. Open Ableton Live
2. Preferences → Link/Tempo/MIDI
3. Enable **Link**
4. Set tempo (e.g., 120 BPM)

#### iOS Apps
1. Install a Link app (e.g., LinkHut, Patterning)
2. Enable Link in app settings
3. Set tempo

#### Other DAWs
- **Bitwig**: Enable Link in settings
- **FL Studio**: Install Link bridge plugin
- **REAPER**: Install Link VST plugin

### Verify Connection

On CYD device:
1. Navigate to **Settings** mode
2. Check **Ableton Link** status:
   ```
   Ableton Link: 120 BPM, 2 peers
   ```
   (Should show connected status in green)

3. Change tempo in DAW
4. Verify **Shared Tempo** updates on CYD

### Using Link Tempo

Once connected, all tempo-aware modes use Link tempo:
- **BEATS** sequencer
- **SLINK** generative engine
- **LFO** modulation
- **RAGA** drone patterns
- **EUCLID** rhythm patterns

Example in SLINK mode:
1. Select **SLINK** mode
2. Tempo automatically follows Link session
3. Change tempo in DAW → SLINK updates immediately

## Troubleshooting

### Wi-Fi MIDI Issues

**Problem**: No MIDI received

**Check**:
1. ✓ Wi-Fi connected? (Settings → WiFi: Connected)
2. ✓ Wi-Fi MIDI enabled? (check code)
3. ✓ Correct target IP? (verify in `wifi_midi.cpp`)
4. ✓ Receiver listening on port 5004?
5. ✓ Firewall allows UDP port 5004?

**Test with netcat:**
```bash
nc -u -l 5004 | hexdump -C
```

### Link Issues

**Problem**: Link not finding peers

**Check**:
1. ✓ Wi-Fi connected? (Settings → WiFi: Connected)
2. ✓ Same network? (CYD and app on same Wi-Fi)
3. ✓ Link enabled in app?
4. ✓ Clock Master = "Ableton Link"?
5. ✓ Multicast allowed on network?

**Test**: Try with LinkHut (free iOS/macOS app)

**Problem**: Tempo not syncing

**Check**:
1. ✓ Link shows "Connected" with peer count?
2. ✓ Clock Master set to "Ableton Link"?
3. ✓ Mode supports tempo sync? (see mode docs)

### Network Issues

**Problem**: Intermittent connection

**Solutions**:
- Move closer to router
- Use 5GHz Wi-Fi if available
- Disable router's AP Isolation
- Enable IGMP snooping (for Link multicast)

## Feature Matrix

| Feature | Status | How to Enable |
|---------|--------|---------------|
| Wi-Fi MIDI Send | ✅ Working | Code: `setWiFiMIDIEnabled(true)` |
| Wi-Fi MIDI Receive | ⚠️ Logs only | Automatic when enabled |
| Link Tempo Sync | ✅ Working | Settings → Clock Master → Ableton Link |
| Link Phase Sync | ❌ Not supported | Future enhancement |
| Link Transport | ❌ Not supported | Future enhancement |
| UI Toggle (Wi-Fi MIDI) | 📋 Planned | Coming soon |
| UI Toggle (Link) | ✅ Working | Settings → Clock Master |
| IP Configuration UI | 📋 Planned | Coming soon |

## Example Workflows

### Workflow 1: DAW Control Surface

**Goal**: Use CYD as a wireless MIDI controller for Ableton Live

**Setup**:
1. Enable Wi-Fi MIDI on CYD
2. Run Python MIDI bridge on computer
3. Route bridge output to Ableton
4. Select MIDI track in Ableton
5. Play CYD keyboard → controls Ableton instrument

### Workflow 2: Synchronized Jam

**Goal**: Sync CYD with iOS apps and DAW

**Setup**:
1. Enable Link on CYD (Settings → Clock Master → Ableton Link)
2. Enable Link in Ableton Live
3. Enable Link in iOS apps (Patterning, AUM, etc.)
4. All devices now share tempo
5. Change tempo anywhere → all devices follow

### Workflow 3: Wireless MIDI Bridge

**Goal**: Use CYD to bridge Bluetooth MIDI to Wi-Fi MIDI

**Setup**:
1. Connect Bluetooth MIDI device to CYD
2. Enable Wi-Fi MIDI on CYD
3. CYD forwards all MIDI to network
4. Computer receives via Wi-Fi MIDI bridge

**Note**: Full bidirectional bridge requires code modification

## Performance Tips

1. **Reduce Wi-Fi Latency**
   - Use 5GHz Wi-Fi when possible
   - Position CYD close to router
   - Minimize network traffic during performance

2. **Improve Link Stability**
   - Keep all Link devices on same subnet
   - Disable router's multicast filtering
   - Use wired Ethernet for DAW if possible

3. **Battery Performance**
   - Wi-Fi uses ~50mA additional current
   - Link uses ~5mA additional current
   - Disable when not needed to save power

## Next Steps

- Read [WIFI_MIDI.md](WIFI_MIDI.md) for detailed Wi-Fi MIDI documentation
- Read [ABLETON_LINK.md](ABLETON_LINK.md) for detailed Link documentation
- Explore Link-enabled apps: [Ableton Link Products](https://www.ableton.com/en/link/products/)
- Join discussions: [aCYD-MIDI GitHub Discussions](https://github.com/julesdg6/aCYD-MIDI/discussions)

## Support

**Issues?** Open a GitHub issue: [aCYD-MIDI Issues](https://github.com/julesdg6/aCYD-MIDI/issues)

**Questions?** Start a discussion: [aCYD-MIDI Discussions](https://github.com/julesdg6/aCYD-MIDI/discussions)
