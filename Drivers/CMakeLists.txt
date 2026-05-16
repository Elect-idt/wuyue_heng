# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 创建实际的标准外设静态库 (将编译为 .a 文件)
add_library(StdPeriph_Driver STATIC)

######################### 解耦 ##############################
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    # 定义标准外设库的源文件集合
    set(StdPeriph_Driver_Src
    # 系统初始化文件 - 包含时钟配置等关键系统初始化代码
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

    # ARM Cortex-M4 启动文件 - 使用 GCC/RIDE7 工具链的汇编启动代码
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Source/gcc_ride7/startup_stm32f40_41xxx.s

    # 外设驱动源文件
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/misc.c          # 杂项驱动 (NVIC, SysTick等)
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c   # 复位和时钟控制驱动
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c  # GPIO驱动
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c  #USART驱动
    )

    # 创建接口库 (不生成实际库文件，仅管理头文件和宏定义)
    add_library(periph_drivers_interface INTERFACE)

    # 为接口库添加头文件包含路径 (INTERFACE 表示这些路径会影响链接此库的目标)
    target_include_directories(periph_drivers_interface INTERFACE
        # 芯片特定CMSIS头文件 (包含设备相关定义)
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Include

        # 标准外设驱动头文件目录
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/inc
        
        # 核心CMSIS头文件 (包含ARM Cortex-M核心寄存器定义)
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Include
    )
    # 为接口库添加全局宏定义 (这些宏会影响所有使用此外设驱动的代码)
    target_compile_definitions(periph_drivers_interface INTERFACE
    STM32F40_41xxx       # 指定芯片型号为 STM32F40x/41x 系列
    USE_STDPERIPH_DRIVER  # 启用标准外设库 (在库头文件中会检查此宏)
    )
else()
endif()

# 将源文件添加到标准外设库目标
target_sources(StdPeriph_Driver PRIVATE ${StdPeriph_Driver_Src})

# 将接口库链接到标准外设库 (确保编译时能访问正确的头文件和宏定义) 只有Bsp可以链接这个
target_link_libraries(StdPeriph_Driver PUBLIC periph_drivers_interface)