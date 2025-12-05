/**
 ******************************************************************************
 * @file    bsp_led.c
 * @author  Pan
 * @version V1.0
 * @date    2025-08-06
 * @brief   串口
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */
#include "bsp_debug_usart.h"
//  #include "protocol.h"

/**
 * @brief  DEBUG串口的GPIO配置
 * @note   无
 * @param  无
 * @retval 无
 */
static void USART_GPIO_Config(void)
{
    /* 结构体宏定义 */
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启时钟 */
    RCC_AHB1PeriphClockCmd(DEBUG_USART_TX_CLK | DEBUG_USART_RX_CLK, ENABLE);

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

/**
 * @brief  串口1的基础配置
 * @note   115200-8-1-0-No
 * @param  无
 * @retval 无
 */
static void USART_Base_Config(void)
{
    /* 结构体宏定义 */
    USART_InitTypeDef USART_InitStructure;

    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(DEBUG_USART_CLK, ENABLE);

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

    /* 使能串口空闲中断 */
    USART_ITConfig(DEBUG_USART, USART_IT_IDLE, ENABLE);

    /* 使能串口DMA请求 */
    USART_DMACmd(DEBUG_USART, USART_DMAReq_Rx, ENABLE);

    /* 使能串口 */
    USART_Cmd(DEBUG_USART, ENABLE);
}

/**
 * @brief  串口配置
 * @note   无
 * @param  无
 * @retval 无
 */
void Debug_USART_Init(void)
{
    USART_GPIO_Config();
    USART_Base_Config();
}

// 重写_write()（推荐）
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

// /*****************  发送一个字符 **********************/
// void Usart_SendByte(USART_TypeDef* pUSARTx, uint8_t ch)
// {
//     /* 发送一个字节数据到USART */
//     USART_SendData(pUSARTx, ch);

//     /* 等待发送数据寄存器为空 */
//     while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET)
//         ;
// }

// /*****************  发送字符串 **********************/
// void Usart_SendString(USART_TypeDef* pUSARTx, char* str)
// {
//     unsigned int k = 0;
//     do
//     {
//         Usart_SendByte(pUSARTx, *(str + k));
//         k++;
//     } while (*(str + k) != '\0');

//     /* 等待发送完成 */
//     while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET)
//     {
//     }
// }

// /*****************  发送一个16位数 **********************/
// void Usart_SendHalfWord(USART_TypeDef* pUSARTx, uint16_t ch)
// {
//     uint8_t temp_h, temp_l;

//     /* 取出高八位 */
//     temp_h = (ch & 0XFF00) >> 8;
//     /* 取出低八位 */
//     temp_l = ch & 0XFF;

//     /* 发送高八位 */
//     USART_SendData(pUSARTx, temp_h);
//     while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET)
//         ;

//     /* 发送低八位 */
//     USART_SendData(pUSARTx, temp_l);
//     while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET)
//         ;
// }

// /****************** 发送8位的数组 ************************/
// void DATA_USART_SendArray(USART_TypeDef* pUSARTx, uint8_t* array, uint16_t num)
// {
//     uint8_t i;

//     for (i = 0; i < num; i++)
//     {
//         /* 发送一个字节数据到USART */
//         Usart_SendByte(pUSARTx, array[i]);
//     }
//     /* 等待发送完成 */
//     while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET)
//         ;
// }

/// 避免使用半主机模式，不勾选use Microlib时需要使用以下代码,
// 勾选的话在使用LVGL时会出现Undefined Symbol
// __aeabi_assert，在魔术棒中宏定义添加NDEBUG，即不使用assert函数即可 #pragma
// import(__use_no_semihosting) void _sys_exit(int x)
//{
//   x = x;
// }
// struct __FILE
//{
//   int handle;

//};

// FILE __stdout;

// /// 重定向c库函数printf到串口，重定向后可使用printf函数
// int fputc(int ch, FILE* f)
// {
//     /* 发送一个字节数据到串口 */
//     USART_SendData(DEBUG_USART, (uint8_t)ch);

//     /* 等待发送完毕 */
//     while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
//         ;

//     return (ch);
// }

/*********************************************END OF FILE**********************/
