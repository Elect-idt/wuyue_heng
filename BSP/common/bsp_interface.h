#ifndef BSP_INTERFACE_H
#define BSP_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

// LED公共接口
#include "bsp_common_interface.h"
#include "bsp_led_interface.h"

// 声明全局硬件句柄
typedef struct
{
    const led_ops_t *led_ops; // 指向LED操作方法的指针
    // 以后还可以加 key_ops_t *key;
    // 以后还可以加 lcd_ops_t *lcd;
} board_hw_t;

extern board_hw_t Board; // 全局单例对象

#endif