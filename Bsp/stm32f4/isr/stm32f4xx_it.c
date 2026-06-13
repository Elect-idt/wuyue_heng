/**
 ******************************************************************************
 * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c
 * @author  MCD Application Team
 * @version V1.8.1
 * @date    27-January-2022
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f4xx.h"      /* CMSIS：HardFault 的 SCB 寄存器（it.h 不再传递，it.c 按需 include）*/
#include "bsp_isr_map.h"    /* KEY_SCAN_SPI_RX_DMA_IRQHandler → DMA1_Stream3_IRQHandler 路由映射 */
#include <stdio.h>
#include <inttypes.h>

/* SPI DMA ISR handler — 硬件细节封装在 bsp_spi.c 中 */
extern void bsp_spi_dma_isr_handler(void);

/** @addtogroup Template_Project
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
    /* 打印故障寄存器，辅助定位（HFSR=硬故障状态，CFSR=可配置故障状态，
       MMFAR/BFAR=访存错误地址）。调试时配合 LR 反推调用栈 */
    printf("\r\n*** HardFault ***\r\n");
    printf("HFSR =0x%08" PRIX32 "\r\n", SCB->HFSR);
    printf("CFSR =0x%08" PRIX32 "\r\n", SCB->CFSR);
    printf("MMFAR=0x%08" PRIX32 "\r\n", SCB->MMFAR);
    printf("BFAR =0x%08" PRIX32 "\r\n", SCB->BFAR);
    printf("LR   =0x%08" PRIX32 "\r\n", (uint32_t)__builtin_return_address(0));
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

// ===========================================================================
// FreeRTOS handler 说明（以下三个 handler 不要在此定义！）
//   FreeRTOS port.c (RVDS/ARM_CM4F/port.c) 通过 FreeRTOSConfig.h 的宏映射
//   提供了这三个 handler 的实现：
//     #define vPortSVCHandler    SVC_Handler
//     #define xPortPendSVHandler PendSV_Handler
//     #define vPortSysTickHandler SysTick_Handler
//   如果在此处再定义，会与 port.c 冲突，链接时报 multiple definition。
//   若改用裸机（无 RTOS），取消下方注释并删除 FreeRTOSConfig.h 的映射宏即可。
// ===========================================================================
// void SVC_Handler(void) {}
// void PendSV_Handler(void) {}
// void SysTick_Handler(void) {}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
{
}*/

/******************************************************************************/
/*                 SPI DMA Interrupt Handlers                                 */
/******************************************************************************/

/**
 * @brief  SPI RX DMA中断处理（路由到 bsp_spi.c 中的封装函数）
 * @note   DMA1_Stream3，SPI2接收DMA完成/错误通知
 */
void KEY_SCAN_SPI_RX_DMA_IRQHandler(void)
{
    bsp_spi_dma_isr_handler();
}

/**
 * @}
 */
