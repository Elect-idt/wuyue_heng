#ifndef __APP_INIT_H
#define __APP_INIT_H

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"

/* 任务头文件 */
#include "led_status_app.h"
#include "key_scan_app.h"

#define APP_TASK_SUCCESS 0
#define APP_TASK_FAIL -1

extern int32_t AppTaskCreate(void);


#endif




