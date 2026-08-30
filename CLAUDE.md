# wuyue_heng 项目说明

## 项目概述
- STM32F405RGT6 嵌入式项目，Cortex-M4F，FreeRTOS，168MHz
- CMake 3.22 + Ninja + ARM GCC 14.2 交叉编译，构建脚本 `run.sh`

## 目录结构
```
wuyue_heng/
├── Bsp/                    ← BSP层（依赖倒置架构核心）
│   ├── CMakeLists.txt
│   ├── bsp_interface.c/h       平台工厂（宏选描述符）+ 平台无关调度 + Bsp_Init()
│   ├── common/                 平台无关接口（gpio/usart/spi_ops_t，SysTick已移除）
│   └── stm32f4/                STM32F4平台实现
│       ├── stm32f4_bsp.c/h     聚合所有驱动实例 + platform_init(NVIC分组)
│       ├── isr/                 ISR独立目录（OBJECT库Bsp_ISR，编译完全隔离）
│       │   └── stm32f4xx_it.c/h  中断路由（硬件逻辑封装在各驱动中）
│       ├── gpio/ bsp_gpio.c    GPIO驱动（LED、74HC165 PL等引脚）
│       ├── usart/ bsp_usart.c
│       ├── systick/ bsp_systick.c [DEPRECATED] ← FreeRTOS独占SysTick，保留待TIM替代
│       └── spi/ bsp_spi.c      SPI驱动（DMA + sync注入）
├── Component/              ← 器件协议抽象层（RTOS无关）
│   ├── led/                   LED器件（基于gpio_ops_t，active_low映射）
│   ├── 74hc165/               74HC165移位寄存器（PL+CE+SPI DMA）
│   └── ws2812_led/            WS2812B可寻址RGB灯（SPI 4bit编码，纯写器件）
├── Drivers/                ← STM32 SPL标准外设库
├── FreeRTOS/               ← FreeRTOS内核
├── Apps/                   ← 应用层（LED任务、按键扫描任务、RGB灯效任务）
├── Core/src/               ← main.c, syscalls.c, sysmem.c（平台无关）
└── doc/
    ├── project/                ← 项目文档（评审/修复计划/调试记录/硬件待办）
    └── knowledge/              ← 通用知识文档（换项目仍成立的对照/机制讲解）
```

## CMake target 架构（关键！）
```
bsp_common_interface (INTERFACE)  ← 头文件路径 + STM32F4/SYSCLK_MHZ 宏
        ↑
Bsp_Interface (OBJECT)           ← 编译 bsp_interface.c，只链接 bsp_common_interface
        ↑ $<TARGET_OBJECTS:...>     编译隔离：看不到SPL头文件
Bsp_Driver (STATIC)              ← 包含 Bsp_Interface .obj + stm32f4/*.c
        ↑                            PUBLIC Bsp_Interface, PRIVATE StdPeriph_Driver
component_lib (STATIC)           ← 器件协议层，PUBLIC Bsp_Driver，不依赖FreeRTOS
        ↑
app_task_lib (STATIC)            ← 链接 Bsp_Driver + component_lib + FreeRTOS_Lib
```

### 为什么 Bsp_Interface 是 OBJECT（不是 STATIC）
- bsp_interface.c 是平台工厂，`#if` 宏选的具体描述符 `g_stm32f4_bsp_` 定义在 stm32f4_bsp.c（Bsp_Driver）
- 单向依赖：bsp_interface.o 引用 g_stm32f4_bsp_。若分成两个 .a，链接器单遍扫描会让 bsp_interface.o 落在 libBsp_Driver.a 之后，引用无法满足 → undefined reference
- OBJECT 把 bsp_interface.o 注入 libBsp_Driver.a，同一归档内链接器反复扫描收敛，对链接顺序免疫
- bsp_interface.o 进归档有两条冗余通道（CMake≥3.12）：`$<TARGET_OBJECTS:>` 显式注入 + `target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface)` 自动打包 .o（后者兼传播头文件路径）。CMake 去重，删任一条构建不坏，两条全删 Bsp_Init 才消失

### 编译隔离保证
- Bsp_Interface 只链接 bsp_common_interface → 看不到 stm32f4xx.h
- Bsp_Driver PRIVATE StdPeriph_Driver → SPL头文件不传播给上层
- component_lib 只依赖 Bsp_Driver → 看不到 stm32f4xx.h 和 FreeRTOS
- FreeRTOS PRIVATE periph_drivers_interface → SPL头文件不传播给 Apps
- Core/ 只剩 main.c + syscalls.c + sysmem.c → 无平台依赖
- 项目中零个 add_definitions，全部用 target_compile_definitions 精确控制

## 三层架构

### BSP层 — 平台硬件抽象
- `gpio_ops_t` / `usart_ops_t` / `spi_ops_t` — 纯硬件操作（SysTick 已从公共接口移除，由 FreeRTOS 独占）
- `board_hw_bsp_t` 聚合所有 ops_t，全局单例 `g_board_hw_bsp_`
- `gpio_ops_t.init(pin)` 按引脚ID单独初始化，每个引脚有独立完整配置（mode/speed/otype/pupd）
- SPI DMA 同步通过 `spi_dma_sync_t` 函数指针注入，BSP 不知道 FreeRTOS
- `spi_dma_sync_t.wait` 带 `timeout_ms` 参数和 `bool` 返回值，防止 DMA 异常时永久阻塞

