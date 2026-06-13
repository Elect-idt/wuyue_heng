/**
 ******************************************************************************
 * @file    bsp_spi.c
 * @author  Pan
 * @version V2.0
 * @date    2025-12-20
 * @brief   SPI驱动（74HC165键盘扫描）
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_spi.h"
#include <string.h>

static SPI_InitTypeDef stm32f4_spi_base_config[SPI_ID_MAX] = {0};

/* DMA同步相关变量 */
static uint8_t s_dummy_tx = 0xFF;    /* DMA接收时发送dummy字节产生时钟 */
static uint8_t s_dummy_rx;           /* DMA发送时丢弃接收数据 */
const spi_dma_sync_t *g_spi_dma_sync_ptr = NULL; /* ISR访问的同步指针 */
volatile uint32_t g_spi_dma_isr_count = 0;  /* DEBUG: ISR触发计数 */

/**
 * @brief  SPI的GPIO配置
 * @note   无
 * @param  id:SPI设备号
 * @retval 无
 */
static void spi_gpio_config(spi_id_e id)
{
    /* 结构体宏定义 */
    GPIO_InitTypeDef GPIO_InitStructure;

    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        /* KEY SCAN只需要读取不需要写入 */
        KEY_SCAN_SPI_GPIO_CLK_INIT(KEY_SCAN_SPI_SCK_GPIO_CLK | KEY_SCAN_SPI_MISO_GPIO_CLK | KEY_SCAN_SPI_MOSI_GPIO_CLK | KEY_SCAN_CS_GPIO_CLK,
                                   ENABLE);
        /* IO口内部用一个弱上拉增加带载能力 */
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        /* CS配置：GPIO_Mode_OUT（软件控制PL，非AF） */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Pin = KEY_SCAN_CS_GPIO_PIN;
        GPIO_Init(KEY_SCAN_CS_GPIO_PORT, &GPIO_InitStructure);
        GPIO_SetBits(KEY_SCAN_CS_GPIO_PORT, KEY_SCAN_CS_GPIO_PIN);
        /* SPI SCK配置 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = KEY_SCAN_SPI_SCK_GPIO_PIN;
        GPIO_Init(KEY_SCAN_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
        /* SPI MISO配置 */
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = KEY_SCAN_SPI_MISO_GPIO_PIN;
        GPIO_Init(KEY_SCAN_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);
        /* SPI MOSI配置（全双工模式下MOSI也需配置为AF，用于发送dummy字节产生时钟） */
        GPIO_InitStructure.GPIO_Pin = KEY_SCAN_SPI_MOSI_GPIO_PIN;
        GPIO_Init(KEY_SCAN_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

        /* 复用配置 */
        GPIO_PinAFConfig(KEY_SCAN_SPI_SCK_GPIO_PORT, KEY_SCAN_SPI_SCK_PINSOURCE, KEY_SCAN_SPI_SCK_AF);
        GPIO_PinAFConfig(KEY_SCAN_SPI_MISO_GPIO_PORT, KEY_SCAN_SPI_MISO_PINSOURCE, KEY_SCAN_SPI_MISO_AF);
        GPIO_PinAFConfig(KEY_SCAN_SPI_MOSI_GPIO_PORT, KEY_SCAN_SPI_MOSI_PINSOURCE, KEY_SCAN_SPI_MOSI_AF);
    }
    break;
    default:
        break;
    }
}

/**
 * @brief  SPI1的基础配置
 * @note   无
 * @param  id:SPI设备号
 * @retval 无
 */
static void spi_base_config(spi_id_e id)
{

    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        SPI_InitTypeDef SPI_InitStructure;

        /* 时钟=84MHZ/2=42MHZ/2=21MHz */
        KEY_SCAN_SPI_CLK_INIT(KEY_SCAN_SPI_CLK, ENABLE);

        /* 双线但是只读、主模式、八位长度、SPI模式0 */
        SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
        SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
        SPI_InitStructure.SPI_CRCPolynomial = 0;
        SPI_Init(KEY_SCAN_SPI, &SPI_InitStructure);

        SPI_Cmd(KEY_SCAN_SPI, ENABLE);

        /* 不在此处使能SPI DMA请求，使用DMA时才使能，避免空闲时误触发 */

        /* 保存配置 */
        stm32f4_spi_base_config[SPI_ID_KEY_SCAN] = SPI_InitStructure;
    }
    break;
    default:
        break;
    }
}

/**
 * @brief  SPI1的dma中断配置
 * @note   无
 * @param  id:SPI设备号
 * @retval 无
 */
