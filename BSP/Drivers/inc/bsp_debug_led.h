#ifndef __BSP_DEBUG_LED_H
#define __BSP_DEBUG_LED_H

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include <stdint.h>

/******  结构体枚举等定义 ******/

typedef enum
{
    DEBUG_LED_ON = 0,
    DEBUG_LED_OFF = 1
}Debug_Led_Ctrl_e;
/******  LED_R引脚时钟端口、引脚和对应TIM输出比较相关宏定义 ******/
#define LED_R_CLK            RCC_AHB1Periph_GPIOA
#define LED_R_PORT           GPIOA
#define LED_R_PIN            GPIO_Pin_8

// /******  LED_G引脚时钟端口、引脚和对应TIM输出比较相关宏定义 ******/
// #define LED_G_CLK            RCC_AHB1Periph_GPIOA
// #define LED_G_PORT           GPIOA
// #define LED_G_PIN            GPIO_Pin_0
// #define LED_G_PINSRC         GPIO_PinSource0
// #define LED_G_AF             GPIO_AF_TIM5
// #define LED_G_TIM_CLK        RCC_APB1Periph_TIM5
// #define LED_G_TIM            TIM5
// #define LED_G_TIM_OC_INIT    TIM_OC1Init
// #define LED_G_TIM_OC_RELOAD  TIM_OC1PreloadConfig

#define DEBUG_LED_RED(a) if (a) \
    GPIO_SetBits(LED_R_PORT,LED_R_PIN);\
else \
    GPIO_ResetBits(LED_R_PORT,LED_R_PIN)

/****** API函数 ******/
extern void Debug_LED_Init(void);
extern void Debug_LED_Ctrl(uint8_t color, uint8_t set_value);

#endif
