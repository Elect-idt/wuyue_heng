# 只保留标准外设库的构建部分
cmake_minimum_required(VERSION 3.22)

# 标准外设库源文件
set(STM32_StdPeriph_Driver_Src
    # 添加系统和时钟初始化头文件system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/gcc_ride7/startup_stm32f40_41xxx.s
    # 添加其他需要的外设驱动
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/misc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c
    
)

# 创建标准库外设接口库文件,管理Drivers下所有头文件目录
add_library(stm32_periph_drivers_interface INTERFACE)

# 精准为stm32_periph_drivers_interface添加包含目录,比起include_directories（影响所有目标）更优
target_include_directories(stm32_periph_drivers_interface INTERFACE
    # 添加CMSIS和STM32F4xx_StdPeriph_Driver目录
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)
# 为stm32_periph_drivers_interface添加宏定义
target_compile_definitions(stm32_periph_drivers_interface INTERFACE
    STM32F40_41xxx
    USE_STDPERIPH_DRIVER
)

# 创建标准外设库静态库文件
add_library(STM32_StdPeriph_Driver STATIC)

# 为STM32_StdPeriph_Driver添加源文件
target_sources(STM32_StdPeriph_Driver PRIVATE ${STM32_StdPeriph_Driver_Src})

# 为STM32_StdPeriph_Driver库添加外设库的链接
target_link_libraries(STM32_StdPeriph_Driver PUBLIC stm32_periph_drivers_interface)
