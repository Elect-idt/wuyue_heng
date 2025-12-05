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
#include "bsp_interface.h"

/***********************************
 *定义全局变量和函数
 ***********************************/
// TaskHandle_t Led_Status_Task_Handle = NULL; /* led状态任务句柄 */

/**
 * @brief  LED状态任务
 * @note   1s闪烁代表系统正常运行
 * @param  无
 * @retval 无
 */
void Led_Status_Task(void* param)
{
    /* 用于保存上次时间。调用后系统自动更新 */
    static portTickType PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    while (1)
    {
        /* 1. 绝对延时，一秒调用一次 */
        vTaskDelayUntil(&PreviousWakeTime, 5000);
        Board.led_ops->control(LED_ID_STATUS, LED_TOGGLE);
    }
}
