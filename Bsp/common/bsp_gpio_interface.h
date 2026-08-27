#ifndef __BSP_GPIO_INTERFACE_H_
#define __BSP_GPIO_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_def.h"

// GPIO 引脚逻辑 ID
typedef enum
{
    GPIO_PIN_LED_STATUS = 0,    // 状态LED
    GPIO_PIN_HC165_PL   = 1,    // 74HC165 并行加载引脚
    GPIO_PIN_MAX        = 2,
} gpio_pin_e;

// GPIO 引脚状态
typedef enum
{
    GPIO_LOW    = 0,    // 低电平
    GPIO_HIGH   = 1,    // 高电平
    GPIO_TOGGLE = 2     // 翻转
} gpio_state_e;

// GPIO 外设驱动接口，定义 GPIO 的统一操作方法，不同平台实现各自的驱动实例
// 可以理解为这个就是纯虚类，也是基类
typedef struct
{
    const char *name;

    // 初始化指定 GPIO 引脚
    bsp_status_e (*init)(gpio_pin_e pin);

    // 控制指定引脚的状态
    bsp_status_e (*write)(gpio_pin_e pin, gpio_state_e state);
} gpio_ops_t;

#endif // __BSP_GPIO_INTERFACE_H_
