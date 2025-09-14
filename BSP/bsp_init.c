/**
 ******************************************************************************
 * @file    bsp_init.c
 * @author  Pan
 * @version V1.0
 * @date    2025-07-28
 * @brief   bsp_Init
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_init.h"

void Bsp_LED_Init(void)
{
    /* LED 外设初始化 */
    Debug_LED_Init();
}

/**
 * @brief  Bsp初始化
 * @note   无
 * @param  无
 * @retval 无
 */
void Bsp_Init(void)
{
    /* LED 外设初始化 */
    Debug_LED_Init();
    /* Debug 串口初始化 */
    Debug_USART_Init();
}
