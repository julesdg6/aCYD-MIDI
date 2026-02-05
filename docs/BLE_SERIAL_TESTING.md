# BLE Serial Testing Guide

This guide helps you test the dual BLE services feature on aCYD-MIDI hardware.

## Prerequisites

### Hardware
- aCYD-MIDI compatible ESP32 board (ESP32-2432S028Rv2 or similar)
- USB cable for programming
- Mobile device (iOS/Android) OR computer with Bluetooth

### Software
- PlatformIO installed
- nRF Connect app (mobile) OR Python with `bleak` (computer)

## Build Firmware

### Option 1: Default Build (BLE Serial disabled)

```bash
cd aCYD-MIDI
pio run -e esp32-2432S028Rv2
pio run -e esp32-2432S028Rv2 -t upload
```

**Expected:** BLE MIDI works as before, no BLE Serial service

### Option 2: BLE Serial Enabled

```bash
cd aCYD-MIDI
pio run -e esp32-2432S028Rv2-ble-serial
pio run -e esp32-2432S028Rv2-ble-serial -t upload
```

**Expected:** Both BLE MIDI and BLE Serial services available

## Test 1: Verify BLE Services (Mobile - nRF Connect)

### Steps

1. **Install nRF Connect**
   - iOS: [App Store](https://apps.apple.com/app/nrf-connect-for-mobile/id1054362403)
   - Android: [Play Store](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp)

2. **Power on aCYD-MIDI**
   - Connect USB power
   - Wait ~5 seconds for BLE to initialize
   - Look for splash screen and menu

3. **Scan for device**
   - Open nRF Connect
   - Tap "Scan" button
   - Look for device named "aCYD MIDI-XXXXXX" (X = MAC address)

4. **Connect**
   - Tap "Connect" next to your device
   - Wait for connection to establish

5. **Verify services**

   **Default build (BLE Serial disabled):**
   - Should see 1 service: `03b80e5a-...` (BLE MIDI)
   
   **BLE Serial build:**
   - Should see 2 services:
     - `03b80e5a-...` (BLE MIDI)
     - `6e400001-...` (BLE Serial / Nordic UART Service)

### Expected Results

✅ Device appears in scan list with unique name  
✅ Connection succeeds  
✅ Correct service(s) appear based on build  
✅ BLE Serial service shows TX and RX characteristics  

### Troubleshooting

❌ **Device not found:**
- Check device is powered on
- Check Bluetooth is enabled on phone
- Move closer to device
- Try power cycling device

❌ **BLE Serial service not found (when enabled):**
- Verify build: check serial output for "BLE Serial service started successfully"
- Check you flashed the correct build (`-ble-serial` environment)
- Try reflashing firmware

## Test 2: BLE Serial Communication (Mobile - nRF Connect)

**Requires:** BLE Serial enabled build

### Steps

1. **Connect to device** (follow Test 1 steps 1-4)

2. **Find BLE Serial service**
   - Scroll to service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
   - Expand to see characteristics:
     - `6E400002-...` = RX (Write)
     - `6E400003-...` = TX (Notify)

3. **Subscribe to TX notifications**
   - Tap the TX characteristic (`6E400003-...`)
   - Tap the "Notify" icon (three arrows)
   - Should now receive data from device

4. **Send test command**
   - Tap the RX characteristic (`6E400002-...`)
   - Select "Text" format
   - Type: `version` (lowercase)
   - Tap "Send"

5. **Check response**
   - Look at TX characteristic notifications
   - Should receive firmware version (e.g., "0.0.3")

6. **Try more commands**
   ```
   status
   get bpm
   set bpm 140
   help
   list modes
   ```

### Expected Results

✅ TX notifications enabled successfully  
✅ `version` command returns version string  
✅ `status` command returns mode, BPM, connection status  
✅ `get bpm` returns current BPM  
✅ `set bpm 140` returns "OK" and BPM changes  
✅ `help` lists available commands  

### Troubleshooting

❌ **No response to commands:**
- Make sure TX notifications are enabled
- Check command includes newline (or send as text)
- Try simpler command like `version`
- Check serial monitor for debug output

❌ **Commands rejected:**
- Check command spelling (case-insensitive)
- Verify parameters (e.g., BPM must be 40-300)
- Type `help` to see valid commands

## Test 3: BLE Serial Communication (Computer - Python)

**Requires:** 
- BLE Serial enabled build
- Python 3.7+
- `bleak` library (`pip install bleak`)

### Steps

1. **Install dependencies**
   ```bash
   pip install bleak
   ```

2. **Run test client**
   ```bash
   cd aCYD-MIDI/examples
   python test_ble_serial.py
   ```

3. **Follow prompts**
   - Script scans for aCYD-MIDI devices
   - Select your device
   - Choose mode:
     - 1 = Interactive (type commands manually)
     - 2 = Automated tests

4. **Interactive mode**
   ```
   > version
   ← 0.0.3
   
   > status
   ← Mode: Menu
   ← BPM: 120
   ← BLE MIDI: Disconnected
   ← Version: 0.0.3
   
   > set bpm 140
   ← OK
   
   > quit
   ```

5. **Automated mode**
   - Runs predefined tests
   - Checks for responses
   - Reports results

### Expected Results

✅ Device found and connected  
✅ BLE Serial service detected  
✅ All commands receive responses  
✅ Interactive mode allows typing commands  
✅ Automated tests all pass  

### Troubleshooting

❌ **Module not found:**
- Install bleak: `pip install bleak`

❌ **Device not found:**
- Check Bluetooth is enabled
- Make sure device is powered on
- Run scan manually to verify device visible

❌ **BLE Serial service not found:**
- Verify firmware build includes BLE Serial
- Check serial output for initialization message

## Test 4: Concurrent BLE MIDI + BLE Serial

**Requires:** 
- BLE Serial enabled build
- DAW or MIDI monitor software
- Mobile app OR Python script

### Steps

1. **Connect BLE MIDI to DAW**
   - Open your DAW (Ableton, Logic, etc.)
   - Enable aCYD-MIDI as MIDI input
   - Create MIDI track monitoring input

2. **Connect BLE Serial**
   - Open nRF Connect OR run Python script
   - Connect to BLE Serial service
   - Subscribe to TX notifications

3. **Test concurrent operation**
   
   **While connected to both:**
   - Switch to Keyboard mode on aCYD-MIDI
   - Play notes on touchscreen
   - Send `status` command via BLE Serial
   
   **Expected:**
   - MIDI notes received by DAW
   - BLE Serial responds with status
   - No interference between services

4. **Check performance**
   - Play rapid notes on keyboard
   - Send multiple BLE Serial commands
   - Monitor for:
     - Missed notes (shouldn't happen)
     - Command response delays (should be <100ms)
     - Disconnections (shouldn't happen)

### Expected Results

✅ BLE MIDI and BLE Serial work simultaneously  
✅ No MIDI note dropouts  
✅ BLE Serial commands respond promptly  
✅ No BLE disconnections  
✅ Stable operation  

### Troubleshooting

❌ **MIDI notes dropped:**
- BLE Serial may be sending too much data
- Reduce BLE Serial traffic
- Report as bug (should not happen)

❌ **BLE Serial slow:**
- Normal: 20ms auto-flush interval
- Expected latency: <100ms
- If >1 second, report as bug

❌ **Random disconnects:**
- Check Bluetooth interference
- Move closer to device
- Disable other Bluetooth devices

## Test 5: Memory and Performance

### Check Memory Usage

Monitor serial output during boot:

**Default build:**
```
Free heap: ~200KB
```

**BLE Serial build:**
```
Free heap: ~199KB  (1KB less is expected)
```

### Check Performance

1. **Boot time:** Should be same (~5 seconds to BLE init)
2. **MIDI latency:** Should be same (<10ms)
3. **UI responsiveness:** Should be same (no lag)

### Expected Results

✅ ~1KB additional RAM used (BLE Serial buffers)  
✅ No noticeable performance impact  
✅ Boot time unchanged  
✅ MIDI latency unchanged  

## Test Matrix Summary

| Test | Description | Pass Criteria |
|------|-------------|---------------|
| 1 | BLE services visible | Correct services appear |
| 2 | BLE Serial commands (mobile) | All commands work |
| 3 | BLE Serial commands (Python) | Script runs successfully |
| 4 | Concurrent operation | Both services work together |
| 5 | Memory/performance | No degradation |

## Reporting Issues

If you find bugs, please report with:

1. **Build configuration**
   - Environment used (e.g., `esp32-2432S028Rv2-ble-serial`)
   - PlatformIO version
   - Firmware version

2. **Test that failed**
   - Which test (1-5)
   - Expected vs actual result
   - Serial monitor output

3. **Setup details**
   - Client (nRF Connect, Python, etc.)
   - Mobile device / computer model
   - BLE connections (MIDI + Serial, or Serial only)

4. **Steps to reproduce**
   - Detailed steps
   - How often it happens
   - Any error messages

## Success Criteria

Your testing is successful if:

✅ All 5 tests pass  
✅ No regression in BLE MIDI functionality  
✅ BLE Serial commands work as documented  
✅ Concurrent operation is stable  
✅ Performance is acceptable  

## Next Steps

After successful testing:

1. **Customize commands** - Edit `ble_serial_commands.h`
2. **Build custom client** - Use Python script as template
3. **Integrate into workflow** - Use for live performance control
4. **Share feedback** - Help improve the feature

## Additional Resources

- [BLE Serial Usage Guide](../docs/BLE_SERIAL_USAGE.md)
- [BLE Serial Implementation](../docs/BLE_SERIAL_IMPLEMENTATION.md)
- [Python Test Client](../examples/test_ble_serial.py)
- [nRF Connect Documentation](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Mobile)
