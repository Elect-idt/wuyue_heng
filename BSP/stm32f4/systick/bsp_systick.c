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
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "bsp_systick.h"

static u8 fac_us = 0;  // us延时倍乘数
static u16 fac_ms = 0; // ms延时倍乘数
static u16 max_us = 0; // ms延时倍乘数

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
        fac_ms = (u16)fac_us * 1000;
    }
    return BSP_STAT_TRUE;
}

/**
 * @brief  us级别延时
 * @note   同步，注意不要超过798914us（2^24-1）*8/168
 * @param  id:串口设备号
 * @param  ch:要发送的字节
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
        u32 temp;
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
 * @brief  ms级别延时
 * @note   同步，注意nms的范围，SysTick->LOAD为24位寄存器,所以,最大延时为:
           nms< =0xffffff*8*1000/SYSCLK，SYSCLK单位为Hz,nms单位为ms， 对
           168M条件下,nms< = 798
 * @param  id:串口设备号
 * @param  ch:要发送的字节
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
        u32 temp;
        /* 时间重装载值(SysTick->LOAD为24bit) */
        SysTick->LOAD = (u32)ms * fac_ms;
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
 * @brief  s级别延时
 * @note   同步
 * @param  id:串口设备号
 * @param  ch:要发送的字节
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
        u16 i;
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
