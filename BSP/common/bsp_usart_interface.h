#ifndef __BSP_USART_INTERFACE_H_
#define __BSP_USART_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_def.h"

// 定义USART的逻辑ID
typedef enum
{
    USART_ID_DEBUG     = 0,         // debug串口
    USART_ID_BLT       = 1,         // 蓝牙串口
    USART_ID_FINGER    = 2,         // 指纹串口
    USART_ID_MAX       = 3,         // MAX
} uasrt_id_e;

// 核心解耦：定义操作函数指针结构体，其实就是工厂接口，CPP中的方法工厂基类
typedef struct
{
    // 驱动名称
    const char* name;

    // 初始化USART
    bsp_status_e (*init)(uasrt_id_e id);
    
    // 串口发送一个字节
    bsp_status_e (*usart_send_byte)(uasrt_id_e id, uint8_t ch);

    // 串口发送一串字符串
    bsp_status_e (*usart_send_string)(uasrt_id_e id, char* str);

    // 串口发送一个hex数
    bsp_status_e (*usart_send_hex)(uasrt_id_e id, uint16_t hex);

    // 串口发送一个u8数组
    bsp_status_e (*usart_send_array)(uasrt_id_e id, uint8_t *array, uint16_t num);
} usart_ops_t;

#endif // __BSP_USART_INTERFACE_H_
