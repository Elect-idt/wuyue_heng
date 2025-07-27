# 设置构建系统为通用嵌入式系统（无操作系统）
set(CMAKE_SYSTEM_NAME Generic)

# 指定目标处理器架构为ARM
set(CMAKE_SYSTEM_PROCESSOR arm)

# 明确指定使用GNU编译器
set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# 设置工具链前缀（需确保arm-none-eabi-在系统PATH中）
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# 配置编译器路径
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})  # 使用GCC编译汇编
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}gcc)      # 使用GCC作为链接器
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy) # 用于生成HEX/BIN文件
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)       # 用于查看内存占用

# 设置可执行文件后缀为.elf
# set(CMAKE_EXECUTABLE_SUFFIX ".elf") 这个是设置所有语言的可执行文件，设置后所有add_executable()目标都会使用.elf后缀
set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

# 设置交叉编译模式（跳过编译器测试）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#-----------------------------------------------------------
# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F405RGT6_FLASH.ld\"")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")

# 有中文注释的部分，是cubemx不会自动生成的，需要手动添加
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -u _printf_float")  # 支持 printf 函数打印浮点数
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -lm")  # 链接数学库 libm
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections,--no-warn-rwx-segments")  # 取消 rwx 段的警告
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")  # 忽略 C 代码中未使用参数的警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")  # 忽略 C++ 代码中未使用参数的警告

#-----------------------------------------------------------
# # 设置STM32F405RGT6专用标志
# # -mcpu=cortex-m4: 指定Cortex-M4内核
# # -mthumb: 使用Thumb指令集
# # -mfpu=fpv4-sp-d16: 单精度浮点单元
# # -mfloat-abi=hard: 硬件浮点加速
# set(TARGET_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# # 公共编译标志（用于C和ASM）
# set(COMMON_FLAGS
#     "${TARGET_FLAGS}"
#     "-fdata-sections -ffunction-sections"  # 启用段优化
#     "-Wall -Wextra -Wpedantic"             # 严格警告级别
#     "-Wno-unused-parameter"                # 忽略未使用参数警告
# )

# # C编译器标志
# set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
# set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")      # 调试模式：无优化+调试信息
# set(CMAKE_C_FLAGS_RELEASE "-Os -g0")    # 发布模式：尺寸优化

# # 汇编器标志
# set(CMAKE_ASM_FLAGS
#     "${COMMON_FLAGS}"
#     "-x assembler-with-cpp"  # 启用C预处理器处理汇编文件
#     "-MMD -MP"               # 自动生成依赖文件
# )

# # C++编译器标志
# set(CMAKE_CXX_FLAGS
#     "${COMMON_FLAGS}"
#     "-fno-rtti"               # 禁用RTTI
#     "-fno-exceptions"         # 禁用异常
#     "-fno-threadsafe-statics" # 禁用线程安全静态初始化
# )
# set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
# set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
# # 链接器标志
# set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F405RGTx_FLASH.ld")  # F405专用链接脚本

# set(CMAKE_EXE_LINKER_FLAGS
#     "${TARGET_FLAGS}"
#     "-T${LINKER_SCRIPT}"                 # 指定链接脚本
#     "--specs=nano.specs"                 # 使用精简版标准库
#     "-Wl,-Map=${CMAKE_PROJECT_NAME}.map" # 生成内存映射文件
#     "-Wl,--gc-sections"                  # 移除未使用代码段
#     "-Wl,--start-group -lc -lm -Wl,--end-group"  # 链接C库和数学库
#     "-Wl,--print-memory-usage"           # 输出内存使用统计
#     "-u _printf_float"                   # 启用printf浮点支持
#     "-Wl,--no-warn-rwx-segments"         # 禁用RWX段警告
#     "-lm"                                # 显式链接数学库
# )
#-----------------------------------------------------------


# 说明：STM32F405RGT6适配要点：
# 1. 预处理器定义需修改为 STM32F405xx（在CMakeLists.txt中修改）
# 2. 启动文件需改为 startup_stm32f405xx.s（在CMakeLists.txt中修改）
# 3. 链接脚本需改为 STM32F405RGTx_FLASH.ld