#!/usr/bin/env python3
import asyncio
import datetime
from bleak import BleakScanner, BleakClient

BLE_MIDI_CHAR = "7772e5db-3868-4112-a1a9-f2669d106bf3"
TARGET_NAME_FRAGMENT = "aCYD MIDI"

async def main():
    print('Scanning for devices (30s)...')
    devices = await BleakScanner.discover(timeout=30.0)
    target = None
    for d in devices:
        name = d.name or ''
        if TARGET_NAME_FRAGMENT.lower() in name.lower():
            target = d
            break
    if not target:
        print('No device found by name fragment, listing discovered devices:')
        for d in devices:
            print(d.address, d.name, getattr(d,'metadata',{}).get('uuids'))
        return
    print(f'Found device: {target.name} [{target.address}]')

    async with BleakClient(target.address) as client:
        try:
            is_conn = await client.is_connected()
        except Exception:
            is_conn = client.is_connected
        print('Connected:', is_conn)
        try:
            # Support multiple bleak versions: prefer get_services() if present,
            # otherwise fall back to the `services` attribute.
            get_svcs = getattr(client, 'get_services', None)
            if callable(get_svcs):
                svcs = await client.get_services()
            else:
                svcs = getattr(client, 'services', None)
            if svcs:
                print('Services:')
                for s in svcs:
                    suuid = getattr(s, 'uuid', str(s))
                    print(' -', suuid)
                    for c in getattr(s, 'characteristics', []):
                        cuuid = getattr(c, 'uuid', str(c))
                        print('    char', cuuid, getattr(c, 'properties', None))
            else:
                print('No services found')
        except Exception as e:
            print('Failed to list services:', e)

        def handler(sender, data: bytearray):
            ts = datetime.datetime.now().isoformat()
            print(f"{ts} NOTIFY {sender} {data.hex()} {list(data)}")

        try:
            print('Subscribing to MIDI characteristic', BLE_MIDI_CHAR)
            await client.start_notify(BLE_MIDI_CHAR, handler)
        except Exception as e:
            print('Failed to start notify:', e)
            return
        print('Listening for notifications. Trigger notes now; press Ctrl-C to stop.')
        try:
            while True:
                try:
                    connected = await client.is_connected()
                except Exception:
                    connected = client.is_connected
                if not connected:
                    print('Client disconnected')
                    break
                await asyncio.sleep(1.0)
        except KeyboardInterrupt:
            print('Interrupted by user')
        finally:
            try:
                await client.stop_notify(BLE_MIDI_CHAR)
            except Exception:
                pass

if __name__ == '__main__':
    asyncio.run(main())
