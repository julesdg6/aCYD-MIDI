#!/usr/bin/env python3
"""
Log BLE MIDI notifications and PlatformIO serial monitor simultaneously.

Requirements:
- Python 3.8+
- bleak (pip install bleak)
- PlatformIO CLI (pio) on PATH

Usage:
  ./scripts/log_ble_and_serial.py --serial-port /dev/cu.wchusbserial110 --serial-baud 115200 --out-dir logs

The script will:
- Scan for a device advertising the BLE MIDI service UUID and connect
- Subscribe to the BLE MIDI characteristic and log notifications with timestamps
- Spawn `pio device monitor` (or `cat` the provided serial port) and log serial output
- Write two files in the `--out-dir`: `ble_midi.log` and `serial.log`

"""
import argparse
import asyncio
import sys
import os
import datetime
import subprocess
import signal
from pathlib import Path

try:
    from bleak import BleakScanner, BleakClient
except Exception as e:
    print("Missing dependency 'bleak'. Install with: pip install bleak")
    raise

BLE_MIDI_SERVICE = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
BLE_MIDI_CHAR = "7772e5db-3868-4112-a1a9-f2669d106bf3"

async def find_midi_device(timeout=5.0):
    print(f"Scanning for BLE MIDI devices (service={BLE_MIDI_SERVICE}) for {timeout}s...")
    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        uuids = getattr(d, "metadata", {}).get("uuids") or []
        # On some backends, uuids may be under d.details or not present; do a simple name check fallback
        if uuids and any(u.lower() == BLE_MIDI_SERVICE for u in uuids):
            print(f"Found device {d.name} [{d.address}] advertising MIDI service")
            return d
    # fallback: return first device with 'MIDI' in name
    for d in devices:
        if d.name and "midi" in d.name.lower():
            print(f"Found device by name {d.name} [{d.address}]")
            return d
    return None

async def run_ble_logger(out_path, device_addr=None):
    async def handle_notification(sender, data: bytearray):
        ts = datetime.datetime.now().isoformat()
        hexdata = data.hex()
        line = f"{ts} {sender} {hexdata}\n"
        out_path.write_text(out_path.read_text() + line)
        print(line, end='')

    if device_addr is None:
        dev = await find_midi_device()
        if dev is None:
            print("No BLE MIDI device found.")
            return
        device_addr = dev.address

    print(f"Connecting to BLE device {device_addr}...")
    async with BleakClient(device_addr) as client:
        # Some bleak versions expose service discovery differently; avoid hard failure.
        try:
            svcs = await client.get_services()
            has_char = any(c.uuid.lower() == BLE_MIDI_CHAR for s in svcs for c in s.characteristics)
        except Exception:
            has_char = False
        if not has_char:
            print("Warning: device did not advertise MIDI char UUID. Continuing and attempting to subscribe...")
        print(f"Subscribing to characteristic {BLE_MIDI_CHAR}...")
        try:
            await client.start_notify(BLE_MIDI_CHAR, handle_notification)
        except Exception as e:
            print(f"Failed to start notify on {BLE_MIDI_CHAR}: {e}")
            raise
        print("BLE logger running. Press Ctrl-C to stop.")
        try:
            while True:
                await asyncio.sleep(1.0)
        except asyncio.CancelledError:
            pass
        finally:
            await client.stop_notify(BLE_MIDI_CHAR)

serial_proc = None

def tail_serial(port, baud, out_path):
    # Prefer pyserial for robust non-interactive serial capture
    try:
        import serial as pyserial
        print(f"Starting serial monitor via pyserial on {port} @ {baud}")
        # write a startup marker so we can see the monitor started even if no data
        try:
            with open(out_path, "a", buffering=1) as f:
                f.write(f"{datetime.datetime.now().isoformat()} SERIAL_MONITOR_STARTED pyserial on {port} @ {baud}\n")
        except Exception:
            pass
        with pyserial.Serial(port, baud, timeout=1) as ser, open(out_path, "a", buffering=1) as f:
            while True:
                try:
                    raw = ser.readline()
                except Exception:
                    raw = b""
                if not raw:
                    continue
                try:
                    line = raw.decode(errors='replace')
                except Exception:
                    line = str(raw)
                ts = datetime.datetime.now().isoformat()
                f.write(f"{ts} {line}")
                print(line, end='')
    except Exception:
        # Fallback: prefer PlatformIO monitor if available, else cat the device
        pio_cmd = ["pio", "device", "monitor", "-p", port, "-b", str(baud)]
        if shutil.which("pio"):
            cmd = pio_cmd
        else:
            cmd = ["/usr/bin/env", "sh", "-c", f"cat {port}"]

        print(f"Starting serial monitor subprocess: {' '.join(cmd)}")
        # Open subprocess and stream to file
        global serial_proc
        serial_proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, text=True)
        try:
            with open(out_path, "a", buffering=1) as f:
                for line in serial_proc.stdout:
                    ts = datetime.datetime.now().isoformat()
                    f.write(f"{ts} {line}")
                    print(line, end='')
        finally:
            # Ensure subprocess is terminated when this function exits
            try:
                if serial_proc and serial_proc.poll() is None:
                    serial_proc.terminate()
            except Exception:
                pass

def _handle_signal(signum, frame):
    print(f"Received signal {signum}, shutting down...", file=sys.stderr)
    global serial_proc
    try:
        if serial_proc and serial_proc.poll() is None:
            serial_proc.terminate()
            try:
                serial_proc.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                serial_proc.kill()
    except Exception:
        pass
    # Exit process; asyncio.run will be interrupted
    sys.exit(0)

import shutil

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-port", required=False, default=None)
    parser.add_argument("--serial-baud", default=115200, type=int)
    parser.add_argument("--ble-addr", default=None, help="Optional BLE device address to connect to")
    parser.add_argument("--out-dir", default="logs")
    parser.add_argument("--scan-time", default=5.0, type=float)
    args = parser.parse_args()

    outdir = Path(args.out_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    ble_log = outdir / "ble_midi.log"
    serial_log = outdir / "serial.log"

    # Initialize/clear files
    ble_log.write_text("")
    serial_log.write_text("")

    try:
        # Start serial monitor in a thread/process (optional)
        import threading
        serial_thread = None
        if args.serial_port:
            serial_thread = threading.Thread(target=tail_serial, args=(args.serial_port, args.serial_baud, serial_log), daemon=True)
            serial_thread.start()

        # Run BLE logger in asyncio using asyncio.run for modern Python
        coro = run_ble_logger(ble_log, device_addr=args.ble_addr)
        try:
            asyncio.run(coro)
        except KeyboardInterrupt:
            print("Stopping logs (Ctrl-C received)")
    finally:
        # allow serial thread to terminate
        try:
            serial_thread.join(timeout=1.0)
        except Exception:
            pass

if __name__ == '__main__':
    main()
