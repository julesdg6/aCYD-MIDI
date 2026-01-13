# aCYD MIDI Documentation

This folder gathers every deep-dive doc for the current CYD MIDI experience, including the main menu flow, capture behavior, and Wi-Fi/remote display wiring. The top-level `README.md` links here for extra context.

## Main Menu & Modules

- **4×4 grid layout** – Every mode now lives in a dedicated tile (shown in `assets/screenshots/00_main_menu.bmp`) with a darker rainbow gradient, accurate module icons, and black button text for contrast.
- **16 total modules** – The menu connects to KEYS, BEATS, ZEN, DROP, RNG, XY PAD, ARP, GRID, CHORD, LFO, TB3PO, GRIDS, RAGA, EUCLID, MORPH, and SLINK. Each module is isolated in a `module_*.h/.cpp` pair under `src/` so new behaviors can be worked on in one place.
- **Splash screen** – Boot artwork pulls in `assets/aCYD-MIDI.png` and displays the Wi-Fi address (once connected) before landing back in the LVGL loop.

## Capture Flow

- **Back-button capture** – Holding the BACK button on the main menu for ~1.5 s runs `captureAllScreenshots()`; it visits MENU + every module and writes `/screenshots/<board>_<mode>_NNN.bmp` using the sanitized board/mode labels. That callback is shared by the USB shell, so every screenshot now follows the board-aware naming convention described in `README.md`.
- **SD output** – The BMP writer now handles padding, row alignment, and incremental filenames, so keep the `/screenshots` folder on the card and watch the serial log for confirmation.

## Wi-Fi & Remote Display

- **Config files** – Copy `config/wifi_config.local.h.template` into `config/wifi_config.local.h`, update `WIFI_SSID` / `WIFI_PASSWORD`, and keep the new file ignored (`.gitignore` already references `config/wifi_config.local.h`). The main selector now includes `config/wifi_config.h` automatically so the remote display and splash screen can report the current IP.
- **Remote viewer** – See `docs/REMOTE_DISPLAY.md` for the WebSocket viewer setup; it depends on the `config/wifi_config.local.h` credentials documented above.

## Next Steps

- Refer to `docs/SLINK_IMPLEMENTATION.md`, `docs/HARDWARE_MIDI.md`, and `docs/HARDWARE_MIDI_CONFIG.md` for mode-specific implementation notes.
- The legacy modules for RAGA, EUCLID, and MORPH live in `legacy/` for now; move features into `src/module_*.cpp` if you want to mirror those behaviors exactly.