### Component层 — 器件协议（RTOS无关）
- 介于 Apps 和 Bsp_Interface 之间，组合多个 BSP 接口实现器件协议
- LED (`led_t`)：封装 `gpio_ops_t`，处理 active_low 映射
- 74HC165 (`hc165_t`)：组合 `spi_ops_t`（SPI通信）+ `gpio_ops_t`（PL控制）
- 零个 FreeRTOS 头文件，纯 Bsp_Interface 依赖

### Apps层 — 业务逻辑（FreeRTOS任务）
- 创建 FreeRTOS 任务，注入同步机制到 Component
- DMA sync 注入链：Apps 创建 Semaphore → 包装为 `spi_dma_sync_t` → 传给 `hc165_init()`

## 设计模式
- **依赖倒置**：Apps 和 Drivers 都依赖 Bsp_Interface，互不依赖
- **抽象工厂**：board_hw_bsp_t 聚合所有 *_ops_t，平台切换只换一个描述符
- **策略模式**：*_ops_t 函数指针表（= C++虚表），运行时多态
- **依赖注入**：不自己创建依赖，由外部提供。项目中嵌套多层：
  - 具体函数注入到 *_ops_t（如 stm32f4_gpio_write → g_stm32f4_gpio_driver_）
  - 驱动实例注入到 board_hw_bsp_t（如 &g_stm32f4_gpio_driver_ → g_stm32f4_bsp_）
  - 平台描述符绑定到全局指针（&g_stm32f4_bsp_ → g_board_hw_bsp_）；此步由 bsp_interface.c 平台工厂在编译期完成（非外部注入，组合根下沉在 BSP 层）
  - 同步机制注入到 Component（Semaphore → spi_dma_sync_t → hc165_t）

## 分层依赖规则
- 依赖方向单向向下：Apps → Component → Bsp_Interface → StdPeriph_Driver
- Bsp_Driver **不依赖** FreeRTOS（底层不应知道上层 RTOS）
- Component **不依赖** FreeRTOS（器件协议 RTOS 无关）
- 如果 BSP 驱动需要同步机制（如 SPI DMA），通过函数指针从 Apps 层注入
- 芯片可能会换（依赖倒置的价值），RTOS 大概率不换（但仍保持解耦以防万一）

## 编码约定
- BSP/common/ 是平台无关的；bsp_interface.c 是"平台工厂 + 平台无关调度"——编译期用 #if 宏选具体平台描述符（新增平台加 #elif 宏隔离，不波及 Apps）。CMake 编译隔离保证它不能 include 平台头文件（头文件级平台无关），但作为工厂它仍 extern 命名平台描述符
- 外设驱动接口定义在 Bsp/common/*_interface.h，实现在 Bsp/stm32f4/*/
- 器件协议定义在 Component/*/\*.h，组合 BSP 接口实现器件级操作
- **SPI 引脚可选**：`spi_hw_config_t` 的引脚字段填 `SPI_GPIO_PORT_INVALID`（0）表示该引脚不用（如 WS2812 只用 MOSI），gpio_config 逐引脚守卫跳过配置、引脚释放另用；无 CS 的器件 cs_control 为空操作。哨兵只用于 PORT 指针字段，CLK 掩码字段照实填
- **Component 器件统一"预填描述符"构造**：调用方用 C99 指定初始化器预填器件 struct（字段自文档、无参数错位），`xxx_init(dev)` 只做必填字段校验 + 硬件初始化，不再收长参数列表。参考 74hc165.h/led.h 的契约注释。器件出现运行时状态时升级为独立 cfg 结构体 + `init(dev, &cfg)`
- **事务互斥**：多字节/多步器件事务（如 hc165 的 PL+CS+DMA）用 `bsp_lock_t`（Bsp/common）注入保护，Apps 创建锁对象并注入；共享同一总线/器件的多个消费者必须注入**同一个** bsp_lock_t 实例（拓扑决策在组合根）。单次 GPIO 写的简单器件（LED）不需要锁
- g_board_hw_bsp_ 是全局单例，Apps 通过它访问所有硬件
- GPIO/USART/SPI 已完成，SysTick 已从公共接口移除（FreeRTOS 独占）
- NVIC_PriorityGroupConfig 在 stm32f4_platform_init() 中调用（Bsp_Init 最先调用）
- stm32f4xx_it.c 在 `Bsp/stm32f4/isr/` 独立目录，OBJECT 库 Bsp_ISR，只依赖 StdPeriph_Driver
- ISR handler 硬件逻辑封装在各自驱动中（如 `bsp_spi_dma_isr_handler`），ISR 文件只做路由
- `_write()`（printf 重定向）在 `Core/src/syscalls.c` 中，通过 `usart_ops_t` 抽象发送，不在 BSP 驱动文件中
- `configCHECK_FOR_STACK_OVERFLOW` = 2，`vApplicationStackOverflowHook` 和 `vAssertCalled`（断言停车）在 `Apps/common/app_hooks.c`

## 详细知识文档
### 项目文档（doc/project/）
- `bsp-architecture-summary.md` — BSP架构设计详解
- `component-architecture.md` — Component层架构设计
- `architecture-fix-plan-20260609.md` — P0~P3 修复计划与验收
- `硬件待修改.txt` — 硬件待改事项（Q7→SER 接线等）

### 通用知识文档（doc/knowledge/，换项目仍成立）
- `cpp-vs-c-oop-pattern.md` — C实现C++面向对象模式入门对照（vtable、抽象工厂、虚函数）
- `c-oop-static-vs-dynamic.md` — 进阶：ops_t静态注册表 vs 虚表/实例分离动态工厂、多实例组件模板（keypad_dev_t）、vptr脱糖与对象内存布局
- `cpp-build-link-relocation-notes.md` — 编译/链接/重定位机制笔记
