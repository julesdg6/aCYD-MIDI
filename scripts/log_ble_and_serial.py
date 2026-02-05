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
import time
from pathlib import Path
# (shutil already imported at top)

try:
    from bleak import BleakScanner, BleakClient
except Exception as e:
    print("Missing dependency 'bleak'. Install with: pip install bleak")
    raise

BLE_MIDI_SERVICE = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
BLE_MIDI_CHAR = "7772e5db-3868-4112-a1a9-f2669d106bf3"

# BLE Serial (UART-over-BLE) service/char UUIDs (used for testing/debug)
BLE_SERIAL_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
BLE_SERIAL_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

async def find_device_for_service(service_uuid, timeout=5.0):
    print(f"Scanning for BLE devices (service={service_uuid}) for {timeout}s...")
    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        # metadata might contain uuids on some backends
        uuids = getattr(d, "metadata", {}).get("uuids") or []
        if uuids and any(u.lower() == service_uuid.lower() for u in uuids):
            print(f"Found device {d.name} [{d.address}] advertising service {service_uuid}")
            return d
    # fallback: try name hints (MIDI or Serial)
    for d in devices:
        if d.name and ("midi" in d.name.lower() or "serial" in d.name.lower()):
            print(f"Found device by name {d.name} [{d.address}]")
            return d
    return None

async def run_ble_logger(out_path, service_uuid, char_uuid, device_addr=None, scan_time=5.0, pair=False):
    async def handle_notification(sender, data: bytearray):
        ts = datetime.datetime.now().isoformat()
        hexdata = data.hex()
        line = f"{ts} {sender} {hexdata}\n"
        try:
            with out_path.open("a", encoding="utf-8") as f:
                f.write(line)
        except Exception as e:
            print(f"Failed to write BLE log to {out_path}: {e}", file=sys.stderr)
        print(line, end='')

    # Try to find device if address not provided
    if device_addr is None:
        dev = await find_device_for_service(service_uuid, timeout=scan_time)
        if dev is None:
            print(f"No device found advertising {service_uuid}.")
            return
        device_addr = dev.address

    # Loop and reconnect on disconnect/errors
    print(f"BLE logger loop starting for device {device_addr}...")
    while True:
        try:
            print(f"Connecting to BLE device {device_addr}...")
            async with BleakClient(device_addr) as client:
                try:
                    svcs = await client.get_services()
                    # Look for the requested characteristic
                    has_char = any(c.uuid.lower() == char_uuid.lower() for s in svcs for c in s.characteristics)
                except Exception:
                    svcs = None
                    has_char = False

                # Print connection state
                try:
                    connected = False
                    try:
                        is_connected_callable = callable(getattr(client, "is_connected", None))
                    except Exception:
                        is_connected_callable = False
                    if is_connected_callable:
                        connected = await client.is_connected()
                    else:
                        connected = client.is_connected
                    print(f"BLE client connected={connected}")
                except Exception:
                    print("BLE client connection state unknown")

                # Optionally attempt pairing/bonding if requested
                if pair:
                    try:
                        if hasattr(client, 'pair'):
                            print("Attempting to pair with device...")
                            paired = await client.pair()
                            print(f"Pair result: {paired}")
                        else:
                            print("Pairing not supported by this Bleak backend/client")
                    except Exception as e:
                        print(f"Pairing attempt failed: {e}")

                # Dump discovered services/characteristics for debugging (if available)
                if svcs is not None:
                    try:
                        svc_dump_lines = []
                        for s in svcs:
                            svc_dump_lines.append(f"Service {s.uuid}")
                            for c in s.characteristics:
                                props = getattr(c, 'properties', None)
                                svc_dump_lines.append(f"  Char {c.uuid} props={props}")
                        # append to BLE log for later inspection
                        try:
                            with out_path.open("a", encoding="utf-8") as f:
                                f.write(f"{datetime.datetime.now().isoformat()} SERVICES_DUMP\n")
                                for L in svc_dump_lines:
                                    f.write(L + "\n")
                        except Exception:
                            pass
                        # print to stdout for immediate visibility
                        if svc_dump_lines:
                            print("Discovered services/characteristics:")
                            for L in svc_dump_lines:
                                print(L)
                    except Exception as e:
                        print(f"Failed to dump services: {e}")
                if not has_char:
                    print(f"Warning: device did not advertise characteristic {char_uuid}. Attempting to subscribe anyway...")
                print(f"Subscribing to characteristic {char_uuid}...")
                await client.start_notify(char_uuid, handle_notification)
                print("BLE logger connected and subscribed. Waiting for notifications...")
                # Stay connected until disconnected or exception
                # Handle bleak versions where `is_connected` may be a coroutine or a boolean property
                try:
                    is_connected_callable = callable(getattr(client, "is_connected", None))
                except Exception:
                    is_connected_callable = False
                if is_connected_callable:
                    while await client.is_connected():
                        await asyncio.sleep(1.0)
                else:
                    while client.is_connected:
                        await asyncio.sleep(1.0)
                print("BLE client disconnected, will attempt reconnect...")
        except Exception as e:
            print(f"BLE logger error: {e}")
        # Wait a bit before reconnecting
        await asyncio.sleep(2.0)

