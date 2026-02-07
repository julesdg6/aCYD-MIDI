# Changelog

All notable changes to aCYD-MIDI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-06

### First Public Release

This is the initial public release of aCYD-MIDI, a comprehensive touchscreen Bluetooth MIDI controller for ESP32-based displays.

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

#### MIDI Output

- **Bluetooth MIDI** - Wireless BLE MIDI connection to DAWs and music software
- **Hardware MIDI** - Dual BLE + DIN-5 output with selectable UART0 (production) or UART2 (development)
  - Supports simultaneous BLE and hardware MIDI output
  - Configurable UART selection for debugging or production use
  - Conditional debug output that respects UART configuration
  - Complete wiring and circuit diagrams included
- **USB MIDI** - Native USB MIDI device support for ESP32-S3 headless builds (PC/Mac/Android/iOS compatible, no drivers needed)
- **ESP-NOW MIDI** - Low-latency wireless MIDI networking between multiple CYD devices without pairing (<10ms latency, auto-discovery, MIDI clock sync)

#### Display & Interface

- **Touchscreen Interface** - Intuitive visual controls optimized for ESP32 displays
- **Display Autoscaling** - Automatic UI scaling for different screen sizes and resolutions
  - Auto-detects display dimensions at startup
  - Scales all UI elements proportionally
  - Maintains consistent layout across hardware variants
- **Main Menu** - 4×4 grid with darker rainbow palette and clear button labels
- **Splash Screen** - Boot artwork with version display and WiFi status
- **Visual Feedback** - Responsive graphics with consistent color theme
- **Remote Display** - WebSocket viewer with 20 FPS frames, auto-reconnect, and status indicator for browser monitoring (optional, disabled by default)

#### Hardware Support

- **ESP32-2432S028R (CYD)** - 2.8" 320x240 display
- **ESP32-4832S035C** - 3.5" 480x320 display with capacitive touch
- **ESP32-4832S035R** - 3.5" 480x320 display with resistive touch
- **ESP32-4832S040R** - 4.0" 480x320 display with resistive touch
- **ESP32-S3 Dongles** - Headless USB MIDI mode (no display) with native USB MIDI support
- Multiple UART configurations (UART0 for production, UART2 for development)

#### Additional Features

- **Screenshot Capture** - LVGL snapshot to BMP writer with incremental filenames and SD card support
- **Web Installer** - Browser-based firmware flashing using ESP Web Tools (no software installation required)
- **Web Debug Console** - Browser-based MIDI/Serial debugging tool with TB-303-style interface, BLE MIDI and Web Serial support, real-time logging, and event export
- **MIDI Clock Sync** - uClock step sequencer extension integration across all sequencer modules
- **Unique Bluetooth Names** - Each device gets a unique name based on MAC address
- **Memory Optimizations** - Comprehensive DRAM optimization (20-26KB freed)
- **Build System** - PlatformIO-based with multiple board configurations and GitHub Actions CI/CD

#### Documentation

- Comprehensive README with installation instructions
- Hardware MIDI setup guides with circuit diagrams
- Contributing guidelines and code of conduct
- 39 detailed documentation files covering features, implementation, and troubleshooting

#### Technical Highlights

- Mode-based architecture for easy extensibility
- Conditional compilation for optional features (WiFi, Remote Display, ESP-NOW)
- RTOS-based concurrency with thread-safe MIDI transport
- Atomic flags and local packet buffers to avoid cross-core races
- Performance optimizations for smooth UI updates

[0.1.0]: https://github.com/julesdg6/aCYD-MIDI/releases/tag/v0.1.0
