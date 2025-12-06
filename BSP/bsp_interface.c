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

// 选择对应的实例化接口，就是方法工厂中的实例化，即CPP中抽象工厂基类指针=new派生工厂的操作
#if defined(STM32F4)
#include "stm32f4_bsp.h"
#define BSP_DRIVER_INTERFACE g_stm32f4_bsp_
#else
#endif

/**
 * @brief  Bsp初始化
 * @note   无
 * @param  无
 * @retval 无
 */
bsp_status_e Bsp_Init(void)
{
    bsp_status_e status = BSP_STAT_TRUE;
    // 外设驱动挂载，相当于cpp 基类抽象工厂指针 = new 派生类工厂
    g_board_hw_bsp_ = &BSP_DRIVER_INTERFACE;

    // 重要的外设在这里初始化，其他外设RAII
    // 调试串口初始化
    status |= g_board_hw_bsp_->usart_ops->init(USART_ID_DEBUG);

    return status;
}
