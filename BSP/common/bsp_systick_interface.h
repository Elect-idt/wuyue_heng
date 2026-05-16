#ifndef __BSP_SYSTICK_INTERFACE_H_
#define __BSP_SYSTICK_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_def.h"

// 定义SYSTICK的逻辑ID
typedef enum
{
    SYSTICK_ID_DEFAULT = 0,    //
    SYSTICK_ID_MAX    = 1,     //
} systick_id_e;

// 核心解耦：SYSTICK外设驱动接口，定义SYSTICK的统一操作方法，不同平台实现各自的驱动实例
// [C++对照] 对应抽象产品(Abstract Product)，类似于含纯虚函数的基类
typedef struct
{
    // 驱动名称
    const char* name;

    // 初始化SYSTICK
    bsp_status_e (*init)(systick_id_e id, uint16_t sysclk);
    
    // us级别延迟
    bsp_status_e (*delay_us)(systick_id_e id, uint32_t us);

    // ms级别延迟
    bsp_status_e (*delay_ms)(systick_id_e id, uint32_t ms);

    // s级别延迟
    bsp_status_e (*delay_s)(systick_id_e id, uint32_t s);
} systick_ops_t;

#endif //__BSP_SYSTICK_INTERFACE_H_
