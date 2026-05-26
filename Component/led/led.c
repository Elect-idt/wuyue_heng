#include "led.h"

void led_init(led_t *led, const gpio_ops_t *ops, gpio_pin_e pin, bool active_low)
{
    led->gpio_ops   = ops;
    led->pin        = pin;
    led->active_low = active_low;
    ops->init(pin); /* RAII: 初始化指定 GPIO 引脚 */
}

void led_on(led_t *led)
{
    gpio_state_e state = led->active_low ? GPIO_LOW : GPIO_HIGH;
    led->gpio_ops->write(led->pin, state);
}

void led_off(led_t *led)
{
    gpio_state_e state = led->active_low ? GPIO_HIGH : GPIO_LOW;
    led->gpio_ops->write(led->pin, state);
}

void led_toggle(led_t *led)
{
    led->gpio_ops->write(led->pin, GPIO_TOGGLE);
}
