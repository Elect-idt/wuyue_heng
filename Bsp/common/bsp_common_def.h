#ifndef __BSP_COMMON_DEF_H_
#define __BSP_COMMON_DEF_H_

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

// 通用事务互斥抽象（BSP/Component 不依赖 RTOS，由 Apps 注入具体实现）
// 与 spi_dma_sync_t 同性质：机制定义在公共接口，对象创建在组合根（Apps），
// 使用包在器件事务上（如 hc165_read 的 PL+CS+DMA 整个序列）。
// 共享同一总线/器件的多个消费者必须注入同一个 bsp_lock_t 实例
typedef struct
{
    void *handle;                                      // 锁句柄（如 SemaphoreHandle_t，对下层透明）
    bool (*lock)(void *handle, uint32_t timeout_ms);   // 加锁，返回 true=成功 false=超时
    bool (*unlock)(void *handle);                      // 解锁，返回 true=成功
} bsp_lock_t;


#endif //__BSP_COMMON_DEF_H_
