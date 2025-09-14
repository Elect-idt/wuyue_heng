#ifndef __LED_STATUS_TASK_H
#define __LED_STATUS_TASK_H

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* 包含的头文件，除了系统文件外不建议放在这里，建议放在对应的源文件里 */


/************** 宏定义 ***************/


/************** 定义结构 ***************/


/* 函数指针定义 */

/************** 声明extern函数 ***************/
extern void Led_Status_Task(void* param);
extern TaskHandle_t Led_Status_Task_Handle;/* 电位测量任务句柄 */
/************** 声明全局变量 ***************/

#endif


