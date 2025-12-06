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
#include "bsp_interface.h"

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

    taskENTER_CRITICAL(); // 进入临界区;

    /* ①创建LCD显示任务 */
    xReturn = xTaskCreate((TaskFunction_t)Led_Status_Task, /* 任务入口函数 */
                          (const char*)"Led_Status_Task",  /* 任务名字 */
                          (uint16_t)256,                   /* 任务栈大小 */
                          (void*)NULL,          /* 任务入口函数参数 */
                          (UBaseType_t)2,       /* 任务的优先级 */
                          (TaskHandle_t*)NULL); /* 任务控制块指针 */
    if (pdPASS != xReturn)
    {
        status = APP_TASK_FAIL;
    }

    taskEXIT_CRITICAL(); // 退出临界区

    /* 启动任务调度 */
    if (status == APP_TASK_SUCCESS)
        vTaskStartScheduler(); /* 启动任务，开启调度 */

    return status;
}