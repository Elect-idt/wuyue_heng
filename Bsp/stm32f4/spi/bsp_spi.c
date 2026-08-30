/**
 ******************************************************************************
 * @file    bsp_spi.c
 * @author  Pan
 * @version V3.0
 * @date    2026-08-16
 * @brief   SPI驱动（74HC165键盘扫描）
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 * V3.0: 照 FIX-15 USART 模式重构为 spi_hw_config_t 配置描述符表，
 *       消除每函数一段 switch-case 的 copy-paste。新增 SPI 设备只需：
 *       1) bsp_spi.h 加一组 XXX_ 宏
 *       2) s_spi_cfg[] 加一行配置
 *       3) bsp_isr_map.h / stm32f4xx_it.c 加 ISR 路由
 *       同时删除旧的方向校验死代码（SPI_Direction_2Lines_FullDuplex==0，
 *       (x&0)!=0 恒 false，4 处检查永不触发）。
 ******************************************************************************
 */

#include "bsp_spi.h"
#include <string.h>

/* SPI 时钟使能函数类型（RCC_APB1/APB2PeriphClockCmd 同签名） */
typedef void (*spi_clk_cmd_fn)(uint32_t, FunctionalState);

/* SPI 引脚描述符（端口/引脚/引脚源/复用功能号，AF 引脚用） */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint16_t pinsrc;
    uint8_t af;
} spi_pin_t;

/* SPI 硬件配置描述符：把每路 SPI 的寄存器/引脚/时钟/DMA 全部集中到一处 */
typedef struct
{
    SPI_TypeDef *inst;           /* SPI 寄存器基址 */
    spi_clk_cmd_fn base_clk_cmd; /* SPI 时钟使能函数（APB1 或 APB2） */
    uint32_t base_clk;           /* SPI 时钟外设号 */
    uint16_t prescaler;          /* 波特率分频（相对 PCLK） */

    uint32_t gpio_clk;     /* SCK/MISO/MOSI/CS 的 GPIO 时钟外设号（合并） */
    spi_pin_t sck;         /* SCK 引脚（AF） */
    spi_pin_t miso;        /* MISO 引脚（AF） */
    spi_pin_t mosi;        /* MOSI 引脚（AF，全双工发 dummy） */
    GPIO_TypeDef *cs_port; /* CS 端口（软件控制，非 AF，低有效） */
    uint16_t cs_pin;       /* CS 引脚 */

    uint32_t dma_clk;              /* DMA 控制器时钟外设号 */
    DMA_Stream_TypeDef *rx_stream; /* RX DMA 流 */
    uint32_t rx_channel;           /* RX DMA 通道 */
    IRQn_Type rx_irqn;             /* RX DMA 中断号 */
    uint32_t rx_it_tc;             /* RX 传输完成中断标志 */
    uint32_t rx_it_te;             /* RX 传输错误中断标志 */
    uint8_t rx_pre_pri;            /* NVIC 抢占优先级（必须 >= 5，见 app_common_def.h） */
    uint8_t rx_sub_pri;            /* NVIC 子优先级 */
    DMA_Stream_TypeDef *tx_stream; /* TX DMA 流 */
    uint32_t tx_channel;           /* TX DMA 通道 */
    uint32_t tx_it_tc;             /* TX 传输完成中断标志 */
} spi_hw_config_t;

