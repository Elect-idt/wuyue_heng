/**
 ******************************************************************************
 * @file    bsp_spi.c
 * @author  Pan
 * @version V1.0
 * @date    2025-12-20
 * @brief   SPI驱动
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_spi.h"
#include <string.h>

static SPI_InitTypeDef stm32f4_spi_base_config[SPI_ID_MAX] = {};

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
    case SPI_ID_KEY_SACN:
    {
        /* KEY SCAN只需要读取不需要写入 */
        KEY_SCAN_SPI_GPIO_CLK_INIT(KEY_SCAN_SPI_SCK_GPIO_CLK | KEY_SCAN_SPI_MISO_GPIO_CLK | KEY_SCAN_CS_GPIO_CLK,
                                   ENABLE);
        /* IO口内部用一个弱上拉增加带载能力 */
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        /* CS配置 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
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
        GPIO_InitStructure.GPIO_Pin = KEY_SCAN_SPI_SCK_GPIO_PIN;
        GPIO_Init(KEY_SCAN_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

        /* 复用配置,只需要配置SCK和MISO */
        GPIO_PinAFConfig(KEY_SCAN_SPI_SCK_GPIO_PORT, KEY_SCAN_SPI_SCK_PINSOURCE, KEY_SCAN_SPI_SCK_AF);
        GPIO_PinAFConfig(KEY_SCAN_SPI_MISO_GPIO_PORT, KEY_SCAN_SPI_MISO_PINSOURCE, KEY_SCAN_SPI_MISO_AF);
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
    case SPI_ID_KEY_SACN:
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

        /* 使能SPI的DMA,在DR寄存器有数据时会产生请求，但此时数据流没有使能
           每当 RXNE 标志置 1 时，即产生 DMA 请求*/
        SPI_I2S_DMACmd(KEY_SCAN_SPI, SPI_I2S_DMAReq_Rx, ENABLE);

        /* 保存配置 */
        stm32f4_spi_base_config[SPI_ID_KEY_SACN] = SPI_InitStructure;
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
    case SPI_ID_KEY_SACN:
    {
        /* 配置DMA通道为中断源 */
        NVIC_InitStructure.NVIC_IRQChannel = KEY_SCAN_SPI_DMA_IRQn;
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
    case SPI_ID_KEY_SACN:
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
 * @brief  SPI发送一个字节
 * @note   无
 * @param  id:SPI设备号
 * @param  ch:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_byte(spi_id_e id, uint8_t send_data)
{
    uint32_t stm32f4_timeout = SPI_TIME_OUT;
    uint8_t dummy_receive = 0;
    switch (id)
    {
    case SPI_ID_KEY_SACN:
    {
        /* 确保配置模式的准确性 */
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SACN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }
        /* SPI_CS拉低，开始通讯 */
        stm32f4_spi_control(id, SPI_STATE_ENABLE);
        /* 等待发送数据寄存器为空 */
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_TXE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
                return BSP_STAT_TIME_OUT;
            }
        }
        /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
        SPI_SendData(KEY_SCAN_SPI, send_data);
        /* 等待接收数据寄存器非空，并且假读取用于清空标志位 */
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_RXNE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
                return BSP_STAT_TIME_OUT;
            }
        }
        dummy_receive = SPI_ReceiveData(KEY_SCAN_SPI);
        /* 等待BSY寄存器为0 */
        stm32f4_timeout = SPI_TIME_OUT;
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_BSY) != RESET)
        {
            if (stm32f4_timeout-- == 0)
            {
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
                return BSP_STAT_TIME_OUT;
            }
        }
        /* SPI_CS拉高，结束通讯 */
        stm32f4_spi_control(id, SPI_STATE_DISABLE);
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
 * @brief  SPI读取一个字节
 * @note   无
 * @param  id:SPI设备号
 * @param  receive_data:要保存读取字节的地址
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_receive_byte(spi_id_e id, uint8_t* receive_data)
{
    uint32_t stm32f4_timeout = SPI_TIME_OUT;
    uint8_t dummy_send = 0XFF;
    if (receive_data == NULL)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    switch (id)
    {
    case SPI_ID_KEY_SACN:
    {
        /* 确保配置模式的准确性 */
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SACN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }
        /* SPI_CS拉低，开始通讯 */
        stm32f4_spi_control(id, SPI_STATE_ENABLE);
        /* 等待发送数据寄存器为空，并且假写用于启动时钟 */
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_TXE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
                return BSP_STAT_TIME_OUT;
            }
        }
        /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
        SPI_SendData(KEY_SCAN_SPI, dummy_send);
        /* 等待接收数据寄存器非空 */
        while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_RXNE) != SET)
        {
            if (stm32f4_timeout-- == 0)
            {
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
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
                /* SPI_CS拉高，结束通讯 */
                stm32f4_spi_control(id, SPI_STATE_DISABLE);
                return BSP_STAT_TIME_OUT;
            }
        }
        /* SPI_CS拉高，结束通讯 */
        stm32f4_spi_control(id, SPI_STATE_DISABLE);
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
 * @brief  SPI 只发多个字节, 默认用DMA
 * @note   无
 * @param  id:SPI设备号
 * @param  receive_data:要保存读取字节的地址
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_multi_data_dma(spi_id_e id, const uint8_t* send_data, uint32_t data_size)
{
    uint32_t stm32f4_timeout = SPI_TIME_OUT;
    uint8_t dummy_receive = 0;
    switch (id)
    {
    case SPI_ID_KEY_SACN:
    {
        /* 确保配置模式的准确性 */
        if ((stm32f4_spi_base_config[SPI_ID_KEY_SACN].SPI_Direction & SPI_Direction_2Lines_FullDuplex) !=
            SPI_Direction_2Lines_FullDuplex)
        {
            return BSP_STAT_INVALID_PARAMS;
        }

        /* 结构体宏定义 TODO这里后面应该不用每次初始化 */
        DMA_InitTypeDef DMA_InitStructure;
        /* 开启DMA时钟 */
        KEY_SCAN_SPI_DMA_CLK_INIT(KEY_SCAN_SPI_DMA_CLK, ENABLE);
        /* 因为同一个数据流同一时刻只能使能一个通道，所以先失能 */
        DMA_Cmd(KEY_SCAN_SPI_DMA_STREAM, DISABLE);
        while (DMA_GetCmdStatus(KEY_SCAN_SPI_DMA_STREAM) != DISABLE)
            ;
        /* 清除传输完成标志位 */
        DMA_ClearITPendingBit(KEY_SCAN_SPI_DMA_STREAM, KEY_SCAN_SPI_DMA_IT_TC);
        /* 去初始化 */
        DMA_DeInit(KEY_SCAN_SPI_DMA_STREAM);
        /* DMA结构体配置 */
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(KEY_SCAN_SPI->DR)); // 外设基址为：ADC 数据寄存器地址
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)send_data;     // 存储器地址，实际上就是一个内部SRAM的变量
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;          // 数据传输方向为存储器到外设
        DMA_InitStructure.DMA_BufferSize = data_size;                    // 缓冲区大小为，指一次传输的数据量
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设寄存器只有一个，地址不用递增
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 存储器地址递增
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 外设数据大小为字节，也就是节拍大小
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;   // 存储器数据大小也为字节，也就是节拍大小
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                     // 单次传输模式
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;               // DMA 传输通道优先级为高，优先级设置不影响
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;            // 禁止DMA FIFO	，使用直连模式
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull; // FIFO 大小，FIFO模式禁止时，这个不用配置
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_InitStructure.DMA_Channel = KEY_SCAN_SPI_DMA_CHANNEL; // 选择 DMA 通道，通道存在于流中
        DMA_Init(KEY_SCAN_SPI_DMA_STREAM, &DMA_InitStructure); // 初始化DMA流，流相当于一个大的管道，管道里面有很多通道
        /* 清除中断标志位 */
        DMA_ClearITPendingBit(KEY_SCAN_SPI_DMA_STREAM, DMA_IT_TC);
        /* 使能DMA传输完成中断 */
        DMA_ITConfig(KEY_SCAN_SPI_DMA_STREAM, DMA_IT_TC, ENABLE);
        /* 使能DMA流 */
        DMA_Cmd(KEY_SCAN_SPI_DMA_STREAM, ENABLE);
        /* SPI_CS拉低，开始通讯 */
        stm32f4_spi_control(id, SPI_STATE_ENABLE);
        /* 等待开始传输，这里几乎不耗时，但是还是需要加一个判断，
           可能占用两三个SPI时钟的事件 */
        while (SPI_I2S_GetFlagStatus(KEY_SCAN_SPI, SPI_I2S_FLAG_BSY) != SET)
            ;
        //

        /* SPI_CS拉高，结束通讯 */
        stm32f4_spi_control(id, SPI_STATE_DISABLE);
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

// // STM32F4平台SPI驱动实例，实现spi_ops_t定义的统一操作接口
// // [C++对照] 对应具体产品(Concrete Product)
// // 注：C中无继承，具体产品与抽象产品是同一类型，区别仅为函数指针指向了具体实现（类似填好的虚表）
// const spi_ops_t g_stm32f4_spi_driver_ = {
//     .name = "STM32F4_SPI_DRIVER",
//     .init = stm32f4_uasrt_init,
//     .spi_send_byte = stm32f4_spi_send_byte,
//     .spi_send_string = stm32f4_spi_send_string,
//     .spi_send_hex = stm32f4_spi_send_hex,
//     .spi_send_array = stm32f4_spi_send_array,
// };
