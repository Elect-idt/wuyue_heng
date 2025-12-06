# 设置构建系统为通用嵌入式系统（无操作系统）
set(CMAKE_SYSTEM_NAME Generic)  # 表示目标平台是嵌入式系统，没有操作系统

# 指定目标处理器架构为ARM
set(CMAKE_SYSTEM_PROCESSOR arm)  # 告知CMake目标平台使用ARM处理器架构

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 明确指定使用GNU编译器
set(CMAKE_C_COMPILER_ID GNU)     # 指定C编译器为GNU GCC
set(CMAKE_CXX_COMPILER_ID GNU)   # 指定C++编译器为GNU G++

# 设置工具链前缀（需确保arm-none-eabi-在系统PATH中）
set(TOOLCHAIN_PREFIX arm-none-eabi-)  # ARM嵌入式工具链前缀

# 配置编译器路径
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)      # 设置C编译器路径
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})       # 使用GCC编译汇编代码
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)    # 设置C++编译器路径
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}gcc)           # 使用GCC作为链接器
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)      # 用于生成HEX/BIN文件
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)            # 用于查看内存占用

# 设置可执行文件后缀为.elf
# 注意：这里只设置C/C++/ASM的可执行文件后缀，避免影响其他语言
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")  # 汇编程序输出为.elf
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")    # C程序输出为.elf
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")  # C++程序输出为.elf

# 设置交叉编译模式（跳过编译器测试）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)  # 加速配置过程，避免完整编译器测试

if (PROJECT_PLATFORM STREQUAL "STM32F4")
    #-----------------------------------------------------------
    # MCU特定编译标志
    # Cortex-M4核心 + FPU + 硬件浮点ABI
    set(TARGET_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

    # 应用目标标志到C编译器
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")

    # 汇编器标志：启用汇编预处理，生成依赖文件
    set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")

    # C编译器附加标志：启用严格警告，分离数据/函数段
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections")

    # 调试/发布模式下的优化设置
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")       # 调试模式：无优化，最大调试信息
    set(CMAKE_C_FLAGS_RELEASE "-Os -g0")     # 发布模式：尺寸优化，无调试信息
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")     # C++调试模式
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")   # C++发布模式

    # C++编译器标志：禁用RTTI和异常，优化静态初始化
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

    # 链接器标志配置
    set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")  # 继承目标架构标志

    # 设置链接脚本路径（指定内存布局）
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F405RGT6_FLASH.ld\"")

    # 使用nano标准库（减小体积）
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")

    # 生成内存映射文件，启用无用段回收
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")

    # 链接标准库（解决循环依赖）
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")

    # 输出内存使用报告
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

    # C++链接器附加标准库支持
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")

    # 以下是有中文注释的部分，是cubemx不会自动生成的，需要手动添加
    # 支持printf打印浮点数
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -u _printf_float")

    # 链接数学库（解决浮点运算）
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -lm")

    # 禁用rwx段警告（常见于嵌入式系统）
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections,--no-warn-rwx-segments")

    # 忽略未使用参数的编译器警告
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
else()

endif()