/* 每个逻辑 ID 对应一份硬件配置（新增 SPI 设备只需在此加一行） */
static const spi_hw_config_t s_spi_cfg[SPI_ID_MAX] = {
    [SPI_ID_KEY_SCAN] =
        {
            .inst = KEY_SCAN_SPI,
            .base_clk_cmd = KEY_SCAN_SPI_CLK_INIT,
            .base_clk = KEY_SCAN_SPI_CLK,
            .prescaler = SPI_BaudRatePrescaler_2, /* 时钟=42MHz/2=21MHz */
            .gpio_clk = KEY_SCAN_SPI_SCK_GPIO_CLK | KEY_SCAN_SPI_MISO_GPIO_CLK | KEY_SCAN_SPI_MOSI_GPIO_CLK |
                        KEY_SCAN_CS_GPIO_CLK,
            .sck = {KEY_SCAN_SPI_SCK_GPIO_PORT, KEY_SCAN_SPI_SCK_GPIO_PIN, KEY_SCAN_SPI_SCK_PINSOURCE,
                    KEY_SCAN_SPI_SCK_AF},
            .miso = {KEY_SCAN_SPI_MISO_GPIO_PORT, KEY_SCAN_SPI_MISO_GPIO_PIN, KEY_SCAN_SPI_MISO_PINSOURCE,
                     KEY_SCAN_SPI_MISO_AF},
            .mosi = {KEY_SCAN_SPI_MOSI_GPIO_PORT, KEY_SCAN_SPI_MOSI_GPIO_PIN, KEY_SCAN_SPI_MOSI_PINSOURCE,
                     KEY_SCAN_SPI_MOSI_AF},
            .cs_port = KEY_SCAN_CS_GPIO_PORT,
            .cs_pin = KEY_SCAN_CS_GPIO_PIN,
            .dma_clk = KEY_SCAN_SPI_DMA_CLK,
            .rx_stream = KEY_SCAN_SPI_RX_DMA_STREAM,
            .rx_channel = KEY_SCAN_SPI_RX_DMA_CHANNEL,
            .rx_irqn = KEY_SCAN_SPI_RX_DMA_IRQn,
            .rx_it_tc = KEY_SCAN_SPI_RX_DMA_IT_TC,
            .rx_it_te = KEY_SCAN_SPI_RX_DMA_IT_TE,
            .rx_pre_pri = 6,
            .rx_sub_pri = 0,
            .tx_stream = KEY_SCAN_SPI_TX_DMA_STREAM,
            .tx_channel = KEY_SCAN_SPI_TX_DMA_CHANNEL,
            .tx_it_tc = KEY_SCAN_SPI_TX_DMA_IT_TC,
        },
    [SPI_ID_WS2812_LED] =
        {
            .inst = WS2812_LED_SPI,
            .base_clk_cmd = WS2812_LED_SPI_CLK_INIT,
            .base_clk = WS2812_LED_SPI_CLK,
            /* 42MHz/16=2.625MHz，4 SPI bit 编码 1 个 WS2812 bit：
             * 1码=0b1100（高762ns>580）、0码=0b1000（高381ns<470），每灯12字节 */
            .prescaler = SPI_BaudRatePrescaler_16,
            .gpio_clk = WS2812_LED_SPI_SCK_GPIO_CLK | WS2812_LED_SPI_MISO_GPIO_CLK | WS2812_LED_SPI_MOSI_GPIO_CLK |
                        WS2812_LED_CS_GPIO_CLK,
            .sck = {WS2812_LED_SPI_SCK_GPIO_PORT, WS2812_LED_SPI_SCK_GPIO_PIN, WS2812_LED_SPI_SCK_PINSOURCE,
                    WS2812_LED_SPI_SCK_AF},
            .miso = {WS2812_LED_SPI_MISO_GPIO_PORT, WS2812_LED_SPI_MISO_GPIO_PIN, WS2812_LED_SPI_MISO_PINSOURCE,
                     WS2812_LED_SPI_MISO_AF},
            .mosi = {WS2812_LED_SPI_MOSI_GPIO_PORT, WS2812_LED_SPI_MOSI_GPIO_PIN, WS2812_LED_SPI_MOSI_PINSOURCE,
                     WS2812_LED_SPI_MOSI_AF},
            .cs_port = WS2812_LED_CS_GPIO_PORT, /* INVALID：无 CS，见 bsp_spi.h 注释 */
            .cs_pin = WS2812_LED_CS_GPIO_PIN,
            .dma_clk = WS2812_LED_SPI_DMA_CLK,
            .rx_stream = WS2812_LED_SPI_RX_DMA_STREAM,
            .rx_channel = WS2812_LED_SPI_RX_DMA_CHANNEL,
            .rx_irqn = WS2812_LED_SPI_RX_DMA_IRQn,
            .rx_it_tc = WS2812_LED_SPI_RX_DMA_IT_TC,
            .rx_it_te = WS2812_LED_SPI_RX_DMA_IT_TE,
            .rx_pre_pri = 6,
            .rx_sub_pri = 0,
            .tx_stream = WS2812_LED_SPI_TX_DMA_STREAM,
            .tx_channel = WS2812_LED_SPI_TX_DMA_CHANNEL,
            .tx_it_tc = WS2812_LED_SPI_TX_DMA_IT_TC,
        },
};

