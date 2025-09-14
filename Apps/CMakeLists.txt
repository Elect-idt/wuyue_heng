# 设置 CMake 最低版本要求 (3.22)
cmake_minimum_required(VERSION 3.22)

# 定义App库的启动源文件
set(App_Task_Src
    ${CMAKE_SOURCE_DIR}/Apps/app_init.c
)
# Apps Drivers源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/Apps/led_status_app App_Task_Src)

# 创建接口库 (不生成实际库文件，仅管理头文件和宏定义)
add_library(app_task_interface INTERFACE)

# 为接口库添加头文件包含路径 (INTERFACE 表示这些路径会影响链接此库的目标)
target_include_directories(app_task_interface INTERFACE
    # apps文件
    ${CMAKE_SOURCE_DIR}/Apps

    # 各个app驱动头文件
    ${CMAKE_SOURCE_DIR}/Apps/led_status_app
)

# 为接口库添加全局宏定义 (这些宏会影响所有使用此外设驱动的代码)
target_compile_definitions(app_task_interface INTERFACE
    # STM32F40_41xxx       # 指定芯片型号为 STM32F40x/41x 系列
    # USE_STDPERIPH_DRIVER  # 启用标准外设库 (在库头文件中会检查此宏)
)

# 创建实际的标准外设静态库 (将编译为 .a 文件)
add_library(app_task_lib STATIC)

# 将源文件添加到标准外设库目标
target_sources(app_task_lib PRIVATE ${App_Task_Src})

# 将接口库链接到标准外设库 (确保编译时能访问正确的头文件和宏定义)
target_link_libraries(app_task_lib PUBLIC 
    STM32_Bsp_Driver 
    stm32_bsp_interface 
    STM32_StdPeriph_Driver 
    freertos_interface app_task_interface)