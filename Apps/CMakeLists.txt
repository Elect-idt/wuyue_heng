# =====================================================================
# Apps — 应用层（FreeRTOS 任务实现）
#
# 产出：
#   app_task_interface (INTERFACE) — Apps 头文件路径
#   app_task_lib (STATIC) → libapp_task_lib.a
#
# Apps 层只依赖 Bsp_Driver 的接口（*_ops_t 函数指针）
# 不直接调用 STM32 SPL 函数，实现平台无关
# =====================================================================
cmake_minimum_required(VERSION 3.22)

# App 任务源文件
set(App_Task_Src
    ${CMAKE_SOURCE_DIR}/Apps/app_init.c   # 应用初始化（创建 FreeRTOS 任务）
)
# LED 状态任务
aux_source_directory(${CMAKE_SOURCE_DIR}/Apps/led_status_app App_Task_Src)

# Apps 头文件路径（INTERFACE：传播给链接 app_task_lib 的 target）
add_library(app_task_interface INTERFACE)

target_include_directories(app_task_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Apps                     # app_init.h 等公共头文件
    ${CMAKE_SOURCE_DIR}/Apps/led_status_app       # LED 任务头文件
)

add_library(app_task_lib STATIC)

target_sources(app_task_lib PRIVATE ${App_Task_Src})

# =====================================================================
# app_task_lib 的依赖
#
# PUBLIC Bsp_Driver:
#   Apps 通过 g_board_hw_bsp_->led_ops->xxx() 访问硬件
#   Bsp_Driver PUBLIC 传播 Bsp_Interface 的头文件路径（-IBsp/common -IBsp）
#   Apps 能看到 *_interface.h，但看不到 stm32f4xx.h（StdPeriph_Driver 是 PRIVATE）
#
# PUBLIC FreeRTOS_Lib:
#   Apps 使用 FreeRTOS API（xTaskCreate, vTaskDelay 等）
#   FreeRTOS_Lib PUBLIC 传播 freertos_interface（-IFreeRTOS/inc 等）
#
# PUBLIC app_task_interface:
#   传播 Apps 自身的头文件路径
# =====================================================================
target_link_libraries(app_task_lib PUBLIC
    Bsp_Driver           # BSP 驱动（通过接口访问硬件）
    FreeRTOS_Lib         # FreeRTOS 内核（任务调度）
    app_task_interface   # Apps 头文件
)
