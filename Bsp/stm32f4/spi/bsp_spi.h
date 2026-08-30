#ifndef __BSP_SPI_H_
#define __BSP_SPI_H_

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include "bsp_spi_interface.h"

#define SPI_GPIO_PORT_INVALID  0

// TODO 后续改成可配置宏，且用定时器实现
#define SPI_TIME_OUT  ((uint32_t)0x1000)

/******  KEY_SCAN_SPI引脚时钟端口、引脚宏定义 ******/
#define KEY_SCAN_SPI_GPIO_CLK_INIT             RCC_AHB1PeriphClockCmd
#define KEY_SCAN_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOB
#define KEY_SCAN_SPI_SCK_GPIO_PORT             GPIOB
#define KEY_SCAN_SPI_SCK_GPIO_PIN              GPIO_Pin_13                               
#define KEY_SCAN_SPI_SCK_PINSOURCE             GPIO_PinSource13
#define KEY_SCAN_SPI_SCK_AF                    GPIO_AF_SPI2

#define KEY_SCAN_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOB
#define KEY_SCAN_SPI_MISO_GPIO_PORT            GPIOB
#define KEY_SCAN_SPI_MISO_GPIO_PIN             GPIO_Pin_14
#define KEY_SCAN_SPI_MISO_PINSOURCE            GPIO_PinSource14
#define KEY_SCAN_SPI_MISO_AF                   GPIO_AF_SPI2

#define KEY_SCAN_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOB
#define KEY_SCAN_SPI_MOSI_GPIO_PORT            GPIOB
#define KEY_SCAN_SPI_MOSI_GPIO_PIN             GPIO_Pin_15
#define KEY_SCAN_SPI_MOSI_PINSOURCE            GPIO_PinSource15
#define KEY_SCAN_SPI_MOSI_AF                   GPIO_AF_SPI2

#define KEY_SCAN_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOB
#define KEY_SCAN_CS_GPIO_PORT                  GPIOB 
#define KEY_SCAN_CS_GPIO_PIN                   GPIO_Pin_12               
#define KEY_SCAN_CS_DISABLE                    GPIO_SetBits(KEY_SCAN_CS_GPIO_PORT, KEY_SCAN_CS_GPIO_PIN)
#define KEY_SCAN_CS_ENABLE                     GPIO_ResetBits(KEY_SCAN_CS_GPIO_PORT, KEY_SCAN_CS_GPIO_PIN)

/****** KEY_SCAN_SPI相关配置宏定义 ******/
#define KEY_SCAN_SPI                           SPI2
#define KEY_SCAN_SPI_CLK                       RCC_APB1Periph_SPI2
#define KEY_SCAN_SPI_CLK_INIT                  RCC_APB1PeriphClockCmd

/****** KEY_SCAN_SPI RX DMA配置 ******/
#define KEY_SCAN_SPI_DMA_CLK_INIT          RCC_AHB1PeriphClockCmd
#define KEY_SCAN_SPI_DMA_CLK               RCC_AHB1Periph_DMA1
#define KEY_SCAN_SPI_RX_DMA_CHANNEL        DMA_Channel_0
#define KEY_SCAN_SPI_RX_DMA_STREAM         DMA1_Stream3
#define KEY_SCAN_SPI_RX_DMA_IRQn           DMA1_Stream3_IRQn
#define KEY_SCAN_SPI_RX_DMA_IT_TC          DMA_IT_TCIF3
#define KEY_SCAN_SPI_RX_DMA_IT_TE          DMA_IT_TEIF3

/****** KEY_SCAN_SPI TX DMA配置（全双工DMA接收时发送dummy字节产生时钟） ******/
#define KEY_SCAN_SPI_TX_DMA_STREAM         DMA1_Stream4
#define KEY_SCAN_SPI_TX_DMA_CHANNEL        DMA_Channel_0
#define KEY_SCAN_SPI_TX_DMA_IRQn           DMA1_Stream4_IRQn
#define KEY_SCAN_SPI_TX_DMA_IT_TC          DMA_IT_TCIF4

