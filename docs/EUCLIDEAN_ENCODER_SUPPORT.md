# Euclidean Mode - M5Stack 8Encoder Support

## Overview

The Euclidean Rhythm Sequencer mode now supports the M5Stack 8Encoder expansion unit for hardware control of rhythm patterns. This feature is enabled when the `ENABLE_M5_8ENCODER` flag is defined in the build configuration.

## Encoder Mapping

The 8 encoders are organized into **4 pairs**, with each pair controlling one of the first four voices:

| Encoder Pair | Voice | Encoder 1 (Even) | Encoder 2 (Odd) |
|--------------|-------|------------------|-----------------|
| 0-1          | V1    | Steps (1-32)     | Hits (0-steps)  |
| 2-3          | V2    | Steps (1-32)     | Hits (0-steps)  |
| 4-5          | V3    | Steps (1-32)     | Hits (0-steps)  |
| 6-7          | V4    | Steps (1-32)     | Hits (0-steps)  |

### Voice Configuration

The mode supports **8 total voices**, but only the first 4 are controllable via hardware encoders:

- **Voices 1-4**: Controlled by 4 encoder pairs (hardware)
- **Voices 5-8**: Display-only with preset patterns

### Parameters

**Steps (Even Encoders: 0, 2, 4, 6)**
- Range: 1-32 steps
- Controls the total length of the Euclidean pattern
- Rotation adjusts the position of the first step

**Hits (Odd Encoders: 1, 3, 5, 7)**
- Range: 0 to current step count
- Controls how many hits are distributed across the pattern
- Uses Bjorklund algorithm for optimal distribution

## Default Voice Settings

| Voice | Steps | Hits | Rotation | MIDI Note | Color        |
|-------|-------|------|----------|-----------|--------------|
| V1    | 16    | 4    | 0        | 36 (C2)   | Red          |
| V2    | 16    | 4    | 2        | 38 (D2)   | Yellow       |
| V3    | 16    | 8    | 0        | 42 (F#2)  | Green        |
| V4    | 16    | 5    | 1        | 39 (D#2)  | Cyan         |
| V5    | 16    | 3    | 0        | 45 (A2)   | Blue         |
| V6    | 16    | 6    | 1        | 47 (B2)   | Orange       |
| V7    | 16    | 7    | 3        | 49 (C#3)  | Purple       |
| V8    | 16    | 2    | 2        | 51 (D#3)  | Pink         |

## Usage

1. **Connect Hardware**: Attach the M5Stack 8Encoder unit to your ESP32 via I2C
2. **Enable in Build**: Uncomment `ENABLE_M5_8ENCODER` in `platformio.ini`
3. **Upload Firmware**: Build and upload with encoder support enabled
4. **Enter Mode**: Navigate to the Euclidean mode from the main menu
5. **Adjust Parameters**:
   - Turn even encoders (0,2,4,6) to change pattern length
   - Turn odd encoders (1,3,5,7) to change hit density
   - Patterns regenerate in real-time as you adjust

## Visual Feedback

- **Circular Display**: Shows all 8 voices as concentric rings
- **Active Steps**: Highlighted in voice color
- **Current Position**: White indicator with sweeping line
- **Voice Labels**: Display format `V1: hits/steps` (e.g., "4/16")

## Implementation Notes

- Encoder polling happens every loop cycle in `handleEuclideanMode()`
- Pattern regeneration uses the Bjorklund Euclidean rhythm algorithm
- Changes trigger immediate UI refresh via `requestRedraw()`
- Encoder deltas are reset after processing to prevent double-counting
- Hardware detection is automatic via `encoder8.isConnected()`

## Enabling Encoder Support

In `platformio.ini`, find the `build_flags` section and uncomment:

```ini
# Uncomment the next line to enable M5Stack 8Encoder expansion unit
-D ENABLE_M5_8ENCODER=1
```

Then rebuild and upload the firmware.

## Troubleshooting

**Encoders not responding:**
- Verify I2C connection (default address: 0x41)
- Check that `ENABLE_M5_8ENCODER` is defined at compile time
- Ensure encoder unit has power

**Pattern not updating:**
- Encoder hardware must be connected before entering mode
- Check serial output for encoder connection status

**UI crowded with 8 voices:**
- UI automatically scales for different display sizes
- Spacing is optimized for 320x240 default resolution
- Larger displays will have better visibility
