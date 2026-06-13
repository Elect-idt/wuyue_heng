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

## $<TARGET_OBJECTS:> vs target_link_libraries 的区别

两者做不同的事，缺一不可：

| | `$<TARGET_OBJECTS:Bsp_Interface>` | `target_link_libraries(Bsp_Driver PUBLIC Bsp_Interface)` |
|---|---|---|
| 作用 | 拿来 .o 文件 | 传播使用需求（头文件路径等） |
| 影响什么 | 最终 .a 里包含什么 | 编译时 -I 参数 |

- 删 $<TARGET_OBJECTS:> → bsp_interface.o 没人用，Bsp_Init() 消失
- 删 target_link_libraries → stm32f4_bsp.c 找不到 bsp_interface.h

## OBJECT 库的 PUBLIC 传播

OBJECT 库没有 .a 文件，所以 PUBLIC 只传播头文件路径，不传播 .o 文件：
- STATIC + PUBLIC → 传播头文件路径 + -lxxx（链接命令加 .a）
- OBJECT + PUBLIC → 只传播头文件路径（没有 .a 可以传播）

因此最终链接命令里不会出现 -lBsp_Interface，只有 -lBsp_Driver。

## 编译 vs 链接

### 编译阶段（.c → .o）
- extern 告诉编译器符号的类型，编译器生成代码时在 .o 里留重定位条目（占位符）
- 不需要知道符号在哪里定义，用 arm-none-eabi-nm 可以看到 U（Undefined）标记

### 链接阶段（.o + .a → .elf）
- 链接器是唯一解析符号的工具
- .o 文件无条件全量读入
- .a 文件惰性拉入：只有当 .o 能满足当前未解析符号时才拉入

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
