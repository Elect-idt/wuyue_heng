#ifndef __BSP_DEBUG_LED_H
#define __BSP_DEBUG_LED_H

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include <stdint.h>
#include "stm32f4xx.h"
#include "bsp_led_interface.h"

/******  结构体枚举等定义 ******/
typedef enum
{
    DEBUG_LED_ON = 0,
    DEBUG_LED_OFF = 1,
    DEBUG_LED_TOGGLE = 2
} Debug_Led_Ctrl_e;

/******  LED_R引脚时钟端口、引脚和对应TIM输出比较相关宏定义 ******/
#define LED_R_CLK            RCC_AHB1Periph_GPIOA
#define LED_R_PORT           GPIOA
#define LED_R_PIN            GPIO_Pin_8

/******  定义宏，这个宏只给bsp_debug_led.c用 ******/
#define DEBUG_LED_RED(a) \
    if (a == DEBUG_LED_OFF) \
        GPIO_SetBits(LED_R_PORT,LED_R_PIN);\
    else if(a == DEBUG_LED_TOGGLE)\
        GPIO_ToggleBits(LED_R_PORT,LED_R_PIN);\
    else \
        GPIO_ResetBits(LED_R_PORT,LED_R_PIN)

#endif
