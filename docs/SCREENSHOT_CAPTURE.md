# Screenshot Capture Implementation

## Overview

The screenshot capture system automatically captures all screens and states of the aCYD MIDI application, saving them as BMP files to the SD card along with a documentation file.

## Features

### Comprehensive Screen Capture

The system captures:

1. **Main Menu** - The 4×4 grid of mode tiles
2. **Settings Menu** - All scroll positions if content extends beyond one screen
3. **All Interactive Modes** - Each of the 16+ music generation modes
4. **Multi-Screen Modes** - All tabs/pages for modes with multiple screens:
   - **SLINK**: All 7 tabs (MAIN, TRIGGER, PITCH, CLOCK, SCALE, MOD, SETUP)
   - **WAAAVE**: All 3 pages (Transport, Controls 1-4, Controls 5-8)
   - **ENCODER_PANEL**: All 3 pages (if enabled)

### Automatic Documentation

A text file (`documentation.txt`) is generated with each capture session containing:
- Board name and firmware version
- List of all screenshots with descriptions
- Organized by mode and screen type

## Usage

### From Settings Menu
1. Navigate to Settings
2. Scroll to find "Screenshots" section
3. Tap "Capture All Screens" button
4. Wait for completion (progress shown in serial monitor)

### From Main Menu (Hidden Feature)
1. From main menu, hold the Settings cog icon for ~1.5 seconds
2. Release when screenshot capture starts
3. Wait for completion

## Technical Details

### Screenshot Timing

Each screen capture follows this sequence:
1. Switch to the target mode/screen
2. Request redraw
3. Process LVGL updates (10 cycles × 25ms = 250ms)
4. Wait for visual settling (5000ms by default)
5. Capture screenshot
6. Save to SD card

The 5-second delay ensures:
- All UI elements are fully rendered
- Animations have completed
- Dynamic content (e.g., waveforms) is visible

### File Naming Convention

Screenshots are named: `<board>-<version>_<label>_NNN.bmp`

Examples:
- `esp32-2432S028Rv2-0-1-6_menu_000.bmp`
- `esp32-2432S028Rv2-0-1-6_slink_main_015.bmp`
- `esp32-2432S028Rv2-0-1-6_settings_top_002.bmp`

The counter (`NNN`) increments across the entire capture session.

### Storage Requirements

Approximate storage needed for full capture:
- Display size: 320×240 = 76,800 pixels
- BMP format: 3 bytes per pixel + headers ≈ 230 KB per screenshot
- Total modes/screens: ~40-50 screenshots
- **Total space required: ~10-12 MB**

Ensure your SD card has sufficient free space before starting.

## Implementation Details

### Mode State Management

The capture system includes helper functions to manipulate mode internal state:

**Settings Mode:**
- `getSettingsScrollOffset()` - Get current scroll position
- `setSettingsScrollOffset(int)` - Set scroll position
- `getSettingsMaxScroll()` - Get maximum scroll amount

**WAAAVE Mode:**
- `getWaaavePage()` - Get current page number
- `setWaaavePage(int)` - Set page number
- `getWaaaveNumPages()` - Get total number of pages

**SLINK Mode:**
- Direct access to `slink_state_ptr->current_tab` for tab switching

**Encoder Panel Mode:**
- Direct access to `currentEncoderPage` for page switching

### Code Organization

Key files:
- `src/app/app_menu.cpp` - Main capture orchestration
- `src/screenshot.cpp` - BMP file writing and SD card management
- `include/screenshot.h` - Public API
- Module-specific `.cpp/.h` files - State accessor functions

### SD Card Format

The system uses the Arduino SD library with FAT filesystem support:
- Supported formats: FAT16, FAT32
- Maximum filename length: 96 characters (includes path)
- Directory structure: `/screenshots/` (created automatically)

## Troubleshooting

### SD Card Not Detected
- Check that card is properly inserted
- Verify card is formatted as FAT16 or FAT32
- Try a different SD card
- Check serial monitor for detailed error messages

### Screenshots Missing or Incomplete
- Ensure SD card has sufficient free space (12+ MB recommended)
- Check that card is not write-protected
- Verify SD_CS_PIN is correct for your board (default: pin 5)
- Some boards may not have SD card support

### Documentation File Not Generated
- Check that all screenshots completed successfully
- Verify SD card is not full
- Look for write errors in serial monitor

## Serial Monitor Output

During capture, you'll see:
```
Capturing all screens to SD...
Taking screenshot...
SD Card detected: SDHC/SDXC, 7936MB
Display size: 320x240
Snapshot captured, saving to BMP...
Screenshot saved to /screenshots/esp32-2432S028Rv2-0-1-6_menu_000.bmp
...
Documentation saved to /screenshots/esp32-2432S028Rv2-0-1-6_documentation.txt
Screen capture complete. Captured 42 screenshots.
```

## Future Enhancements

Potential improvements:
- Adjustable delay between captures
- Selective mode capture (choose which modes to include)
- PNG format support for smaller file sizes
- Real-time clock (RTC) timestamps instead of millis() uptime
- Progress indicator on display during capture
