# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# # 定义FreeRTOS库的内存管理文件
set(FreeRTOS_Src
    ${CMAKE_SOURCE_DIR}/FreeRTOS/port/MemMang/heap_4.c
)
# FreeRTOS src源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/FreeRTOS/src FreeRTOS_Src)
# FreeRTOS Component源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/FreeRTOS/port/RVDS/ARM_CM4F FreeRTOS_Src)

# 创建接口库 (不生成实际库文件，仅管理头文件和宏定义)
add_library(freertos_interface INTERFACE)

# 为接口库添加头文件包含路径 (INTERFACE 表示这些路径会影响链接此库的目标)
target_include_directories(freertos_interface INTERFACE
    # FreeRTOS头文件目录
    ${CMAKE_SOURCE_DIR}/FreeRTOS/inc
    
    # FreeRTOS port头文件
    ${CMAKE_SOURCE_DIR}/FreeRTOS/port/RVDS/ARM_CM4F
)

# 创建实际的标准外设静态库 (将编译为 .a 文件)
add_library(FreeRTOS_Lib STATIC)

# 将源文件添加到标准外设库目标
target_sources(FreeRTOS_Lib PRIVATE ${FreeRTOS_Src})

# 将接口库链接到标准外设库 (确保编译时能访问正确的头文件和宏定义)
target_link_libraries(FreeRTOS_Lib PUBLIC freertos_interface stm32_periph_drivers_interface stm32_bsp_interface)