/* DMA同步相关变量 */
static uint8_t s_dummy_tx = 0xFF; /* DMA接收时发送dummy字节产生时钟 */
static uint8_t s_dummy_rx;        /* DMA发送时丢弃接收数据 */
/* ISR访问的同步指针（按 spi_id 索引，多路 SPI 设备各自 DMA 并发互不覆盖；
 * 单例指针的时代已过去：两路 SPI 并发时后者会覆盖前者的 sync，
 * 前者的 TC 中断会唤醒错误的等待者） */
static const spi_dma_sync_t *s_spi_dma_sync_ptrs[SPI_ID_MAX] = {0};

/**
 * @brief  取设备硬件配置（id 非法或该 ID 未配置时返回 NULL）
 */
static const spi_hw_config_t *spi_get_cfg(spi_id_e id)
{
    if (id >= SPI_ID_MAX || s_spi_cfg[id].inst == NULL)
    {
        return NULL;
    }
    return &s_spi_cfg[id];
}

/**
 * @brief  SPI的GPIO配置（所有 ID 共享一条路径）
 * @param  cfg: 该设备的硬件配置
 */
static void spi_gpio_config(const spi_hw_config_t *cfg)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启全部相关 GPIO 时钟。
     * 时钟使能函数故意不进配置表：F4 上所有 GPIO 端口焊死在 AHB1，无设备间差异，
     * 参数化常量只会掩盖"只有 SPI 本体时钟（APB1/APB2）真正可变"这一事实 */
    RCC_AHB1PeriphClockCmd(cfg->gpio_clk, ENABLE);

    /* CS 配置：GPIO_Mode_OUT（软件控制 CS，非 AF），初始拉高（失能）。
     * 无 CS 的器件（如 WS2812 单线器件）填 SPI_GPIO_PORT_INVALID 跳过 */
    if (cfg->cs_port != SPI_GPIO_PORT_INVALID)
    {
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Pin = cfg->cs_pin;
        GPIO_Init(cfg->cs_port, &GPIO_InitStructure);
        GPIO_SetBits(cfg->cs_port, cfg->cs_pin);
    }

    /* SCK/MISO/MOSI 配置为 AF。逐引脚守卫：未使用的引脚填
     * SPI_GPIO_PORT_INVALID（如 WS2812 只用 MOSI，SCK/MISO 释放另用），
     * 每个引脚独立判断，互不牵连 */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    if (cfg->sck.port != SPI_GPIO_PORT_INVALID)
    {
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Pin = cfg->sck.pin;
        GPIO_Init(cfg->sck.port, &GPIO_InitStructure);
        GPIO_PinAFConfig(cfg->sck.port, cfg->sck.pinsrc, cfg->sck.af);
    }
    if (cfg->miso.port != SPI_GPIO_PORT_INVALID)
    {
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Pin = cfg->miso.pin;
        GPIO_Init(cfg->miso.port, &GPIO_InitStructure);
        GPIO_PinAFConfig(cfg->miso.port, cfg->miso.pinsrc, cfg->miso.af);
    }
    if (cfg->mosi.port != SPI_GPIO_PORT_INVALID)
    {
        /* 全双工发 dummy 时也需 MOSI；单线器件（WS2812）只有这个引脚是本体的 */
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
        GPIO_InitStructure.GPIO_Pin = cfg->mosi.pin;
        GPIO_Init(cfg->mosi.port, &GPIO_InitStructure);
        GPIO_PinAFConfig(cfg->mosi.port, cfg->mosi.pinsrc, cfg->mosi.af);
    }
}

