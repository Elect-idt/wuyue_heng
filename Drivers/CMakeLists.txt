# =====================================================================
# Drivers — STM32 标准外设库 (SPL)
#
# 产出：
#   periph_drivers_interface (INTERFACE) — 头文件路径 + 宏定义
#   StdPeriph_Driver (STATIC) → libStdPeriph_Driver.a
#
# 重要：StdPeriph_Driver 是 Bsp_Driver 的 PRIVATE 依赖
# 只有 Bsp_Driver（平台驱动层）可以链接它，Apps 层看不到 STM32 头文件
# =====================================================================
cmake_minimum_required(VERSION 3.22)

add_library(StdPeriph_Driver STATIC)

######################### 平台选择 ##############################
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    # 关闭 misleading-indentation 警告（ST 官方 SPL 代码风格问题，不影响功能）
    target_compile_options(StdPeriph_Driver PRIVATE -Wno-misleading-indentation)

    # SPL 源文件（只编译实际用到的外设，减少代码体积）
    set(StdPeriph_Driver_Src
        # 系统初始化（SystemInit 配置时钟树）
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

        # 启动文件（向量表 + Reset_Handler → main）
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Source/gcc_ride7/startup_stm32f40_41xxx.s

        # 外设驱动
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/misc.c           # NVIC, SysTick
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c   # 时钟控制
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c  # GPIO
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c # USART
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spi.c   # SPI
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c   # DMA
    )

    # =====================================================================
    # periph_drivers_interface (INTERFACE)
    # 管理所有 STM32 相关的头文件路径和宏定义
    # 通过 StdPeriph_Driver PUBLIC 传播给链接它的 target
    #
    # ⚠️ 注意：只有 Bsp_Driver 应该链接 StdPeriph_Driver（PRIVATE）
    # 如果 Apps 层意外链接到这里，就能 #include "stm32f4xx.h"，破坏依赖倒置
    # =====================================================================
    add_library(periph_drivers_interface INTERFACE)

    target_include_directories(periph_drivers_interface INTERFACE
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Device/ST/STM32F4xx/Include       # 设备头文件（stm32f4xx.h）
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/STM32F4xx_StdPeriph_Driver/inc           # SPL 驱动头文件
        ${CMAKE_SOURCE_DIR}/Drivers/STM32F4_SPL/CMSIS/Include                            # ARM CMSIS 核心头文件（core_cm4.h）
    )

    target_compile_definitions(periph_drivers_interface INTERFACE
        STM32F40_41xxx        # 芯片型号选择（SPL 头文件用条件编译区分不同型号）
        USE_STDPERIPH_DRIVER  # 启用 SPL（conf.h 中检查此宏）
    )
else()
endif()

target_sources(StdPeriph_Driver PRIVATE ${StdPeriph_Driver_Src})

# PUBLIC：头文件路径和宏定义会传播给链接 StdPeriph_Driver 的 target
# 例如 Bsp_Driver PRIVATE StdPeriph_Driver → Bsp_Driver 内部可见 SPL 头文件
# 注意：此句也在 if 块内，因为 periph_drivers_interface 只在 STM32F4 平台时才创建
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    target_link_libraries(StdPeriph_Driver PUBLIC periph_drivers_interface)
endif()
