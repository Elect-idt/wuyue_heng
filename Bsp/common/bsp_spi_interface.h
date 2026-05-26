#ifndef __BSP_SPI_INTERFACE_H_
#define __BSP_SPI_INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bsp_common_def.h"

// 定义SPI的逻辑ID
typedef enum
{
    // SPI_ID_LCD        = 0,    // LCD SPI
    SPI_ID_KEY_SACN   = 0,    // 键盘扫描74HC165 HSPI
    // SPI_ID_LED_Array  = 2,    // 指纹SPI
    SPI_ID_MAX        = 1,    // MAX
} spi_id_e;

// DMA同步机制（BSP层不依赖RTOS，通过函数指针注入同步原语）
typedef struct
{
    void *handle;                          // 同步句柄（如SemaphoreHandle_t，对BSP透明）
    void (*wait)(void *handle);            // 阻塞等待（如xSemaphoreTake）
    void (*notify_from_isr)(void *handle); // ISR中通知（如xSemaphoreGiveFromISR）
} spi_dma_sync_t;

// 定义SPI的逻辑ID
typedef enum
{
    SPI_STATE_ENABLE    = 0,   // SPI使能
    SPI_STATE_DISABLE   = 1,   // SPI失能
} spi_control_e;

// 核心解耦：SPI外设驱动接口，定义SPI的统一操作方法，不同平台实现各自的驱动实例
// [C++对照] 对应抽象产品(Abstract Product)，类似于含纯虚函数的基类
typedef struct
{
    // 驱动名称
    const char* name;

    // 初始化SPI
    bsp_status_e (*init)(spi_id_e id);
    
    // SPI 使/失能
    bsp_status_e (*spi_cs_control)(spi_id_e id, spi_control_e state);

    // SPI 发送一个字节
    bsp_status_e (*spi_send_byte)(spi_id_e id, uint8_t send_data);

    // SPI 接收一个字节
    bsp_status_e (*spi_receive_byte)(spi_id_e id, uint8_t* receive_data);

    // SPI 只发多个字节, 默认用DMA
    bsp_status_e (*spi_send_multi_data_dma)(spi_id_e id, const uint8_t* send_data, uint32_t data_size, const spi_dma_sync_t *sync);

    // SPI 只读多个字节, 默认用DMA
    bsp_status_e (*spi_receive_multi_data_dma)(spi_id_e id, uint8_t* receive_data, uint32_t data_size, const spi_dma_sync_t *sync);

} spi_ops_t;

#endif // __BSP_SPI_INTERFACE_H_
