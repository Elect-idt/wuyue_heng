#ifndef __BSP_USART_H_
#define __BSP_USART_H_

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include "bsp_usart_interface.h"

/* 忙等超时保护（与 SPI_TIME_OUT 对称）：外设异常（时钟/引脚配置错、总线错误）
 * 时避免任务死锁在 while 等待里，保住 printf 诊断通道不僵死 */
#define USART_TIME_OUT  ((uint32_t)0x1000)

/******  DEBUG_USART_TX引脚时钟端口、引脚宏定义 ******/
#define DEBUG_USART_GPIO_CLK_CMD  RCC_AHB1PeriphClockCmd
#define DEBUG_USART_TX_CLK        RCC_AHB1Periph_GPIOC
#define DEBUG_USART_TX_PORT       GPIOC
#define DEBUG_USART_TX_PIN        GPIO_Pin_10
#define DEBUG_USART_TX_PINSRC     GPIO_PinSource10
#define DEBUG_USART_TX_AF         GPIO_AF_USART3

/******  DEBUG_USART_RX引脚时钟端口、引脚宏定义 ******/
#define DEBUG_USART_RX_CLK        RCC_AHB1Periph_GPIOC
#define DEBUG_USART_RX_PORT       GPIOC
#define DEBUG_USART_RX_PIN        GPIO_Pin_11
#define DEBUG_USART_RX_PINSRC     GPIO_PinSource11
#define DEBUG_USART_RX_AF         GPIO_AF_USART3

/****** 串口相关配置宏定义 ******/
#define DEBUG_USART_BASE_CLK_CMD  RCC_APB1PeriphClockCmd
#define DEBUG_USART               USART3
#define DEBUG_USART_CLK           RCC_APB1Periph_USART3
#define DEBUG_USART_BAUD          115200

/******  BLT_USART_TX引脚时钟端口、引脚宏定义 ******/
#define BLT_USART_GPIO_CLK_CMD  RCC_AHB1PeriphClockCmd
#define BLT_USART_TX_CLK        RCC_AHB1Periph_GPIOA
#define BLT_USART_TX_PORT       GPIOA
#define BLT_USART_TX_PIN        GPIO_Pin_2
#define BLT_USART_TX_PINSRC     GPIO_PinSource2
#define BLT_USART_TX_AF         GPIO_AF_USART2

/******  BLT_USART_RX引脚时钟端口、引脚宏定义 ******/
#define BLT_USART_RX_CLK        RCC_AHB1Periph_GPIOA
#define BLT_USART_RX_PORT       GPIOA
#define BLT_USART_RX_PIN        GPIO_Pin_3
#define BLT_USART_RX_PINSRC     GPIO_PinSource3
#define BLT_USART_RX_AF         GPIO_AF_USART2

/****** 串口相关配置宏定义 ******/
#define BLT_USART_BASE_CLK_CMD  RCC_APB1PeriphClockCmd
#define BLT_USART               USART2
#define BLT_USART_CLK           RCC_APB1Periph_USART2
#define BLT_USART_BAUD          115200

/******  FINGER_USART_TX引脚时钟端口、引脚宏定义 ******/
#define FINGER_USART_GPIO_CLK_CMD  RCC_AHB1PeriphClockCmd
#define FINGER_USART_TX_CLK        RCC_AHB1Periph_GPIOC
#define FINGER_USART_TX_PORT       GPIOC
#define FINGER_USART_TX_PIN        GPIO_Pin_6
#define FINGER_USART_TX_PINSRC     GPIO_PinSource6
#define FINGER_USART_TX_AF         GPIO_AF_USART6

/******  FINGER_USART_RX引脚时钟端口、引脚宏定义 ******/
#define FINGER_USART_RX_CLK        RCC_AHB1Periph_GPIOC
#define FINGER_USART_RX_PORT       GPIOC
#define FINGER_USART_RX_PIN        GPIO_Pin_7
#define FINGER_USART_RX_PINSRC     GPIO_PinSource7
#define FINGER_USART_RX_AF         GPIO_AF_USART6

/****** 串口相关配置宏定义 ******/
#define FINGER_USART_BASE_CLK_CMD  RCC_APB2PeriphClockCmd
#define FINGER_USART               USART6
#define FINGER_USART_CLK           RCC_APB2Periph_USART6
#define FINGER_USART_BAUD          115200

extern const usart_ops_t g_stm32f4_usart_driver_;

#endif //__BSP_USART_H_
