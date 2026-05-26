#ifndef __BSP_GPIO_H_
#define __BSP_GPIO_H_

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include "bsp_gpio_interface.h"

/******  LED 引脚配置 (PA15) ******/
#define LED_R_CLK            RCC_AHB1Periph_GPIOA
#define LED_R_PORT           GPIOA
#define LED_R_PIN            GPIO_Pin_15

/******  74HC165 PL 引脚配置 (PB1) ******/
#define HC165_PL_CLK         RCC_AHB1Periph_GPIOB
#define HC165_PL_PORT        GPIOB
#define HC165_PL_PIN         GPIO_Pin_1

extern const gpio_ops_t g_stm32f4_gpio_driver_;

#endif // __BSP_GPIO_H_
