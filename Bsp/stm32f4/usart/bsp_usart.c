/**
 ******************************************************************************
 * @file    bsp_usart.c
 * @author  Pan
 * @version V1.0
 * @date    2025-12-06
 * @brief   USART驱动
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_usart.h"

/**
 * @brief  USART的GPIO配置
 * @note   无
 * @param  id:串口设备号
 * @retval 无
 */
static void usart_gpio_config(uasrt_id_e id)
{
    /* 结构体宏定义 */
    GPIO_InitTypeDef GPIO_InitStructure;

    switch (id)
    {
    case USART_ID_DEBUG:
    {
        /* 开启时钟 */
        DEBUG_USART_GPIO_CLK_CMD(DEBUG_USART_TX_CLK | DEBUG_USART_RX_CLK, ENABLE);
        /* IO口内部用一个弱上拉增加带载能力 */
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        /* TX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_PIN;
        GPIO_Init(DEBUG_USART_TX_PORT, &GPIO_InitStructure);
        /* RX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_PIN;
        GPIO_Init(DEBUG_USART_RX_PORT, &GPIO_InitStructure);
        /* 连接 PXX 到 USARTX_Tx */
        GPIO_PinAFConfig(DEBUG_USART_TX_PORT, DEBUG_USART_TX_PINSRC, DEBUG_USART_TX_AF);
        /*  连接 PXX 到 USARTX_Rx */
        GPIO_PinAFConfig(DEBUG_USART_RX_PORT, DEBUG_USART_RX_PINSRC, DEBUG_USART_RX_AF);
    }
    break;
    case USART_ID_BLT:
    {
        /* 开启时钟 */
        BLT_USART_GPIO_CLK_CMD(BLT_USART_TX_CLK | BLT_USART_RX_CLK, ENABLE);
        /* IO口内部用一个弱上拉增加带载能力 */
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        /* TX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = BLT_USART_TX_PIN;
        GPIO_Init(BLT_USART_TX_PORT, &GPIO_InitStructure);
        /* RX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = BLT_USART_RX_PIN;
        GPIO_Init(BLT_USART_RX_PORT, &GPIO_InitStructure);
        /* 连接 PXX 到 USARTX_Tx */
        GPIO_PinAFConfig(BLT_USART_TX_PORT, BLT_USART_TX_PINSRC, BLT_USART_TX_AF);
        /*  连接 PXX 到 USARTX_Rx */
        GPIO_PinAFConfig(BLT_USART_RX_PORT, BLT_USART_RX_PINSRC, BLT_USART_RX_AF);
    }
    break;
    case USART_ID_FINGER:
    {
        /* 开启时钟 */
        FINGER_USART_GPIO_CLK_CMD(FINGER_USART_TX_CLK | FINGER_USART_RX_CLK, ENABLE);
        /* IO口内部用一个弱上拉增加带载能力 */
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        /* TX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = FINGER_USART_TX_PIN;
        GPIO_Init(FINGER_USART_TX_PORT, &GPIO_InitStructure);
        /* RX复用 */
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Pin = FINGER_USART_RX_PIN;
        GPIO_Init(FINGER_USART_RX_PORT, &GPIO_InitStructure);
        /* 连接 PXX 到 USARTX_Tx */
        GPIO_PinAFConfig(FINGER_USART_TX_PORT, FINGER_USART_TX_PINSRC, FINGER_USART_TX_AF);
        /*  连接 PXX 到 USARTX_Rx */
        GPIO_PinAFConfig(FINGER_USART_RX_PORT, FINGER_USART_RX_PINSRC, FINGER_USART_RX_AF);
    }
    break;
    default:
        break;
    }
}

/**
 * @brief  串口1的基础配置
 * @note   115200-8-1-0-No
 * @param  id:串口设备号
 * @retval 无
 */
