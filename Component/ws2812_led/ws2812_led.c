#include "ws2812_led.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ============ WS2812B 编码层 ============ */
/* 每 WS bit 用 4 个 SPI bit 编码（SPI 2.625MHz，SPI bit = 381ns）：
 *   '1' 码 = 0b1100 -> 高 2/4 = 762ns（>580ns 且 <1us）
 *   '0' 码 = 0b1000 -> 高 1/4 = 381ns（<470ns）
 * 每 24 WS bit（GRB 序、MSB first）= 12 字节，一个字节装 2 个码（高半字节先发） */
#define WS2812_LED_ENC_BYTES_PER_LED 12
#define WS2812_LED_CODE_1 0xC0 /* '1' 码模板，放高半字节 */
#define WS2812_LED_CODE_0 0x80 /* '0' 码模板 */

/* 编码缓冲：必须文件级 static（落 .bss / 主 SRAM），DMA 够不着 CCMRAM。
 * 定长 WS2812_LED_MAX_NUM*12 = 1536B */
static uint8_t s_encoded[WS2812_LED_MAX_NUM * WS2812_LED_ENC_BYTES_PER_LED];

/**
 * @brief  可选事务锁：进入事务（无锁注入时直接放行）
 * @return true=可进入事务（无锁或加锁成功）false=加锁超时
 */
static bool ws2812_led_transaction_begin(ws2812_led_t *dev)
{
    if (dev->lock && dev->lock->lock)
    {
        return dev->lock->lock(dev->lock->handle, WS2812_LED_LOCK_TIMEOUT_MS);
    }
    return true;
}

/**
 * @brief  可选事务锁：退出事务
 */
static void ws2812_led_transaction_end(ws2812_led_t *dev)
{
    if (dev->lock && dev->lock->unlock)
    {
        (void)dev->lock->unlock(dev->lock->handle);
    }
}

/**
 * @brief  RGB 帧 -> WS2812 SPI 编码流
 * @note   ws2812_color_t 内存布局即 GRB 发送序，拼成 32bit 后从 MSB 起每 2 位
 *         编成 1 字节（高半字节 = 前一位的码，SPI MSB first 保证了发送序）
 */
static void ws2812_led_encode(const ws2812_led_t *dev, const ws2812_color_t *frame)
{
    uint32_t out = 0;

    for (uint32_t led = 0; led < dev->num_leds; led++)
    {
        /* 拼接 24bit：G 在高位 = 最先发（手册：GRB 顺序，高位先发） */
        uint32_t grb = ((uint32_t)frame[led].g << 16) | ((uint32_t)frame[led].r << 8) | (uint32_t)frame[led].b;

        /* shift 从 23 递减到 1，每次取相邻 2 个 WS bit 编成 1 字节，共 12 字节 */
        for (int8_t shift = 23; shift >= 1; shift -= 2)
        {
            uint8_t bit_hi = (grb >> shift) & 0x1u;
            uint8_t bit_lo = (grb >> (shift - 1)) & 0x1u;
            s_encoded[out++] = (uint8_t)((bit_hi ? WS2812_LED_CODE_1 : WS2812_LED_CODE_0) |
                                         ((bit_lo ? WS2812_LED_CODE_1 : WS2812_LED_CODE_0) >> 4));
        }
    }
}

bsp_status_e ws2812_led_init(ws2812_led_t *dev)
{
    /* 1. 校验预填字段：漏填在这里拦截，而不是变成硬件层的怪错误 */
    if (dev == NULL || dev->spi_ops == NULL || dev->num_leds == 0 || dev->num_leds > WS2812_LED_MAX_NUM)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    /* dma_sync / lock 为 NULL 合法（轮询 / 不加锁），见头文件契约 */

    /* 2. RAII: 初始化 SPI 硬件 */
    bsp_status_e status = dev->spi_ops->init(dev->spi_id);
    if (status != BSP_STAT_TRUE)
    {
        return status;
    }

    /* 3. 发一帧全黑：洗掉锁存器上电随机态（全 '0' 码 = 0x88 字节）。
     * 码元末位必为 0，发完后 MOSI 停在低电平，随后的空闲即 RESET 完成提交 */
    uint32_t n = (uint32_t)dev->num_leds * WS2812_LED_ENC_BYTES_PER_LED;
    memset(s_encoded, WS2812_LED_CODE_0 | (WS2812_LED_CODE_0 >> 4), n);
    return dev->spi_ops->spi_send_multi_data_dma(dev->spi_id, s_encoded, n, dev->dma_sync);
}

/**
 * @brief  DMA 事务体（不含锁，由 ws2812_led_write 负责事务互斥）
 */
static bsp_status_e ws2812_led_write_dma_body(ws2812_led_t *dev, const ws2812_color_t *frame)
{
    ws2812_led_encode(dev, frame);
    /* WS2812 单线协议：无 CS，帧由时序界定。发完后 MOSI 停在低电平（码元末位
     * 必为 0），帧间空闲低电平 >=80us 即 RESET 提交——由调用方帧间隔保证 */
    return dev->spi_ops->spi_send_multi_data_dma(dev->spi_id, s_encoded,
                                                 (uint32_t)dev->num_leds * WS2812_LED_ENC_BYTES_PER_LED, dev->dma_sync);
}

/**
 * @brief  轮询事务体（不含锁，由 ws2812_led_write 负责事务互斥）
 */
static bsp_status_e ws2812_led_write_polling_body(ws2812_led_t *dev, const ws2812_color_t *frame)
{
    ws2812_led_encode(dev, frame);
    uint32_t n = (uint32_t)dev->num_leds * WS2812_LED_ENC_BYTES_PER_LED;

    /* 逐字节发送：字节间隙（us 级）只是拉长了位间的低电平，远小于 80us 的
     * RESET 线，不会误触发提交，也不会破坏码元（码宽只看高电平） */
    for (uint32_t i = 0; i < n; i++)
    {
        bsp_status_e s = dev->spi_ops->spi_send_byte(dev->spi_id, s_encoded[i]);
        if (s != BSP_STAT_TRUE)
        {
            return s;
        }
    }
    return BSP_STAT_TRUE;
}

bsp_status_e ws2812_led_write(ws2812_led_t *dev, const ws2812_color_t *frame)
{
    bsp_status_e status;

    if (dev == NULL || frame == NULL)
    {
        return BSP_STAT_INVALID_PARAMS;
    }

    /* 事务互斥（可选锁，无注入时零开销直通），DMA/轮询两路都在锁内 */
    if (!ws2812_led_transaction_begin(dev))
    {
        return BSP_STAT_TIME_OUT;
    }
    status = (dev->dma_sync != NULL) ? ws2812_led_write_dma_body(dev, frame) : ws2812_led_write_polling_body(dev, frame);
    ws2812_led_transaction_end(dev);
    return status;
}
