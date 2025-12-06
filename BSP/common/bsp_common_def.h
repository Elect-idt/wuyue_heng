#ifndef __BSP_COMMON_INTERFACE_H_
#define __BSP_COMMON_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>


typedef enum
{
    BSP_STAT_ERROR                = -1,   // 默认错误
    BSP_STAT_TRUE                 = 0,    // 正常
    BSP_STAT_CHOOSE_ERROR_TARGET  = 1,    // 选择了一个错误的外设对象，如不存在的LED
} bsp_status_e;


#endif //__BSP_COMMON_INTERFACE_H_
