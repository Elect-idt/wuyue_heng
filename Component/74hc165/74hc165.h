#ifndef __74HC165_H_
#define __74HC165_H_

#include "bsp_spi_interface.h"
#include "bsp_gpio_interface.h"
#include "bsp_common_def.h"

/* 事务互斥等待超时（lock 注入且争用时，最多等这么久） */
#define HC165_LOCK_TIMEOUT_MS 100

// 74HC165 器件描述符（所有依赖从外部注入，调用方用 C99 指定初始化器预填，
// 再交 hc165_init 校验 + 硬件初始化）
typedef struct {
    const spi_ops_t      *spi_ops;    // SPI 操作接口（必填）
    spi_id_e             spi_id;      // SPI 设备 ID（必填）
    const spi_dma_sync_t *dma_sync;   // DMA 同步（Apps 注入，NULL = 轮询模式）
    uint8_t              num_chips;   // 级联芯片数量，>=1（必填）
    const gpio_ops_t     *gpio_ops;   // GPIO 接口，PL 引脚控制（必填）
    gpio_pin_e           pl_pin;      // PL 引脚 ID（必填）
    const bsp_lock_t     *lock;       // 事务互斥（可选，NULL = 不加锁；
                                      //  共享总线/多消费者时由 Apps 注入同一实例）
} hc165_t;

// 初始化器件（校验预填描述符 + 初始化 SPI/GPIO 硬件）
// 契约：调用方预填 hc165_t 后调用本函数：
//   必填：spi_ops / spi_id / num_chips(>=1) / gpio_ops / pl_pin
//   可选：dma_sync（NULL = 轮询模式）、lock（NULL = 不加锁）
// 返回 BSP_STAT_INVALID_PARAMS（必填缺失）或底层 init 转发的状态
bsp_status_e hc165_init(hc165_t *dev);

// DMA 读取所有级联芯片数据到 buf（buf 长度 >= num_chips，必须在主 SRAM：DMA 够不着 CCMRAM）
bsp_status_e hc165_read(hc165_t *dev, uint8_t *buf);

// 轮询读取（无需 sync，逐字节；lock 已注入时同样受事务互斥保护）
bsp_status_e hc165_read_polling(hc165_t *dev, uint8_t *buf);

#endif // __74HC165_H_
