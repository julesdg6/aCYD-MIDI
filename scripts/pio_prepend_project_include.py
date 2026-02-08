Import("env")

import os
import re


def patch_esp32_smartdisplay_lvgl_panel_common() -> None:
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    pioenv = env.subst("$PIOENV")
    header_path = os.path.join(
        libdeps_dir, pioenv, "esp32_smartdisplay", "include", "lvgl_panel_common.h"
    )

    if not os.path.isfile(header_path):
        # Library not installed yet (or renamed). Nothing to patch.
        return

    text = open(header_path, "r", encoding="utf-8").read()

    # Idempotent: if already patched, skip.
    if "ESP_ERR_NOT_SUPPORTED" in text and "esp_lcd_panel_disp_on_off" in text:
        return

    if "#include <esp_err.h>" not in text:
        text = text.replace(
            "#include <esp32_smartdisplay.h>\n",
            "#include <esp32_smartdisplay.h>\n#include <esp_err.h>\n",
            1,
        )

    # Replace the unconditional abort on ESP_ERR_NOT_SUPPORTED.
    # RGB panels can legally return ESP_ERR_NOT_SUPPORTED here.
    replaced = False
    needle = "ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));"
    if needle in text:
        text = text.replace(
            needle,
            "esp_err_t err = esp_lcd_panel_disp_on_off(panel_handle, true);\n"
            "    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED)\n"
            "    {\n"
            "        ESP_ERROR_CHECK(err);\n"
            "    }",
            1,
        )
        replaced = True
    else:
        # Fall back to regex in case upstream formatting changes.
        new_text, count = re.subn(
            r"ESP_ERROR_CHECK\(\s*esp_lcd_panel_disp_on_off\s*\(\s*panel_handle\s*,\s*true\s*\)\s*\);\s*",
            "esp_err_t err = esp_lcd_panel_disp_on_off(panel_handle, true);\n"
            "    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED)\n"
            "    {\n"
            "        ESP_ERROR_CHECK(err);\n"
            "    }\n",
            text,
            count=1,
        )
        if count:
            text = new_text
            replaced = True

    if not replaced:
        return

    open(header_path, "w", encoding="utf-8").write(text)


patch_esp32_smartdisplay_lvgl_panel_common()


def patch_esp32_smartdisplay_lvgl_touch_gt911_i2c() -> None:
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    pioenv = env.subst("$PIOENV")
    source_path = os.path.join(
        libdeps_dir, pioenv, "esp32_smartdisplay", "src", "lvgl_touch_gt911_i2c.c"
    )

    if not os.path.isfile(source_path):
        return

    text = open(source_path, "r", encoding="utf-8").read()

    # Idempotent: if already patched, skip.
    if "Elecrow PCA9557 touch timing control" in text:
        return

    needle = "ESP_ERROR_CHECK(i2c_driver_install(GT911_I2C_HOST, i2c_config.mode, 0, 0, 0));"
    if needle not in text:
        return

    patch = """

#if defined(ELECROW_ESP32_7_0)
    // Elecrow CrowPanel 7.0\\" V3.0 uses a PCA9557 (0x18) to control touch reset/INT timing.
    // Bring it up here so the GT911 is ready before we probe it over I2C.
    {
        const uint8_t pca9557_addr = 0x18;
        uint8_t reg = 0x00;
        uint8_t val = 0;
        esp_err_t pca_err = i2c_master_write_read_device(GT911_I2C_HOST, pca9557_addr,
                                                         &reg, 1, &val, 1, pdMS_TO_TICKS(100));
        if (pca_err == ESP_OK)
        {
            uint8_t buf[2];
            // reset(): all input, all high, non-inverted
            buf[0] = 0x03; buf[1] = 0xFF;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
            buf[0] = 0x01; buf[1] = 0xFF;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
            buf[0] = 0x02; buf[1] = 0x00;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));

            // setMode(IO_OUTPUT): all pins output
            buf[0] = 0x03; buf[1] = 0x00;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));

            // IO0 low, IO1 low (others high)
            buf[0] = 0x01; buf[1] = 0xFC;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(20));

            // IO0 high, IO1 low (others high)
            buf[0] = 0x01; buf[1] = 0xFD;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(100));

            // IO1 input (others output)
            buf[0] = 0x03; buf[1] = 0x02;
            i2c_master_write_to_device(GT911_I2C_HOST, pca9557_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
            log_i("Elecrow PCA9557 touch timing control applied");
        }
        else
        {
            log_w("Elecrow PCA9557 (0x18) not responding, skipping timing control (err=0x%x)", pca_err);
        }
    }
#endif
"""

    text = text.replace(needle, needle + patch, 1)
    open(source_path, "w", encoding="utf-8").write(text)


patch_esp32_smartdisplay_lvgl_touch_gt911_i2c()
