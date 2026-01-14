# aCYD MIDI Documentation

This folder groups the supporting docs that expand on the features summarized in the top-level `README.md`. Leave the feature list and menu coverage in the main README so this folder can stay focused on deeper implementation notes.

## Document Map

- `CIRCUIT_DIAGRAMS.md` – Wiring diagrams for the board, hardware MIDI breakout, and sensor pinouts.
- `CONFIG_RULES.md` – PlatformIO/configuration conventions and helper scripts that keep the ESP32 builds identical.
- `HARDWARE_MIDI.md` / `HARDWARE_MIDI_CONFIG.md` – Hardware MIDI bus usage, UART selection, and debug handling.
- `IMPLEMENTATION_SUMMARY.md` – Engine-level notes for the current code layout and key trade-offs.
- `REMOTE_DISPLAY.md` – Wi-Fi + remote WebSocket viewer instructions (references `config/wifi_config.local.h.template`).
- `SLINK_IMPLEMENTATION.md` – Deep dive into the SLINK generative mode, including tabbed UI and MIDI routing.

## Keeping Docs Focused

1. Add new content under this folder only when it dives deeper than the overview in `README.md`.
2. Link back to this document (or the main README) when the same topic is mentioned elsewhere so readers always land on the most relevant page.