/**
 * @brief  SPI基础配置（所有 ID 共享一条路径）
 * @note   双线全双工、主模式、8位、模式0、MSB；只有分频随设备配置
 * @param  cfg: 该设备的硬件配置
 */
static void spi_base_config(const spi_hw_config_t *cfg)
{
    SPI_InitTypeDef SPI_InitStructure;

    /* 开启 SPI 时钟（APB1 或 APB2 由描述符中的函数指针决定） */
    cfg->base_clk_cmd(cfg->base_clk, ENABLE);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = cfg->prescaler;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 0;
    SPI_Init(cfg->inst, &SPI_InitStructure);

    SPI_Cmd(cfg->inst, ENABLE);

    /* 不在此处使能SPI DMA请求，使用DMA时才使能，避免空闲时误触发 */
}

/**
 * @brief  SPI RX DMA 的 NVIC 配置（所有 ID 共享一条路径）
 * @param  cfg: 该设备的硬件配置
 */
static void spi_dma_nvic_config(const spi_hw_config_t *cfg)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = cfg->rx_irqn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = cfg->rx_pre_pri;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = cfg->rx_sub_pri;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief  SPI初始化
 * @param  id:SPI设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_spi_init(spi_id_e id)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    spi_gpio_config(cfg);
    spi_base_config(cfg);
    spi_dma_nvic_config(cfg);
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI片选使能/失能（CS 低有效）
 * @param  id:SPI设备号
 * @param  state:SPI_STATE_ENABLE 拉低选中 / SPI_STATE_DISABLE 拉高释放
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_spi_control(spi_id_e id, spi_control_e state)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    /* 无 CS 的器件（SPI_GPIO_PORT_INVALID）：片选是空操作，直接成功，
     * 防止对 NULL 端口做 GPIO_SetBits 触发 HardFault */
    if (cfg->cs_port == SPI_GPIO_PORT_INVALID)
    {
        return BSP_STAT_TRUE;
    }
    if (SPI_STATE_ENABLE == state)
    {
        GPIO_ResetBits(cfg->cs_port, cfg->cs_pin); /* CS 低有效：拉低选中 */
    }
    else
    {
        GPIO_SetBits(cfg->cs_port, cfg->cs_pin);
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI发送一个字节（纯传输，CS由调用方管理）
 * @param  id:SPI设备号
 * @param  send_data:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_byte(spi_id_e id, uint8_t send_data)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    uint32_t stm32f4_timeout;
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    /* 等待发送数据寄存器为空 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_TXE) != SET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
    SPI_SendData(cfg->inst, send_data);
    /* 等待接收数据寄存器非空，并且假读取用于清空标志位 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_RXNE) != SET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    (void)SPI_ReceiveData(cfg->inst);
    /* 等待BSY寄存器为0 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_BSY) != RESET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI读取一个字节（纯传输，CS由调用方管理）
 * @param  id:SPI设备号
 * @param  receive_data:要保存读取字节的地址
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_receive_byte(spi_id_e id, uint8_t *receive_data)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    uint32_t stm32f4_timeout;
    uint8_t dummy_send = 0XFF;
    if (receive_data == NULL)
    {
        return BSP_STAT_INVALID_PARAMS;
    }
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    /* 等待发送数据寄存器为空，并且假写用于启动时钟 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_TXE) != SET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
    SPI_SendData(cfg->inst, dummy_send);
    /* 等待接收数据寄存器非空 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_RXNE) != SET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    *receive_data = SPI_ReceiveData(cfg->inst);
    /* 等待BSY寄存器为0 */
    stm32f4_timeout = SPI_TIME_OUT;
    while (SPI_GetFlagStatus(cfg->inst, SPI_FLAG_BSY) != RESET)
    {
        if (stm32f4_timeout-- == 0)
        {
            return BSP_STAT_TIME_OUT;
        }
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  配置TX DMA流（辅助函数）
 * @param  cfg: 该设备的硬件配置
 * @param  mem_addr: 内存地址
 * @param  data_size: 数据长度
 * @param  mem_inc: 内存地址是否递增
 */
static void spi_tx_dma_config(const spi_hw_config_t *cfg, const uint8_t *mem_addr, uint32_t data_size, bool mem_inc)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 同一个DMA流同一时刻只能使能一个通道，配置前必须先失能 */
    DMA_Cmd(cfg->tx_stream, DISABLE);
    /* 等待流完全停止（硬件有延迟，写EN=0后需要几个DMA时钟周期才真正停止） */
    {
        uint32_t dma_stop_timeout = SPI_TIME_OUT;
        while (DMA_GetCmdStatus(cfg->tx_stream) != DISABLE)
        {
            if (dma_stop_timeout-- == 0)
                break;
        }
    }
    /* 复位流的所有寄存器到默认值，防止上次配置残留 */
    DMA_DeInit(cfg->tx_stream);
    /* 清除上次传输遗留的中断标志位，避免配置后立即误触发中断 */
    DMA_ClearITPendingBit(cfg->tx_stream, cfg->tx_it_tc);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(cfg->inst->DR)); // 外设基址：SPI数据寄存器
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)mem_addr;              // 存储器地址（发送数据源）
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;                  // 方向：内存->外设（发送）
    DMA_InitStructure.DMA_BufferSize = data_size;                            // 传输数据个数
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;         // 外设地址不递增（DR只有一个）
    DMA_InitStructure.DMA_MemoryInc = mem_inc ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable; // 内存地址递增/不递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;                   // 外设数据宽度：字节
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;                           // 内存数据宽度：字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                                             // 单次传输模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                                       // 优先级：高
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;                                    // 直连模式（不用FIFO）
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;   // FIFO阈值（直连模式下无效）
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;         // 单次突发
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // 单次突发
    DMA_InitStructure.DMA_Channel = cfg->tx_channel;                    // DMA通道（通道存在于流中）
    DMA_Init(cfg->tx_stream, &DMA_InitStructure);                       // 初始化TX DMA流
}

