#include "led.h"
#include <stddef.h>

bsp_status_e led_init(led_t *led)
{
    /* 校验预填字段：漏填在这里拦截，而不是变成硬件层的怪错误 */
    if (led == NULL || led->gpio_ops == NULL)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    return led->gpio_ops->init(led->pin); /* RAII: 初始化指定 GPIO 引脚 */
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
