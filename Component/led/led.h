#ifndef __LED_H_
#define __LED_H_

#include "bsp_gpio_interface.h"
#include "bsp_common_def.h"

// LED 器件描述符（所有依赖从外部注入，调用方用 C99 指定初始化器预填，
// 再交 led_init 校验 + 硬件初始化）
typedef struct {
    const gpio_ops_t *gpio_ops;    // GPIO 操作接口（必填）
    gpio_pin_e       pin;          // GPIO 引脚 ID（必填）
    bool             active_low;   // true: 低电平点亮, false: 高电平点亮（不填默认 false）
} led_t;

// 初始化 LED（校验预填描述符 + 初始化 GPIO 硬件），转发 GPIO init 的返回状态
// 契约：必填 gpio_ops / pin；active_low 可不填（默认 false）
// 注：LED 无多字节事务（单次 GPIO 写），暂不需要 bsp_lock_t 注入
bsp_status_e led_init(led_t *led);

// 点亮 LED
void led_on(led_t *led);

// 熄灭 LED
void led_off(led_t *led);

// 翻转 LED
void led_toggle(led_t *led);

#endif // __LED_H_
