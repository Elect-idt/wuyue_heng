#ifndef __BSP_LED_INTERFACE_H_
#define __BSP_LED_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_def.h"

// 定义LED的逻辑ID
typedef enum
{
    LED_ID_STATUS = 0,    // 比如：系统运行指示灯
    LED_ID_MAX    = 1,     // 比如：系统运行指示灯
} led_id_e;

// 定义 LED 的状态
typedef enum
{
    LED_OFF = 0,
    LED_ON  = 1,
    LED_TOGGLE = 2
} led_state_e;

// 核心解耦：定义操作函数指针结构体，其实就是工厂接口，CPP中的方法工厂基类
typedef struct
{
    // 驱动名称
    const char* name;

    // 初始化所有LED的硬件引脚
    bsp_status_e (*init)(void);
    
    // 控制指定LED的状态
    bsp_status_e (*control)(led_id_e id, led_state_e state);
} led_ops_t;

#endif //__BSP_LED_INTERFACE_H_