/**
 * @brief  配置RX DMA流（辅助函数）
 * @param  cfg: 该设备的硬件配置
 * @param  mem_addr: 内存地址
 * @param  data_size: 数据长度
 * @param  mem_inc: 内存地址是否递增
 */
static void spi_rx_dma_config(const spi_hw_config_t *cfg, uint8_t *mem_addr, uint32_t data_size, bool mem_inc)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 同一个DMA流同一时刻只能使能一个通道，配置前必须先失能 */
    DMA_Cmd(cfg->rx_stream, DISABLE);
    /* 等待流完全停止（硬件有延迟，写EN=0后需要几个DMA时钟周期才真正停止） */
    {
        uint32_t dma_stop_timeout = SPI_TIME_OUT;
        while (DMA_GetCmdStatus(cfg->rx_stream) != DISABLE)
        {
            if (dma_stop_timeout-- == 0)
                break;
        }
    }
    /* 复位流的所有寄存器到默认值，防止上次配置残留 */
    DMA_DeInit(cfg->rx_stream);
    /* 清除上次传输遗留的中断标志位，避免配置后立即误触发中断 */
    DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_tc);
    DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_te);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(cfg->inst->DR)); // 外设基址：SPI数据寄存器
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)mem_addr;              // 存储器地址（接收数据目标）
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;                  // 方向：外设->内存（接收）
    DMA_InitStructure.DMA_BufferSize = data_size;                            // 传输数据个数
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;         // 外设地址不递增（DR只有一个）
    DMA_InitStructure.DMA_MemoryInc = mem_inc ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable; // 内存地址递增/不递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;                   // 外设数据宽度：字节
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;                           // 内存数据宽度：字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                                             // 单次传输模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                                       // 优先级：高
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;                                    // 直连模式（不用FIFO）
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;   // FIFO阈值（直连模式下无效）
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;         // 单次突发
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // 单次突发
    DMA_InitStructure.DMA_Channel = cfg->rx_channel;                    // DMA通道（通道存在于流中）
    DMA_Init(cfg->rx_stream, &DMA_InitStructure);                       // 初始化RX DMA流
}

