/**
 ******************************************************************************
 * @file    bsp_led.c
 * @author  Pan
 * @version V1.0
 * @date    2025-07-28
 * @brief   LED控制
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_debug_led.h"
#include <stdint.h>

/**
 * @brief  LED的GPIO配置
 * @note   无
 * @param  无
 * @retval 无
 */
static void LED_GPIO_Config(void)
{
    /* 结构体宏定义 */
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启时钟 */
    RCC_AHB1PeriphClockCmd(LED_R_CLK, ENABLE);

    /* IO口内部用一个弱上拉增加带载能力 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* LED_R推挽输出 */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = LED_R_PIN;
    GPIO_Init(LED_R_PORT, &GPIO_InitStructure);
}

/**
 * @brief  LED初始化
 * @note   无
 * @param  无
 * @retval 无
 */
void Debug_LED_Init(void) { LED_GPIO_Config(); }
