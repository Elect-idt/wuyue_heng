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

// 全局单例对象实例化
board_hw_t Board;

#if defined(STM32F4)
extern const led_ops_t stm32f4_led_driver;
#define LED_DRIVER stm32f4_led_driver

#else
#endif

/**
 * @brief  Bsp初始化
 * @note   无
 * @param  无
 * @retval 无
 */
void Bsp_Init(void)
{
    /* LED 外设挂载 */
    Board.led_ops = &LED_DRIVER;

    /* LED 初始化 */
    if (Board.led_ops && Board.led_ops->init)
    {
        Board.led_ops->init();
    }
    // Debug_LED_Init();
    /* Debug 串口初始化 */
    // Debug_USART_Init();
}
