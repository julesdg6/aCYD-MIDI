# Firmware Manifests

This directory contains manifest files for the ESP Web Tools installer.

## What are these files?

Each `.json` file defines the firmware flashing configuration for a specific board variant. These manifests tell ESP Web Tools:

- Which firmware binary to use
- Where to place the bootloader, partition table, and firmware in flash memory
- What chip family to target (ESP32, ESP32-S3, etc.)

## Manifest Files

- **esp32-2432S028Rv2.json** - CYD 2.8" default build (UART2)
- **esp32-2432S028Rv2-uart0.json** - CYD 2.8" production build (UART0)
- **esp32-2432S028Rv2-uart2.json** - CYD 2.8" development build (UART2 explicit)
- **esp32-4832S035C.json** - 3.5" capacitive touch default build
- **esp32-4832S035C-uart0.json** - 3.5" capacitive touch production build
- **esp32-4832S035C-uart2.json** - 3.5" capacitive touch development build
- **esp32-4832S035R.json** - 3.5" resistive touch default build
- **esp32-4832S035R-uart0.json** - 3.5" resistive touch production build
- **esp32-4832S035R-uart2.json** - 3.5" resistive touch development build
- **esp32-4832S040R.json** - 4.0" resistive touch default build
- **esp32-4832S040R-uart0.json** - 4.0" resistive touch production build
- **esp32-4832S040R-uart2.json** - 4.0" resistive touch development build
- **esp32-headless-midi-master.json** - ESP32 headless build (no display)
- **esp32s3-headless.json** - ESP32-S3 headless build with native USB MIDI

## How They're Used

1. User visits the web installer at `https://julesdg6.github.io/aCYD-MIDI/flash.html`
2. User selects their board model
3. ESP Web Tools reads the corresponding manifest file
4. Firmware is flashed according to the manifest configuration

## Firmware Files

During releases, GitHub Actions automatically adds firmware binaries to this directory:

- `firmware-{board}.bin` - Main firmware for each board
- `bootloader.bin` - ESP32 bootloader
- `bootloader-s3.bin` - ESP32-S3 bootloader
- `partitions.bin` - ESP32 partition table
- `partitions-s3.bin` - ESP32-S3 partition table
- `boot_app0.bin` - Boot application

## Technical Details

Standard ESP32 flash memory layout:

| Offset | Offset (hex) | Contents |
|--------|--------------|----------|
| 4096   | 0x1000       | Bootloader |
| 32768  | 0x8000       | Partition Table |
| 57344  | 0xe000       | Boot App |
| 65536  | 0x10000      | Firmware |

For more information, see [docs/WEB_INSTALLER.md](../docs/WEB_INSTALLER.md).
