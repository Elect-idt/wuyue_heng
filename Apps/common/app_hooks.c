/**
 ******************************************************************************
 * @file    app_hooks.c
 * @author  Pan
 * @brief   FreeRTOS 应用层钩子函数（内核回调，策略由 Apps 层决定）
 *
 * FreeRTOS 约定：configUSE_* 宏开启后，内核会引用以下钩子符号，
 * 必须由应用提供定义，否则链接报 undefined reference。
 * 新增钩子（MallocFailed/Idle/Tick 等）统一放本文件，不要散落到各任务文件。
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/**
 * @brief  FreeRTOS configASSERT 断言失败处理（FreeRTOSConfig.h 的宏调用到这）
 * @param  file: 断言所在的源文件名
 * @param  line: 断言所在的行号
 * @note   断言失败意味着系统状态已不可信，必须停车：关中断（冻结调度和一切
 *         异步事件）-> 打印位置 -> 死循环等复位/调试器。继续运行只会把
 *         可定位的错误变成无诊断的 HardFault
 */
void vAssertCalled(const char* file, int line)
{
    taskDISABLE_INTERRUPTS();
    printf("ASSERT: %s:%d\r\n", file, line);
    while (1)
    {
    }
}

/**
 * @brief  FreeRTOS 栈溢出检测回调（configCHECK_FOR_STACK_OVERFLOW = 2 时需要）
 * @param  xTask: 溢出任务句柄
 * @param  pcTaskName: 溢出任务名称
 * @note   级别 2 通过栈底填充模式检测，进入此函数时栈很可能已破坏，
 *         printf 本身耗栈，存在打印失败的可能（但通常还能打出任务名）
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    (void)xTask;
    printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while (1)
    {
    }
}
