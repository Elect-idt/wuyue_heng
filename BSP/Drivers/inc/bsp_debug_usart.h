#ifndef __BSP_DEBUG_USART_H
#define __BSP_DEBUG_USART_H

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include <stdio.h>

/******  开关中断响应宏定义 ******/
#define OFF_IT                        __set_PRIMASK(1)
#define ON_IT                         __set_PRIMASK(0)

/******  DEBUG_USART_TX引脚时钟端口、引脚宏定义 ******/
#define DEBUG_USART_TX_CLK        RCC_AHB1Periph_GPIOA
#define DEBUG_USART_TX_PORT       GPIOA
#define DEBUG_USART_TX_PIN        GPIO_Pin_9
#define DEBUG_USART_TX_PINSRC     GPIO_PinSource9
#define DEBUG_USART_TX_AF         GPIO_AF_USART1

/******  DEBUG_USART_RX引脚时钟端口、引脚宏定义 ******/
#define DEBUG_USART_RX_CLK        RCC_AHB1Periph_GPIOA
#define DEBUG_USART_RX_PORT       GPIOA
#define DEBUG_USART_RX_PIN        GPIO_Pin_10
#define DEBUG_USART_RX_PINSRC     GPIO_PinSource10
#define DEBUG_USART_RX_AF         GPIO_AF_USART1

/****** 串口相关配置宏定义 ******/
#define DEBUG_USART               USART1
#define DEBUG_USART_CLK           RCC_APB2Periph_USART1
#define DEBUG_USART_BAUD          115200

/****** 串口接收DMA宏定义 ******/
#define DEBUG_USART_DMA_CLK               RCC_AHB1Periph_DMA2        
#define DEBUG_USART_DMA_CHANNEL           DMA_Channel_4
#define DEBUG_USART_DMA_STREAM            DMA2_Stream5

/****** 串口空闲中断宏定义 ******/
#define DEBUG_USART_IDLE_IRQn             USART1_IRQn
#define DEBUG_USART_IDLE_DMA_IRQHandler   USART1_IRQHandler

/****** API函数 ******/
extern void Debug_USART_Init(void);

#endif

