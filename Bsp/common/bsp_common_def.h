#ifndef __BSP_COMMON_INTERFACE_H_
#define __BSP_COMMON_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>


typedef enum
{
    BSP_STAT_ERROR                = -1,   // 默认错误
    BSP_STAT_TRUE                 = 0,    // 正常
    BSP_STAT_CHOOSE_ERROR_TARGET  = 1,    // 选择了一个错误的外设对象，如不存在的LED
    BSP_STAT_INVALID_PARAMS       = 2,    // 给了一个错误的参数，比如systick定时器系统时钟输入111,SPI配置为只读，但是用了send
    BSP_STAT_TIME_OUT             = 3,    // 超时返回
} bsp_status_e;


#endif //__BSP_COMMON_INTERFACE_H_
