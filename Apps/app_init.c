/**
 ******************************************************************************
 * @file    app_init.c
 * @author  Pan
 * @version V1.0
 * @date    2025-09-14
 * @brief   app_init
 ******************************************************************************
 * @attention
 *
 * Project: wuyue_heng
 *
 ******************************************************************************
 */

#include "app_init.h"
#include "app_common_def.h"
#include "bsp_interface.h"
#include <stdio.h>

/**
  * @brief  主函数
  * @note   第一步：初始化所有app任务
            第二步：启动FreeRTOS，开始多任务调度
  * @param  无
  * @retval 0:初始化成功 ，1失败
  */
int32_t AppTaskCreate(void)
{
    configASSERT(BSP_STAT_TRUE == Bsp_Init());
    configASSERT(NULL != g_board_hw_bsp_);

    int32_t status = APP_TASK_SUCCESS;
    BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */

    /* 调度器未启动，无需 taskENTER_CRITICAL（且 xTaskCreate 内部会分配内存，
       长时间屏蔽中断会增加延迟），直接创建即可 */

    /* ①创建按键扫描任务（高优先级，10ms周期） */
    xReturn = xTaskCreate((TaskFunction_t)Key_Scan_Task, (const char*)"Key_Scan_Task", (uint16_t)256, (void*)NULL,
                          (UBaseType_t)KEY_SCAN_TASK_PRI, (TaskHandle_t*)NULL);
    if (pdPASS != xReturn)
    {
        status = APP_TASK_FAIL;
    }

    /* ②创建RGB灯效任务（20ms周期刷新） */
    xReturn = xTaskCreate((TaskFunction_t)Led_RGB_Display_Task, (const char*)"Led_RGB_Task", (uint16_t)256, (void*)NULL,
                          (UBaseType_t)LED_RGB_DISPLAY_TASK_PRI, (TaskHandle_t*)NULL);
    if (pdPASS != xReturn)
    {
        status = APP_TASK_FAIL;
    }

    /* ③创建LED状态任务 */
    xReturn = xTaskCreate((TaskFunction_t)Led_Status_Task, (const char*)"Led_Status_Task", (uint16_t)256, (void*)NULL,
                          (UBaseType_t)LED_STATUS_TASK_PRI, (TaskHandle_t*)NULL);
    if (pdPASS != xReturn)
    {
        status = APP_TASK_FAIL;
    }

    /* 启动任务调度 */
    if (status == APP_TASK_SUCCESS)
        vTaskStartScheduler(); /* 启动任务，开启调度 */

    return status;
}

/* vApplicationStackOverflowHook 已移至 Apps/common/app_hooks.c
 *（FreeRTOS 钩子统一放那里，不与任务创建逻辑混放）*/
