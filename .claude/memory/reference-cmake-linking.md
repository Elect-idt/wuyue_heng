# CMake 链接机制详解

## 为什么 Bsp_Interface 必须是 OBJECT

### 问题：循环符号依赖
```
bsp_interface.c                    stm32f4_bsp.c
─────────────────                  ──────────────────
extern g_stm32f4_bsp_;      ←────  定义 g_stm32f4_bsp_ = { ... }
g_board_hw_bsp_ = &g_stm32f4_bsp_; ────→ #include "bsp_interface.h"（使用 board_hw_bsp_t 类型）
```

### 如果用 STATIC 库：链接顺序导致报错
假设 Bsp_Interface 是 STATIC，CMake 生成的链接命令：
```
arm-none-eabi-gcc main.o app_task.o -lBsp_Driver -lBsp_Interface -o wuyue_heng.elf
```

链接器从左到右单遍扫描 .a，不回头：
1. 扫描 libBsp_Driver.a → 找 Bsp_Init() → 没有（在 libBsp_Interface.a 里）→ 跳过
2. 扫描 libBsp_Interface.a → 找到 Bsp_Init() → 拉入 bsp_interface.o
3. 新增未解析 g_stm32f4_bsp_ → 但 libBsp_Driver.a 已扫过 → 报错！

### OBJECT 库的解决方式
```cmake
target_sources(Bsp_Driver PRIVATE
    ${Bsp_Platform_Src}
    $<TARGET_OBJECTS:Bsp_Interface>   # bsp_interface.o 直接注入 Bsp_Driver
)
```
- bsp_interface.o 和 stm32f4_bsp.o 在同一个 libBsp_Driver.a 内
- 同一个 .a 内部链接器会反复扫描直到收敛
- 不存在链接顺序问题

### 为什么不能用其他方案
| 方案 | 问题 |
|------|------|
| `--start-group` | 需要在最上层手动管理分组，链接变慢 |
| 合并成一个 STATIC | 失去编译隔离（bsp_interface.c 能看到 STM32 头文件） |
| 注册机制消除循环 | 需要 `__attribute__((constructor))`，裸机不一定支持 |

## $<TARGET_OBJECTS:> vs target_link_libraries 的关系

**两条 .o 注入通道冗余**（CMake>=3.12；2026-08-28 实验验证：单删
$<TARGET_OBJECTS:> 后 build.ninja 归档规则仍含 bsp_interface.c.obj，
ninja 报 no work to do，构建不坏）：

| | `$<TARGET_OBJECTS:Bsp_Interface>` | `target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface)` |
|---|---|---|
| .o 进归档 | 是（显式列出，自文档化） | 是（链接 OBJECT 库到 STATIC 库自动打包 .o） |
| 头文件路径 | 不传播 | 传播（-I 给本 target 和下游） |
| 下游 -l | — | 不产生（OBJECT 无 .a 可链） |

- 两条全删 → bsp_interface.o 无人引用（不在任何 .a、不上链接行），
  Bsp_Init()/g_board_hw_bsp_ 消失，Apps/Core 调用点全部 undefined reference
- 只删 target_link_libraries → stm32f4_bsp.c 编译时找不到 bsp_interface.h
  （.o 注入仍有 $<TARGET_OBJECTS:> 兜底，链接不受影响）

## OBJECT 库的 PUBLIC 传播

OBJECT 库没有 .a 文件，PUBLIC 不给下游传播 -l（下游链接命令不出现
-lBsp_Interface，只有 -lBsp_Driver），但除头文件路径外还会把 .o 打进
**直接消费者**的 STATIC 归档：
- STATIC + PUBLIC → 传播头文件路径 + -lxxx（下游链接命令加 .a）
- OBJECT + PUBLIC → 传播头文件路径 + .o 打进直接消费者的归档
  （不再向下游传播 .o：验证过 libcomponent_lib.a 归档规则中无
  bsp_interface.o，下游用的是已含该 .o 的 libBsp_Driver.a）

## 编译 vs 链接

### 编译阶段（.c → .o）
- extern 告诉编译器符号的类型，编译器生成代码时在 .o 里留重定位条目（占位符）
- 不需要知道符号在哪里定义，用 arm-none-eabi-nm 可以看到 U（Undefined）标记

