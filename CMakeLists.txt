# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 配置 C 编译器标准设置
set(CMAKE_C_STANDARD 11)          # 强制使用 C11 标准
set(CMAKE_C_STANDARD_REQUIRED ON)  # 必须严格遵守 C11 标准
set(CMAKE_C_EXTENSIONS ON)         # 启用编译器扩展功能

# 定义构建类型 (Debug/Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")  # 若未指定构建类型，默认为 Debug 模式
endif()

# 设置项目名称变量
set(CMAKE_PROJECT_NAME wuyue_heng)  # 定义项目名称为 "wuyue_heng"

# 包含 ARM 交叉编译工具链配置
include("cmake/gcc-arm-none-eabi.cmake")  # 加载 ARM Cortex-M 的 GCC 工具链配置

# 生成编译命令数据库 (用于 IDE 索引)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)  # 生成 compile_commands.json 文件

# 正式声明项目
project(${CMAKE_PROJECT_NAME})  # 初始化项目，使用前面定义的名称
message("Build type: " ${CMAKE_BUILD_TYPE})  # 打印当前构建类型

# 启用 C 和汇编语言支持
enable_language(C ASM)  # 告知 CMake 项目使用 C 和汇编语言

# 创建可执行目标
add_executable(${CMAKE_PROJECT_NAME})  # 生成名为 wuyue_heng 的可执行文件

# 添加标准外设库子目录
add_subdirectory(Drivers)  # 编译 Drivers 目录下的库 (标准外设库)
add_subdirectory(Bsp)  # 编译 Bsp 目录下的库 (Bsp库)

# 添加应用程序源文件
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/src/main.c            # 主程序入口
    ${CMAKE_SOURCE_DIR}/Core/src/stm32f4xx_it.c    # 中断服务程序
    ${CMAKE_SOURCE_DIR}/Core/src/syscalls.c        # 系统调用重定向
    ${CMAKE_SOURCE_DIR}/Core/src/sysmem.c          # 内存管理实现
    ${CMAKE_SOURCE_DIR}/Core/src/SysTick.c         # SysTick定时器实现
    
)

# 添加头文件包含路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/inc  # 包含项目核心头文件目录
)

# 添加预处理器宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F40_41xxx            # 指定使用的 STM32 系列型号
    USE_STDPERIPH_DRIVER      # 启用标准外设库
    $<$<CONFIG:Debug>:DEBUG>  # Debug 模式下自动定义 DEBUG 宏
)

# 链接依赖库
target_link_libraries(${CMAKE_PROJECT_NAME}
    STM32_StdPeriph_Driver          # 链接标准外设库 (来自 Drivers)
    STM32_Bsp_Driver                # 链接Bsp库 (来自 Bsp)
)

# [修改点8] 设置构建后需清理的附加文件
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    ADDITIONAL_CLEAN_FILES 
    "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.hex;${CMAKE_PROJECT_NAME}.bin"  # 清理生成的目标文件
)