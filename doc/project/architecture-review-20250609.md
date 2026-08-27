# wuyue_heng 架构审查报告 (2025-06-09)

## 审查范围

完整审查了 BSP 层、Component 层、Apps 层、CMake 构建系统、FreeRTOS 集成及所有架构文档。

---

##   CRITICAL（运行时风险）

### 1. BSP Systick 驱动与 FreeRTOS 的 SysTick 资源冲突

**位置**: `Bsp/stm32f4/systick/bsp_systick.c` + `Bsp/stm32f4/stm32f4xx_it.c:131-142`

**问题链**:
1. `Bsp_Init()` 在 `bsp_interface.c:48` 调用 `systick_ops->init(SYSTICK_ID_DEFAULT, SYSCLK_MHZ)`，配置 SysTick 为 HCLK/8 时钟源，计算 `fac_us` / `fac_ms`
2. 紧接着 `vTaskStartScheduler()` 启动后，**FreeRTOS 重新初始化 SysTick** 为 HCLK（不是 HCLK/8），周期 = 1ms
3. 此后任何代码调用 `delay_us()` / `delay_ms()` 会直接覆写 `SysTick->LOAD` / `SysTick->VAL` / `SysTick->CTRL`，**破坏 FreeRTOS 滴答计数**

作者自己的注释也承认：`bsp_systick.c:23` — "freertos会重新初始化systick，所以这个驱动意义不大"

**建议方案**: 让 `systick_ops_t` 的 delay 函数在调度器启动后使用硬件定时器（如 TIM2-TIM5）做延时，或直接使用 FreeRTOS 的 `vTaskDelay()` 替代。SysTick 在 FreeRTOS 启动后应视为 RTOS 独占资源。这是**硬件资源所有权在架构上没有明确定义**的根本问题。

### 2. `spi_send_byte` / `spi_receive_byte` 超时计数器未重置

**位置**: `Bsp/stm32f4/spi/bsp_spi.c:213-251` 和 `Bsp/stm32f4/spi/bsp_spi.c:270-325`

```c
// bsp_spi.c:225-250 — 同一个 stm32f4_timeout 被多个wait循环复用
uint32_t stm32f4_timeout = SPI_TIME_OUT;
// wait TXE — 消耗部分计数
while (SPI_GetFlagStatus(..., SPI_FLAG_TXE) != SET) {
    if (stm32f4_timeout-- == 0) return BSP_STAT_TIME_OUT; // ← 假设这里消耗了 N 次
}
SPI_SendData(KEY_SCAN_SPI, send_data);
// wait RXNE — 剩余 (SPI_TIME_OUT - N) 次就超时！BUG！
while (SPI_GetFlagStatus(..., SPI_FLAG_RXNE) != SET) {
    if (stm32f4_timeout-- == 0) return BSP_STAT_TIME_OUT;
}
```

**建议**: 每个 `while` 循环前独立重置 `stm32f4_timeout = SPI_TIME_OUT`。

### 3. `Bsp_Init()` 的 `|=` 错误累积模式与枚举值不兼容

**位置**: `Bsp/bsp_interface.c:37-51`

```c
// bsp_common_def.h
BSP_STAT_ERROR = -1,       // 0xFFFFFFFF — 设置所有位
BSP_STAT_TRUE  = 0,        // 0x00000000
BSP_STAT_CHOOSE_ERROR_TARGET = 1,   // bit 0
BSP_STAT_INVALID_PARAMS = 2,        // bit 1
BSP_STAT_TIME_OUT = 4,              // bit 2

// bsp_interface.c
status |= g_board_hw_bsp_->platform_init();   // 若有错返回 -1 → status=0xFFFFFFFF ✓ (碰巧)
status |= g_board_hw_bsp_->usart_ops->init();  // 若有错返回 1 → status=0xFFFFFFFF|1=0xFFFFFFFF
status |= g_board_hw_bsp_->systick_ops->init(); // 若有错返回 2 → status=0xFFFFFFFF|2=0xFFFFFFFF
```

`BSP_STAT_ERROR` 碰巧是全 1 所以 OR 后能"保留"错误信息。但如果有多个非 `BSP_STAT_ERROR` 的错误同时发生（如返回 1 和 2），结果是 `1|2=3`，**无法区分是哪种错误**。而且 `3` 不等于任何已定义的错误码。

**建议**: 两种方案：
- **方案A**: 将错误码改为真正的 bit flags（`1<<0, 1<<1, 1<<2`），且 `BSP_STAT_ERROR` 改为一个特定的 reserved bit
- **方案B**: 改为短路模式 — 第一个非零就返回，不累积

---

##   HIGH（设计脆弱性，潜在大问题）

### 4. `_write()` 在静态库中 — printf 支持建立在巧合之上

**位置**: `Bsp/stm32f4/usart/bsp_usart.c:457-469` vs `Core/src/syscalls.c:80-90`

