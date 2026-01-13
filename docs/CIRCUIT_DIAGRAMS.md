# Hardware MIDI Circuit Diagrams

## MIDI OUT Circuit (Standard Implementation)

### Breadboard Layout

```
                                   DIN-5 Female Connector
                                   (Looking at solder side)
                                         3     1
                                           ___
                                          /   \
                                         |  5  |
                                          \___/
                                         4     2

ESP32 Board                 6N138 Optocoupler              MIDI Connector
-----------                 ------------------             --------------

GPIO1/GPIO17 ----[ 220Ω ]---- Pin 2 (Anode)
    (TX)                      Pin 3 (Cathode) ----+-------- DIN-5 Pin 2 (GND)
                              Pin 8 (VCC) --------|-------- +5V
                              Pin 6 (Emitter) ----|
GND -------------------------+--------------------+
                              Pin 5 (Collector) ----------- DIN-5 Pin 5
                                                  +--------- DIN-5 Pin 4
```

### Detailed Schematic

```
         ESP32                    6N138                    MIDI OUT
       ┌────────┐              ┌─────────┐              ┌──────────┐
       │        │              │    8    │───── +5V     │          │
       │  TX    │──┬──220Ω──┬─│2      5 │──────────────│  Pin 5   │
       │GPIO1/17│  │         │ │         │              │          │
       │        │  │         │ │    6    │──────────────│  Pin 4   │
       └────────┘  │         │ │         │              │          │
                   │         │ │3        │              │          │
       ┌────────┐  │         └─│         │              │  Pin 2   │
       │  GND   │──┴───────────│─────────│──────────────│  (GND)   │
       └────────┘              └─────────┘              └──────────┘
       ┌────────┐
       │  +5V   │──────────────── +5V
       └────────┘
```

## Component Identification

### 6N138 Optocoupler (DIP-8 Package)

```
    ┌─────┐
  1 │●    │ 8  ← Pin 8: VCC (+5V)
  2 │     │ 7
  3 │6N138│ 6  ← Pin 6: Emitter
  4 │     │ 5  ← Pin 5: Collector
    └─────┘
     ▲   ▲
    Pin  Pin
     2    3
```

**Pin Functions:**
- Pin 1: Not used
- Pin 2: Anode (from ESP32 TX via 220Ω)
- Pin 3: Cathode (to GND)
- Pin 4: Not used
- Pin 5: Collector (to MIDI OUT Pin 5)
- Pin 6: Emitter (to MIDI OUT Pin 4)
- Pin 7: Not used
- Pin 8: VCC (+5V power)

### DIN-5 Connector (Female, Solder Side View)

```
      3     1
        ___
       /   \
      |  5  |
       \___/
      4     2
```

**Pin Functions:**
- Pin 1: Not connected
- Pin 2: Cable shield / Ground
- Pin 3: Not connected
- Pin 4: MIDI signal (from 6N138 Pin 6)
- Pin 5: MIDI signal (from 6N138 Pin 5)

## UART0 Configuration (Serial Breakout)

### CYD Board Pinout

```
CYD Serial Breakout Header:
┌─────┬─────┬─────┬─────┐
│ GND │ TX  │ RX  │ 5V  │
│     │GPIO1│GPIO3│     │
└─────┴─────┴─────┴─────┘
   │     │     │     │
   │     │     │     └─── To 6N138 Pin 8 (VCC)
   │     │     └───────── MIDI IN (optional)
   │     └─────────────── MIDI OUT (via 220Ω to 6N138 Pin 2)
   └───────────────────── To 6N138 Pin 3 and DIN-5 Pin 2
```

## UART2 Configuration (Expansion GPIOs)

### GPIO Pinout

```
CYD Expansion Header (varies by board):
┌─────┬─────┬─────┬─────┬─────┬─────┐
│ ... │GPIO │GPIO │ ... │ GND │ 5V  │
│     │ 16  │ 17  │     │     │     │
│     │ RX2 │ TX2 │     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┘
         │     │             │     │
         │     │             │     └─── To 6N138 Pin 8
         │     │             └───────── To 6N138 Pin 3
         │     └─────────────────────── MIDI OUT (via 220Ω)
         └───────────────────────────── MIDI IN (optional)
```

## Complete Breadboard Wiring (UART0)

