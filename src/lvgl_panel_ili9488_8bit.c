#ifdef DISPLAY_ILI9488_8BIT

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp32-hal-log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_panel_ili9488.h>
#include <lvgl_panel_common.h>

lv_display_t *lvgl_lcd_init()
{
    lv_display_t *display = lvgl_create_display();
    log_v("display:0x%08x", display);

#if ILI9488_8BIT_RD >= 0
    pinMode(ILI9488_8BIT_RD, OUTPUT);
    digitalWrite(ILI9488_8BIT_RD, HIGH);
#endif

    const esp_lcd_i80_bus_config_t i80_bus_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .dc_gpio_num = ILI9488_8BIT_RS,
        .wr_gpio_num = ILI9488_8BIT_WR,
        .data_gpio_nums =
            {
                ILI9488_8BIT_D0,
                ILI9488_8BIT_D1,
                ILI9488_8BIT_D2,
                ILI9488_8BIT_D3,
                ILI9488_8BIT_D4,
                ILI9488_8BIT_D5,
                ILI9488_8BIT_D6,
                ILI9488_8BIT_D7},
        .bus_width = 8,
        .max_transfer_bytes = LVGL_BUFFER_PIXELS * 2,
        .psram_trans_align = 64,
        .sram_trans_align = 64};

    esp_lcd_i80_bus_handle_t i80_bus;
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&i80_bus_config, &i80_bus));

    const esp_lcd_panel_io_i80_config_t io_i80_config = {
        .cs_gpio_num = GPIO_NUM_NC,
        .pclk_hz = ILI9488_8BIT_PCLK_HZ,
        .on_color_trans_done = lvgl_panel_color_trans_done,
        .user_ctx = display,
        .trans_queue_depth = 2,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 1,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1},
        .flags = {
            .cs_active_high = 0,
            .reverse_color_bits = 0,
            .swap_color_bytes = 0,
            .pclk_active_neg = ILI9488_8BIT_PCLK_ACTIVE_NEG,
            .pclk_idle_low = !ILI9488_8BIT_PCLK_IDLE_HIGH}};

    esp_lcd_panel_io_handle_t io_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_i80_config, &io_handle));

    const esp_lcd_panel_dev_config_t panel_dev_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .color_space = ILI9488_DEV_CONFIG_COLOR_SPACE,
        .bits_per_pixel = ILI9488_DEV_CONFIG_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = ILI9488_DEV_CONFIG_FLAGS_RESET_ACTIVE_HIGH},
        .vendor_config = NULL};
    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(io_handle, &panel_dev_config, &panel_handle));

    lvgl_setup_panel(panel_handle);
    display->user_data = panel_handle;
    display->flush_cb = lv_flush_hardware;
    return display;
}

#endif
