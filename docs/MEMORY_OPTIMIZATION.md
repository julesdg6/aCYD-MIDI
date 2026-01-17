# ESP32 Memory Optimization Guide

## Overview

The ESP32-2432S028R has limited DRAM (internal RAM) which can cause build failures when the firmware grows too large. This document explains the memory constraints and how they're managed in aCYD-MIDI.

## ⚠️ CRITICAL: lv_conf.h Configuration Issue

**If you see DRAM overflow errors, first check that `platformio.ini` does NOT contain `-D LV_CONF_H`.**

This flag pre-defines the include guard and prevents `lv_conf.h` from being processed. Without this, all memory optimizations in `lv_conf.h` are ignored and LVGL uses its default (much larger) memory allocations.

**Correct `platformio.ini` build_flags should include:**
```ini
-D LV_CONF_PATH=\"lv_conf.h\"
-D LV_CONF_INCLUDE_SIMPLE=1
-I ${project_dir}/include
```

**Should NOT include:**
```ini
-D LV_CONF_H  # ❌ This breaks lv_conf.h processing!
```

## Build Error

When DRAM is exceeded, you'll see a linker error like:

```
/path/to/ld: .pio/build/esp32-2432S028Rv2/firmware.elf section `.dram0.bss' will not fit in region `dram0_0_seg'
/path/to/ld: DRAM segment data does not fit.
/path/to/ld: region `dram0_0_seg' overflowed by XXXX bytes
```

## Memory Layout

The ESP32's DRAM is shared among several consumers:

1. **LVGL heap** (`LV_MEM_SIZE`): Memory pool for UI objects
2. **Global variables**: All global/static variables in `.bss` and `.data` sections
3. **Stack**: Runtime stack for function calls
4. **System overhead**: FreeRTOS, WiFi, BLE stacks

## Current Configuration

In `include/lv_conf.h`:

```c
#define LV_MEM_SIZE (40 * 1024U)                    // 40 KB - LVGL heap
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (16 * 1024)   // 16 KB - Layer buffer
#define LV_DRAW_THREAD_STACK_SIZE (4 * 1024)        // 4 KB - Thread stack
```

**Total LVGL allocation**: ~60 KB

## Major Memory Consumers in aCYD-MIDI

### Global State Structures

- **EuclideanState**: ~140 bytes (4 voices × 32-step bool arrays)
- **GridsState**: ~80 bytes (3 × 16-byte pattern arrays)
- **TB3POState**: ~60 bytes
- **SlinkState**: Heap-allocated via `new` (not counted in DRAM limit)
- **RagaState**: ~80 bytes
- **MorphState**: ~40 bytes

### Static Data in Flash (PROGMEM)

These don't count against DRAM:
- `SPLASH_BITMAP`: 1360 bytes in flash
- `PATTERN_MAP`: 192 bytes in flash

## How the Fix Works

The recent build failure showed DRAM overflow by 1488 bytes. The fix:

1. **Reduced `LV_MEM_SIZE`** from 48 KB to 44 KB
2. **Freed 4096 bytes** of DRAM (more than 2.7× the overflow)
3. **Maintained functionality**: 44 KB is still sufficient for the UI

## Guidelines for Future Development

### DO:

1. **Use heap allocation** for large structures (like `SlinkState`)
   ```cpp
   MyLargeStruct *ptr = new (std::nothrow) MyLargeStruct();
   ```

2. **Use PROGMEM** for read-only data (images, lookup tables)
   ```cpp
   static const uint8_t PROGMEM myData[] = {...};
   ```

3. **Use local variables** instead of globals when possible

4. **Profile memory usage** after adding features:
   - Check build output for DRAM usage
   - Monitor `.bss` section size

### DON'T:

1. **Don't increase LVGL settings** without necessity:
   - `LV_MEM_SIZE`
   - `LV_DRAW_LAYER_SIMPLE_BUF_SIZE`
   - `LV_DRAW_THREAD_STACK_SIZE`

2. **Don't create large global arrays**:
   ```cpp
   // BAD: Uses DRAM
   uint8_t bigBuffer[10000];
   
   // GOOD: Uses heap
   uint8_t *bigBuffer = (uint8_t*)malloc(10000);
   
   // BETTER: Uses flash
   static const uint8_t PROGMEM bigBuffer[10000] = {...};
   ```

3. **Don't stack-allocate large arrays** in functions:
   ```cpp
   // BAD: May overflow stack
   void myFunction() {
       char buffer[5000];
       ...
   }
   
   // GOOD: Use heap
   void myFunction() {
       char *buffer = (char*)malloc(5000);
       ...
       free(buffer);
   }
   ```

## Monitoring Memory Usage

After building, check the memory report in PlatformIO output:

```
RAM:   [====      ]  XX.X% (used XXXXX bytes from XXXXXX bytes)
Flash: [====      ]  XX.X% (used XXXXXX bytes from XXXXXX bytes)
```

Aim to keep RAM usage below 80% to leave headroom for:
- Stack growth
- Heap fragmentation
- WiFi/BLE operations

## Alternative Solutions

If DRAM becomes critically constrained in the future:

1. **Enable PSRAM**: ESP32 can use external PSRAM for some allocations
2. **Partition schemes**: Use different flash partition layouts
3. **Lazy loading**: Only allocate mode structures when needed
4. **Compression**: Compress large read-only data

## Reference

- ESP32 Technical Reference: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
- LVGL Memory Management: https://docs.lvgl.io/master/overview/layer.html
- PlatformIO ESP32: https://docs.platformio.org/en/latest/platforms/espressif32.html
