# =====================================================================
# Component — 器件协议抽象层（介于 Apps 和 Bsp_Interface 之间）
#
# 产出：
#   component_lib (STATIC) → libcomponent_lib.a
#
# 依赖 Bsp_Interface（通过 Bsp_Driver PUBLIC 传播）
# 不依赖 FreeRTOS，纯平台无关
# =====================================================================
cmake_minimum_required(VERSION 3.22)

add_library(component_lib STATIC)

# 源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/Component/led Component_Src)
aux_source_directory(${CMAKE_SOURCE_DIR}/Component/74hc165 Component_Src)
target_sources(component_lib PRIVATE ${Component_Src})

# 头文件路径（PUBLIC：Apps 包含头文件时能找到）
target_include_directories(component_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/Component/led
    ${CMAKE_SOURCE_DIR}/Component/74hc165
)

# Component 只依赖 Bsp_Interface（通过 Bsp_Driver PUBLIC 传播）
# 不依赖 FreeRTOS
target_link_libraries(component_lib PUBLIC Bsp_Driver)
