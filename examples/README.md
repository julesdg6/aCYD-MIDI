# aCYD-MIDI Examples

This directory contains example scripts and tools for working with aCYD-MIDI.

## BLE Serial Test Client

**File:** `test_ble_serial.py`

A Python script to interact with aCYD-MIDI's BLE Serial service. Useful for:
- Testing BLE Serial functionality
- Remote configuration
- Debugging
- Building custom control interfaces

### Requirements

```bash
pip install bleak
```

### Usage

**Interactive Mode:**
```bash
python test_ble_serial.py
```

Select option 1 for interactive session. Type commands and see responses in real-time.

**Automated Tests:**
Select option 2 to run a series of automated test commands.

### Example Session

```
$ python test_ble_serial.py
aCYD-MIDI BLE Serial Test Client
============================================================
Scanning for aCYD-MIDI devices...
Found: aCYD MIDI-1A2B3C (AA:BB:CC:DD:EE:FF)

Connecting to aCYD MIDI-1A2B3C...
✓ Connected!
✓ BLE Serial service found

Select mode:
  1. Interactive session
  2. Automated tests
Choice (1-2): 1

============================================================
Interactive BLE Serial Session
============================================================
Type commands and press Enter. Type 'quit' to exit.
Try: status, help, get bpm, set bpm 140, list modes
============================================================

> status
→ status
← Mode: Menu
← BPM: 120
← BLE MIDI: Disconnected
← Version: 0.0.3

> set bpm 140
→ set bpm 140
← OK

> get bpm
→ get bpm
← 140

> quit
```

### Supported Commands

- `status` - Get device status
- `get bpm` - Get current BPM
- `set bpm <value>` - Set BPM (40-300)
- `get mode` - Get current mode number
- `list modes` - List all available modes
- `help` - Show available commands
- `version` - Show firmware version

### Troubleshooting

**Device not found:**
- Make sure aCYD-MIDI is powered on
- Check Bluetooth is enabled on your computer
- Move closer to the device
- Make sure firmware was built with `ENABLE_BLE_SERIAL=1`

**Service not found:**
- Verify firmware includes BLE Serial: `pio run -e esp32-2432S028Rv2-ble-serial`
- Check serial monitor for "BLE Serial service started successfully"

**No responses to commands:**
- Check TX characteristic notifications are subscribed
- Wait a bit longer (increase `asyncio.sleep()` duration)
- Try simpler commands like `version` first

## Adding Your Own Scripts

When creating scripts to interact with aCYD-MIDI:

1. Use the Nordic UART Service UUIDs (see `test_ble_serial.py`)
2. Subscribe to TX characteristic for receiving data
3. Write to RX characteristic for sending data
4. Use newline-terminated text commands
5. Wait for responses (device may take 20ms to flush)

### Example Python Snippet

```python
from bleak import BleakClient

SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

async with BleakClient(address) as client:
    # Subscribe to notifications
    await client.start_notify(TX_UUID, lambda s, d: print(d.decode()))
    
    # Send command
    await client.write_gatt_char(RX_UUID, b"status\n")
    await asyncio.sleep(0.5)
```

## See Also

- [BLE Serial Usage Guide](../docs/BLE_SERIAL_USAGE.md) - Complete API documentation
- [BLE Serial Implementation](../docs/BLE_SERIAL_IMPLEMENTATION.md) - Technical details