```
┌───────────────────────────────────────────────────────────┐
│                     BREADBOARD                            │
│                                                           │
│  CYD Serial Header        6N138              DIN-5       │
│  ┌──┬──┬──┬──┐        ┌────────┐           Connector    │
│  │G │TX│RX│5V│        │  8   5 │              ┌─┐       │
│  │N │  │  │  │        │        │              │5│       │
│  │D │  │  │  │        │6N138   │          ┌───┴─┴───┐   │
│  └┬─┴┬─┴──┴┬─┘        │  3   2 │          │  4   2  │   │
│   │  │     │          └┬──┬─┬─┬┘          └─────────┘   │
│   │  │     │           │  │ │ │                          │
│   │  │  ┌──┴───────────┘  │ │ └───[ 220Ω ]───┐          │
│   │  │  │  ┌──────────────┘ │                 │          │
│   └──┼──┼──┼────────────────┴─────────────────┼──────┐   │
│      │  │  └──────────────────────────────────┼────┐ │   │
│      │  └─────────────────────────────────────┼──┐ │ │   │
│      └────────────────────────────────────────┼┐ │ │ │   │
│                                               ││ │ │ │   │
└───────────────────────────────────────────────┘│ │ │ │   │
                                                 │ │ │ │   │
                                    To Pin 4 ────┘ │ │ │   │
                                    To Pin 5 ──────┘ │ │   │
                                    To Pin 2 ────────┘ │   │
                                    +5V ───────────────┘   │
                                    GND ───────────────────┘
```

## MIDI IN Circuit (Optional - Future Implementation)

### Circuit Diagram

```
MIDI IN                 6N138                    ESP32
Connector               Optocoupler              GPIO
────────                ───────────              ─────

Pin 5 ────[ 220Ω ]──── Pin 2 (Anode)
Pin 4 ──────────────── Pin 3 (Cathode)
                       Pin 8 (VCC) ───── +5V
                       Pin 6 (Emitter) ── GND
                       Pin 5 (Collector) ─┬─[ 4.7kΩ ]─ +5V
                                          │
                                          └──────────── GPIO3/GPIO16
                                                        (RX)
```

### Component List (MIDI IN)

```
1x  6N138 Optocoupler
1x  220Ω Resistor (current limiting)
1x  4.7kΩ Resistor (pull-up)
1x  Female DIN-5 Connector
```

## Testing Points

### Voltage Measurements (MIDI OUT)

```
Measurement Points (with MIDI data transmitting):

Point A (GPIO TX pin):        3.3V (idle) → 0V/3.3V (data)
Point B (6N138 Pin 2):         ~3.1V (through 220Ω)
Point C (6N138 Pin 8):         +5V (constant)
Point D (6N138 Pin 3):         0V (ground)
Point E (DIN-5 Pin 5):         ~5V (idle) → pulsing (data)
Point F (DIN-5 Pin 4):         0V/5V (inverse of Pin 5)
Point G (DIN-5 Pin 2):         0V (ground)
```

### Signal Verification

Use a logic analyzer or oscilloscope to verify:

**TX Pin (GPIO1 or GPIO17):**
- Idle: HIGH (3.3V)
- Data: Square wave @ 31,250 baud (32μs per bit)
- Logic: UART 8-N-1 format

**MIDI OUT (DIN-5 Pins 4-5):**
- Idle: Current loop ~5mA
- Data: Current pulses (ON/OFF)
- Standard MIDI current loop

## Power Requirements

**Total Current Draw (MIDI OUT only):**
- 6N138 quiescent: ~0.5mA
- MIDI signal current: ~5mA (when active)
- **Total: ~6mA @ 5V**

**Power Supply Options:**
- CYD 5V pin (from USB)
- External 5V regulator
- USB power bank

## Safety Notes

⚠️ **Important:**
- MIDI uses a current loop, not voltage levels
- Never connect MIDI directly to ESP32 pins without optocoupler
- 6N138 provides electrical isolation
- Always verify polarity before powering on
- Use ESD protection when handling components

## Common Build Mistakes

### ❌ Wrong: Direct Connection (NO OPTOCOUPLER)
```
ESP32 TX ──────────────── DIN-5 Pin 5  ← DANGEROUS!
ESP32 GND ─────────────── DIN-5 Pin 2
```
**Why:** No electrical isolation, wrong voltage levels

### ✅ Correct: With Optocoupler
```
ESP32 TX ───[ 220Ω ]───┬─ 6N138 ─┬─ DIN-5 Pin 5
ESP32 GND ─────────────┴─────────┴─ DIN-5 Pin 2
```
**Why:** Proper isolation and current limiting

## Troubleshooting LED Indicator (Optional)

Add visual feedback for MIDI transmission:

```
ESP32 TX ───[ 220Ω ]───┬─ 6N138 ─┬─ DIN-5 Pin 5
                       │         │
                       └─[LED]───┘─ DIN-5 Pin 4
                          220Ω
                          
LED blinks when MIDI data is sent
```

## References

- **MIDI 1.0 Spec:** https://www.midi.org/specifications
- **6N138 Datasheet:** https://www.onsemi.com/pdf/datasheet/6n138-d.pdf
- **ESP32 Pinout:** https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

## License

Circuit diagrams are part of the aCYD MIDI project.
Open source - see LICENSE file.
