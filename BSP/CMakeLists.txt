# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 定义Bsp库的源文件集合
set(STM32_Bsp_Src
    ${CMAKE_SOURCE_DIR}/Bsp/bsp_init.c
)
# Bsp Drivers源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/Drivers/src STM32_Bsp_Src)
# Bsp Component源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/Components/src STM32_Bsp_Src)

# 创建接口库 (不生成实际库文件，仅管理头文件和宏定义)
add_library(stm32_bsp_interface INTERFACE)

# 为接口库添加头文件包含路径 (INTERFACE 表示这些路径会影响链接此库的目标)
target_include_directories(stm32_bsp_interface INTERFACE
    # 标准外设驱动头文件目录
    ${CMAKE_SOURCE_DIR}/Bsp
    
    # Bsp Drivers头文件
    ${CMAKE_SOURCE_DIR}/Bsp/Drivers/inc
    
    # Bsp Component头文件
    ${CMAKE_SOURCE_DIR}/Bsp/Components/inc
)

# 为接口库添加全局宏定义 (这些宏会影响所有使用此外设驱动的代码)
target_compile_definitions(stm32_bsp_interface INTERFACE
    STM32F40_41xxx       # 指定芯片型号为 STM32F40x/41x 系列
    USE_STDPERIPH_DRIVER  # 启用标准外设库 (在库头文件中会检查此宏)
)

# 创建实际的标准外设静态库 (将编译为 .a 文件)
add_library(STM32_Bsp_Driver STATIC)

# 将源文件添加到标准外设库目标
target_sources(STM32_Bsp_Driver PRIVATE ${STM32_Bsp_Src})

# 将接口库链接到标准外设库 (确保编译时能访问正确的头文件和宏定义)
target_link_libraries(STM32_Bsp_Driver PUBLIC stm32_bsp_interface STM32_StdPeriph_Driver)