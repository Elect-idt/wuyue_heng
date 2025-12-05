# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 创建 common 接口库
add_library(bsp_common_interface INTERFACE)

# 添加 common 头文件路径(INTERFACE 表示这些路径会影响链接此库的目标)
target_include_directories(bsp_common_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/Bsp/common
    ${CMAKE_SOURCE_DIR}/Bsp
)

# 创建实际的标准外设静态库 (将编译为 .a 文件)
add_library(Bsp_Driver STATIC)

######################### 解耦 ##############################
if (PROJECT_PLATFORM STREQUAL "STM32F4")
    # 定义Bsp库的源文件集合
    set(Bsp_Src
        ${CMAKE_SOURCE_DIR}/Bsp/bsp_init.c
    )
    # Bsp Drivers源文件
    aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/src Bsp_Src)

    # 为BSP库添加头文件包含路径 
    target_include_directories(Bsp_Driver PRIVATE
        # Bsp Drivers头文件
        ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/inc
    )

    # 为StdPeriph_Driver库添加全局宏定义
    target_compile_definitions(Bsp_Driver PRIVATE
        STM32F40_41xxx       # 指定芯片型号为 STM32F40x/41x 系列
        USE_STDPERIPH_DRIVER  # 启用标准外设库 (在库头文件中会检查此宏)
    )

else()
endif()

# 将源文件添加到标准外设库目标
target_sources(Bsp_Driver PRIVATE ${Bsp_Src})

# 将接口库链接到标准外设库 (确保编译时能访问正确的头文件和宏定义)
target_link_libraries(Bsp_Driver PUBLIC bsp_common_interface StdPeriph_Driver)