static void spi_dma_nvic_config(spi_id_e id)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        /* 配置DMA通道为中断源 */
        NVIC_InitStructure.NVIC_IRQChannel = KEY_SCAN_SPI_RX_DMA_IRQn;
        /* 抢断优先级 */
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
        /* 子优先级 */
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        /* 使能中断 */
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        /* 初始化配置NVIC */
        NVIC_Init(&NVIC_InitStructure);
    }
    break;
    default:
        break;
    }
}

/**
 * @brief  SPI初始化
 * @note   无
 * @param  id:SPI设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_spi_init(spi_id_e id)
{
    if (id >= SPI_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        spi_gpio_config(id);
        spi_base_config(id);
        spi_dma_nvic_config(id);
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI使能/失能
 * @note   无
 * @param  id:SPI设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_spi_control(spi_id_e id, spi_control_e state)
{
    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        if (SPI_STATE_ENABLE == state)
        {
            KEY_SCAN_CS_ENABLE;
        }
        else
        {
            KEY_SCAN_CS_DISABLE;
        }
    }
    break;
    default:
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    break;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI发送一个字节（纯传输，CS由调用方管理）
 * @note   无
 * @param  id:SPI设备号
 * @param  ch:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_byte(spi_id_e id, uint8_t send_data)
{
    uint32_t stm32f4_timeout;
    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        /* 确保配置模式的准确性 */
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SCAN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }
        /* 等待发送数据寄存器为空 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_TXE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
        /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
        SPI_SendData(KEY_SCAN_SPI, send_data);
        /* 等待接收数据寄存器非空，并且假读取用于清空标志位 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_RXNE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
        (void)SPI_ReceiveData(KEY_SCAN_SPI);
        /* 等待BSY寄存器为0 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_BSY) != RESET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
    }
    break;
    default:
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    break;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI读取一个字节（纯传输，CS由调用方管理）
 * @note   无
 * @param  id:SPI设备号
 * @param  receive_data:要保存读取字节的地址
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_receive_byte(spi_id_e id, uint8_t* receive_data)
{
    uint32_t stm32f4_timeout;
    uint8_t dummy_send = 0XFF;
    if (receive_data == NULL)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        /* 确保配置模式的准确性 */
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SCAN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }
        /* 等待发送数据寄存器为空，并且假写用于启动时钟 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_TXE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
        /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
        SPI_SendData(KEY_SCAN_SPI, dummy_send);
        /* 等待接收数据寄存器非空 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_RXNE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
        *receive_data = SPI_ReceiveData(KEY_SCAN_SPI);
        /* 等待BSY寄存器为0 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_BSY) != RESET)
        {
            if (stm32f4_timeout-- == 0)
            {
                return BSP_STAT_TIME_OUT;
            }
        }
    }
    break;
    default:
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    break;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  配置TX DMA流（辅助函数）
 * @param  mem_addr: 内存地址
 * @param  data_size: 数据长度
 * @param  mem_inc: 内存地址是否递增
 */
static void spi_tx_dma_config(const uint8_t *mem_addr, uint32_t data_size, bool mem_inc)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 同一个DMA流同一时刻只能使能一个通道，配置前必须先失能 */
    DMA_Cmd(KEY_SCAN_SPI_TX_DMA_STREAM, DISABLE);
    /* 等待流完全停止（硬件有延迟，写EN=0后需要几个DMA时钟周期才真正停止） */
    {
        uint32_t dma_stop_timeout = SPI_TIME_OUT;
        while (DMA_GetCmdStatus(KEY_SCAN_SPI_TX_DMA_STREAM) != DISABLE)
        {
            if (dma_stop_timeout-- == 0) break;
        }
    }
    /* 复位流的所有寄存器到默认值，防止上次配置残留 */
    DMA_DeInit(KEY_SCAN_SPI_TX_DMA_STREAM);
    /* 清除上次传输遗留的中断标志位，避免配置后立即误触发中断 */
    DMA_ClearITPendingBit(KEY_SCAN_SPI_TX_DMA_STREAM, KEY_SCAN_SPI_TX_DMA_IT_TC);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(KEY_SCAN_SPI->DR)); // 外设基址：SPI2数据寄存器
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)mem_addr;                 // 存储器地址（发送数据源）
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;                     // 方向：内存→外设（发送）
    DMA_InitStructure.DMA_BufferSize = data_size;                               // 传输数据个数
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;            // 外设地址不递增（DR只有一个）
    DMA_InitStructure.DMA_MemoryInc = mem_inc ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable; // 内存地址递增/不递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;     // 外设数据宽度：字节
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;             // 内存数据宽度：字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                               // 单次传输模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                         // 优先级：高
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;                      // 直连模式（不用FIFO）
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;           // FIFO阈值（直连模式下无效）
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;                 // 单次突发
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;         // 单次突发
    DMA_InitStructure.DMA_Channel = KEY_SCAN_SPI_TX_DMA_CHANNEL;                   // DMA通道（通道存在于流中）
    DMA_Init(KEY_SCAN_SPI_TX_DMA_STREAM, &DMA_InitStructure);                  // 初始化TX DMA流
}

