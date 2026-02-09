# CYD Web Debug Console

A web-based debugging tool for the aCYD MIDI controller that provides real-time MIDI and Serial monitoring with an integrated TB-303 bass synthesizer.

## Features

### Dual Connection Support
- **BLE MIDI (Web Bluetooth)**: Connect wirelessly to CYD for MIDI communication
- **Web Serial**: Connect via USB for firmware log monitoring
- **Simultaneous connections**: Both can be active at the same time

### TB-303 Bass Synthesizer
- **Real-time playback**: Plays all incoming MIDI messages as a TB-303 acid bass synth
- **Web Audio API**: High-quality analog-modeled synthesis
- **Classic TB-303 controls**: Cutoff, Resonance, Env Mod, Decay, Accent, Volume
- **Sawtooth/Square waveforms**: Authentic TB-303 oscillator options
- **Slide/Portamento**: Smooth note transitions
- **Visual feedback**: Live waveform display and parameter indicators

### Unified Event Logging
- **Single panel design**: MIDI and Serial events in one view
- **Smart formatting**: 
  - MIDI events: Left-aligned in orange (#ff8800)
  - Serial events: Right-aligned in green (#00ff88)
- **High-resolution timestamps**: Millisecond precision with delta times
- **Source indicators**: Clear labeling of UI→MIDI, CYD←BLE, CYD→SER
- **MIDI parsing**: Automatic decoding of Note On/Off, CC, Program Change, etc.
- **Real-time updates**: Live streaming with auto-scroll

### Log Controls
- **Filters**: Toggle MIDI and Serial independently
- **Search**: Real-time substring filtering
- **Pause/Resume**: Freeze log updates for inspection
- **Save Log**: Download combined log with timestamps and MIDI/TTY prefixes
- **Clear**: Reset log buffer

### Optimized Layout
- **No scrolling required**: Entire interface fits in viewport (100vh)
- **40/60 split**: Synth panel (top 40%), Log panel (bottom 60%)
- **Responsive design**: Adapts to different screen sizes

## Browser Requirements

This tool requires a **Chromium-based browser** with Web Bluetooth and Web Serial support:

✅ **Supported:**
- Google Chrome 89+
- Microsoft Edge 89+
- Opera 75+
- Brave (with flags enabled)

❌ **Not Supported:**
- Firefox (no Web Bluetooth/Serial API)
- Safari (no Web Bluetooth/Serial API)
- Mobile browsers (limited support)

## Getting Started

### Running Locally

1. **Install dependencies:**
   ```bash
   cd debug-console
   npm install
   ```

2. **Start dev server:**
   ```bash
   npm run dev
   ```

3. **Open in browser:**
   Navigate to `http://localhost:5173`

### Using the Debug Console

1. **Connect to CYD:**
   - Click "Connect BLE MIDI" and select your CYD device
   - Audio context will start automatically on first connection
   - Optionally click "Connect Serial" for firmware logs (115200 baud)

2. **Listen to MIDI playback:**
   - All incoming MIDI notes are played through the TB-303 synth
   - Use MIDI CC messages to control synth parameters:
     - CC 74: Cutoff
     - CC 71: Resonance
     - CC 75: Envelope Modulation
     - CC 76: Decay
     - CC 77: Accent
     - CC 7: Volume
     - CC 65: Slide (portamento on/off)

3. **Monitor events:**
   - Watch the unified log panel for all MIDI and Serial events
   - MIDI events appear on the left in orange
   - Serial events appear on the right in green
   - Use filters to focus on specific event types
   - Search for specific messages
   - Save logs for sharing or analysis (timestamp + MIDI/TTY prefix format)

## Building for Production

```bash
npm run build
```

Output will be in `dist/` directory, ready for deployment to GitHub Pages.

## Architecture

### Core Modules

- **`eventBus.ts`**: Central event system with high-resolution timing
- **`bleMidi.ts`**: Web Bluetooth MIDI connection and parsing
- **`serial.ts`**: Web Serial connection and line reading
- **`logger.ts`**: Ring buffer log management with filtering
- **`controller.ts`**: Application state and MIDI message generation
- **`synth303.ts`**: TB-303 bass synthesizer using Web Audio API
- **`main.ts`**: UI binding and event handlers

### Data Flow

```
BLE/Serial → EventBus → Logger → Unified Log Display
                ↓
             Synth303 (Web Audio)
                ↓
            Audio Output
```

### Time Synchronization

All events use `performance.now()` as a high-resolution clock source:
- `tAbsMs`: Wall clock time (Date.now())
- `tRelMs`: Session-relative time (performance.now() - t0)
- `tDeltaMs`: Time since previous event

This enables accurate correlation between UI actions, MIDI messages, and firmware logs.

## Configuration

### MIDI Settings (default)
- **Channel**: 1
- **CC Mapping**:
  - Cutoff: CC 74
  - Resonance: CC 71
  - Env Mod: CC 75
  - Decay: CC 76
  - Accent: CC 77
  - Volume: CC 7

### Log Settings
- **Max Entries**: 10,000 (ring buffer)
- **Auto-scroll**: Enabled
- **Show Delta Times**: Enabled

Settings can be modified in `src/types.ts` (DEFAULT_SETTINGS).

## BLE MIDI Service UUIDs

The console connects using standard BLE MIDI UUIDs (same as CYD firmware):
- **Service**: `03b80e5a-ede8-4b33-a751-6ce34ec4c700`
- **Characteristic**: `7772e5db-3868-4112-a1a9-f2669d106bf3`

## Troubleshooting

### "Bluetooth not available"
- Ensure you're using a supported browser
- Check that Bluetooth is enabled on your system
- Try restarting the browser

### "Serial port access denied"
- Grant permission when browser prompts
- Close other programs using the serial port (Arduino IDE, PlatformIO monitor)
- Try a different USB cable/port

### "Device not found"
- Ensure CYD is powered on and BLE MIDI is active
- Check that CYD is not already connected to another device
- Reset CYD and try again

### Log performance issues
- Reduce max log entries in settings
- Disable auto-scroll when inspecting old entries
- Use filters to reduce displayed entries

## Performance

The console is designed to handle:
- **500+ events/second** in short bursts
- **10,000 log entries** in ring buffer
- **Real-time rendering** with requestAnimationFrame batching

For extreme loads, consider:
- Pausing the log during high-traffic periods
- Exporting and clearing periodically
- Using filters to reduce display overhead

## Known Limitations

- No Settings UI (edit `src/types.ts` and rebuild)
- No LocalStorage persistence (resets on reload)
- No device-side timestamp correlation
- Synth parameters controlled via MIDI CC only (no manual UI controls)
- Desktop only (mobile Web Bluetooth/Serial support is limited)

## Future Enhancements

- Settings panel UI for synth and MIDI configuration
- LocalStorage for preferences
- Latency estimation (round-trip time)
- Device timestamp correlation
- Improved virtualized log rendering for large datasets
- MIDI message recording/playback
- Interactive synth controls (knobs/sliders in UI)
- Multiple waveform options for synth
- Built-in sequencer for pattern testing

## Contributing

To add features:

1. Edit TypeScript files in `src/`
2. Test locally with `npm run dev`
3. Build with `npm run build`
4. Submit PR with description of changes

## License

MIT (same as main aCYD MIDI project)
