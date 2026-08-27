# wuyue_heng

STM32F405RGT6 嵌入式项目，Cortex-M4F，FreeRTOS，168MHz

## 构建方式

```bash
bash run.sh
```

CMake 3.22 + Ninja + ARM GCC 14.2 交叉编译

## 目录结构

```
wuyue_heng/
├── CMakeLists.txt                  # 项目总入口
├── CLAUDE.md                       # AI 辅助开发的项目说明
├── cmake/                          # CMake 辅助脚本
│   ├── target_config.cmake         # 平台选择、芯片型号
│   └── gcc-arm-none-eabi.cmake     # 交叉编译工具链配置
│
├── Bsp/                            # BSP层（依赖倒置架构核心）
│   ├── CMakeLists.txt
│   ├── bsp_interface.c/h           # 平台工厂（宏选描述符）+ 平台无关调度 + Bsp_Init()
│   ├── common/                     # 平台无关接口（led/usart/systick/spi_ops_t）
│   └── stm32f4/                    # STM32F4平台实现
│       ├── stm32f4_bsp.c/h         # 聚合所有驱动实例 + platform_init
│       ├── stm32f4xx_it.c/h        # 中断处理函数
│       ├── led/bsp_debug_led.c
│       ├── usart/bsp_usart.c
│       ├── systick/bsp_systick.c
│       └── spi/bsp_spi.c           # （未完成）
│
├── Drivers/                        # STM32 SPL 标准外设库
│   └── STM32F4_SPL/
│       ├── CMSIS/                  # ARM CMSIS 核心头文件
│       └── STM32F4xx_StdPeriph_Driver/  # SPL 驱动源码
│
├── FreeRTOS/                       # FreeRTOS 内核
│   ├── inc/                        # FreeRTOS API 头文件
│   ├── src/                        # 内核源码（tasks.c, queue.c 等）
│   └── port/                       # 移植层（ARM_CM4F + heap_4）
│
├── Apps/                           # 应用层（FreeRTOS 任务）
│   ├── app_init.c/h                # 任务创建
│   └── led_status_app/             # LED 状态任务
│
├── Core/                           # 核心文件（平台无关）
│   ├── src/
│   │   ├── main.c                  # 主程序入口
│   │   ├── syscalls.c              # 系统调用重定向（printf → USART）
│   │   └── sysmem.c                # 堆内存管理
│   └── inc/
│       └── main.h
│
├── doc/                            # 文档
│   ├── project/                    # 项目文档（评审/修复计划/调试记录）
│   └── knowledge/                  # 通用知识文档（C/C++ 对照、链接机制）
│
└── .claude/                        # AI 辅助开发配置
    ├── MEMORY.md                   # 跨会话记忆
    ├── cmake-linking-knowledge.md  # CMake 链接机制详解
    └── commands/                   # 自定义 slash command
```

## 架构设计

### 依赖倒置

```
依赖方向单向向下：

Apps（应用层）→ 只调 Bsp_Interface 接口（平台无关）
    ↓
Bsp_Interface（抽象层）→ 定义 *_ops_t 函数指针类型
    ↓
Bsp/stm32f4（实现层）→ 填充函数指针，操作寄存器
    ↓
Drivers（SPL库）→ 寄存器操作函数
```

### CMake target 架构

```
bsp_common_interface (INTERFACE)  ← 头文件路径 + 板级配置宏
        ↑
Bsp_Interface (OBJECT)           ← 编译 bsp_interface.c（编译隔离：看不到SPL头文件）
        ↑ $<TARGET_OBJECTS:...>
Bsp_Driver (STATIC)              ← 包含 Bsp_Interface .o + stm32f4 平台驱动
        ↑                            PUBLIC Bsp_Interface, PRIVATE StdPeriph_Driver
app_task_lib (STATIC)            ← Apps 层，看不到 STM32 头文件
```

### 设计模式

- **依赖倒置**：Apps 和 Drivers 都依赖 Bsp_Interface，互不依赖
- **抽象工厂**：board_hw_bsp_t 聚合所有 *_ops_t，切换芯片只换一个描述符
- **策略模式**：*_ops_t 函数指针表（= C++ 虚表），运行时多态
- **平台工厂装配（组合根在 BSP 层）**：bsp_interface.c 编译期用宏选 g_stm32f4_bsp_，赋值给抽象指针 g_board_hw_bsp_；组合根下沉在 BSP 层以隔离平台改动

## 外设驱动状态

| 外设 | 状态 |
|------|------|
| GPIO/LED | 已完成 |
| USART | 已完成 |
| SYSTICK | 已完成 |
| SPI | 未完成 |
