#ifndef __LED_H_
#define __LED_H_

#include "bsp_gpio_interface.h"

// LED 器件描述符
typedef struct {
    const gpio_ops_t *gpio_ops;    // GPIO 操作接口
    gpio_pin_e       pin;          // GPIO 引脚 ID
    bool             active_low;   // true: 低电平点亮, false: 高电平点亮
} led_t;

// 初始化 LED（配置描述符 + 初始化 GPIO 硬件）
void led_init(led_t *led, const gpio_ops_t *ops, gpio_pin_e pin, bool active_low);

// 点亮 LED
void led_on(led_t *led);

// 熄灭 LED
void led_off(led_t *led);

// 翻转 LED
void led_toggle(led_t *led);

#endif // __LED_H_
