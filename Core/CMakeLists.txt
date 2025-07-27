# 只保留系统核心的构建部分
cmake_minimum_required(VERSION 3.22)

# 系统核心源文件
set(STM32_System_Core_Src
    ${CMAKE_SOURCE_DIR}/Core/src/main.c
    ${CMAKE_SOURCE_DIR}/Core/src/stm32f4xx_it.c
    ${CMAKE_SOURCE_DIR}/Core/src/syscalls.c
    ${CMAKE_SOURCE_DIR}/Core/src/sysmem.c
)

# 创建系统核心接口库文件,管理Core下所有头文件目录
add_library(stm32_System_Core_drivers_interface INTERFACE)

# 精准为stm32_System_Core_drivers_interface添加包含目录,比起include_directories（影响所有目标）更优
target_include_directories(stm32_System_Core_drivers_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Core/inc
)
# 为stm32_System_Core_drivers_interface添加宏定义
target_compile_definitions(stm32_System_Core_drivers_interface INTERFACE
    STM32F40_41xxx
    USE_STDPERIPH_DRIVER
    $<$<CONFIG:Debug>:DEBUG>  # 调试模式专用宏
)

# 创建系统核心库静态库文件
add_library(STM32_System_Core STATIC)

# 为STM32_System_Core添加源文件
target_sources(STM32_System_Core PRIVATE ${STM32_System_Core_Src})

# 为STM32_System_Core库添加外设库的链接
target_link_libraries(STM32_System_Core PUBLIC 
    stm32_System_Core_drivers_interface 
    stm32_periph_drivers_interface
)
