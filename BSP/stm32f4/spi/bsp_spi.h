#ifndef __BSP_SPI_H_
#define __BSP_SPI_H_

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include "bsp_spi_interface.h"

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

#define KEY_SCAN_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOB
#define KEY_SCAN_CS_GPIO_PORT                  GPIOB 
#define KEY_SCAN_CS_GPIO_PIN                   GPIO_Pin_12               
#define KEY_SCAN_CS_DISABLE                    GPIO_SetBits(KEY_SCAN_CS_GPIO_PORT, KEY_SCAN_CS_GPIO_PIN)
#define KEY_SCAN_CS_ENABLE                     GPIO_ResetBits(KEY_SCAN_CS_GPIO_PORT, KEY_SCAN_CS_GPIO_PIN)

/****** KEY_SCAN_SPI相关配置宏定义 ******/
#define KEY_SCAN_SPI                           SPI2
#define KEY_SCAN_SPI_CLK                       RCC_APB1Periph_SPI2
#define KEY_SCAN_SPI_CLK_INIT                  RCC_APB1PeriphClockCmd

/****** KEY_SCAN_SPI用到的DMA配置相关配置宏定义 ******/
#define KEY_SCAN_SPI_DMA_CLK_INIT          RCC_AHB1PeriphClockCmd
#define KEY_SCAN_SPI_DMA_CLK               RCC_AHB1Periph_DMA1            
#define KEY_SCAN_SPI_DMA_CHANNEL           DMA_Channel_3
#define KEY_SCAN_SPI_DMA_STREAM            DMA1_Stream3
#define KEY_SCAN_SPI_DMA_IRQn              DMA1_Stream3_IRQn
#define KEY_SCAN_SPI_DMA_IRQHandler        DMA1_Stream3_IRQHandler
#define KEY_SCAN_SPI_DMA_IT_TC             DMA_IT_TCIF3
#define KEY_SCAN_SPI_DMA_CLK_INIT          RCC_AHB1PeriphClockCmd


extern const spi_ops_t g_stm32f4_spi_driver_;

#endif //__BSP_SPI_H_