/**
 * @brief  DMA传输完成后的清理
 * @note   cleanup 完成后（TC/TE 中断已关 + 标志已清），ISR 不可能再给出
 *         通知令牌 -- 这是超时路径"cleanup 后排空令牌"能关死竞态窗口的前提
 */
static void spi_dma_cleanup(spi_id_e id)
{
    const spi_hw_config_t *cfg = &s_spi_cfg[id];

    /* 关闭SPI的DMA请求，防止DMA流已停止但SPI仍在发出无人响应的请求 */
    SPI_I2S_DMACmd(cfg->inst, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);
    /* 关闭RX DMA传输完成/错误中断，和传输前的 DMA_ITConfig ENABLE 对称 */
    DMA_ITConfig(cfg->rx_stream, DMA_IT_TC | DMA_IT_TE, DISABLE);
    /* 停止TX/RX DMA流，下次传输前会重新配置 */
    DMA_Cmd(cfg->tx_stream, DISABLE);
    DMA_Cmd(cfg->rx_stream, DISABLE);
    /* 清除DMA中断标志位，防止残留标志导致下次误触发 */
    DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_tc);
    DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_te);
    DMA_ClearITPendingBit(cfg->tx_stream, cfg->tx_it_tc);
    /* 清除同步指针，ISR不再访问已完成的sync */
    s_spi_dma_sync_ptrs[id] = NULL;
}

/**
 * @brief  SPI DMA发送多个字节（全双工，sync同步）
 * @note   TX DMA发送用户数据，RX DMA丢弃接收数据
 * @param  id:SPI设备号
 * @param  send_data:发送数据缓冲区（必须在主 SRAM，DMA 够不着 CCMRAM）
 * @param  data_size:数据长度
 * @param  sync:DMA同步机制（NULL时不等待）
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_send_multi_data_dma(spi_id_e id, const uint8_t *send_data, uint32_t data_size,
                                            const spi_dma_sync_t *sync)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }

    /* DMA1/DMA2 在 F4 上均焊死 AHB1，使能函数不进配置表（理由同 GPIO 时钟）。
     * 注意：此处开的是 DMA 控制器时钟；SPI 本体时钟走 cfg->base_clk_cmd（APB1/APB2 随设备变） */
    RCC_AHB1PeriphClockCmd(cfg->dma_clk, ENABLE);

    /* 保存sync指针供ISR使用（按 id 索引，多设备不冲突） */
    s_spi_dma_sync_ptrs[id] = sync;

    /* 配置TX DMA：发送用户数据 */
    spi_tx_dma_config(cfg, send_data, data_size, true);

    /* 配置RX DMA：丢弃接收数据（全双工必须同时配置） */
    spi_rx_dma_config(cfg, &s_dummy_rx, data_size, false);

    /* 使能SPI DMA请求 */
    SPI_I2S_DMACmd(cfg->inst, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

    /* 使能RX DMA传输完成/错误中断（TC=完成通知；TE=总线错误，不用等满超时） */
    DMA_ITConfig(cfg->rx_stream, DMA_IT_TC | DMA_IT_TE, ENABLE);

    /* 先启动RX DMA，再启动TX DMA */
    DMA_Cmd(cfg->rx_stream, ENABLE);
    DMA_Cmd(cfg->tx_stream, ENABLE);

    /* 阻塞等待DMA完成，带超时保护 */
    if (sync && sync->wait)
    {
        if (!sync->wait(sync->handle, 100))
        {
            spi_dma_cleanup(id);
            /* cleanup 后中断源已熄灭，不可能再产生新令牌；
             * 排空熄灭前竞态窗口漏进来的陈旧令牌，否则下一轮 wait
             * 会立即取到旧令牌"假成功"，把半成品缓冲区当有效数据返回 */
            while (sync->wait(sync->handle, 0))
            {
            }
            return BSP_STAT_TIME_OUT;
        }
    }

    spi_dma_cleanup(id);
    return BSP_STAT_TRUE;
}

