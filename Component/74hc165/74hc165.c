#include "74hc165.h"
#include <stddef.h>

/**
 * @brief  可选事务锁：进入事务（无锁注入时直接放行）
 * @return true=可进入事务（无锁或加锁成功）false=加锁超时
 */
static bool hc165_transaction_begin(hc165_t* dev)
{
    if (dev->lock && dev->lock->lock)
    {
        return dev->lock->lock(dev->lock->handle, HC165_LOCK_TIMEOUT_MS);
    }
    return true;
}

/**
 * @brief  可选事务锁：退出事务
 */
static void hc165_transaction_end(hc165_t* dev)
{
    if (dev->lock && dev->lock->unlock)
    {
        (void)dev->lock->unlock(dev->lock->handle);
    }
}

bsp_status_e hc165_init(hc165_t* dev)
{
    /* 1. 校验预填字段：漏填在这里拦截，而不是变成硬件层的怪错误 */
    if (dev == NULL || dev->spi_ops == NULL || dev->gpio_ops == NULL || dev->num_chips == 0)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    /* dma_sync / lock 为 NULL 合法（轮询 / 不加锁），见头文件契约 */

    /* 2. RAII: 初始化 SPI + GPIO 硬件，任一失败即返回 */
    bsp_status_e status = dev->spi_ops->init(dev->spi_id);
    if (status != BSP_STAT_TRUE)
    {
        return status;
    }
    return dev->gpio_ops->init(dev->pl_pin);
}

/**
 * @brief  DMA 事务体（不含锁，由 hc165_read 负责事务互斥）
 */
static bsp_status_e hc165_read_dma_body(hc165_t* dev, uint8_t* buf)
{
    /* 1. PL LOW -> 锁存并行输入
     * 时序说明：手册要求 PL 低电平 >= 80ns。两次 write 经 ops_t 函数指针
     * 间接调用 + SPL 函数体，168MHz 下低电平持续约 150~600ns，裕量充足，
     * 无需额外延时。注意：若将来改成直接 BSRR 寄存器写（1~2 周期，约 12ns）
     * 或换平台，需重新评估并按需插入 NOP */
    dev->gpio_ops->write(dev->pl_pin, GPIO_LOW);
    /* 2. PL HIGH -> 进入移位模式 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_HIGH);
    /* 3. CE LOW -> 使能 SPI 时钟 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_ENABLE);
    /* 4. SPI DMA 接收 */
    bsp_status_e status = dev->spi_ops->spi_receive_multi_data_dma(dev->spi_id, buf, dev->num_chips, dev->dma_sync);
    /* 5. CE HIGH -> 结束 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
    return status;
}

/**
 * @brief  轮询事务体（不含锁，由 hc165_read_polling 负责事务互斥）
 */
static bsp_status_e hc165_read_polling_body(hc165_t* dev, uint8_t* buf)
{
    /* 1. PL LOW -> 锁存（时序裕量说明见 hc165_read_dma_body） */
    dev->gpio_ops->write(dev->pl_pin, GPIO_LOW);
    /* 2. PL HIGH -> 移位模式 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_HIGH);
    /* 3. CE LOW -> 使能 SPI 时钟 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_ENABLE);
    /* 4. 逐字节读取 */
    for (uint8_t i = 0; i < dev->num_chips; i++)
    {
        bsp_status_e s = dev->spi_ops->spi_receive_byte(dev->spi_id, &buf[i]);
        if (s != BSP_STAT_TRUE)
        {
            dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
            return s;
        }
    }
    /* 5. CE HIGH -> 结束 */
    dev->spi_ops->spi_cs_control(dev->spi_id, SPI_STATE_DISABLE);
    return BSP_STAT_TRUE;
}

bsp_status_e hc165_read(hc165_t* dev, uint8_t* buf)
{
    bsp_status_e status;

    /* sync 为 NULL 时走轮询（遵守头文件契约） */
    if (dev->dma_sync == NULL)
    {
        return hc165_read_polling(dev, buf);
    }

    /* 事务互斥：锁住整个 PL+CS+DMA 序列（可选锁，无注入时零开销直通） */
    if (!hc165_transaction_begin(dev))
    {
        return BSP_STAT_TIME_OUT;
    }
    status = hc165_read_dma_body(dev, buf);
    hc165_transaction_end(dev);
    return status;
}

bsp_status_e hc165_read_polling(hc165_t* dev, uint8_t* buf)
{
    bsp_status_e status;

    /* 事务互斥（可选锁，无注入时零开销直通） */
    if (!hc165_transaction_begin(dev))
    {
        return BSP_STAT_TIME_OUT;
    }
    status = hc165_read_polling_body(dev, buf);
    hc165_transaction_end(dev);
    return status;
}
