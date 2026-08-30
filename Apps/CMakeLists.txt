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

# App 任务源文件（显式列出，新增任务需手动添加）
set(App_Task_Src
    ${CMAKE_SOURCE_DIR}/Apps/app_init.c                 # 应用初始化（创建 FreeRTOS 任务）
    ${CMAKE_SOURCE_DIR}/Apps/led_status_app/led_status_app.c    # LED 状态任务
    ${CMAKE_SOURCE_DIR}/Apps/key_scan_app/key_scan_app.c        # 按键扫描任务
    ${CMAKE_SOURCE_DIR}/Apps/led_rgb_display_app/led_rgb_display_app.c   # RGB 灯效任务
)

# =====================================================================
# FreeRTOS 应用钩子（OBJECT 库，直接注入可执行文件）
# 必须直接链接，不能放进 STATIC 库！归档提取时机陷阱：
#   链接器扫描 libapp_task_lib.a 时，只为"当前已未定义"的符号提取 .o。
#   此时 tasks.o（引用 vApplicationStackOverflowHook）还没被提取，
#   app_hooks.o 不会被拉出；等 tasks.o 因 xTaskCreate 被提取时，
#   已没有后续归档能提供钩子 -> undefined reference。
# （钩子曾定义在 app_init.c 里恰好能链接上：提取 app_init.o 时顺带带出钩子，
#   属于隐蔽的位置耦合，故移出后必须用 OBJECT 库注入，同 Bsp_ISR 模式。）
# =====================================================================
add_library(App_Hooks OBJECT)
target_sources(App_Hooks PRIVATE
    ${CMAKE_SOURCE_DIR}/Apps/common/app_hooks.c
)
target_link_libraries(App_Hooks PRIVATE freertos_interface)

# Apps 头文件路径（INTERFACE：传播给链接 app_task_lib 的 target）
add_library(app_task_interface INTERFACE)

target_include_directories(app_task_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Apps                     # app_init.h 等公共头文件
    ${CMAKE_SOURCE_DIR}/Apps/common              # app_common_def.h（任务优先级等公共配置）
    ${CMAKE_SOURCE_DIR}/Apps/led_status_app       # LED 任务头文件
    ${CMAKE_SOURCE_DIR}/Apps/key_scan_app         # 按键扫描任务头文件
    ${CMAKE_SOURCE_DIR}/Apps/led_rgb_display_app     # RGB 灯效任务头文件
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
    component_lib        # 器件协议层（74HC165 等）
    app_task_interface   # Apps 头文件
)
