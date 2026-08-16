/**
 ******************************************************************************
 * @file    led_status_app.c
 * @author  Pan
 * @version V2.0
 * @brief   led_status_app（使用 Component LED）
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "led_status_app.h"
#include "led.h"
#include "bsp_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

static led_t s_status_led;

/**
 * @brief  LED状态任务
 * @note   300ms闪烁代表系统正常运行
 */
void Led_Status_Task(void* param)
{
    /* 用于保存上次时间。调用后系统自动更新 */
    static TickType_t PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* RAII: 初始化 LED：预填描述符（低有效），再校验+硬件 init */
    s_status_led = (led_t){
        .gpio_ops   = g_board_hw_bsp_->gpio_ops,
        .pin        = GPIO_PIN_LED_STATUS,
        .active_low = true,
    };
    configASSERT(BSP_STAT_TRUE == led_init(&s_status_led));

    while (1)
    {
        vTaskDelayUntil(&PreviousWakeTime, pdMS_TO_TICKS(300));
        led_toggle(&s_status_led);
    }
}
