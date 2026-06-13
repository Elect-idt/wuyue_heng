#include "stm32f4_bsp.h"
#include "bsp_gpio.h"
// #include "bsp_systick.h"  ← FIX-05: SysTick 由 FreeRTOS 独占，从公共接口移除
#include "bsp_usart.h"
#include "bsp_spi.h"
#include "misc.h"

/**
 * @brief  STM32F4平台级初始化（全局中断分组等配置）
 * @note   NVIC_PriorityGroupConfig影响所有中断，必须在使用任何中断前调用
 * @retval status:0 无错误；其他 有错误
 */
static bsp_status_e stm32f4_platform_init(void)
{
    /* 4位抢占优先级，0位响应优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    return BSP_STAT_TRUE;
}

// STM32F4平台板级BSP描述符，聚合该平台所有外设的驱动实例
// [C++对照] 对应具体工厂(Concrete Factory)，类似于抽象工厂的派生类实例
const board_hw_bsp_t g_stm32f4_bsp_ = {.platform_init = stm32f4_platform_init,
                                       .gpio_ops = &g_stm32f4_gpio_driver_,
                                       .usart_ops = &g_stm32f4_usart_driver_,
                                       .spi_ops = &g_stm32f4_spi_driver_};
