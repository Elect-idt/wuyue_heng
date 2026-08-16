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
} usart_id_e;

// 核心解耦：USART外设驱动接口，定义USART的统一操作方法，不同平台实现各自的驱动实例
// [C++对照] 对应抽象产品(Abstract Product)，类似于含纯虚函数的基类
typedef struct
{
    // 驱动名称
    const char* name;

    // 初始化USART
    bsp_status_e (*init)(usart_id_e id);
    
    // 串口发送一个字节
    bsp_status_e (*usart_send_byte)(usart_id_e id, uint8_t ch);

    // 串口发送一串字符串
    bsp_status_e (*usart_send_string)(usart_id_e id, const char* str);

    // 串口发送一个hex数
    bsp_status_e (*usart_send_hex)(usart_id_e id, uint16_t hex);

    // 串口发送一个u8数组
    bsp_status_e (*usart_send_array)(usart_id_e id, const uint8_t* array, uint16_t num);

    // 串口接收一个字节（轮询，带超时）
    // 适用：调试交互、简单轮询协议。收发协议设备（蓝牙/指纹）请勿用此接口
    // 逐字节轮询组帧，应等待 RX 异步机制（规划：IDLE+DMA + rx_notify 注入，
    // 模式照抄 spi_dma_sync_t -- Apps 创建同步原语注入，BSP 不依赖 RTOS）
    bsp_status_e (*usart_receive_byte)(usart_id_e id, uint8_t* ch);
} usart_ops_t;

#endif // __BSP_USART_INTERFACE_H_
