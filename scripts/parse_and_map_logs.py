#!/usr/bin/env python3
import sys
from datetime import datetime

BLE_LOG = 'logs/ble_midi.log'
SERIAL_LOG = 'logs/serial.log'

# parse ISO timestamps like: 2026-02-04T01:38:47.246637
FMT = '%Y-%m-%dT%H:%M:%S.%f'

def parse_ble():
    entries = []
    if not os.path.exists(BLE_LOG):
        print(f"BLE log not found: {BLE_LOG}", file=sys.stderr)
        return entries
    with open(BLE_LOG, 'r') as f:
        for line in f:
            line=line.strip()
            if not line: continue
            parts = line.split(' ',2)
            ts = parts[0]
            rest = parts[2] if len(parts)>2 else ''
            # hex follows last colon
            if ':' in rest:
                hexpart = rest.split(':',1)[1].strip()
            else:
                hexpart = rest.strip()
            try:
                t = datetime.strptime(ts, FMT)
            except Exception:
                continue
            parts = hexpart.split()
            if parts:
                hexstr = parts[-1]
            else:
                hexstr = ''
            # normalize
            hexstr = hexstr.lower()
            # validate even-length hex and parse
            if len(hexstr) % 2 != 0:
                print(f"Odd-length hex string in BLE log: '{hexstr}'", file=sys.stderr)
                b = []
            else:
                try:
                    b = list(bytes.fromhex(hexstr))
                except ValueError as e:
                    print(f"Failed to parse hex '{hexstr}': {e}", file=sys.stderr)
                    b = []
            entries.append((t,b,line))
    return entries


def parse_serial():
    tb_entries = []
    if not os.path.exists(SERIAL_LOG):
        print(f"Serial log not found: {SERIAL_LOG}", file=sys.stderr)
        return tb_entries
    with open(SERIAL_LOG,'r') as f:
        for line in f:
            if '[TB3PO]' in line:
                # extract timestamp
                try:
                    ts = line.split()[0]
                    t = datetime.strptime(ts, FMT)
                except Exception:
                    continue
                # find tick=NNN
                idx = line.find('tick=')
                if idx!=-1:
                    rest = line[idx:]
                    try:
                        tok = rest.split()[0]
                        tick = int(tok.split('=')[1].strip(','))
                    except Exception:
                        continue
                    tb_entries.append((t,tick,line.strip()))
    return tb_entries


def find_nearest(serial_list, t):
    # binary search
    import bisect
    times = [s[0] for s in serial_list]
    pos = bisect.bisect_left(times, t)
    cand = []
    if pos < len(times): cand.append((abs((times[pos]-t).total_seconds()), pos))
    if pos-1 >= 0: cand.append((abs((times[pos-1]-t).total_seconds()), pos-1))
    if not cand: return None
    cand.sort()
    return serial_list[cand[0][1]]


def main():
    ble = parse_ble()
    serial = parse_serial()
    print('Parsed BLE entries:', len(ble))
    print('Parsed TB3PO serial entries:', len(serial))

    # extract clock (0xF8) and note-on (0x90) events from BLE
    clocks = [e for e in ble if any(b==0xf8 for b in e[1])]
    notes = [e for e in ble if any(b==0x90 for b in e[1])]
    print('BLE clock (F8) events:', len(clocks))
    print('BLE Note On (0x90) events:', len(notes))

    # find first clock and first TB3PO sequencer start-ish tick
    if clocks and serial:
        first_clock = clocks[0][0]
        first_tb = serial[0][0]
        delta = (first_tb - first_clock).total_seconds()
        print(f'First BLE clock at {first_clock}, first TB3PO entry at {first_tb}, delta={delta:.3f}s')

    # Map first 200 BLE clocks to nearest TB3PO ticks (if any)
    print('\nSample mapping of early BLE clock events to TB3PO tick entries:')
    for i,c in enumerate(clocks[:200]):
        nearest = find_nearest(serial, c[0])
        if nearest:
            d = (nearest[0]-c[0]).total_seconds()
            print(f'{i:3d} BLE {c[0].time()} -> TB3PO tick={nearest[1]} at {nearest[0].time()} (dt={d:+.3f}s)')
        else:
            print(f'{i:3d} BLE {c[0].time()} -> no TB3PO match')

    # Map first 100 BLE note-on events to TB3PO ticks
    print('\nSample mapping of BLE Note On events to TB3PO tick entries:')
    for i,n in enumerate(notes[:100]):
        nearest = find_nearest(serial, n[0])
        if nearest:
            d = (nearest[0]-n[0]).total_seconds()
            print(f'{i:3d} BLE {n[0].time()} -> TB3PO tick={nearest[1]} at {nearest[0].time()} (dt={d:+.3f}s) ; ble_line="{n[2]}"')
        else:
            print(f'{i:3d} BLE {n[0].time()} -> no TB3PO match ; ble_line="{n[2]}"')

if __name__=='__main__':
    main()
