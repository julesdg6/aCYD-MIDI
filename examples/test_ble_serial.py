#!/usr/bin/env python3
"""
BLE Serial Test Client for aCYD-MIDI

This script demonstrates how to connect to aCYD-MIDI's BLE Serial service
and send/receive commands.

Requirements:
    pip install bleak

Usage:
    python test_ble_serial.py
"""

import asyncio
import sys
from bleak import BleakClient, BleakScanner

# Nordic UART Service UUIDs used by aCYD-MIDI
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Device -> Client (Notify)
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Client -> Device (Write)


async def scan_for_acyd():
    """Scan for aCYD-MIDI devices."""
    print("Scanning for aCYD-MIDI devices...")
    devices = await BleakScanner.discover(timeout=5.0)
    
    acyd_devices = [d for d in devices if d.name and "aCYD MIDI" in d.name]
    
    if not acyd_devices:
        print("No aCYD-MIDI devices found!")
        print("\nMake sure:")
        print("  1. Device is powered on")
        print("  2. BLE is enabled on device")
        print("  3. You're close to the device")
        return None
    
    if len(acyd_devices) == 1:
        device = acyd_devices[0]
        print(f"Found: {device.name} ({device.address})")
        return device
    
    # Multiple devices found, let user choose
    print(f"\nFound {len(acyd_devices)} aCYD-MIDI devices:")
    for i, device in enumerate(acyd_devices):
        print(f"  {i+1}. {device.name} ({device.address})")
    
    while True:
        try:
            choice = int(input("\nSelect device (1-{}): ".format(len(acyd_devices))))
            if 1 <= choice <= len(acyd_devices):
                return acyd_devices[choice - 1]
        except (ValueError, KeyboardInterrupt):
            return None


async def send_command(client, command):
    """Send a command to the device."""
    if not command.endswith('\n'):
        command += '\n'
    
    await client.write_gatt_char(RX_CHAR_UUID, command.encode('utf-8'))
    print(f"→ {command.rstrip()}")


async def interactive_session(client):
    """Run an interactive command session."""
    print("\n" + "="*60)
    print("Interactive BLE Serial Session")
    print("="*60)
    print("Type commands and press Enter. Type 'quit' to exit.")
    print("Try: status, help, get bpm, set bpm 140, list modes")
    print("="*60 + "\n")
    
    # Set up notification handler
    received_data = []
    
    def notification_handler(sender, data):
        """Handle incoming notifications."""
        try:
            text = data.decode('utf-8', errors='replace')
            received_data.append(text)
            print(f"← {text.rstrip()}")
        except Exception as e:
            print(f"Error decoding: {e}")
    
    # Subscribe to notifications
    await client.start_notify(TX_CHAR_UUID, notification_handler)
    
    # Interactive loop
    try:
        while True:
            try:
                command = input("> ")
                
                if command.lower() in ['quit', 'exit', 'q']:
                    break
                
                if command.strip():
                    await send_command(client, command)
                    # Give device time to respond
                    await asyncio.sleep(0.2)
                    
            except KeyboardInterrupt:
                print("\nExiting...")
                break
            except Exception as e:
                print(f"Error: {e}")
    
    finally:
        # Clean up
        await client.stop_notify(TX_CHAR_UUID)


async def run_automated_tests(client):
    """Run automated test commands."""
    print("\n" + "="*60)
    print("Running Automated Tests")
    print("="*60)
    
    # Set up notification handler
    responses = []
    
    def notification_handler(sender, data):
        try:
            text = data.decode('utf-8', errors='replace')
            responses.append(text)
            print(f"← {text.rstrip()}")
        except Exception as e:
            print(f"Error: {e}")
    
    await client.start_notify(TX_CHAR_UUID, notification_handler)
    
    # Test commands
    test_commands = [
        "version",
        "status",
        "get bpm",
        "get mode",
        "help",
    ]
    
    for cmd in test_commands:
        responses.clear()
        print(f"\n--- Test: {cmd} ---")
        await send_command(client, cmd)
        await asyncio.sleep(0.5)
        
        if not responses:
            print("⚠ No response received")
        else:
            print("✓ Response received")
    
    await client.stop_notify(TX_CHAR_UUID)
    
    print("\n" + "="*60)
    print("Automated tests complete")
    print("="*60)


async def main():
    """Main entry point."""
    print("aCYD-MIDI BLE Serial Test Client")
    print("="*60)
    
    # Scan for device
    device = await scan_for_acyd()
    if not device:
        sys.exit(1)
    
    print(f"\nConnecting to {device.name}...")
    
    try:
        async with BleakClient(device.address) as client:
            print("✓ Connected!")
            
            # Verify BLE Serial service exists
            services = await client.get_services()
            if SERVICE_UUID.lower() not in [str(s.uuid).lower() for s in services]:
                print("\n⚠ BLE Serial service not found!")
                print("Make sure the firmware was built with ENABLE_BLE_SERIAL=1")
                return
            
            print("✓ BLE Serial service found")
            
            # Ask user for mode
            print("\nSelect mode:")
            print("  1. Interactive session")
            print("  2. Automated tests")
            
            try:
                choice = input("Choice (1-2): ").strip()
            except KeyboardInterrupt:
                print("\nCancelled")
                return
            
            if choice == "1":
                await interactive_session(client)
            elif choice == "2":
                await run_automated_tests(client)
            else:
                print("Invalid choice")
            
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nExiting...")
