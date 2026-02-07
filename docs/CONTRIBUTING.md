# Contributing to aCYD-MIDI

Thank you for your interest in contributing to aCYD-MIDI! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)

## Code of Conduct

This project follows a simple code of conduct:
- Be respectful and constructive in your communications
- Welcome newcomers and help them get started
- Focus on what is best for the project and community
- Show empathy towards other community members

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/aCYD-MIDI.git
   cd aCYD-MIDI
   ```
3. **Add the upstream repository**:
   ```bash
   git remote add upstream https://github.com/julesdg6/aCYD-MIDI.git
   ```

## Development Setup

### Prerequisites

- **PlatformIO** (recommended) or Arduino IDE
- **ESP32 development board** (ESP32-2432S028R or compatible)
- **USB cable** for programming
- **Git** for version control

### Building the Project

Using PlatformIO (recommended):
```bash
# Install PlatformIO if you haven't already
pip install platformio

# Build for your board
pio run -e esp32-2432S028Rv2

# Upload to your board
pio run -e esp32-2432S028Rv2 -t upload

# Monitor serial output
pio device monitor
```

### Project Structure

- `src/` - Main application code
- `include/` - Header files for modes and features
- `docs/` - Documentation
- `platformio.ini` - Build configuration
- `boards/` - Custom board definitions

## How to Contribute

### Types of Contributions

We welcome various types of contributions:

1. **Bug Fixes** - Fix issues in existing code
2. **New Modes** - Add new interactive MIDI modes
3. **Feature Enhancements** - Improve existing features
4. **Documentation** - Improve or translate documentation
5. **Hardware Support** - Add support for new boards
6. **UI Improvements** - Enhance the user interface
7. **Performance Optimizations** - Make the code faster or more efficient

### Before You Start

1. **Check existing issues** to see if someone is already working on it
2. **Open an issue** to discuss major changes before implementing them
3. **Keep changes focused** - one feature or fix per pull request
4. **Test your changes** on actual hardware if possible

## Coding Standards

### Code Style

- Use **camelCase** for function and variable names
- Use **UPPER_SNAKE_CASE** for constants and macros
- Add **comments** for complex logic
- Use the existing **color scheme** defined in `common_definitions.h`
- Follow the **mode-based architecture** pattern for new modes

### Display Scaling

Always use scaling macros for UI elements:
```cpp
int x = SCALE_X(10);      // Scale X coordinate
int y = SCALE_Y(20);      // Scale Y coordinate
int width = SCALE_X(100); // Scale width
```

### MIDI Output

Use the helper functions for MIDI:
```cpp
sendMIDI(status, data1, data2);           // BLE MIDI
sendHardwareMIDI3(status, data1, data2);  // Hardware MIDI
```

### Configuration

- **Never edit library files** - all configuration goes in `platformio.ini` build flags
- Use **conditional compilation** for optional features
- Respect the **build flag conventions**

## Pull Request Process

1. **Create a feature branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following the coding standards

3. **Test your changes**:
   - Build successfully on all affected configurations
   - Test on actual hardware if possible
   - Verify existing functionality still works

4. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Add: Brief description of your changes"
   ```
   
   Use prefixes:
   - `Add:` for new features
   - `Fix:` for bug fixes
   - `Update:` for improvements
   - `Docs:` for documentation
   - `Refactor:` for code restructuring

5. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create a Pull Request** on GitHub:
   - Provide a clear description of the changes
   - Reference any related issues (e.g., "Fixes #123")
   - Include screenshots for UI changes
   - Describe how you tested the changes

7. **Address review feedback** if requested

### Pull Request Checklist

- [ ] Code builds successfully
- [ ] Changes are tested on hardware (or explain why not)
- [ ] Documentation is updated if needed
- [ ] Commit messages are clear and descriptive
- [ ] No unrelated changes included
- [ ] Follows existing code style and patterns

## Reporting Bugs

When reporting bugs, please include:

1. **Description** - Clear description of the issue
2. **Hardware** - Board model and configuration
3. **Firmware Version** - Version you're using
4. **Steps to Reproduce** - Detailed steps to reproduce the bug
5. **Expected Behavior** - What should happen
6. **Actual Behavior** - What actually happens
7. **Serial Output** - Any relevant serial monitor output
8. **Screenshots** - If applicable

Use the GitHub issue template when available.

## Suggesting Features

We love feature suggestions! When suggesting a feature:

1. **Check existing issues** to avoid duplicates
2. **Describe the feature** clearly and in detail
3. **Explain the use case** - why is this useful?
4. **Consider implementation** - any thoughts on how it could work?
5. **Hardware requirements** - any specific hardware needed?

## Development Tips

### Adding a New Mode

To add a new interactive mode:

1. Create `include/your_mode.h`
2. Implement three functions:
   - `initializeYourMode()` - Setup/reset
   - `drawYourMode()` - Full screen redraw
   - `handleYourMode()` - Touch input and updates
3. Add to `AppMode` enum in `common_definitions.h`
4. Add menu item in `main.cpp`
5. Include header and add switch cases

See existing modes for examples.

### Testing Without Hardware

If you don't have physical hardware:
- Use the **Remote Display** feature (WiFi enabled builds)
- Test in simulation if available
- Clearly mark PRs as "untested on hardware"

### Debugging

- Use `Serial.println()` wrapped in `MIDI_DEBUG` checks
- Monitor `pio device monitor` for output
- Use the **screenshot** feature to capture UI states
- Enable **Remote Display** for visual debugging

## Questions?

- **Open an issue** for questions about development
- **Check documentation** in the `docs/` folder
- **Review existing code** for examples

Thank you for contributing to aCYD-MIDI! 🎹🎶
