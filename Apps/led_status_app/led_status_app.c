/**
 ******************************************************************************
 * @file    led_status_app.c
 * @author  Pan
 * @version V1.0
 * @date    2025-09-14
 * @brief   led_status_app
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "led_status_app.h"
#include "bsp_debug_led.h"

/***********************************
 *定义全局变量和函数
 ***********************************/
static float Surface_Volt = 0.0f;
TaskHandle_t Led_Status_Task_Handle = NULL; /* led状态任务句柄 */

/**
 * @brief  LED状态任务
 * @note   1s闪烁代表系统正常运行
 * @param  无
 * @retval 无
 */
void Led_Status_Task(void* param)
{
    /* 调用BSP初始化函数 */
    Debug_Led_Ctrl_e led_status = DEBUG_LED_OFF;

    /* 用于保存上次时间。调用后系统自动更新 */
    static portTickType PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* 初始化对应外设 */
    Debug_LED_Init();

    while (1)
    {
        /* 1. 绝对延时，一秒调用一次 */
        vTaskDelayUntil(&PreviousWakeTime, 1000);

        /* 2. LED翻转 */
        if (led_status == DEBUG_LED_OFF)
        {
            DEBUG_LED_RED(DEBUG_LED_ON);
            led_status = DEBUG_LED_ON;
        }
        else
        {
            DEBUG_LED_RED(DEBUG_LED_OFF);
            led_status = DEBUG_LED_OFF;
        }
    }
}