/**
 * @brief  配置RX DMA流（辅助函数）
 * @param  mem_addr: 内存地址
 * @param  data_size: 数据长度
 * @param  mem_inc: 内存地址是否递增
 */
static void spi_rx_dma_config(uint8_t *mem_addr, uint32_t data_size, bool mem_inc)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 同一个DMA流同一时刻只能使能一个通道，配置前必须先失能 */
    DMA_Cmd(KEY_SCAN_SPI_RX_DMA_STREAM, DISABLE);
    /* 等待流完全停止（硬件有延迟，写EN=0后需要几个DMA时钟周期才真正停止） */
    {
        uint32_t dma_stop_timeout = SPI_TIME_OUT;
        while (DMA_GetCmdStatus(KEY_SCAN_SPI_RX_DMA_STREAM) != DISABLE)
        {
            if (dma_stop_timeout-- == 0) break;
        }
    }
    /* 复位流的所有寄存器到默认值，防止上次配置残留 */
    DMA_DeInit(KEY_SCAN_SPI_RX_DMA_STREAM);
    /* 清除上次传输遗留的中断标志位，避免配置后立即误触发中断 */
    DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(KEY_SCAN_SPI->DR)); // 外设基址：SPI2数据寄存器
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)mem_addr;                 // 存储器地址（接收数据目标）
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;                     // 方向：外设→内存（接收）
    DMA_InitStructure.DMA_BufferSize = data_size;                               // 传输数据个数
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;            // 外设地址不递增（DR只有一个）
    DMA_InitStructure.DMA_MemoryInc = mem_inc ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable; // 内存地址递增/不递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;     // 外设数据宽度：字节
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;             // 内存数据宽度：字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                               // 单次传输模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                         // 优先级：高
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;                      // 直连模式（不用FIFO）
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;           // FIFO阈值（直连模式下无效）
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;                 // 单次突发
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;         // 单次突发
    DMA_InitStructure.DMA_Channel = KEY_SCAN_SPI_RX_DMA_CHANNEL;                   // DMA通道（通道存在于流中）
    DMA_Init(KEY_SCAN_SPI_RX_DMA_STREAM, &DMA_InitStructure);                     // 初始化RX DMA流
}

/**
 * @brief  DMA传输完成后的清理
 */
static void spi_dma_cleanup(void)
{
    /* 关闭SPI的DMA请求，防止DMA流已停止但SPI仍在发出无人响应的请求 */
    SPI_I2S_DMACmd(KEY_SCAN_SPI, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);
    /* 关闭RX DMA传输完成中断，和传输前的 DMA_ITConfig ENABLE 对称 */
    DMA_ITConfig(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TC, DISABLE);
    /* 停止TX/RX DMA流，下次传输前会重新配置 */
    DMA_Cmd(KEY_SCAN_SPI_TX_DMA_STREAM, DISABLE);
    DMA_Cmd(KEY_SCAN_SPI_RX_DMA_STREAM, DISABLE);
    /* 清除DMA中断标志位，防止残留标志导致下次误触发 */
    DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC);
    DMA_ClearITPendingBit(KEY_SCAN_SPI_TX_DMA_STREAM, KEY_SCAN_SPI_TX_DMA_IT_TC);
    /* 清除同步指针，ISR不再访问已完成的sync */
    g_spi_dma_sync_ptr = NULL;
}

