Title: Display panel drivers - overflow checks and graceful cleanup on failures

Affected files:
- src/esp_panel_ili9488.c
- src/lvgl_panel_ili9488_8bit.c

Description
-----------
Several issues in the display driver code can cause integer overflow or resource leaks when initialization fails:

- `esp_panel_ili9488.c`: calculation of `len` and assembly of CASET/RASET parameters can overflow for large coordinates or when gaps are added; multiplication should be performed in `size_t` and coordinates clamped to 0..0xFFFF.
- `lvgl_panel_ili9488_8bit.c`: when `esp_lcd_new_i80_bus` fails, previously created display/panel handles are not freed leading to leaks; `ESP_ERROR_CHECK` is used inconsistently and aborts on failure instead of graceful cleanup.

Suggested fixes
---------------
- Clamp `x_start`, `x_end`, `y_start`, `y_end` after adding `ph->x_gap`/`ph->y_gap` to the 16-bit range and ensure `x_end >= x_start` and `y_end >= y_start` before assembling parameters.
- Compute `len` using `size_t` casts: `len = (size_t)(x_end - x_start) * (size_t)(y_end - y_start) * (size_t)bytes_per_pixel`.
- Replace `ESP_ERROR_CHECK` usages with `esp_err_t rc = ...; if (rc != ESP_OK) { cleanup; return NULL; }` and ensure cleanup mirrors earlier error handling paths (delete/free io handles, bus, panel, etc.).

Severity: Medium (resource leaks and potential overflow/crash on malformed inputs)

Notes
-----
Split into smaller PRs: overflow/clamping fixes first, then replace `ESP_ERROR_CHECK` usage with graceful error handling.