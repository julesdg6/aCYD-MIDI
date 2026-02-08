// Thin link-time wrapper for gpio_set_level to log first invalid gpio usage.
// Uses linker --wrap=gpio_set_level so references to gpio_set_level are
// redirected to __wrap_gpio_set_level; the real function is available as
// __real_gpio_set_level.

#include <driver/gpio.h>
#include <esp32-hal-log.h>
#include <stdbool.h>

extern "C" {
// Declaration of the real function provided by the linker when --wrap is used.
esp_err_t __real_gpio_set_level(gpio_num_t gpio_num, uint32_t level);

esp_err_t __wrap_gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
  static bool logged_invalid = false;

  // If an invalid GPIO is used, log it (only the first time) including the
  // caller return address so we can trace where it originated.
  if (!GPIO_IS_VALID_GPIO(gpio_num) && !logged_invalid) {
    logged_invalid = true;
    log_e("GPIO_WRAP: invalid gpio_set_level(%d) called from %p", (int)gpio_num, __builtin_return_address(0));
  }

  // Forward to the real implementation to preserve original behavior.
  return __real_gpio_set_level(gpio_num, level);
}
} // extern "C"
