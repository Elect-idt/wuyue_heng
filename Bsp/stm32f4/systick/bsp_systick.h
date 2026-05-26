#ifndef __BSP_SYSTICK_H_
#define __BSP_SYSTICK_H_

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */
#include "stm32f4xx.h"
#include "bsp_systick_interface.h"

extern const systick_ops_t g_stm32f4_systick_driver_;

#endif //__BSP_SYSTICK_H_