/**
 * @brief  SPI DMA发送多个字节（全双工，sync同步）
 * @note   TX DMA发送用户数据，RX DMA丢弃接收数据
 * @param  id:SPI设备号
 * @param  send_data:发送数据缓冲区
 * @param  data_size:数据长度
 * @param  sync:DMA同步机制（NULL时不等待）
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_multi_data_dma(spi_id_e id, const uint8_t* send_data, uint32_t data_size,
                                             const spi_dma_sync_t *sync)
{
    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SCAN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }

        KEY_SCAN_SPI_DMA_CLK_INIT(KEY_SCAN_SPI_DMA_CLK, ENABLE);

        /* 保存sync指针供ISR使用 */
        g_spi_dma_sync_ptr = sync;

        /* 配置TX DMA：发送用户数据 */
        spi_tx_dma_config(send_data, data_size, true);

        /* 配置RX DMA：丢弃接收数据（全双工必须同时配置） */
        spi_rx_dma_config(&s_dummy_rx, data_size, false);

        /* 使能SPI DMA请求 */
        SPI_I2S_DMACmd(KEY_SCAN_SPI, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

        /* 使能RX DMA传输完成中断（用于sync通知） */
        DMA_ITConfig(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TC, ENABLE);

        /* 先启动RX DMA，再启动TX DMA */
        DMA_Cmd(KEY_SCAN_SPI_RX_DMA_STREAM, ENABLE);
        DMA_Cmd(KEY_SCAN_SPI_TX_DMA_STREAM, ENABLE);

        /* 阻塞等待DMA完成，带超时保护 */
        if (sync && sync->wait)
        {
            if (!sync->wait(sync->handle, 100))
            {
                spi_dma_cleanup();
                return BSP_STAT_TIME_OUT;
            }
        }

        spi_dma_cleanup();
    }
    break;
    default:
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    break;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI DMA接收多个字节（全双工，TX发送dummy产生时钟，sync同步）
 * @note   TX DMA发送dummy(0xFF)产生时钟，RX DMA接收数据到用户缓冲区
 * @param  id:SPI设备号
 * @param  receive_data:接收数据缓冲区
 * @param  data_size:数据长度
 * @param  sync:DMA同步机制（NULL时不等待）
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_receive_multi_data_dma(spi_id_e id, uint8_t* receive_data, uint32_t data_size,
                                                const spi_dma_sync_t *sync)
{
    switch (id)
    {
    case SPI_ID_KEY_SCAN:
    {
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SCAN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }

        KEY_SCAN_SPI_DMA_CLK_INIT(KEY_SCAN_SPI_DMA_CLK, ENABLE);

        /* 保存sync指针供ISR使用 */
        g_spi_dma_sync_ptr = sync;

        /* 配置TX DMA：发送dummy字节产生时钟 */
        spi_tx_dma_config(&s_dummy_tx, data_size, false);

        /* 配置RX DMA：接收数据到用户缓冲区 */
        spi_rx_dma_config(receive_data, data_size, true);

        /* 使能SPI DMA请求 */
        SPI_I2S_DMACmd(KEY_SCAN_SPI, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

        /* 使能RX DMA传输完成中断（用于sync通知） */
        DMA_ITConfig(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TC, ENABLE);

        /* 先启动RX DMA，再启动TX DMA */
        DMA_Cmd(KEY_SCAN_SPI_RX_DMA_STREAM, ENABLE);
        DMA_Cmd(KEY_SCAN_SPI_TX_DMA_STREAM, ENABLE);

        /* 阻塞等待DMA完成，带超时保护 */
        if (sync && sync->wait)
        {
            if (!sync->wait(sync->handle, 100))
            {
                spi_dma_cleanup();
                return BSP_STAT_TIME_OUT;
            }
        }

        spi_dma_cleanup();
    }
    break;
    default:
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    break;
    }
    return BSP_STAT_TRUE;
}

// STM32F4平台SPI驱动实例，实现spi_ops_t定义的统一操作接口
// [C++对照] 对应具体产品(Concrete Product)
// 注：C中无继承，具体产品与抽象产品是同一类型，区别仅为函数指针指向了具体实现（类似填好的虚表）
const spi_ops_t g_stm32f4_spi_driver_ = {
    .name = "STM32F4_SPI_DRIVER",
    .init = stm32f4_spi_init,
    .spi_cs_control = stm32f4_spi_control,
    .spi_send_byte = spi_send_byte,
    .spi_receive_byte = spi_receive_byte,
    .spi_send_multi_data_dma = spi_send_multi_data_dma,
    .spi_receive_multi_data_dma = spi_receive_multi_data_dma,
};

/**
 * @brief  SPI RX DMA 中断处理（封装硬件细节，ISR 只需调用此函数）
 * @note   处理传输完成(TC)和传输错误(TE)，通知等待任务
 */
void bsp_spi_dma_isr_handler(void)
{
    /* 传输完成 */
    if (DMA_GetITStatus(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC))
    {
        DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC);
        g_spi_dma_isr_count++;
        if (g_spi_dma_sync_ptr && g_spi_dma_sync_ptr->notify_from_isr)
        {
            g_spi_dma_sync_ptr->notify_from_isr(g_spi_dma_sync_ptr->handle);
        }
    }
    /* 传输错误（TE）— 也通知等待任务，避免永久阻塞 */
    if (DMA_GetITStatus(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TEIF3))
    {
        DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TEIF3);
        if (g_spi_dma_sync_ptr && g_spi_dma_sync_ptr->notify_from_isr)
        {
            g_spi_dma_sync_ptr->notify_from_isr(g_spi_dma_sync_ptr->handle);
        }
    }
}
