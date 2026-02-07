# Changelog

All notable changes to aCYD-MIDI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.3] - 2026-02-05

### Added
- ESP32-4832S040R (4" 480x320 resistive touch) board support with UART variants (#98)
- **ESP32-S3 headless USB MIDI dongle support with full USB MIDI implementation** (#98)
- **USB MIDI device support for headless builds** (PC/Mac/Android/iOS compatible)
- **ESP-NOW MIDI networking enabled for headless builds** (master role)
- ESP-NOW MIDI networking support between CYD devices (#56)
- Unique Bluetooth names for each device based on MAC address (#53)
- TB-303-style 4-row pattern display for TB3PO mode (#91)
- uClock step sequencer extension integration across all sequencer modules (#90)

### Changed
- Renamed existing board configurations for clarity (#98)
- Optimized TB3PO layout with expanded grid and repositioned controls (#93)
- Improved animated module screen updates and UI usability (#50)

### Fixed
- **GitHub release workflow now properly includes firmware binaries** (this release)
- MIDI clock timing issues with uClock library integration (#86)
- Splash screen initialization by setting up render layer before drawing (#96)
- UART0 build error in midi_transport.cpp (#71)
- Clock timing issues in sequencer (#63)
- DRAM overflow with comprehensive memory optimization (20-26KB freed) (#46, #44)
- Compilation issues with AsyncTCP dependencies via conditional compilation (#48)

### Removed
- Dead ISR code from modules using consumeReadySteps (Sequencer, Arpeggiator, Euclidean, Random Generator, Raga) (#100)

## [0.0.1] - 2026-01-19

## [0.1.0] - 2026-02-07

### Added
- Further SLINK behavior and UI improvements; update-per-PPQN visuals

### Changed
- Bumped project version to `0.1.0`
- Various concurrency and BLE/ESP-NOW robustness improvements

### Fixed
- Additional mode UI fixes and timing corrections

[0.1.0]: https://github.com/julesdg6/aCYD-MIDI/compare/v0.0.5...v0.1.0


## [0.0.5] - 2026-02-05

### Added
- Support and fixes for ESP32-4832S040R and 4" CYD builds

### Changed
- Multiple UI and concurrency fixes across SLINK, ESP-NOW, BLE and transport layers

### Fixed
- Resource leaks in `remote_display` when WiFi is unavailable
- Races involving shared `midiPacket` replaced with local packet buffers
- MIDI transport flags/timestamps made atomic to avoid cross-core races
- Fixed display redraws in several modes so visuals update per PPQN tick
- Many mode-specific fixes (Euclid triplet timing, grids layout, physics spawn logic, morph labels)

[0.0.5]: https://github.com/julesdg6/aCYD-MIDI/compare/v0.0.3...v0.0.5

### Added
- Initial public release
- Multi-mode touchscreen UI (keyboard, sequencer, arpeggiator, XY pad, generative music)
- BLE MIDI wireless output
- Optional hardware MIDI via UART (DIN-5 connector)
- Display scaling for different screen sizes
- Mode-based architecture
- Remote display via WiFi streaming
- Screenshot capture to SD card
- Consistent theming and scalable UI
- Support for ESP32-2432S028R (CYD) and ESP32-3248S035C/R boards

[0.0.3]: https://github.com/julesdg6/aCYD-MIDI/compare/v0.0.1...v0.0.3
[0.0.1]: https://github.com/julesdg6/aCYD-MIDI/releases/tag/v0.0.1