### 链接阶段（.o + .a → .elf）
- 链接器是唯一解析符号的工具
- .o 文件无条件全量读入
- .a 文件惰性拉入：只有当 .o 能满足当前未解析符号时才拉入

## FreeRTOS 应用钩子的归档提取时机陷阱（实战 2026-08-16）

**现象**：`vApplicationStackOverflowHook` 从 app_init.c 移到独立的 app_hooks.c
（同在 libapp_task_lib.a）后，链接报 undefined reference。

**机理**（比"弱符号不提取"更隐蔽的一类归档陷阱）：
链接器扫描 .a 时，只为"**当前已未定义**"的符号提取 .o。时间线：
1. main.o 引用 AppTaskCreate -> 扫描 libapp_task_lib.a 时提取 app_init.o；
2. app_init.o 引用 xTaskCreate，但**此时没人引用钩子**（tasks.o 还没被提取）
   -> app_hooks.o 留在归档里不提取；
3. 后面的 libFreeRTOS_Lib.a 因 xTaskCreate 提取 tasks.o，它引用钩子
   -> 之后没有归档能提供 -> undefined。

**以前为什么能链接上**：钩子定义在 app_init.o 里，提取 app_init.o（为
AppTaskCreate）时顺带带出钩子定义--**隐蔽的位置耦合**，挪走就断。

**解法**（同 Bsp_ISR 模式）：OBJECT 库注入最终 elf，.o 无条件上链接命令行：
```cmake
add_library(App_Hooks OBJECT)
target_sources(App_Hooks PRIVATE Apps/common/app_hooks.c)
target_link_libraries(App_Hooks PRIVATE freertos_interface)
# 根 CMakeLists：$<TARGET_OBJECTS:App_Hooks> 加入 add_executable 的 sources
```

**普适判据**：凡是"被库 B 引用、定义在库 A、而 A 的提取时机早于 B"的回调
（FreeRTOS 钩子、驱动回调表、注册函数），都不能依赖 STATIC 归档扫描，
要么放会被无条件提取的 .o，要么用 OBJECT 库注入。

## 链接器扫描 .a 的行为
```
同一个 .a 内：
  链接器反复扫描直到没有新的未解析符号（收敛）

不同 .a 之间：
  从左到右单遍扫描，不回头
  前面的 .a 产生的未解析符号只能由后面的 .a 解析
```

## ISR 落入 Default_Handler 死循环的诊断（实战 2026-06-13）

**现象**：某中断触发后 CPU 卡死，GDB 调用栈顶显示 `WWDG_IRQHandler` 之类的
weak 别名，指向 startup.s 的 `Default_Handler: b Infinite_Loop`。
陷阱：GDB 把 Default_Handler 地址显示成首个 `.thumb_set` 别名（WWDG 排第一），
**不代表真是该中断触发**——任何落到 Default_Handler 的中断都这么显示。

**一把梭诊断**：`arm-none-eabi-nm -n xxx.elf | grep <IRQHandler名>`
- `W` + 地址 == Default_Handler → 弱符号**没被覆盖**（病根）
- `T` + 独立地址 → 已正确覆盖

**弱符号未被覆盖的两种成因**：
1. ISR 放 STATIC 库（本文上半主题）→ 链接器不提取 .o 覆盖弱符号。
   解法：OBJECT 库 + `$<TARGET_OBJECTS:>` 注入 elf。
2. **ISR 函数名宏未展开**（本次 bug）：it.c 用 `KEY_SCAN_SPI_RX_DMA_IRQHandler`
   宏但没 include 定义它的头 → 宏不展开 → 编译出名字错误的孤立函数 → 向量表
   的 `DMA1_Stream3_IRQHandler` 仍 weak → 孤立函数无人引用被 `--gc-sections`
   回收。判据：nm it.c.obj 能看到孤立函数（T）+ U 引用，但 elf 里找不到它
   （被 gc），而同 .obj 内被向量表引用的 NMI_Handler 等仍在 elf → 说明 it.o
   链接了、只是这个函数名不对。
   解法：ISR 路由宏（逻辑名→IRQHandler 符号）集中到 `bsp_isr_map.h`，it.c
   include 它（不 include 整个 bsp_spi.h，保持 Bsp_ISR 编译隔离）。
