/**
 ******************************************************************************
 * @file    bsp_systick.c
 * @author  Pan
 * @version V1.0
 * @date    2025-12-08
 * @brief   systick驱动
 ******************************************************************************
 * @attention
 *
 * [DEPRECATED] 此驱动已从公共 BSP 接口 (board_hw_bsp_t) 中移除。
 * 原因：SysTick 在 FreeRTOS 系统中属于 RTOS port 独占资源，
 *        vTaskStartScheduler() 会重新初始化 SysTick，此后调用
 *        delay_us/delay_ms 会破坏 FreeRTOS tick 节拍。
 * 未来：微秒级延时将改用 TIM 基础定时器实现。
 * 文件保留但不应在调度器启动后使用 delay 函数。
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_systick.h"

static uint8_t fac_us = 0;  // us延时倍乘数
static uint16_t fac_ms = 0; // ms延时倍乘数

/**
 * @brief  SysTick定时器初始化
 * @note   注意，freertos会重新初始化systick，所以这个驱动意义不大
 * @param  id:设备号
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_systick_init(systick_id_e id, uint16_t sysclk)
{
    if (id >= SYSTICK_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        if (sysclk % 8 != 0)
        {
            return BSP_STAT_INVALID_PARAMS;
        }
        SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
        /* 1us所需要到达的计数值 */
        fac_us = sysclk / 8;
        /* 1ms所需要到达的计数值 */
        fac_ms = (uint16_t)fac_us * 1000;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  微秒级延时（DEPRECATED：FreeRTOS 启动后会破坏 tick，勿用）
 * @note   同步阻塞，注意不要超过798914us（2^24-1）*8/168
 * @param  id:SysTick 设备号（未使用，保留接口一致性）
 * @param  us:延时的微秒数
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_systick_delay_us(systick_id_e id, uint32_t us)
{
    if (id >= SYSTICK_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        uint32_t temp;
        /* 时间重装载值 */
        SysTick->LOAD = us * fac_us;
        /* 清空计数器 */
        SysTick->VAL = 0x00;
        /* 开始倒数 */
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        /* 等待时间到达 */
        do
        {
            temp = SysTick->CTRL;
        } while ((temp & 0x01) && !(temp & (1 << 16)));

        /* 关闭计数器 */
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        /* 清空计数器 */
        SysTick->VAL = 0X00;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  毫秒级延时（DEPRECATED：FreeRTOS 启动后会破坏 tick，勿用）
 * @note   同步阻塞，注意nms的范围，SysTick->LOAD为24位寄存器,所以,最大延时为:
           nms< =0xffffff*8*1000/SYSCLK，SYSCLK单位为Hz,nms单位为ms， 对
           168M条件下,nms< = 798
 * @param  id:SysTick 设备号（未使用，保留接口一致性）
 * @param  ms:延时的毫秒数
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_systick_delay_ms(systick_id_e id, uint32_t ms)
{
    if (id >= SYSTICK_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        uint32_t temp;
        /* 时间重装载值(SysTick->LOAD为24bit) */
        SysTick->LOAD = (uint32_t)ms * fac_ms;
        /* 清空计数器 */
        SysTick->VAL = 0x00;
        /* 开始倒数 */
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        /* 等待时间到达 */
        do
        {
            temp = SysTick->CTRL;
        } while ((temp & 0x01) && !(temp & (1 << 16)));

        /* 关闭计数器 */
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        /* 清空计数器 */
        SysTick->VAL = 0X00;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  秒级延时（DEPRECATED：FreeRTOS 启动后会破坏 tick，勿用）
 * @note   同步阻塞，内部循环调用 delay_ms(500)
 * @param  id:SysTick 设备号（未使用，保留接口一致性）
 * @param  s:延时的秒数
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_systick_delay_s(systick_id_e id, uint32_t s)
{
    if (id >= SYSTICK_ID_MAX)
    {
        return BSP_STAT_CHOOSE_ERROR_TARGET;
    }
    else
    {
        uint32_t i;
        for (i = 0; i < s * 2; i++)
        {
            stm32f4_systick_delay_ms(id, 500);
        }
    }
    return BSP_STAT_TRUE;
}

// STM32F4平台SYSTICK驱动实例，实现systick_ops_t定义的统一操作接口
// [C++对照] 对应具体产品(Concrete Product)
// 注：C中无继承，具体产品与抽象产品是同一类型，区别仅为函数指针指向了具体实现（类似填好的虚表）
const systick_ops_t g_stm32f4_systick_driver_ = {
    .name = "STM32F4_SYSTICK_DRIVER",
    .init = stm32f4_systick_init,
    .delay_us = stm32f4_systick_delay_us,
    .delay_ms = stm32f4_systick_delay_ms,
    .delay_s = stm32f4_systick_delay_s,
};
