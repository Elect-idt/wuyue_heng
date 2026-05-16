# =====================================================================
# FreeRTOS — 实时操作系统内核
#
# 产出：
#   freertos_interface (INTERFACE) — FreeRTOS 头文件路径
#   FreeRTOS_Lib (STATIC) → libFreeRTOS_Lib.a
#
# FreeRTOS kernel + heap_4 内存管理 + ARM Cortex-M4F 移植层
# =====================================================================
cmake_minimum_required(VERSION 3.22)

# FreeRTOS 源文件
set(FreeRTOS_Src
    ${CMAKE_SOURCE_DIR}/FreeRTOS/port/MemMang/heap_4.c   # 内存管理（支持 pvPortMalloc/vPortFree）
)
# FreeRTOS 内核源文件（tasks.c, queue.c, list.c 等）
aux_source_directory(${CMAKE_SOURCE_DIR}/FreeRTOS/src FreeRTOS_Src)
# ARM Cortex-M4F 移植层（上下文切换、中断处理）
aux_source_directory(${CMAKE_SOURCE_DIR}/FreeRTOS/port/RVDS/ARM_CM4F FreeRTOS_Src)

# FreeRTOS 头文件路径（INTERFACE：传播给链接 FreeRTOS_Lib 的 target）
add_library(freertos_interface INTERFACE)

target_include_directories(freertos_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/FreeRTOS/inc                    # FreeRTOS API 头文件（FreeRTOS.h, task.h 等）
    ${CMAKE_SOURCE_DIR}/FreeRTOS/port/RVDS/ARM_CM4F     # 移植层头文件（portmacro.h）
)

add_library(FreeRTOS_Lib STATIC)

target_sources(FreeRTOS_Lib PRIVATE ${FreeRTOS_Src})

# =====================================================================
# FreeRTOS_Lib 的依赖
#
# PUBLIC freertos_interface:  FreeRTOS API 头文件传播给 Apps 层
#   → Apps 可以 #include "FreeRTOS.h", #include "task.h"
#
# PRIVATE periph_drivers_interface:
#   FreeRTOS 移植层需要 CMSIS 核心头文件（core_cm4.h）访问 Cortex-M4 寄存器
#   PRIVATE 防止 SPL 头文件泄露给 Apps 层
# =====================================================================
target_link_libraries(FreeRTOS_Lib PUBLIC freertos_interface PRIVATE periph_drivers_interface)
