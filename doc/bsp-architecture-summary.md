# BSP 层架构设计总结

## 一、依赖倒置架构总览

### 使用的四种设计模式

| 模式 | 在项目中的体现 |
|------|---------------|
| **依赖倒置（DIP）** | Apps 和 Drivers 都依赖 `Bsp_Interface`（抽象层），互不依赖 |
| **抽象工厂** | `board_hw_bsp_t` 聚合所有外设接口，平台切换只换一个描述符实例 |
| **策略模式** | `led_ops_t` 等函数指针表，运行时可替换具体实现 |
| **依赖注入** | `Bsp_Init()` 中 `g_board_hw_bsp_ = &g_stm32f4_bsp_`，将具体工厂注入抽象指针 |

### 依赖方向图

```
        Apps (高层)                  Drivers/STM32F4 (低层)
        应用业务逻辑                  硬件具体实现
            │                              │
            │    都指向抽象，不互相指向       │
            └──────────┬───────────────────┘
                       │
                       ▼
              Bsp_Interface (抽象层)
              board_hw_bsp_t / *_ops_t
              函数指针结构体 = "虚表"
```

### 业界对照：Linux 内核

Linux 内核的 `file_operations` 与本项目 `led_ops_t` 是同一模式：

```c
// Linux 内核 - 文件操作"虚表"
struct file_operations {
    int (*open)(struct inode *, struct file *);
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
};

// 不同文件系统填不同的表
const struct file_operations ext4_fops = { .read = ext4_read, ... };
const struct file_operations fat_fops  = { .read = fat_read, ... };
```

本项目的 `led_ops_t` 和内核的 `file_operations` 本质相同——都是函数指针表实现多态。

### 架构优点

- **可移植**：加新平台只改 `Bsp_Driver`，`Bsp_Interface` 和 `Apps` 零改动
- **可测试**：可注入 mock 驱动做单元测试，不需要真实硬件
- **可协作**：接口层定义后，驱动和应用可并行开发
- **编译保护**：CMake target 隔离，应用层误用平台头文件直接报错

---

## 二、虚函数表（vtable）理解

### 核心概念

"虚"就虚在——**调用方不知道具体实现是谁，只认函数指针的签名（参数和返回值），运行时查表跳转**。同一份调用代码，换一张表就换了一套行为，这就是多态。

### 函数指针表 = 虚表

项目中的 `led_ops_t` 就是一张虚表：

```c
typedef struct {
    const char* name;
    bsp_status_e (*init)(void);           // 槽位0
    bsp_status_e (*control)(led_id_e, led_state_e);  // 槽位1
} led_ops_t;
```

不同平台填入不同地址：

```c
// STM32 平台的虚表
const led_ops_t g_stm32f4_led_driver_ = {
    .name    = "STM32F4_LED_DRIVER",
    .init    = stm32f4_led_init,
    .control = stm32f4_led_control,
};

// 未来 GD32 平台的虚表
const led_ops_t g_gd32f1_led_driver_ = {
    .name    = "GD32F1_LED_DRIVER",
    .init    = gd32f1_led_init,
    .control = gd32f1_led_control,
};
```

调用方代码**完全不变**：

```c
g_board_hw_bsp_->led_ops->control(id, state);
// STM32: 查表 → stm32f4_led_control
// GD32:  查表 → gd32f1_led_control
// 同一行代码，不同行为——多态
```

### 调用跳转过程

以 `g_board_hw_bsp_->led_ops->control(LED_ID_STATUS, LED_TOGGLE)` 为例：

**编译期**：编译器只看到类型定义，不知道具体实现在哪，生成间接调用指令：

```asm
LDR  R0, =g_board_hw_bsp_     ; 加载全局指针变量的地址
LDR  R0, [R0]                 ; 解引用 → 得到 g_stm32f4_bsp_ 的地址
LDR  R1, [R0, #led_ops偏移]    ; 得到 g_stm32f4_led_driver_ 的地址
LDR  R2, [R1, #control偏移]    ; 得到 stm32f4_led_control 的函数地址
BLX  R2                       ; 间接跳转！调用函数指针
```

**链接期**：链接器把所有 `.a` 汇总到可执行文件，解析全局符号地址，但函数指针的目标地址还在 `.rodata` 段中。

**运行时**：`Bsp_Init()` 把 `&g_stm32f4_bsp_` 赋给 `g_board_hw_bsp_`（依赖注入），之后每次调用通过三级指针解引用跳转：

```
g_board_hw_bsp_ → g_stm32f4_bsp_       （选平台）
  →.led_ops     → g_stm32f4_led_driver_ （选外设驱动）
    →.control   → stm32f4_led_control() （选具体函数，BLX跳转）
```

### 与 C++ 的对应关系