static void usart_base_config(uasrt_id_e id)
{

    switch (id)
    {
    case USART_ID_DEBUG:
    {
        /* 结构体宏定义 */
        USART_InitTypeDef USART_InitStructure;
        /* 开启时钟 */
        DEBUG_USART_BASE_CLK_CMD(DEBUG_USART_CLK, ENABLE);
        /* 串口基础配置 115200-8-1-0-No */
        USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUD;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
        USART_Init(DEBUG_USART, &USART_InitStructure);
        /* 失能串口接收中断 */
        USART_ITConfig(DEBUG_USART, USART_IT_RXNE, DISABLE);
        // /* 使能串口空闲中断 */
        // USART_ITConfig(DEBUG_USART, USART_IT_IDLE, ENABLE);
        // /* 使能串口DMA请求 */
        // USART_DMACmd(DEBUG_USART, USART_DMAReq_Rx, ENABLE);
        /* 使能串口 */
        USART_Cmd(DEBUG_USART, ENABLE);
    }
    break;
    case USART_ID_BLT:
    {
        /* 结构体宏定义 */
        USART_InitTypeDef USART_InitStructure;
        /* 开启时钟 */
        BLT_USART_BASE_CLK_CMD(BLT_USART_CLK, ENABLE);
        /* 串口基础配置 115200-8-1-0-No */
        USART_InitStructure.USART_BaudRate = BLT_USART_BAUD;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
        USART_Init(BLT_USART, &USART_InitStructure);
        /* 失能串口接收中断 */
        USART_ITConfig(BLT_USART, USART_IT_RXNE, DISABLE);
        // /* 使能串口空闲中断 */
        // USART_ITConfig(BLT_USART, USART_IT_IDLE, ENABLE);
        // /* 使能串口DMA请求 */
        // USART_DMACmd(BLT_USART, USART_DMAReq_Rx, ENABLE);
        /* 使能串口 */
        USART_Cmd(BLT_USART, ENABLE);
    }
    break;
    case USART_ID_FINGER:
    {
        /* 结构体宏定义 */
        USART_InitTypeDef USART_InitStructure;
        /* 开启时钟 */
        FINGER_USART_BASE_CLK_CMD(FINGER_USART_CLK, ENABLE);
        /* 串口基础配置 115200-8-1-0-No */
        USART_InitStructure.USART_BaudRate = FINGER_USART_BAUD;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
        USART_Init(FINGER_USART, &USART_InitStructure);
        /* 失能串口接收中断 */
        USART_ITConfig(FINGER_USART, USART_IT_RXNE, DISABLE);
        // /* 使能串口空闲中断 */
        // USART_ITConfig(FINGER_USART, USART_IT_IDLE, ENABLE);
        // /* 使能串口DMA请求 */
        // USART_DMACmd(FINGER_USART, USART_DMAReq_Rx, ENABLE);
        /* 使能串口 */
        USART_Cmd(FINGER_USART, ENABLE);
    }
    break;
    default:
        break;
    }
}

