# 只保留标准外设库的构建部分
cmake_minimum_required(VERSION 3.22)

# 标准外设库源文件
set(STM32_StdPeriph_Driver_Src
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/misc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c
    # 添加其他需要的外设驱动
)

# 创建接口库用于管理全局包含路径和宏定义
add_library(stm32_interface INTERFACE)
target_include_directories(stm32_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include

    # 添加 Core/Inc 目录 - 这是关键修复
    ${CMAKE_SOURCE_DIR}/Core/Inc
    
)
target_compile_definitions(stm32_interface INTERFACE
    STM32F40_41xxx
    USE_STDPERIPH_DRIVER
)

# 创建标准外设库静态库
add_library(STM32_StdPeriph_Driver STATIC)
target_sources(STM32_StdPeriph_Driver PRIVATE ${STM32_StdPeriph_Driver_Src})
target_link_libraries(STM32_StdPeriph_Driver PUBLIC stm32_interface)

# 移除以下部分（已移到外层）：
# - MX_Application_Src 相关定义
# - 对 ${CMAKE_PROJECT_NAME} 目标的修改