C 的函数指针表等价于 C++ 的虚函数：

```cpp
// C++ 写法
class LedOps {
public:
    virtual bsp_status_e init() = 0;          // 虚函数 = 函数指针
    virtual bsp_status_e control(led_id_e, led_state_e) = 0;
};
// C++ 编译器自动生成 vtable（和手写的 led_ops_t 一样的东西）
```

---

## 三、OBJECT 库编译隔离方案

### 要解决的问题

确保 `BSP/common/` 和 `Bsp/bsp_interface.c` 等平台无关代码，**编译时无法引入**任何芯片平台相关的头文件（如 `stm32f4xx.h`、`misc.h`）。

### 方案：CMake target 级别编译隔离

将 BSP 拆成三个 CMake target：

```
bsp_common_interface (INTERFACE)    ← 纯头文件路径，无源文件
        ↑
  Bsp_Interface (OBJECT)           ← 独立编译，只链接 bsp_common_interface
  (bsp_interface.c)                   无法 #include "stm32f4xx.h"
        ↑ .obj 文件合并
  Bsp_Driver (STATIC)              ← 包含 Bsp_Interface 的 .obj + 平台驱动 .obj
  (stm32f4/*.c)                       PRIVATE StdPeriph_Driver
        ↑
  app_task_lib (STATIC)            ← 链接 Bsp_Driver
  (Apps/*.c)                          看不到 STM32 头文件（StdPeriph_Driver 是 PRIVATE）
```

### 为什么用 OBJECT 而不是 STATIC？

`Bsp_Interface` 和 `Bsp_Driver` 之间存在**循环链接依赖**：

- `Bsp_Interface`（`bsp_interface.c`）引用 `g_stm32f4_bsp_`（定义在 `Bsp_Driver` 中）
- `Bsp_Driver` 引用 `board_hw_bsp_t` 类型（定义在 `Bsp_Interface` 头文件中）

如果 `Bsp_Interface` 是 STATIC 库（生成独立 `libBsp_Interface.a`），由于 GNU 链接器从左到右单向扫描 `.a` 文件，CMake 将 `Bsp_Interface.a` 排在 `Bsp_Driver.a` 后面，链接器扫到 `Bsp_Interface.a` 时 `Bsp_Driver.a` 已经过去了，`g_stm32f4_bsp_` 无法解析。

OBJECT 库不生成独立 `.a` 文件，其 `.obj` 被直接合并进 `Bsp_Driver.a`。链接器扫描**同一个 `.a` 内部**时会反复解析所有交叉引用，不存在顺序问题。

```cmake
# Bsp_Interface：OBJECT库，独立编译，不生成独立.a
add_library(Bsp_Interface OBJECT)
target_sources(Bsp_Interface PRIVATE ${CMAKE_SOURCE_DIR}/Bsp/bsp_interface.c)
target_link_libraries(Bsp_Interface PUBLIC bsp_common_interface)

# Bsp_Driver：包含 Bsp_Interface 的 .obj 文件
add_library(Bsp_Driver STATIC)
target_sources(Bsp_Driver PRIVATE
    ${Bsp_Platform_Src}
    $<TARGET_OBJECTS:Bsp_Interface>        # 把 .obj 合并进来
)
target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface PRIVATE StdPeriph_Driver)
```

### 为什么这个循环依赖是正常的？

这不是设计缺陷，而是**依赖注入模式本身的固有特征**——注入器（`Bsp_Init`）必须同时知道"抽象"和"具体实现"。C++ 的 DI 框架、Spring 的配置文件都面临同样情况，只是它们在运行时用反射解决，我们在链接时需要把对象放在一起。

### OBJECT 库不影响架构正确性

- **编译时隔离**：`bsp_interface.c` 编译时只能看到 `bsp_common_interface` 的头文件，看不到 STM32 头文件 ✓
- **依赖倒置方向不变**：Apps → 抽象 ← Drivers ✓
- OBJECT 只影响链接时如何打包（构建系统实现细节），不影响编译时架构

### 其他备选方案

| 方案 | 原理 | 改动量 |
|------|------|--------|
| `--start-group`/`--end-group` | 链接器反复扫描一组库 | 只改 CMake |
| 代码去循环依赖 | 移除 `bsp_interface.c` 对 `g_stm32f4_bsp_` 的引用 | 改 C + CMake |

---

## 四、其他改动记录

### NVIC_PriorityGroupConfig 位置调整

`NVIC_PriorityGroupConfig` 是全局中断分组配置，影响所有中断。从 `bsp_systick.c` 移到 `stm32f4_bsp.c` 的 `stm32f4_platform_init()` 中，通过 `board_hw_bsp_t.platform_init` 回调在 `Bsp_Init()` 中最先调用（在所有外设初始化之前）。