/**
 * @brief  串口初始化
 * @note   无
 * @param  id:串口设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_uasrt_init(uasrt_id_e id)
{
    if (id >= USART_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        usart_gpio_config(id);
        usart_base_config(id);
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  串口发送一个字节
 * @note   无
 * @param  id:串口设备号
 * @param  ch:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_byte(uasrt_id_e id, uint8_t ch)
{
    switch (id)
    {
    case USART_ID_DEBUG:
    {
        /* 发送一个字节数据到USART */
        USART_SendData(DEBUG_USART, ch);

        /* 等待发送数据寄存器为空 */
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;
    }
    break;
    case USART_ID_BLT:
    {
        /* 发送一个字节数据到USART */
        USART_SendData(BLT_USART, ch);

        /* 等待发送数据寄存器为空 */
        while (USART_GetFlagStatus(BLT_USART, USART_FLAG_TXE) == RESET)
            ;
    }
    break;
    case USART_ID_FINGER:
    {
        /* 发送一个字节数据到USART */
        USART_SendData(FINGER_USART, ch);

        /* 等待发送数据寄存器为空 */
        while (USART_GetFlagStatus(FINGER_USART, USART_FLAG_TXE) == RESET)
            ;
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
 * @brief  串口发送一串字符串
 * @note   无
 * @param  id:串口设备号
 * @param  str:要发送的字符串，必须以\0结尾
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_string(uasrt_id_e id, char* str)
{
    unsigned int k = 0;
    switch (id)
    {
    case USART_ID_DEBUG:
    {
        do
        {
            stm32f4_usart_send_byte(id, *(str + k));
            k++;
        } while (*(str + k) != '\0');

        /* 等待发送完成 */
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET)
        {
        }
    }
    break;
    case USART_ID_BLT:
    {
        do
        {
            stm32f4_usart_send_byte(id, *(str + k));
            k++;
        } while (*(str + k) != '\0');

        /* 等待发送完成 */
        while (USART_GetFlagStatus(BLT_USART, USART_FLAG_TC) == RESET)
        {
        }
    }
    break;
    case USART_ID_FINGER:
    {
        do
        {
            stm32f4_usart_send_byte(id, *(str + k));
            k++;
        } while (*(str + k) != '\0');

        /* 等待发送完成 */
        while (USART_GetFlagStatus(FINGER_USART, USART_FLAG_TC) == RESET)
        {
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
 * @brief  串口发送一个hex数
 * @note   无
 * @param  id:串口设备号
 * @param  hex:要发送的半字
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_hex(uasrt_id_e id, uint16_t hex)
{
    uint8_t temp_h, temp_l;
    switch (id)
    {
    case USART_ID_DEBUG:
    {
        /* 取出高八位 */
        temp_h = (hex & 0XFF00) >> 8;
        /* 取出低八位 */
        temp_l = hex & 0XFF;

        /* 发送高八位 */
        USART_SendData(DEBUG_USART, temp_h);
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;

        /* 发送低八位 */
        USART_SendData(DEBUG_USART, temp_l);
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;
    }
    break;
    case USART_ID_BLT:
    {
        /* 取出高八位 */
        temp_h = (hex & 0XFF00) >> 8;
        /* 取出低八位 */
        temp_l = hex & 0XFF;

        /* 发送高八位 */
        USART_SendData(BLT_USART, temp_h);
        while (USART_GetFlagStatus(BLT_USART, USART_FLAG_TXE) == RESET)
            ;

        /* 发送低八位 */
        USART_SendData(BLT_USART, temp_l);
        while (USART_GetFlagStatus(BLT_USART, USART_FLAG_TXE) == RESET)
            ;
    }
    break;
    case USART_ID_FINGER:
    {
        /* 取出高八位 */
        temp_h = (hex & 0XFF00) >> 8;
        /* 取出低八位 */
        temp_l = hex & 0XFF;

        /* 发送高八位 */
        USART_SendData(FINGER_USART, temp_h);
        while (USART_GetFlagStatus(FINGER_USART, USART_FLAG_TXE) == RESET)
            ;

        /* 发送低八位 */
        USART_SendData(FINGER_USART, temp_l);
        while (USART_GetFlagStatus(FINGER_USART, USART_FLAG_TXE) == RESET)
            ;
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
 * @brief  串口发送一个u8数组
 * @note   无
 * @param  id:串口设备号
 * @param  ch:要发送的字节
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_usart_send_array(uasrt_id_e id, uint8_t* array, uint16_t num)
{
    uint8_t i;

    switch (id)
    {
    case USART_ID_DEBUG:
    {
        for (i = 0; i < num; i++)
        {
            /* 发送一个字节数据到USART */
            stm32f4_usart_send_byte(USART_ID_DEBUG, array[i]);
        }
        /* 等待发送完成 */
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET)
            ;
    }
    break;
    case USART_ID_BLT:
    {
        for (i = 0; i < num; i++)
        {
            /* 发送一个字节数据到USART */
            stm32f4_usart_send_byte(USART_ID_BLT, array[i]);
        }
        /* 等待发送完成 */
        while (USART_GetFlagStatus(BLT_USART, USART_FLAG_TC) == RESET)
            ;
    }
    break;
    case USART_ID_FINGER:
    {
        for (i = 0; i < num; i++)
        {
            /* 发送一个字节数据到USART */
            stm32f4_usart_send_byte(USART_ID_FINGER, array[i]);
        }
        /* 等待发送完成 */
        while (USART_GetFlagStatus(FINGER_USART, USART_FLAG_TC) == RESET)
            ;
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

// 重写_write()（推荐）在ARM GCC环境中，通常使用_write函数进行重定向
int _write(int file, char* ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        // 1. 发送单个字符
        USART_SendData(DEBUG_USART, (uint8_t)ptr[i]);

        // 2. 等待发送完成（确保不覆盖数据寄存器）
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;
    }
    return len;
}

// STM32F4平台USART驱动实例，实现usart_ops_t定义的统一操作接口
// [C++对照] 对应具体产品(Concrete Product)
// 注：C中无继承，具体产品与抽象产品是同一类型，区别仅为函数指针指向了具体实现（类似填好的虚表）
const usart_ops_t g_stm32f4_usart_driver_ = {
    .name = "STM32F4_USART_DRIVER",
    .init = stm32f4_uasrt_init,
    .usart_send_byte = stm32f4_usart_send_byte,
    .usart_send_string = stm32f4_usart_send_string,
    .usart_send_hex = stm32f4_usart_send_hex,
    .usart_send_array = stm32f4_usart_send_array,
};
