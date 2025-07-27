/**
  ******************************************************************************
  * @file    SysTick.c
  * @author  Pan
  * @version V1.0
  * @date    2022-12-20
  * @brief   SisTick延时函数，非中断版本
             注意在操作系统中该源文件中的
             函数尽量不用，一般只用在软件
             IIC或SPI中进行us级别延时，否
             则影响系统性能
  ******************************************************************************
  * @attention
  *
  * Project: wuyue_heng
  *
  ******************************************************************************
  */

#include "SysTick.h"

static u8 fac_us = 0;  // us延时倍乘数
static u16 fac_ms = 0; // ms延时倍乘数

/**
 * @brief  初始化延迟函数
 * @note   SYSTICK的时钟固定为AHB时钟的1/8
 * @param  SYSCLK:系统时钟频率
 * @retval 无
 */
void SysTick_Init(u8 SYSCLK)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    /* 1us所需要到达的计数值 */
    fac_us = SYSCLK / 8;
    /* 1ms所需要到达的计数值 */
    fac_ms = (u16)fac_us * 1000;
}

/**
 * @brief  微秒延时函数
 * @note   无
 * @param  nus：要延时的us数
 * @retval 无
 */
void Delay_Us(u32 nus)
{
    u32 temp;
    /* 时间重装载值 */
    SysTick->LOAD = nus * fac_us;
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

/**
  * @brief  毫秒延时函数
  * @note   注意nms的范围，SysTick->LOAD为24位寄存器,所以,最大延时为:
                                                nms< =
  0xffffff*8*1000/SYSCLK，SYSCLK单位为Hz,nms单位为ms， 对168M条件下,nms< = 798
  * @param  nms：要延时的ms数
  * @retval 无
  */
void Delay_Ms(u16 nms)
{
    u32 temp;
    /* 时间重装载值(SysTick->LOAD为24bit) */
    SysTick->LOAD = (u32)nms * fac_ms;
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

/**
 * @brief  秒延时函数
 * @note   无
 * @param  s：要延时的s数
 * @retval 无
 */
void Delay_S(u16 s)
{
    u16 i;
    for (i = 0; i < s; i++)
    {
        Delay_Ms(500);
        Delay_Ms(500);
    }
}

void delay()
{
    u32 i = 0xfffff;

    while (i--)
        ;
}

/**
 * @brief execute delay in all polling api calls : @a
 * VL6180x_RangePollMeasurement() and @a VL6180x_AlsPollMeasurement()
 *
 * A typical multi-thread or RTOs implementation is to sleep the task for some
 * 5ms (with 100Hz max rate faster polling is not needed). if nothing specific
 * is needed, you can define it as an empty/void macro
 * @code
 * #define VL6180x_PollDelay(...) (void)0
 * @endcode
 * @param dev The device
 * @ingroup api_platform
 */
void VL6180x_PollDelay(u8 dev) { Delay_Ms(5); }
