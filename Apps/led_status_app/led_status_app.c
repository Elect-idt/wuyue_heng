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
#include <stdint.h>

static led_t s_status_led;

/**
 * @brief  LED状态任务
 * @note   300ms闪烁代表系统正常运行
 */
void Led_Status_Task(void* param)
{
    /* 用于保存上次时间。调用后系统自动更新 */
    static portTickType PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* RAII: 初始化 LED（低有效） */
    led_init(&s_status_led, g_board_hw_bsp_->gpio_ops, GPIO_PIN_LED_STATUS, true);

    while (1)
    {
        vTaskDelayUntil(&PreviousWakeTime, 300);
        led_toggle(&s_status_led);
    }
}
