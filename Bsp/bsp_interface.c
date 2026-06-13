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

#include "bsp_interface.h"

// 全局单例对象实例化
const board_hw_bsp_t* g_board_hw_bsp_ = NULL;

// 根据目标平台选择对应的板级BSP描述符，实现平台切换
// [C++对照] 类似于依赖注入：将具体工厂的引用赋给抽象工厂指针，即 Base* ptr = new Derived()
// 此处只包含具体的驱动声明，只声明变量，不包含硬件头文件
#if defined(STM32F4)
extern const board_hw_bsp_t g_stm32f4_bsp_;
#define BSP_DRIVER_INTERFACE g_stm32f4_bsp_
#else
#error "No platform selected"
#endif

/**
 * @brief  Bsp初始化
 * @note   无
 * @param  无
 * @retval 无
 */
bsp_status_e Bsp_Init(void)
{
    bsp_status_e status;

    // 挂载当前平台的板级BSP描述符（依赖注入）
    g_board_hw_bsp_ = &BSP_DRIVER_INTERFACE;

    // 平台级全局配置（中断分组等），必须在所有外设初始化之前
    status = g_board_hw_bsp_->platform_init();
    if (status != BSP_STAT_TRUE) return status;

    // 重要的外设在这里初始化，其他外设RAII
    // 调试串口初始化
    status = g_board_hw_bsp_->usart_ops->init(USART_ID_DEBUG);
    if (status != BSP_STAT_TRUE) return status;
    // SysTick 由 FreeRTOS 独占，不再在此初始化（FIX-05）

    return BSP_STAT_TRUE;
}
