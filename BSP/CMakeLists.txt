# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 创建 common 接口库
add_library(bsp_common_interface INTERFACE)

# 添加 common 头文件路径(INTERFACE 表示这些路径会影响链接此库的目标)
target_include_directories(bsp_common_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Bsp/common
    ${CMAKE_SOURCE_DIR}/Bsp
)

# ===== 平台无关层：BSP接口调度 =====
# OBJECT库：独立编译（保持编译隔离，不依赖stm32等库），不生成独立.a文件
# 编译时只链接 bsp_common_interface，无法 include 平台头文件
add_library(Bsp_Interface OBJECT)

target_sources(Bsp_Interface PRIVATE
    ${CMAKE_SOURCE_DIR}/Bsp/bsp_interface.c
)

target_link_libraries(Bsp_Interface PUBLIC bsp_common_interface)

# ===== 平台相关层：具体硬件驱动 =====
add_library(Bsp_Driver STATIC)

######################### 解耦 ##############################
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    # 定义Bsp平台源文件集合
    set(Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4 Bsp_Platform_Src)
    # Bsp Drivers源文件 TODO:后面根据功能宏区分
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/led Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/usart Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/systick Bsp_Platform_Src)

    # 为BSP库添加头文件包含路径
    target_include_directories(Bsp_Driver PRIVATE
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/

        # Bsp Drivers头文件 TODO:后面根据功能宏区分
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/led
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/usart
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/systick
    )

    # 为StdPeriph_Driver库添加全局宏定义
    target_compile_definitions(Bsp_Driver PRIVATE
        STM32F40_41xxx       # 指定芯片型号为 STM32F40x/41x 系列
        USE_STDPERIPH_DRIVER  # 启用标准外设库 (在库头文件中会检查此宏)
    )

    # 设置系统时钟
    add_definitions(-DSYSCLK_MHZ=168)

else()
endif()

# 将平台源文件 + Bsp_Interface的对象文件一起打包进 Bsp_Driver
# 最终只生成一个 libBsp_Driver.a，不存在跨库链接顺序问题
target_sources(Bsp_Driver PRIVATE
    ${Bsp_Platform_Src}
    $<TARGET_OBJECTS:Bsp_Interface>
)

# Bsp_Driver链接：
# PUBLIC  Bsp_Interface：向下传播平台无关的接口头文件（include路径）
# PRIVATE StdPeriph_Driver：平台驱动只在本target内使用，不向下传播
target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface PRIVATE StdPeriph_Driver)
