#ifndef BSP_LED_INTERFACE_H
#define BSP_LED_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_interface.h"

// 定义LED的逻辑ID
typedef enum
{
    LED_ID_STATUS = 0       // 比如：系统运行指示灯
} led_id_t;

// 定义 LED 的状态
typedef enum
{
    LED_OFF = 0,
    LED_ON  = 1,
    LED_TOGGLE = 2
} led_state_e;

// 核心解耦：定义操作函数指针结构体
typedef struct
{
    // 初始化所有LED的硬件引脚
    bsp_status_e (*init)(void);
    
    // 控制指定LED的状态
    // 参数 id: 操作哪个灯
    // 参数 state: 开、关还是翻转
    bsp_status_e (*control)(led_id_t id, led_state_e state);
} led_ops_t;

#endif