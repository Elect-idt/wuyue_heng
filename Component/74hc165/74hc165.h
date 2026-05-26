#ifndef __74HC165_H_
#define __74HC165_H_

#include "bsp_spi_interface.h"
#include "bsp_gpio_interface.h"
#include "bsp_common_def.h"

// 74HC165 器件描述符（所有依赖从外部注入）
typedef struct {
    const spi_ops_t      *spi_ops;    // SPI 操作接口
    spi_id_e             spi_id;      // SPI 设备 ID
    const spi_dma_sync_t *dma_sync;   // DMA 同步（Apps 注入，NULL 则用轮询）
    uint8_t              num_chips;   // 级联芯片数量（1 = 单片，2 = 两片级联...）
    const gpio_ops_t     *gpio_ops;   // GPIO 接口（PL 引脚控制）
    gpio_pin_e           pl_pin;      // PL 引脚 ID
} hc165_t;

// 初始化器件（配置描述符 + 初始化 SPI/GPIO 硬件）
void hc165_init(hc165_t *dev, const spi_ops_t *spi_ops, spi_id_e id,
                const spi_dma_sync_t *sync, uint8_t num_chips,
                const gpio_ops_t *gpio_ops, gpio_pin_e pl_pin);

// DMA 读取所有级联芯片数据到 buf（buf 长度 >= num_chips）
bsp_status_e hc165_read(hc165_t *dev, uint8_t *buf);

// 轮询读取（无需 sync，逐字节）
bsp_status_e hc165_read_polling(hc165_t *dev, uint8_t *buf);

#endif // __74HC165_H_
