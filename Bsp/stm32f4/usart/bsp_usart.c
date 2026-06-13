/**
 ******************************************************************************
 * @file    bsp_usart.c
 * @author  Pan
 * @version V2.0
 * @date    2025-12-06
 * @brief   USART驱动
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 * FIX-15: 用配置描述符表消除每函数 3 段 switch-case 的 copy-paste。
 * 新增 USART 只需在 s_usart_cfg[] 加一行配置。
 ******************************************************************************
 */

#include "bsp_usart.h"

/* USART 时钟使能函数类型（RCC_APB1PeriphClockCmd / RCC_APB2PeriphClockCmd 同签名） */
typedef void (*usart_clk_cmd_fn)(uint32_t, FunctionalState);

/* USART 硬件配置描述符：把每个串口的寄存器/引脚/时钟全部集中到一处 */
typedef struct
{
    USART_TypeDef     *inst;        /* USART 寄存器基址 */
    usart_clk_cmd_fn   base_clk_cmd;/* USART 时钟使能函数（APB1 或 APB2） */
    uint32_t           base_clk;    /* USART 时钟外设号 */
    uint32_t           baud;        /* 波特率 */

    GPIO_TypeDef      *tx_port;     /* TX GPIO 端口 */
    uint16_t           tx_pin;      /* TX 引脚号 */
    uint16_t           tx_pinsrc;   /* TX 引脚源（AF 配置用） */
    uint8_t            tx_af;       /* TX 复用功能号 */
    uint32_t           tx_clk;      /* TX GPIO 时钟外设号 */

    GPIO_TypeDef      *rx_port;     /* RX GPIO 端口 */
    uint16_t           rx_pin;      /* RX 引脚号 */
    uint16_t           rx_pinsrc;   /* RX 引脚源（AF 配置用） */
    uint8_t            rx_af;       /* RX 复用功能号 */
    uint32_t           rx_clk;      /* RX GPIO 时钟外设号 */
} usart_hw_config_t;

/* 每个逻辑 ID 对应一份硬件配置（新增串口只需在此加一行） */
static const usart_hw_config_t s_usart_cfg[USART_ID_MAX] = {
    [USART_ID_DEBUG] = {
        .inst = DEBUG_USART, .base_clk_cmd = DEBUG_USART_BASE_CLK_CMD, .base_clk = DEBUG_USART_CLK, .baud = DEBUG_USART_BAUD,
        .tx_port = DEBUG_USART_TX_PORT, .tx_pin = DEBUG_USART_TX_PIN, .tx_pinsrc = DEBUG_USART_TX_PINSRC, .tx_af = DEBUG_USART_TX_AF, .tx_clk = DEBUG_USART_TX_CLK,
        .rx_port = DEBUG_USART_RX_PORT, .rx_pin = DEBUG_USART_RX_PIN, .rx_pinsrc = DEBUG_USART_RX_PINSRC, .rx_af = DEBUG_USART_RX_AF, .rx_clk = DEBUG_USART_RX_CLK,
    },
    [USART_ID_BLT] = {
        .inst = BLT_USART, .base_clk_cmd = BLT_USART_BASE_CLK_CMD, .base_clk = BLT_USART_CLK, .baud = BLT_USART_BAUD,
        .tx_port = BLT_USART_TX_PORT, .tx_pin = BLT_USART_TX_PIN, .tx_pinsrc = BLT_USART_TX_PINSRC, .tx_af = BLT_USART_TX_AF, .tx_clk = BLT_USART_TX_CLK,
        .rx_port = BLT_USART_RX_PORT, .rx_pin = BLT_USART_RX_PIN, .rx_pinsrc = BLT_USART_RX_PINSRC, .rx_af = BLT_USART_RX_AF, .rx_clk = BLT_USART_RX_CLK,
    },
    [USART_ID_FINGER] = {
        .inst = FINGER_USART, .base_clk_cmd = FINGER_USART_BASE_CLK_CMD, .base_clk = FINGER_USART_CLK, .baud = FINGER_USART_BAUD,
        .tx_port = FINGER_USART_TX_PORT, .tx_pin = FINGER_USART_TX_PIN, .tx_pinsrc = FINGER_USART_TX_PINSRC, .tx_af = FINGER_USART_TX_AF, .tx_clk = FINGER_USART_TX_CLK,
        .rx_port = FINGER_USART_RX_PORT, .rx_pin = FINGER_USART_RX_PIN, .rx_pinsrc = FINGER_USART_RX_PINSRC, .rx_af = FINGER_USART_RX_AF, .rx_clk = FINGER_USART_RX_CLK,
    },
};

