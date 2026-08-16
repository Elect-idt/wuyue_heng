/**
 ******************************************************************************
 * @file    bsp_interface.c
 * @author  Pan
 * @version V1.0
 * @date    2025-07-28
 * @brief   BSP 平台工厂 + 初始化调度（Bsp_Init）
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_interface.h"
#include <stddef.h>     /* NULL */

// 全局单例对象实例化
const board_hw_bsp_t* g_board_hw_bsp_ = NULL;

// 平台工厂：编译期通过宏选择具体平台描述符，绑定到抽象全局指针 g_board_hw_bsp_
// 后续新增平台（如 GD32/H7）在此加 #elif 宏隔离即可，Apps/Component/Core 无需改动
// 注意：此处只 extern 声明平台描述符变量，不 include 任何平台头文件
//       ——"头文件级平台无关"（CMake 编译隔离强制保证），但"符号级平台感知"（工厂职责所在）
// [C++对照] 编译期抽象工厂：抽象指针 g_board_hw_bsp_ 指向宏选定的具体工厂实例 g_stm32f4_bsp_
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
