/**
 ******************************************************************************
 * @file    bsp_gpio.c
 * @author  Pan
 * @version V1.0
 * @brief   GPIO 驱动（LED、74HC165 PL 等通用引脚）
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_gpio.h"
#include <stdint.h>

/**
 * @brief  GPIO 初始化指定引脚
 */
static bsp_status_e stm32f4_gpio_init(gpio_pin_e pin)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    switch (pin)
    {
    case GPIO_PIN_LED_STATUS:
        RCC_AHB1PeriphClockCmd(LED_R_CLK, ENABLE);
        GPIO_InitStructure.GPIO_Pin   = LED_R_PIN;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(LED_R_PORT, &GPIO_InitStructure);
        GPIO_SetBits(LED_R_PORT, LED_R_PIN); /* 默认熄灭（低有效） */
        break;
    case GPIO_PIN_HC165_PL:
        RCC_AHB1PeriphClockCmd(HC165_PL_CLK, ENABLE);
        GPIO_InitStructure.GPIO_Pin   = HC165_PL_PIN;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(HC165_PL_PORT, &GPIO_InitStructure);
        GPIO_SetBits(HC165_PL_PORT, HC165_PL_PIN); /* 默认 HIGH（移位模式） */
        break;
    default:
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  GPIO 写引脚
 */
static bsp_status_e stm32f4_gpio_write(gpio_pin_e pin, gpio_state_e state)
{
    switch (pin)
    {
    case GPIO_PIN_LED_STATUS:
    {
        if (state == GPIO_LOW)
            GPIO_ResetBits(LED_R_PORT, LED_R_PIN);
        else if (state == GPIO_HIGH)
            GPIO_SetBits(LED_R_PORT, LED_R_PIN);
        else
            GPIO_ToggleBits(LED_R_PORT, LED_R_PIN);
    }
    break;
    case GPIO_PIN_HC165_PL:
    {
        if (state == GPIO_LOW)
            GPIO_ResetBits(HC165_PL_PORT, HC165_PL_PIN);
        else if (state == GPIO_HIGH)
            GPIO_SetBits(HC165_PL_PORT, HC165_PL_PIN);
        else
            GPIO_ToggleBits(HC165_PL_PORT, HC165_PL_PIN);
    }
    break;
    default:
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    return BSP_STAT_TRUE;
}

// STM32F4 平台 GPIO 驱动实例
const gpio_ops_t g_stm32f4_gpio_driver_ = {
    .name  = "STM32F4_GPIO_DRIVER",
    .init  = stm32f4_gpio_init,
    .write = stm32f4_gpio_write,
};