serial_proc = None

def tail_serial(port, baud, out_path):
    # Prefer pyserial for robust non-interactive serial capture
    try:
        import serial as pyserial
    except ImportError:
        pyserial = None

    if pyserial is not None:
        print(f"Starting serial monitor via pyserial on {port} @ {baud}")
        # write a startup marker so we can see the monitor started even if no data
        try:
            with open(out_path, "a", buffering=1) as f:
                f.write(f"{datetime.datetime.now().isoformat()} SERIAL_MONITOR_STARTED pyserial on {port} @ {baud}\n")
        except Exception as e:
            print(f"Failed to write serial monitor start marker: {e}", file=sys.stderr)
        # Open the serial port and keep reading. If errors occur, retry with backoff.
        backoff = 0.5
        max_backoff = 30.0
        while True:
            try:
                with pyserial.Serial(port, baud, timeout=1) as ser, open(out_path, "a", buffering=1) as f:
                    # reset backoff on successful open
                    backoff = 0.5
                    while True:
                        try:
                            raw = ser.readline()
                        except Exception as e:
                            print(f"Serial read error: {e}")
                            break
                        if not raw:
                            # no data this iteration; loop again (read timeout=1s)
                            continue
                        try:
                            line = raw.decode(errors='replace')
                        except Exception:
                            line = str(raw)
                        ts = datetime.datetime.now().isoformat()
                        try:
                            f.write(f"{ts} {line}")
                        except Exception as e:
                            print(f"Failed to write serial log: {e}", file=sys.stderr)
                        print(line, end='')
            except Exception as e:
                print(f"Serial monitor error/open failed: {e}")
            # On error, sleep with exponential backoff then retry opening the port
            print(f"Retrying serial monitor in {backoff} seconds...")
            time.sleep(backoff)
            backoff = min(backoff * 2.0, max_backoff)
    else:
        # Fallback: prefer PlatformIO monitor if available, else cat the device
        pio_cmd = ["pio", "device", "monitor", "-p", port, "-b", str(baud)]
        if shutil.which("pio"):
            cmd = pio_cmd
        else:
            # Use direct exec of cat with argv list to avoid shell interpolation
            cmd = ["cat", port]

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
    parser.add_argument("--ble-protocol", choices=['auto', 'serial', 'midi'], default='auto',
                        help="Which BLE protocol to prefer: 'serial' for UART-over-BLE, 'midi' for BLE-MIDI, or 'auto'")
    parser.add_argument("--scan-time", default=5.0, type=float)
    parser.add_argument("--pair", action='store_true', help="Attempt to pair/bond with the device before subscribing (if supported)")
    args = parser.parse_args()

    outdir = Path(args.out_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    # Choose BLE protocol target: auto -> prefer BLE-Serial then BLE-MIDI
    ble_protocol = args.ble_protocol
    if ble_protocol == 'auto':
        # try serial first
        target_service = BLE_SERIAL_SERVICE
        target_char = BLE_SERIAL_TX
        ble_log = outdir / "ble_serial.log"
    elif ble_protocol == 'serial':
        target_service = BLE_SERIAL_SERVICE
        target_char = BLE_SERIAL_TX
        ble_log = outdir / "ble_serial.log"
    else:
        target_service = BLE_MIDI_SERVICE
        target_char = BLE_MIDI_CHAR
        ble_log = outdir / "ble_midi.log"
    serial_log = outdir / "serial.log"

    # Initialize/clear files
    ble_log.write_text("")
    serial_log.write_text("")

    # Register signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)

    try:
        # Start serial monitor in a thread/process (optional)
        import threading
        serial_thread = None
        if args.serial_port:
            serial_thread = threading.Thread(target=tail_serial, args=(args.serial_port, args.serial_baud, serial_log), daemon=True)
            serial_thread.start()

        # Run BLE logger in asyncio using asyncio.run for modern Python
        coro = run_ble_logger(ble_log, target_service, target_char, device_addr=args.ble_addr, scan_time=args.scan_time, pair=args.pair)
        try:
            asyncio.run(coro)
        except KeyboardInterrupt:
            print("Stopping logs (Ctrl-C received)")
    finally:
        # allow serial thread to terminate
        try:
            if serial_thread:
                serial_thread.join(timeout=1.0)
        except Exception as e:
            print(f"Error joining serial thread: {e}", file=sys.stderr)

if __name__ == '__main__':
    main()
