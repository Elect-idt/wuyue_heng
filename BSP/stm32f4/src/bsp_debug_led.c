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
static void led_gpio_config(void)
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
bsp_status_e stm32f4_led_init(void)
{
    led_gpio_config();
    return BSP_STAT_TRUE;
}

/**
 * @brief  LED控制
 * @note   无
 * @param  无
 * @retval 无
 */
bsp_status_e stm32f4_led_control(led_id_t id, led_state_e state)
{
    if (state == LED_OFF)
    {
        DEBUG_LED_RED(DEBUG_LED_OFF);
    }
    else if (state == LED_ON)
    {
        DEBUG_LED_RED(DEBUG_LED_ON);
    }
    else if (state == LED_TOGGLE)
    {
        DEBUG_LED_RED(DEBUG_LED_TOGGLE);
    }
    else
    {
        return BSP_STAT_ERROR;
    }
    return BSP_STAT_TRUE;
}

// 实例接口
const led_ops_t stm32f4_led_driver = {.init = stm32f4_led_init, .control = stm32f4_led_control};
