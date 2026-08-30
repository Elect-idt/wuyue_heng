#ifndef __WS2812_LED_H_
#define __WS2812_LED_H_

#include "bsp_spi_interface.h"
#include "bsp_common_def.h"

/* 事务互斥等待超时（lock 注入且争用时，最多等这么久） */
#define WS2812_LED_LOCK_TIMEOUT_MS 100

/* 级联灯珠数上限：组件内编码缓冲是定长 static，改此值即可扩容（12 字节/灯） */
#define WS2812_LED_MAX_NUM  128

// 颜色三元组。字段故意按 GRB 序声明：内存布局即 WS2812 的发送顺序（G 先发），
// 编码时按字节顺序取位、零重映射；使用时仍按习惯的 RGB 语义赋值（红 = .r=0xFF）
typedef struct
{
    uint8_t g;
    uint8_t r;
    uint8_t b;
} ws2812_color_t;

// WS2812B 器件描述符（所有依赖从外部注入，调用方用 C99 指定初始化器预填，
// 再交 ws2812_led_init 校验 + 硬件初始化）
typedef struct {
    const spi_ops_t      *spi_ops;    // SPI 操作接口（必填）
    spi_id_e             spi_id;      // SPI 设备 ID（必填）
    const spi_dma_sync_t *dma_sync;   // DMA 同步（Apps 注入，NULL = 轮询模式）
    uint8_t              num_leds;    // 级联灯珠数量，1~WS2812_LED_MAX_NUM（必填）
    const bsp_lock_t     *lock;       // 事务互斥（可选，NULL = 不加锁；
                                      //  共享总线/多消费者时由 Apps 注入同一实例）
} ws2812_led_t;

// 初始化器件（校验预填描述符 + 初始化 SPI + 发一帧全黑洗掉上电随机色）
// 契约：必填 spi_ops / spi_id / num_leds(1~WS2812_LED_MAX_NUM)；
//       可选 dma_sync（NULL = 轮询模式）、lock（NULL = 不加锁）
// 返回 BSP_STAT_INVALID_PARAMS（必填缺失或超上限）或底层 init 转发状态
bsp_status_e ws2812_led_init(ws2812_led_t *dev);

// 把一帧颜色写到灯链（frame 长度 = num_leds）
// frame 放哪都行（栈/CCMRAM 均可）——编码在组件内 static 缓冲完成，DMA 只碰主 SRAM
// ⚠ 两次 write 间隔须 >= 80us：WS2812 靠帧间低电平(RESET)提交/锁存上一帧；
//   组件为 RTOS-free 不做等待，由调用方帧率保证（50Hz 周期 20ms，余量 250 倍）
bsp_status_e ws2812_led_write(ws2812_led_t *dev, const ws2812_color_t *frame);

#endif // __WS2812_LED_H_