/******  WS2812_LED_SPI引脚时钟端口、引脚宏定义 ******/
/* WS2812B 单线器件：只用 MOSI。
 * 未使用的引脚：PORT 填 SPI_GPIO_PORT_INVALID（配置时跳过该引脚的 AF），
 * 引脚得以另作他用；GPIO_CLK 掩码照实填（时钟多开无害，SCK/MOSI 同在 GPIOB）。
 * 注意哨兵只用于 PORT 指针字段，不能用于 CLK 掩码字段（两者是不同的东西） */
#define WS2812_LED_SPI_GPIO_CLK_INIT             RCC_AHB1PeriphClockCmd
#define WS2812_LED_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOB
#define WS2812_LED_SPI_SCK_GPIO_PORT             SPI_GPIO_PORT_INVALID /* PB3 释放另用 */
#define WS2812_LED_SPI_SCK_GPIO_PIN              GPIO_Pin_3
#define WS2812_LED_SPI_SCK_PINSOURCE             GPIO_PinSource3
#define WS2812_LED_SPI_SCK_AF                    GPIO_AF_SPI3

#define WS2812_LED_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOB
#define WS2812_LED_SPI_MISO_GPIO_PORT            SPI_GPIO_PORT_INVALID /* 纯写器件无 MISO */
#define WS2812_LED_SPI_MISO_GPIO_PIN             GPIO_Pin_4
#define WS2812_LED_SPI_MISO_PINSOURCE            GPIO_PinSource4
#define WS2812_LED_SPI_MISO_AF                   GPIO_AF_SPI3

#define WS2812_LED_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOB
#define WS2812_LED_SPI_MOSI_GPIO_PORT            GPIOB
#define WS2812_LED_SPI_MOSI_GPIO_PIN             GPIO_Pin_5
#define WS2812_LED_SPI_MOSI_PINSOURCE            GPIO_PinSource5
#define WS2812_LED_SPI_MOSI_AF                   GPIO_AF_SPI3

/* 无 CS：WS2812 的 DIN 由数据时序本身界定帧，不需要片选 */
#define WS2812_LED_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOB
#define WS2812_LED_CS_GPIO_PORT                  SPI_GPIO_PORT_INVALID
#define WS2812_LED_CS_GPIO_PIN                   0

/****** WS2812_LED_SPI相关配置宏定义 ******/
#define WS2812_LED_SPI                           SPI3
#define WS2812_LED_SPI_CLK                       RCC_APB1Periph_SPI3
#define WS2812_LED_SPI_CLK_INIT                  RCC_APB1PeriphClockCmd

/****** WS2812_LED_SPI RX DMA配置 ******/
#define WS2812_LED_SPI_DMA_CLK_INIT          RCC_AHB1PeriphClockCmd
#define WS2812_LED_SPI_DMA_CLK               RCC_AHB1Periph_DMA1
#define WS2812_LED_SPI_RX_DMA_CHANNEL        DMA_Channel_0
#define WS2812_LED_SPI_RX_DMA_STREAM         DMA1_Stream0
#define WS2812_LED_SPI_RX_DMA_IRQn           DMA1_Stream0_IRQn
#define WS2812_LED_SPI_RX_DMA_IT_TC          DMA_IT_TCIF0
#define WS2812_LED_SPI_RX_DMA_IT_TE          DMA_IT_TEIF0

/****** WS2812_LED_SPI TX DMA配置（全双工DMA接收时发送dummy字节产生时钟） ******/
#define WS2812_LED_SPI_TX_DMA_STREAM         DMA1_Stream5
#define WS2812_LED_SPI_TX_DMA_CHANNEL        DMA_Channel_0
#define WS2812_LED_SPI_TX_DMA_IRQn           DMA1_Stream5_IRQn
#define WS2812_LED_SPI_TX_DMA_IT_TC          DMA_IT_TCIF5

extern const spi_ops_t g_stm32f4_spi_driver_;

#endif //__BSP_SPI_H_