/**
 * @brief  SPI DMA接收多个字节（全双工，TX发送dummy产生时钟，sync同步）
 * @note   TX DMA发送dummy(0xFF)产生时钟，RX DMA接收数据到用户缓冲区
 * @param  id:SPI设备号
 * @param  receive_data:接收数据缓冲区（必须在主 SRAM，DMA 够不着 CCMRAM）
 * @param  data_size:数据长度
 * @param  sync:DMA同步机制（NULL时不等待）
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e spi_receive_multi_data_dma(spi_id_e id, uint8_t *receive_data, uint32_t data_size,
                                               const spi_dma_sync_t *sync)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    if (cfg == NULL)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }

    /* DMA1/DMA2 在 F4 上均焊死 AHB1，使能函数不进配置表（理由同 GPIO 时钟）。
     * 注意：此处开的是 DMA 控制器时钟；SPI 本体时钟走 cfg->base_clk_cmd（APB1/APB2 随设备变） */
    RCC_AHB1PeriphClockCmd(cfg->dma_clk, ENABLE);

    /* 保存sync指针供ISR使用（按 id 索引，多设备不冲突） */
    s_spi_dma_sync_ptrs[id] = sync;

    /* 配置TX DMA：发送dummy字节产生时钟 */
    spi_tx_dma_config(cfg, &s_dummy_tx, data_size, false);

    /* 配置RX DMA：接收数据到用户缓冲区 */
    spi_rx_dma_config(cfg, receive_data, data_size, true);

    /* 使能SPI DMA请求 */
    SPI_I2S_DMACmd(cfg->inst, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

    /* 使能RX DMA传输完成/错误中断（TC=完成通知；TE=总线错误，不用等满超时） */
    DMA_ITConfig(cfg->rx_stream, DMA_IT_TC | DMA_IT_TE, ENABLE);

    /* 先启动RX DMA，再启动TX DMA */
    DMA_Cmd(cfg->rx_stream, ENABLE);
    DMA_Cmd(cfg->tx_stream, ENABLE);

    /* 阻塞等待DMA完成，带超时保护 */
    if (sync && sync->wait)
    {
        if (!sync->wait(sync->handle, 100))
        {
            spi_dma_cleanup(id);
            /* cleanup 后中断源已熄灭，不可能再产生新令牌；
             * 排空熄灭前竞态窗口漏进来的陈旧令牌，否则下一轮 wait
             * 会立即取到旧令牌"假成功"，把半成品缓冲区当有效数据返回 */
            while (sync->wait(sync->handle, 0))
            {
            }
            return BSP_STAT_TIME_OUT;
        }
    }

    spi_dma_cleanup(id);
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
 * @brief  SPI RX DMA 中断处理（封装硬件细节，ISR 只需按 id 路由调用）
 * @param  id: 该中断流所属的 SPI 设备号（由 isr 路由层传入，用于索引 sync 表）
 * @note   处理传输完成(TC)和传输错误(TE)，通知等待任务
 */
void bsp_spi_dma_isr_handler(spi_id_e id)
{
    const spi_hw_config_t *cfg = spi_get_cfg(id);
    const spi_dma_sync_t *sync = (id < SPI_ID_MAX) ? s_spi_dma_sync_ptrs[id] : NULL;

    if (cfg == NULL)
    {
        return;
    }

    /* 传输完成 */
    if (DMA_GetITStatus(cfg->rx_stream, cfg->rx_it_tc))
    {
        DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_tc);
        if (sync && sync->notify_from_isr)
        {
            sync->notify_from_isr(sync->handle);
        }
    }
    /* 传输错误（TE）- 也通知等待任务，避免永久阻塞 */
    if (DMA_GetITStatus(cfg->rx_stream, cfg->rx_it_te))
    {
        DMA_ClearITPendingBit(cfg->rx_stream, cfg->rx_it_te);
        if (sync && sync->notify_from_isr)
        {
            sync->notify_from_isr(sync->handle);
        }
    }
}
