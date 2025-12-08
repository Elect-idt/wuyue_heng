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
#include <stdint.h>

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

    // 遵循RAII，初始化led
    configASSERT(BSP_STAT_TRUE == g_board_hw_bsp_->led_ops->init());

    while (1)
    {
        /* 1. 绝对延时，一秒调用一次 */
        printf("led control :%d\n", LED_TOGGLE);
        g_board_hw_bsp_->usart_ops->usart_send_byte(USART_ID_DEBUG, 'a');
        putchar('\n');
        g_board_hw_bsp_->usart_ops->usart_send_string(USART_ID_DEBUG, "panjiale");
        putchar('\n');
        uint16_t hex = 0X4241;
        g_board_hw_bsp_->usart_ops->usart_send_hex(USART_ID_DEBUG, hex);
        putchar('\n');
        uint8_t str[5] = {'a', 'b', 'c', 'd', '\0'};
        g_board_hw_bsp_->usart_ops->usart_send_array(USART_ID_DEBUG, str, 5);
        putchar('\n');

        vTaskDelayUntil(&PreviousWakeTime, 300);
        configASSERT(BSP_STAT_TRUE == g_board_hw_bsp_->led_ops->control(LED_ID_STATUS, LED_TOGGLE));
    }
}