```
syscalls.c (直接链接入 .elf)
└── _write() [weak] → __io_putchar() [weak] → 地址 0 → HardFault ❌

bsp_usart.c (在 libBsp_Driver.a 中)
└── _write() [strong] → USART_SendData(...) ✓
```

强符号 `_write` 之所以能被链接器提取，**仅仅因为** `g_stm32f4_usart_driver_` 也定义在同一个 `bsp_usart.o` 中，且被 `stm32f4_bsp.o` 引用。如果：
- 把 `_write` 移到单独文件
- 条件编译移除 USART 驱动
- 调整编译单元拆分

→ `bsp_usart.o` 不再被提取 → `_write` 回退到 syscalls.c 的 weak 版本 → `printf` 直接 HardFault。

**建议**: 将 `_write` 函数移到 `Core/src/syscalls.c`，调用 `g_board_hw_bsp_->usart_ops->usart_send_byte()`。这样 printf 支持永久可靠，且不绕过 BSP 抽象层直接调 SPL API。

### 5. SPI DMA 全局状态不是多设备安全的

**位置**: `Bsp/stm32f4/spi/bsp_spi.c:24-25` + `Bsp/stm32f4/stm32f4xx_it.c:168-180`

```c
// 全局单例 — 所有 SPI 设备、所有调用者共享
spi_dma_sync_t *g_spi_dma_sync_ptr = NULL;
```

当前只有一个 SPI 设备（`SPI_ID_KEY_SACN`），但如果后续添加 LCD SPI、SPI Flash 等，多个设备共享一个 `g_spi_dma_sync_ptr`，DMA 完成通知可能发给错误的任务。

**建议**: 将 `g_spi_dma_sync_ptr` 改为按 `spi_id_e` 索引的数组：
```c
static spi_dma_sync_t *g_spi_dma_sync_ptrs[SPI_ID_MAX];
```

### 6. `spi_dma_sync_t*` 的 const 被强制转换丢弃

**位置**: `Bsp/stm32f4/spi/bsp_spi.c:427` → `Bsp/stm32f4/spi/bsp_spi.c:442`

```c
// 接口声明:                                      实际存储:
static bsp_status_e spi_receive_multi_data_dma(...,
    const spi_dma_sync_t *sync);                 g_spi_dma_sync_ptr = (spi_dma_sync_t *)sync;
                                                // ↑ 强制丢弃 const
```

`sync` 参数被声明为 `const` 是正确的（BSP 不应修改同步对象）。但 `g_spi_dma_sync_ptr` 是非 const 全局指针，导致强制类型转换。

**建议**: 将 `g_spi_dma_sync_ptr` 声明为 `const spi_dma_sync_t *`。

### 7. FreeRTOSConfig.h 中 SysTick 宏映射与实际 ISR 注释不一致

**位置**: `FreeRTOS/inc/FreeRTOSConfig.h:304` vs `Bsp/stm32f4/stm32f4xx_it.c:131-142`

```c
// FreeRTOSConfig.h
#define vPortSysTickHandler SysTick_Handler    // 声称重命名

// stm32f4xx_it.c
// void SysTick_Handler(void) { xPortSysTickHandler(); }  // 被注释掉了！
```

实际上 SysTick 由 FreeRTOS port.c 中的定义覆盖。但三种 ISR（SVC/PendSV/SysTick）的注释状态没有文档说明。新开发者可能取消注释导致重复定义。

**建议**: 在注释处明确说明："SysTick_Handler/SVC_Handler/PendSV_Handler 由 FreeRTOS port.c 定义，此处不能重复定义"。

---

##   MEDIUM（可维护性问题）

### 8. `aux_source_directory` 全局引入源文件

**位置**: `Bsp/CMakeLists.txt:57-61`, `Apps/CMakeLists.txt:17-20`, `Component/CMakeLists.txt:15-16`

```cmake
aux_source_directory(${CMAKE_SOURCE_DIR}/Bsp/stm32f4/gpio Bsp_Platform_Src)
```

此命令无差别收集目录下**所有** `.c` 文件。如果有人在目录里放临时测试文件、替代实现备份，都会被自动编译进固件。

**建议**: 明确列出源文件，替代 `aux_source_directory`。

### 9. `run.sh` 中 IOC 文件命名推导脆弱

**位置**: `run.sh:19`

```bash
ioc_file=$(basename ../*.ioc .ioc)   # glob 展开：0个或多个 .ioc 文件都会出错
elf_file="$ioc_file.elf"
```

根 CMakeLists.txt 中项目名硬编码为 `wuyue_heng`，`run.sh` 却从 IOC 文件名推导，二者不一致。

### 10. `__io_putchar` 弱符号悬挂引用

**位置**: `Core/src/syscalls.c:35-36` + `Core/src/syscalls.c:74` + `Core/src/syscalls.c:87`

