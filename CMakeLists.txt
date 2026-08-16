# =====================================================================
# 根 CMakeLists.txt — 项目总入口
# STM32F405RGT6, Cortex-M4F, FreeRTOS, 168MHz
# CMake 3.22 + Ninja + ARM GCC 14.2 交叉编译
# =====================================================================
cmake_minimum_required(VERSION 3.22)

# C 语言标准设置
set(CMAKE_C_STANDARD 11)          # C11 标准
set(CMAKE_C_STANDARD_REQUIRED ON)  # 必须严格遵守
set(CMAKE_C_EXTENSIONS ON)         # 启用 GCC 扩展（如 __attribute__）

# 默认 Debug 构建（若未指定 -DCMAKE_BUILD_TYPE=Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# 项目名称
set(CMAKE_PROJECT_NAME wuyue_heng)

# =====================================================================
# 工具链配置（必须在 project() 之前加载）
# target_config.cmake:  定义 PROJECT_PLATFORM、芯片型号等变量
# gcc-arm-none-eabi.cmake: 配置交叉编译器、链接脚本、编译选项
# =====================================================================
include("cmake/target_config.cmake")
include("cmake/gcc-arm-none-eabi.cmake")

# 平台宏（已移到 bsp_common_interface，随平台选择自动生效）
# 不再使用 add_definitions 全局污染

# 生成 compile_commands.json（供 IDE 代码跳转和语法补全使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# 正式声明项目（必须在工具链配置之后）
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# 诊断选项（调试 #include 路径或宏展开时取消注释）
# -dD：预处理时输出所有宏定义
# -Wp,-v：打印预处理器头文件搜索路径
# add_compile_options(-dD -Wp,-v)

# 启用 C 和汇编语言支持（启动文件 startup_stm32f40_41xxx.s 是汇编）
enable_language(C ASM)

# =====================================================================
# 创建最终可执行目标 wuyue_heng.elf
# =====================================================================
add_executable(${CMAKE_PROJECT_NAME})

# 子目录（按依赖顺序添加，CMake 会自动处理链接顺序）
add_subdirectory(Drivers)    # → StdPeriph_Driver (STM32 SPL 标准外设库)
add_subdirectory(Bsp)        # → Bsp_Driver (BSP 驱动，依赖 StdPeriph_Driver)
add_subdirectory(FreeRTOS)   # → FreeRTOS_Lib (FreeRTOS 内核)
add_subdirectory(Component)  # → component_lib (器件协议层，依赖 Bsp_Interface)
add_subdirectory(Apps)       # → app_task_lib (应用任务，依赖 Bsp_Driver + FreeRTOS)

# Core 层源文件（不属于任何子库，直接加入可执行文件）
# stm32f4xx_it.c 已移到 Bsp/stm32f4/，属于平台相关代码
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/src/main.c            # 主程序入口
    ${CMAKE_SOURCE_DIR}/Core/src/syscalls.c        # 系统调用重定向（printf → USART）
    ${CMAKE_SOURCE_DIR}/Core/src/sysmem.c          # 堆内存管理
    $<TARGET_OBJECTS:Bsp_ISR>                      # ISR handlers（直接链接，覆盖启动文件弱符号）
    $<TARGET_OBJECTS:App_Hooks>                    # FreeRTOS 应用钩子（直接链接，规避归档提取时机陷阱）
)

# Core 头文件路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/inc
)

# ============================================================
# 注意：这里不能定义 STM32F40_41xxx 和 USE_STDPERIPH_DRIVER
# 它们已经在 Drivers/CMakeLists.txt 的 periph_drivers_interface 中定义
# Apps 层不应该看到 STM32 平台宏，这是依赖倒置的保证
# ============================================================
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>  # Debug 模式下自动定义 DEBUG 宏
)

# =====================================================================
# 链接依赖库
# 顺序：app_task_lib → Bsp_Driver → FreeRTOS_Lib
# 实际链接顺序由 CMake 根据依赖图自动排列，这里只是声明依赖关系
# =====================================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    Bsp_Driver          # BSP 驱动（包含 Bsp_Interface 的 .o + STM32 平台驱动）
    FreeRTOS_Lib        # FreeRTOS 内核
    component_lib       # 器件协议层（74HC165 等）
    app_task_lib        # 应用任务（LED 等任务实现）
)

# 清理时额外删除生成的 .map .hex .bin 文件
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    ADDITIONAL_CLEAN_FILES
    "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.hex;${CMAKE_PROJECT_NAME}.bin"
)
