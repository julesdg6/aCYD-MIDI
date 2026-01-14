# Changelog

All notable changes to aCYD-MIDI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-14

### Initial Release

#### Interactive Modes (16 total)

- **KEYS** - Virtual piano keyboard with scale and key controls
- **BEATS** - 16-step sequencer with 4 tracks and tempo control
- **ZEN** - Ambient bouncing ball mode for generative music
- **DROP** - Physics-based ball drop with customizable platforms
- **RNG** - Random music generator for creative exploration
- **XY PAD** - Touch-controlled X/Y pad for real-time parameter control
- **ARP** - Arpeggiator with chord-based patterns
- **GRID** - Grid piano with 4ths layout for unique playing style
- **CHORD** - Auto-chord mode with diatonic chord progressions
- **LFO** - Low-frequency oscillator for modulation effects
- **TB3PO** - TB-3 inspired generator with density, accent, and regen controls for rapid phrase creation
- **GRIDS** - Multi-layer arpeggiator blending kick/snare/hat densities with touch-based XY tweaking
- **RAGA** - Indian raga explorer for scale selection, drone toggles, and tempo-aware dronescapes
- **EUCLID** - Euclidean rhythm sequencer with per-voice patterns, BPM, and triplet toggles
- **MORPH** - Gesture morphing surface that blends stored slots, X/Y sliders, and rec/play controls
- **SLINK** - Dual-wave generative engine with 16-band triggering, tabbed parameter panels, and MIDI scale/clock modulation for evolving textures

#### Core Features

##### MIDI Output
- **Bluetooth MIDI** - Wireless BLE MIDI connection to DAWs and music software
- **Hardware MIDI** - Dual BLE + DIN-5 output with selectable UART0 (production) or UART2 (development)
  - Supports simultaneous BLE and hardware MIDI output
  - Configurable UART selection for debugging or production use
  - Conditional debug output that respects UART configuration
  - Complete wiring and circuit diagrams included

##### Display & Interface
- **Touchscreen Interface** - Intuitive visual controls optimized for ESP32-2432S028R (CYD)
- **Display Autoscaling** - Automatic UI scaling for different screen sizes and resolutions
  - Auto-detects display dimensions at startup
  - Scales all UI elements proportionally
  - Maintains consistent layout across hardware variants
- **Main Menu** - 4×4 grid with darker rainbow palette and clear button labels
- **Splash Screen** - Boot artwork with version display and WiFi status
- **Visual Feedback** - Responsive graphics with consistent color theme

##### Advanced Features
- **Remote Display** - WebSocket-based display streaming to web browser
  - 20 FPS frame streaming
  - Auto-reconnect capability
  - Status indicator for monitoring
  - WiFi configuration template for easy setup
- **Screenshot Capture** - Save display snapshots to SD card
  - LVGL snapshot to BMP conversion
  - Incremental filenames (screen000.bmp, screen001.bmp, etc.)
  - Padding-safe row handling
  - Batch mode for capturing all modes

#### Technical Features

##### Build System
- **PlatformIO Support** - Modern build system with multiple board configurations
  - esp32-2432S028Rv2 (default)
  - esp32-2432S028Rv2-uart2 (development with GPIO16/17)
  - esp32-2432S028Rv2-uart0 (production with GPIO1/3)
- **GitHub Actions CI/CD** - Automated firmware builds for all configurations
  - Automatic release creation on tag push
  - Build artifacts for all board variants

##### Architecture
- **Mode-based Architecture** - Clean separation of interactive modes
- **Dual MIDI Output** - All messages sent to both BLE and hardware MIDI
- **Music Theory Utilities** - Scale quantization, chord generation, and MIDI helpers
- **Touch State Management** - Robust touch handling with press/release events

##### Graphics
- **LVGL v9.2.2** - Modern graphics library via esp32_smartdisplay
- **TFT_eSPI Compatibility** - Compatibility layer for easy migration
- **Consistent Theme** - Professional color scheme throughout UI
- **Scaling Macros** - SCALE_X() and SCALE_Y() for resolution independence

#### Documentation

- Comprehensive README with setup instructions
- Hardware MIDI wiring guides and circuit diagrams
- Remote display setup documentation
- Configuration rules for build system
- Implementation details for complex modes (SLINK)
- Module deep-dives and technical notes

#### Hardware Support

- **Primary Board**: ESP32-2432S028R "Cheap Yellow Display" (CYD)
- **Display**: 320x240 TFT with ILI9341 driver
- **Touch**: XPT2046 resistive touchscreen
- **Optional**: MicroSD card for screenshot capture
- **Optional**: Hardware MIDI circuit (DIN-5 connector)

### Notes

This is the first stable release of aCYD-MIDI, providing a complete Bluetooth MIDI controller with 16 interactive modes, hardware MIDI output, remote display capabilities, and comprehensive documentation.

[1.0.0]: https://github.com/julesdg6/aCYD-MIDI/releases/tag/v1.0.0
