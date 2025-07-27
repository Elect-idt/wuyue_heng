cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME led_blink)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# 修改点1：添加源文件到可执行目标
add_executable(${CMAKE_PROJECT_NAME}
    # 添加启动文件（必须至少有一个源文件）
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/gcc_ride7/startup_stm32f40_41xxx.s
)

# 修改点2：添加标准库子目录
add_subdirectory(cmake/stm32_stdperiph_driver)

# 修改点3：添加应用程序源文件
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Src/main.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Core/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/Core/Src/sysmem.c
)

# 修改点4：添加包含目录
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_StdPeriph_Driver/inc
)

# 修改点5：添加宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F40_41xxx
    USE_STDPERIPH_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
)

# 修改点6：链接库
target_link_libraries(${CMAKE_PROJECT_NAME}
    STM32_StdPeriph_Driver  # 标准外设库
    stm32_interface         # 接口库（包含目录和宏定义）
)

# 修改点7：设置链接脚本
# set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F405RGT6_FLASH.ld")
# set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
#     LINK_FLAGS "-T \"${LINKER_SCRIPT}\""
# )

# 修改点8：清理文件
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    ADDITIONAL_CLEAN_FILES "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.hex;${CMAKE_PROJECT_NAME}.bin"
)