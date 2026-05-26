#include "74hc165.h"

void hc165_init(hc165_t *dev, const spi_ops_t *spi_ops, spi_id_e id,
                const spi_dma_sync_t *sync, uint8_t num_chips,
                const gpio_ops_t *gpio_ops, gpio_pin_e pl_pin)
{
    dev->spi_ops   = spi_ops;
    dev->spi_id    = id;
    dev->dma_sync  = sync;
    dev->num_chips = num_chips;
    dev->gpio_ops  = gpio_ops;
    dev->pl_pin    = pl_pin;

    spi_ops->init(id);   /* RAII: 初始化 SPI 硬件 */
    gpio_ops->init(pl_pin); /* RAII: 初始化 PL 引脚 */
}

bsp_status_e hc165_read(hc165_t *dev, uint8_t *buf)
{
    /* 1. PL LOW → 锁存并行输入 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_LOW);
    /* 2. PL HIGH → 进入移位模式 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_HIGH);
    /* 3. CE LOW → 使能 SPI 时钟 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_ENABLE);
    /* 4. SPI DMA 接收 */
    bsp_status_e status = dev->spi_ops->spi_receive_multi_data_dma(
        dev->spi_id, buf, dev->num_chips, dev->dma_sync);
    /* 5. CE HIGH → 结束 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
    return status;
}

bsp_status_e hc165_read_polling(hc165_t *dev, uint8_t *buf)
{
    /* 1. PL LOW → 锁存 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_LOW);
    /* 2. PL HIGH → 移位模式 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_HIGH);
    /* 3. CE LOW → 使能 SPI 时钟 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_ENABLE);
    /* 4. 逐字节读取 */
    for (uint8_t i = 0; i < dev->num_chips; i++) {
        bsp_status_e s = dev->spi_ops->spi_receive_byte(dev->spi_id, &buf[i]);
        if (s != BSP_STAT_TRUE) {
            dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
            return s;
        }
    }
    /* 5. CE HIGH → 结束 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
    return BSP_STAT_TRUE;
}
