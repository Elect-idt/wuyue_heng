# wuyue_heng 项目说明

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

### 为什么 Bsp_Interface 是 OBJECT（不是 STATIC）
- bsp_interface.c 和 stm32f4_bsp.c 存在循环符号依赖：
  - bsp_interface.c 引用 g_stm32f4_bsp_（extern）
  - stm32f4_bsp.c 定义 g_stm32f4_bsp_，同时使用 board_hw_bsp_t 类型
- 如果用 STATIC：两个 .o 分属不同 .a，链接器单遍扫描会导致链接顺序报错
- OBJECT 把两个 .o 放进同一个 libBsp_Driver.a，同一个 .a 内链接器反复扫描直到收敛
- $<TARGET_OBJECTS:> 拿来 .o 文件，target_link_libraries 传播头文件路径，两者缺一不可

### OBJECT 库的 PUBLIC 只传播头文件路径
- STATIC + PUBLIC → 传播头文件路径 + -lxxx
- OBJECT + PUBLIC → 只传播头文件路径（没有 .a 可以传播）
- 因此最终链接命令里只有 -lBsp_Driver，不会出现 -lBsp_Interface

## 设计模式
- **依赖倒置**：Apps 和 Drivers 都依赖 Bsp_Interface，互不依赖
- **抽象工厂**：board_hw_bsp_t 聚合所有 *_ops_t，平台切换只换一个描述符
- **策略模式**：*_ops_t 函数指针表（= C++虚表），运行时多态
- **依赖注入**：Bsp_Init() 中 g_board_hw_bsp_ = &g_stm32f4_bsp_

## 编码约定
- BSP/common/ 和 bsp_interface.c 是平台无关的，禁止引入平台头文件
- 外设驱动接口定义在 Bsp/common/*_interface.h，实现在 Bsp/stm32f4/*/
- g_board_hw_bsp_ 是全局单例，Apps 通过它访问所有硬件
- GPIO/USART/SYSTICK 已完成，SPI 驱动未完成
- NVIC_PriorityGroupConfig 在 stm32f4_platform_init() 中调用（Bsp_Init 最先调用）

## 详细知识文档
- `.claude/cmake-linking-knowledge.md` — 链接器行为、编译vs链接详解
- `doc/bsp-architecture-summary.md` — BSP架构设计详解
