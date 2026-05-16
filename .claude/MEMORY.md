# wuyue_heng 项目记忆

## 项目概述
- STM32F405RGT6 嵌入式项目，Cortex-M4F，FreeRTOS，168MHz
- CMake 3.22 + Ninja + ARM GCC 14.2 交叉编译，构建脚本 `run.sh`

## 目录结构
```
wuyue_heng/
├── Bsp/                    ← BSP层（依赖倒置架构核心）
│   ├── CMakeLists.txt
│   ├── bsp_interface.c/h       平台调度 + Bsp_Init()
│   ├── common/                 平台无关接口（led/usart/systick/spi_ops_t）
│   └── stm32f4/                STM32F4平台实现
│       ├── stm32f4_bsp.c/h     聚合所有驱动实例 + platform_init(NVIC分组)
│       ├── led/ bsp_debug_led.c
│       ├── usart/ bsp_usart.c
│       ├── systick/ bsp_systick.c
│       └── spi/ bsp_spi.c      （未完成）
├── Drivers/                ← STM32 SPL标准外设库
├── FreeRTOS/               ← FreeRTOS内核
├── Apps/                   ← 应用层（LED任务等）
├── Core/src/               ← main.c, stm32f4xx_it.c
└── doc/
    └── bsp-architecture-summary.md  ← 架构详细文档
```

## CMake target 架构（关键！）
```
bsp_common_interface (INTERFACE)  ← Bsp/common/ + Bsp/ 头文件路径
        ↑
Bsp_Interface (OBJECT)           ← 编译 bsp_interface.c，只链接 bsp_common_interface
        ↑ $<TARGET_OBJECTS:...>     编译隔离：看不到STM32头文件
Bsp_Driver (STATIC)              ← 包含 Bsp_Interface .obj + stm32f4/*.c
        ↑                            PUBLIC Bsp_Interface, PRIVATE StdPeriph_Driver
app_task_lib (STATIC)            ← 链接 Bsp_Driver，看不到STM32头文件
```
- Bsp_Interface 用 OBJECT 库是因为 bsp_interface.c 和 stm32f4_bsp.c 有循环链接依赖
- StdPeriph_Driver 是 Bsp_Driver 的 PRIVATE 依赖，Apps 层看不到 STM32 头文件
- 详细原因见 doc/bsp-architecture-summary.md

## 设计模式
- **依赖倒置**：Apps 和 Drivers 都依赖 Bsp_Interface，互不依赖
- **抽象工厂**：board_hw_bsp_t 聚合所有 *_ops_t，平台切换只换一个描述符
- **策略模式**：*_ops_t 函数指针表（= C++虚表），运行时多态
- **依赖注入**：Bsp_Init() 中 g_board_hw_bsp_ = &g_stm32f4_bsp_
- 对照 Linux 内核 file_operations，模式相同

## 关键约定
- BSP/common/ 和 bsp_interface.c 是平台无关的，禁止引入平台头文件
- 外设驱动接口定义在 Bsp/common/*_interface.h，实现在 Bsp/stm32f4/*/
- g_board_hw_bsp_ 是全局单例，Apps 通过它访问所有硬件
- GPIO/USART/SYSTICK 已完成，SPI 驱动未完成（驱动实例注释掉了）
- NVIC_PriorityGroupConfig 在 stm32f4_platform_init() 中调用（Bsp_Init 最先调用）

## 详细知识文档
- [CMake链接机制详解](cmake-linking-knowledge.md) — OBJECT库原理、链接器行为、编译vs链接