```c
extern int __io_putchar(int ch) __attribute__((weak));  // 无强符号实现！
```

如果没有强符号 `_write` 覆盖，调用链 `printf → _write(weak) → __io_putchar(weak)` 全部解析到地址 0。

**建议**: 在 syscalls.c 中删除弱 `_write`（因为强 `_write` 在 bsp_usart.c 中），并增加 `__io_putchar` 的默认实现或在文档中说明依赖关系。

### 11. `spi_dma_sync_t` 定义位置不当

**位置**: `Bsp/common/bsp_spi_interface.h:18-23`

```c
typedef struct {
    void *handle;
    void (*wait)(void *handle);
    void (*notify_from_isr)(void *handle);
} spi_dma_sync_t;
```

`spi_dma_sync_t` 是**同步机制类型**（包含 `handle`、`wait`、`notify_from_isr`），不属于 SPI 硬件操作范畴。将其放在 SPI 接口文件中混淆了关注点。

**建议**: 移到独立的同步接口头文件（如 `bsp_sync_interface.h`），SPI/USART 等需要同步的驱动都引用它。

### 12. FreeRTOSConfig.h 中包含 `<stdio.h>`

**位置**: `FreeRTOS/inc/FreeRTOSConfig.h:73`

```c
#include <stdio.h>  // 仅为 configASSERT 中的 printf 使用
```

在头文件中包含 `<stdio.h>` 会将标准库依赖传播给所有包含 FreeRTOSConfig.h 的编译单元。应该仅在需要 `printf` 的 `configASSERT` 实现中按需包含。

---

##   LOW（改善建议）

### 13. `hc165_init` 的参数设计可简化

**位置**: `Component/74hc165/74hc165.h:19-21`

```c
void hc165_init(hc165_t *dev, const spi_ops_t *spi_ops, spi_id_e id,
                const spi_dma_sync_t *sync, uint8_t num_chips,
                const gpio_ops_t *gpio_ops, gpio_pin_e pl_pin);
```

7 个参数，全部已存在于 `hc165_t` 结构体字段中。可改为只传 `hc165_t *dev`，让调用方预填结构体（C 中惯用的"命名参数"模式）。

### 14. GPIO 宏命名暗示特殊用途

**位置**: `Bsp/stm32f4/gpio/bsp_gpio.h:9-11`

宏名 `LED_R_*` 暗示红色 LED，但这是一个通用 GPIO 驱动文件。更好的命名是 `GPIO_PIN_LED_STATUS_PORT` 等。

### 15. `g_spi_dma_isr_count` 调试变量仅累加不消费

**位置**: `Bsp/stm32f4/spi/bsp_spi.c:25` + `Bsp/stm32f4/stm32f4xx_it.c:173`

这个计数器在 ISR 中递增但从未被任何逻辑读取或复位。要么移除，要么配套实现健康监测。

### 16. FreeRTOS V9.0.0 (2016) 版本较老

当前使用 FreeRTOS V9，V10+ 提供了更好的静态分配 API、Stream Buffer、Idle 任务可释放内存等改进。

### 17. 缺少 `.clang-format`

代码风格不一致（K&R vs Allman 括号混用），建议添加格式化配置。

---

## 架构健康的方面

大方向是正确的，以下设计值得肯定：

| 方面 | 评价 |
|------|------|
| 依赖倒置方向 | ✅ Apps→Component→BSP_Interface←BSP_Driver，方向正确 |
| CMake 编译隔离 | ✅ Bsp_Interface(OBJECT) + PRIVATE StdPeriph_Driver，强制隔离 |
| Component 层 RTOS 无关 | ✅ 零个 FreeRTOS 头文件，同步通过函数指针注入 |
| 弱符号+静态库陷阱 | ✅ 已用 Bsp_ISR(OBJECT) 解决，方法论已沉淀到文档 |
| SPI DMA sync 注入链 | ✅ Apps→Semaphore→spi_dma_sync_t→hc165_t，链式依赖注入清晰 |
| ISR 链接规则 | ✅ 所有覆盖弱符号的 ISR 文件通过 OBJECT 库注入可执行文件 |

---

## 修复优先级建议

1. **P0（立即）**: SysTick 冲突（#1）— 架构层面定义 SysTick 硬件资源所有权
2. **P0（立即）**: SPI 超时计数器（#2）— 纯代码 bug，可能造成假超时
3. **P1（近期）**: `_write` 位置（#4）— 消除 printf HardFault 隐患
4. **P1（近期）**: SPI DMA 多设备安全（#5）— 为多 SPI 设备场景做准备
5. **P1（近期）**: Bsp_Init 错误累积（#3）— 修正错误码设计
6. **P2（计划中）**: const 正确性（#6）、ISR 注释文档（#7）、sync 类型位置（#11）
7. **P3（可延后）**: aux_source_directory（#8）、IOC 命名（#9）、代码风格（#17）
