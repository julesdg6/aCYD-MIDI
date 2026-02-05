#!/usr/bin/env python3
import asyncio
from bleak import BleakScanner

async def main():
    print('Scanning for BLE devices for 8s...')
    devices = await BleakScanner.discover(timeout=8.0)
    if not devices:
        print('No devices found')
        return
    for d in devices:
        uuids = getattr(d, 'metadata', {}).get('uuids') or []
        print(f"{d.address}  name={d.name!r}  uuids={uuids}")

if __name__ == '__main__':
    asyncio.run(main())
