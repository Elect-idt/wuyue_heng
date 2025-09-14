#ifndef __bsp_INIT_H
#define __bsp_INIT_H

#include "bsp_debug_led.h"
#include "bsp_debug_usart.h"
#include "bsp_debug_usart.h"
#include "stm32f4xx_rcc.h"

/****** API函数 ******/
extern void Bsp_Init(void);
extern void Bsp_LED_Init(void);

#endif