/**
 * @brief  USART的GPIO配置（所有 ID 共享一条路径）
 * @param  cfg: 指向该串口的硬件配置
 */
static void usart_gpio_config(const usart_hw_config_t *cfg)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启 TX/RX GPIO 时钟 */
    RCC_AHB1PeriphClockCmd(cfg->tx_clk | cfg->rx_clk, ENABLE);

    /* IO口内部用一个弱上拉增加带载能力 */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* TX 复用 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = cfg->tx_pin;
    GPIO_Init(cfg->tx_port, &GPIO_InitStructure);
    /* RX 复用 */
    GPIO_InitStructure.GPIO_Pin = cfg->rx_pin;
    GPIO_Init(cfg->rx_port, &GPIO_InitStructure);

    /* 连接引脚到 USART 复用功能 */
    GPIO_PinAFConfig(cfg->tx_port, cfg->tx_pinsrc, cfg->tx_af);
    GPIO_PinAFConfig(cfg->rx_port, cfg->rx_pinsrc, cfg->rx_af);
}

/**
 * @brief  USART基础配置（所有 ID 共享一条路径）
 * @note   115200-8-1-0-No
 * @param  cfg: 指向该串口的硬件配置
 */
static void usart_base_config(const usart_hw_config_t *cfg)
{
    USART_InitTypeDef USART_InitStructure;

    /* 开启 USART 时钟（APB1 或 APB2 由描述符中的函数指针决定） */
    cfg->base_clk_cmd(cfg->base_clk, ENABLE);

    /* 串口基础配置 115200-8-1-0-No */
    USART_InitStructure.USART_BaudRate = cfg->baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(cfg->inst, &USART_InitStructure);
    /* 失能串口接收中断 */
    USART_ITConfig(cfg->inst, USART_IT_RXNE, DISABLE);
    /* 使能串口 */
    USART_Cmd(cfg->inst, ENABLE);
}

/**
 * @brief  串口初始化
 * @param  id:串口设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_init(usart_id_e id)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    usart_gpio_config(cfg);
    usart_base_config(cfg);
    return BSP_STAT_TRUE;
}

/**
 * @brief  串口发送一个字节
 * @param  id:串口设备号
 * @param  ch:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_byte(usart_id_e id, uint8_t ch)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    USART_SendData(cfg->inst, ch);
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TXE) == RESET)
        ;
    return BSP_STAT_TRUE;
}

/**
 * @brief  串口发送一串字符串
 * @param  id:串口设备号
 * @param  str:要发送的字符串，必须以\0结尾
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_string(usart_id_e id, char* str)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    unsigned int k = 0;
    while (str[k] != '\0')
    {
        stm32f4_usart_send_byte(id, str[k]);
        k++;
    }
    /* 等待发送完成 */
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TC) == RESET)
        ;
    return BSP_STAT_TRUE;
}

/**
 * @brief  串口发送一个hex数
 * @param  id:串口设备号
 * @param  hex:要发送的半字
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_hex(usart_id_e id, uint16_t hex)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    uint8_t temp_h = (hex & 0xFF00) >> 8;
    uint8_t temp_l = hex & 0xFF;
    /* 发送高八位 */
    USART_SendData(cfg->inst, temp_h);
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TXE) == RESET)
        ;
    /* 发送低八位 */
    USART_SendData(cfg->inst, temp_l);
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TXE) == RESET)
        ;
    return BSP_STAT_TRUE;
}

/**
 * @brief  串口发送一个u8数组
 * @param  id:串口设备号
 * @param  array:要发送的数组
 * @param  num:数组长度
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_array(usart_id_e id, uint8_t* array, uint16_t num)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    uint16_t i;
    for (i = 0; i < num; i++)
    {
        stm32f4_usart_send_byte(id, array[i]);
    }
    /* 等待发送完成 */
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TC) == RESET)
        ;
    return BSP_STAT_TRUE;
}

// _write() 已移到 Core/src/syscalls.c，通过 BSP 抽象层发送（FIX-11）

// STM32F4平台USART驱动实例，实现usart_ops_t定义的统一操作接口
// [C++对照] 对应具体产品(Concrete Product)
// 注：C中无继承，具体产品与抽象产品是同一类型，区别仅为函数指针指向了具体实现（类似填好的虚表）
const usart_ops_t g_stm32f4_usart_driver_ = {
    .name = "STM32F4_USART_DRIVER",
    .init = stm32f4_usart_init,
    .usart_send_byte = stm32f4_usart_send_byte,
    .usart_send_string = stm32f4_usart_send_string,
    .usart_send_hex = stm32f4_usart_send_hex,
    .usart_send_array = stm32f4_usart_send_array,
};
