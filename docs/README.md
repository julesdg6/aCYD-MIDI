# aCYD MIDI Documentation

This folder contains supporting documentation that expands on the features summarized in the top-level `README.md`.

## Quick Links

- [Main README](../README.md) - Project overview, installation, and quick start
- [CHANGELOG](../CHANGELOG.md) - Version history and release notes
- [CONTRIBUTING](../CONTRIBUTING.md) - How to contribute to the project
- [LICENSE](../LICENSE) - MIT License

## Feature Documentation

### Hardware & Configuration
- [**CONFIG_RULES.md**](CONFIG_RULES.md) - PlatformIO configuration conventions and build guidelines
- [**CIRCUIT_DIAGRAMS.md**](CIRCUIT_DIAGRAMS.md) - Wiring diagrams for hardware MIDI and sensor pinouts
- [**HARDWARE_MIDI.md**](HARDWARE_MIDI.md) / [**HARDWARE_MIDI_CONFIG.md**](HARDWARE_MIDI_CONFIG.md) - Hardware MIDI DIN-5 output setup, UART selection, and debugging

### Wireless & Networking
- [**ESP_NOW_MIDI.md**](ESP_NOW_MIDI.md) - ESP-NOW wireless MIDI networking between devices (disabled by default)
- [**REMOTE_DISPLAY.md**](REMOTE_DISPLAY.md) - WiFi remote display viewer setup (disabled by default)
- [**HEADLESS_USB_MIDI.md**](HEADLESS_USB_MIDI.md) - ESP32-S3 headless USB MIDI dongle build guide

### Mode Implementation
- [**SLINK_IMPLEMENTATION.md**](SLINK_IMPLEMENTATION.md) - SLINK generative mode deep dive (dual-wave engine with tabbed UI)
- [**TB3PO_INTERFACE.md**](TB3PO_INTERFACE.md) - TB-303 inspired generator interface
- **UI Update Documentation:**
  - [**UI_UPDATE_ANALYSIS.md**](UI_UPDATE_ANALYSIS.md) - Comprehensive UI analysis for all modules
  - [**UI_SUB_ISSUES.md**](UI_SUB_ISSUES.md) - Organized UI update tasks
  - [**UI_PROGRESS_TRACKER.md**](UI_PROGRESS_TRACKER.md) - Implementation progress tracking

### Technical Details
- [**IMPLEMENTATION_SUMMARY.md**](IMPLEMENTATION_SUMMARY.md) - Core architecture and code layout
- [**PERFORMANCE_OPTIMIZATIONS.md**](PERFORMANCE_OPTIMIZATIONS.md) - Performance improvements and benchmarks
- [**MEMORY_OPTIMIZATION.md**](MEMORY_OPTIMIZATION.md) - Memory usage optimization strategies
- [**DRAM_FIX_EXPLANATION.md**](DRAM_FIX_EXPLANATION.md) - SPIRAM workaround for LVGL stability
- [**RTOS_IMPLEMENTATION_PLAN.md**](RTOS_IMPLEMENTATION_PLAN.md) - RTOS usage and task management

### MIDI Clock & Timing
- [**MIDI_CLOCK_SYNC.md**](MIDI_CLOCK_SYNC.md) - MIDI clock synchronization
- [**MIDI_CLOCK_TIMING_FIX.md**](MIDI_CLOCK_TIMING_FIX.md) - Clock timing fixes
- [**MIDI_CLOCK_FIX.md**](MIDI_CLOCK_FIX.md) - MIDI clock implementation details
- [**UCLOCK_INTEGRATION.md**](UCLOCK_INTEGRATION.md) - uClock library integration
- [**UCLOCK_API_FIX.md**](UCLOCK_API_FIX.md) - uClock API fixes

### Build & Release
- [**BUILD_VERIFICATION.md**](BUILD_VERIFICATION.md) - Build verification checklist for releases
- [**RELEASE.md**](RELEASE.md) - Release process documentation
- [**RELEASE_CHECKLIST.md**](RELEASE_CHECKLIST.md) - Pre-release checklist
- [**WEB_INSTALLER.md**](WEB_INSTALLER.md) - ESP Web Tools integration
- [**ESP_WEB_TOOLS_IMPLEMENTATION.md**](ESP_WEB_TOOLS_IMPLEMENTATION.md) - Web installer implementation details

### CI/CD & Automation
- [**CI_FAILURE_REPORTING.md**](CI_FAILURE_REPORTING.md) - Automated CI failure tracking
- [**CI_FAILURE_IMPLEMENTATION.md**](CI_FAILURE_IMPLEMENTATION.md) - CI failure system implementation
- [**CHANGELOG_AUTOMATION.md**](CHANGELOG_AUTOMATION.md) - Changelog automation

### Implementation Notes & Summaries
- [**ESP_NOW_IMPLEMENTATION_SUMMARY.md**](ESP_NOW_IMPLEMENTATION_SUMMARY.md) - ESP-NOW implementation summary
- [**SPLASH_SCREEN_SIGNATURE_FIX.md**](SPLASH_SCREEN_SIGNATURE_FIX.md) - Splash screen fixes
- [**UI_SUMMARY.md**](UI_SUMMARY.md) - UI implementation summary
- [**UI_QUICK_START.md**](UI_QUICK_START.md) - UI development quick start
- [**FINAL_SUMMARY.md**](FINAL_SUMMARY.md) - Project summary
- [**PR_SUMMARY.md**](PR_SUMMARY.md) - Pull request summaries
- [**RELEASE_SUMMARY.md**](RELEASE_SUMMARY.md) - Release summaries

## Documentation Guidelines

1. **User-facing docs** go in the main [README.md](../README.md)
2. **Technical deep-dives** belong in this `docs/` folder
3. **Link between docs** to help readers find related information
4. **Keep docs focused** - one topic per file
5. **Update docs** when making code changes

## Contributing to Documentation

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines on improving documentation.

Documentation improvements we welcome:
- Fixing typos and unclear explanations
- Adding examples and screenshots
- Translating documentation
- Expanding hardware setup guides
- Adding troubleshooting tips

