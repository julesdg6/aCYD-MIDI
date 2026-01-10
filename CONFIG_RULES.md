# ⚠️ CONFIGURATION RULE - DO NOT VIOLATE

## ALL hardware configuration goes in `platformio.ini` - ONLY

### ✅ REQUIRED in platformio.ini build_flags:
- ILI9341_DRIVER
- TFT_RGB_ORDER
- TFT_WIDTH / TFT_HEIGHT
- TFT_MISO, TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL
- SPI_FREQUENCY, SPI_READ_FREQUENCY
- XPT2046 pin defines (IRQ, MOSI, MISO, CLK, CS)
- TFT_INVERSION_ON/OFF (if needed)

### ❌ DO NOT EDIT:
- `lib/TFT_eSPI/User_Setup.h` - Leave this file completely alone
- Any library User_Setup files - Ever

### WHY:
When library User_Setup.h is modified, it creates:
1. Configuration duplication (same defines in two places)
2. Build flag precedence conflicts
3. Confusion about where actual config lives
4. Makes it impossible to swap library versions cleanly

### CORRECT WORKFLOW:
1. All hardware config → `platformio.ini` build_flags (with `-D` prefix)
2. Library files → Read-only, no edits
3. When troubleshooting display issues, adjust `platformio.ini`, not library files

---
**If you see anyone editing `lib/TFT_eSPI/User_Setup.h` with hardware config, STOP and move it to `platformio.ini` instead.**
