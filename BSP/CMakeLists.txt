# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# =====================================================================
# 第一层：公共接口（INTERFACE库，没有源文件）
# 管理头文件路径 + 板级配置宏
# 所有依赖它的 target 都会继承这些 -I 路径和宏定义
# =====================================================================
add_library(bsp_common_interface INTERFACE)

target_include_directories(bsp_common_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Bsp/common    # *_interface.h（led_ops_t, usart_ops_t 等类型定义）
    ${CMAKE_SOURCE_DIR}/Bsp           # bsp_interface.h（board_hw_bsp_t 聚合描述符）
)

# 板级配置宏（不是平台寄存器操作，Apps 看到无害）
target_compile_definitions(bsp_common_interface INTERFACE
    STM32F4           # 平台选择（bsp_interface.c 用 #if defined(STM32F4) 选择平台描述符）
    SYSCLK_MHZ=168    # 系统时钟频率（Bsp_Init 中 systick 初始化需要）
)

# =====================================================================
# 第二层：平台无关的BSP调度层（OBJECT库）
#
# 为什么是OBJECT不是STATIC？
#   bsp_interface.c 引用 g_stm32f4_bsp_（extern）
#   stm32f4_bsp.c   定义 g_stm32f4_bsp_
#   两者互相引用，如果分成两个 .a，链接器单遍扫描会导致链接顺序报错
#   OBJECT库不生成 .a，其 .o 文件会被注入 Bsp_Driver，在同一个 .a 内解析符号
#
# 编译隔离机制：
#   Bsp_Interface 只链接 bsp_common_interface
#   → 编译时只有 -IBsp/common -IBsp，以及 STM32F4/SYSCLK_MHZ 宏
#   → 没有 -IBsp/stm32f4，没有 -IDrivers/（看不到 SPL 头文件）
#   → bsp_interface.c 里 #include "stm32f4xx.h" 会直接编译报错
#   → 用工具强制保证平台无关性，不靠人自觉
# =====================================================================
add_library(Bsp_Interface OBJECT)

target_sources(Bsp_Interface PRIVATE
    ${CMAKE_SOURCE_DIR}/Bsp/bsp_interface.c
)

# PUBLIC：让依赖 Bsp_Interface 的 target 也能找到 common 头文件
target_link_libraries(Bsp_Interface PUBLIC bsp_common_interface)

# =====================================================================
# 第三层：平台相关驱动（STATIC库）
# 最终生成 libBsp_Driver.a，包含 Bsp_Interface 的 .o + 平台驱动的 .o
# =====================================================================
add_library(Bsp_Driver STATIC)

######################### 平台选择 ##############################
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    # 收集平台驱动源文件
    set(Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4 Bsp_Platform_Src)
    # TODO: 后面根据功能宏区分
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/led Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/usart Bsp_Platform_Src)
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/systick Bsp_Platform_Src)

    # 平台头文件路径（PRIVATE：只有 Bsp_Driver 内部可见）
    target_include_directories(Bsp_Driver PRIVATE
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/
        # TODO: 后面根据功能宏区分
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/led
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/usart
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/systick
    )

else()
endif()

# =====================================================================
# 将平台源文件 + Bsp_Interface的对象文件一起打包进 Bsp_Driver
#
# $<TARGET_OBJECTS:Bsp_Interface> 的作用：
#   把 Bsp_Interface 编译出的 bsp_interface.o 直接注入 Bsp_Driver
#   最终 bsp_interface.o 和 stm32f4_bsp.o 都在同一个 libBsp_Driver.a 内
#   同一个 .a 内链接器会反复扫描直到所有符号解析完毕（收敛）
#
# 这句不能删：删了则 bsp_interface.o 没人用，Bsp_Init() 从最终 .elf 中消失
# =====================================================================
target_sources(Bsp_Driver PRIVATE
    ${Bsp_Platform_Src}
    $<TARGET_OBJECTS:Bsp_Interface>
)

# =====================================================================
# Bsp_Driver 的依赖关系
#
# PUBLIC Bsp_Interface：
#   传播头文件路径（-IBsp/common -IBsp）给依赖 Bsp_Driver 的 target（如 Apps）
#   注意：Bsp_Interface 是 OBJECT 库，没有 .a 文件，PUBLIC 只传播 -I 路径
#   不传播 -lBsp_Interface，所以最终链接命令里只有 -lBsp_Driver
#   这句不能删：删了则 stm32f4_bsp.c 编译时找不到 bsp_interface.h
#
# PRIVATE StdPeriph_Driver：
#   STM32 SPL 库只在本 target 内使用
#   Apps 层链接 Bsp_Driver 时看不到 stm32f4xx.h 等平台头文件
#   这实现了依赖倒置：Apps 依赖接口，不依赖具体平台
# =====================================================================
target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface PRIVATE StdPeriph_Driver)
