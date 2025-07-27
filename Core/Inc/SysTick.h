#ifndef __SYSTICK_H
#define __SYSTICK_H

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"

/****** API函数 ******/
void SysTick_Init(u8 SYSCLK);
void Delay_Ms(u16 nms);
void Delay_Us(u32 nus);
void Delay_S(u16 s);
extern void delay(void);

#endif
