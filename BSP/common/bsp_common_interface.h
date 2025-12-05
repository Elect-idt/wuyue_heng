#ifndef BSP_COMMON_INTERFACE_H
#define BSP_COMMON_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BSP_STAT_ERROR = -1,    // 默认错误
    BSP_STAT_TRUE  = 0      // 正常
} bsp_status_e;


#endif