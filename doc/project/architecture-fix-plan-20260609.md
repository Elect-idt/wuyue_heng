# wuyue_heng 架构修复计划

> 综合三份审查来源：Sonnet 深度分析、Opus 4.6 复审、本会话逐行代码审查。
> 去重合并后按优先级分层，每条包含：问题描述、实际影响、修复方案（含代码）。

---

## 优先级定义

| 级别 | 含义 | 修复时间建议 |
|------|------|-------------|
| **P0** | 运行时 Bug，当前代码可直接导致死循环/数据错误/系统崩溃 | 立即修复 |
| **P1** | 健壮性隐患，异常场景下可导致任务阻塞/信息丢失/调试困难 | 本周内修复 |
| **P2** | 架构/可维护性债务，影响代码扩展和团队协作 | 计划中修复 |
| **P3** | 编码风格/细节改善，不影响功能 | 有空时修复 |

---

## P0 — 运行时 Bug（立即修复）

### FIX-01. `usart_send_array` 循环计数器溢出导致死循环

**文件**: `Bsp/stm32f4/usart/bsp_usart.c:407`

**问题**: `uint8_t i` 作为循环计数器，但 `num` 是 `uint16_t`。当 `num > 255` 时 `i` 回绕到 0，永远无法达到 `num`，函数死循环。

```c
// 当前代码（BUG）
static bsp_status_e stm32f4_usart_send_array(uasrt_id_e id, uint8_t* array, uint16_t num)
{
    uint8_t i;                    // ← 最大 255
    for (i = 0; i < num; i++)    // num 可达 65535
```

**影响**: 任何调用方传入 `num > 255` 时系统卡死，看门狗复位。

**修复**: 改为 `uint16_t`（1 行改动）：

```c
static bsp_status_e stm32f4_usart_send_array(uasrt_id_e id, uint8_t* array, uint16_t num)
{
    uint16_t i;
    for (i = 0; i < num; i++)
```

---

### FIX-02. `usart_send_string` 空字符串时发送 `\0` 字节

**文件**: `Bsp/stm32f4/usart/bsp_usart.c:274-278`（三处相同模式）

**问题**: `do-while` 先发送再检查，空字符串 `""` 会把 `\0` 发到串口线上。

```c
// 当前代码（BUG）
do {
    stm32f4_usart_send_byte(id, *(str + k));  // str[0] == '\0' 也发送
    k++;
} while (*(str + k) != '\0');
```

**影响**: 二进制协议场景（指纹模块、蓝牙模块）收到意外 `0x00` 可能导致协议解析异常。

**修复**: 改为 `while` 循环（三处 switch-case 都要改）：

```c
// 修复后
while (str[k] != '\0') {
    stm32f4_usart_send_byte(id, str[k]);
    k++;
}
```

---

### FIX-03. `systick_delay_s` 循环计数器溢出

**文件**: `Bsp/stm32f4/systick/bsp_systick.c:139-140`

**问题**: `u16 i` 对比 `uint32_t s * 2`，当 `s >= 32768` 时 `i` 溢出，无限循环。

```c
// 当前代码（BUG）
u16 i;
for (i = 0; i < s * 2; i++)
```

**影响**: 虽然实际场景不太可能调用 `delay_s(32768)`（等 9 小时），但这是一个明确的类型 Bug，且违反 MISRA-C 隐式类型转换规则。

**修复**:

```c
// 修复后
uint32_t i;
for (i = 0; i < s * 2; i++)
```

---

### FIX-04. SPI polling 超时计数器多循环复用

**文件**: `Bsp/stm32f4/spi/bsp_spi.c:211-261` 和 `Bsp/stm32f4/spi/bsp_spi.c:270-325`

**问题**: 同一个 `stm32f4_timeout` 变量在 `spi_send_byte` 和 `spi_receive_byte` 中被多个 `while` 循环复用。第一个循环消耗部分计数后，第二个循环继承剩余值，可能导致假超时。

```c
// 当前代码（BUG）
uint32_t stm32f4_timeout = SPI_TIME_OUT;
// ... wait TXE（消耗 N 次）
// ... wait RXNE（只剩 SPI_TIME_OUT - N 次）
// ... wait BSY（只剩 SPI_TIME_OUT - N - M 次）
```

**影响**: 在 SPI 总线较慢或从设备响应延迟时，第二个/第三个等待可能误报超时，数据传输被提前中止。

**修复**: 每个 `while` 循环前独立重置超时计数器：

```c
// spi_send_byte 修复示例
uint32_t stm32f4_timeout;

// wait TXE
stm32f4_timeout = SPI_TIME_OUT;
while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_TXE) != SET) {
    if (stm32f4_timeout-- == 0) return BSP_STAT_TIME_OUT;
}
SPI_SendData(KEY_SCAN_SPI, send_data);

// wait RXNE
stm32f4_timeout = SPI_TIME_OUT;
while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_RXNE) != SET) {
    if (stm32f4_timeout-- == 0) return BSP_STAT_TIME_OUT;
}
(void)SPI_ReceiveData(KEY_SCAN_SPI);

// wait BSY
stm32f4_timeout = SPI_TIME_OUT;
while (SPI_GetFlagStatus(KEY_SCAN_SPI, SPI_FLAG_BSY) != RESET) {
    if (stm32f4_timeout-- == 0) return BSP_STAT_TIME_OUT;
}
```

`spi_receive_byte` 做相同修改。

---

### FIX-05. SysTick 驱动与 FreeRTOS 资源冲突

**文件**:
- `Bsp/bsp_interface.h:35` — `const systick_ops_t *systick_ops`
- `Bsp/bsp_interface.c:49` — `systick_ops->init(SYSTICK_ID_DEFAULT, SYSCLK_MHZ)`
- `Bsp/stm32f4/systick/bsp_systick.c` — 直接操作 `SysTick->LOAD/VAL/CTRL`

**问题**:
1. `Bsp_Init()` 初始化 SysTick（HCLK/8），`vTaskStartScheduler()` 立即重新初始化（HCLK，1ms 周期），`Bsp_Init` 的工作全部浪费
2. 调度器启动后任何调用 `delay_us`/`delay_ms` 都会覆写 SysTick 寄存器，**破坏 FreeRTOS tick 节拍**
3. `board_hw_bsp_t` 公开暴露 `systick_ops`，误导后续开发者使用
4. 代码自身注释承认："freertos会重新初始化systick，所以这个驱动意义不大"

**影响**: 如果上层代码（Apps/Component）调用 `g_board_hw_bsp_->systick_ops->delay_ms()`，会导致系统时间漂移、任务调度异常、甚至 HardFault。

**修复**（分步）:

**Step 1** — 从公共接口中移除 SysTick ops（上层目前无调用点，安全）：

```c
// bsp_interface.h — 删除此字段
typedef struct {
    bsp_status_e (*platform_init)(void);
    const gpio_ops_t *gpio_ops;
    const usart_ops_t *usart_ops;
    // const systick_ops_t *systick_ops;  ← 删除此行
    const spi_ops_t *spi_ops;
} board_hw_bsp_t;
```

**Step 2** — 从 `Bsp_Init()` 移除 SysTick 初始化调用：

```c
// bsp_interface.c — 删除此行
// status |= g_board_hw_bsp_->systick_ops->init(SYSTICK_ID_DEFAULT, SYSCLK_MHZ);
```

**Step 3** — 从 `g_stm32f4_bsp_` 移除字段：

```c
// stm32f4_bsp.c — 删除此行
// .systick_ops = &g_stm32f4_systick_driver_,
```

**Step 4**（可选，后续迭代）— 保留 `bsp_systick.c` 和 `bsp_systick_interface.h` 文件，添加 deprecation 注释。如需微秒级延时，引入 DWT cycle counter：

```c
// 未来: bsp_dwt.c
static inline void dwt_delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}
```

---

### FIX-06. 74HC165 NULL sync 合约违反

**文件**:
- `Component/74hc165/74hc165.h:12` — `const spi_dma_sync_t *dma_sync; // NULL 则用轮询`
- `Component/74hc165/74hc165.c:27-28` — 无条件调用 DMA 版 API

**问题**: 头文件声明 `dma_sync == NULL` 时走轮询，但 `hc165_read()` 无条件调用 `spi_receive_multi_data_dma(..., dev->dma_sync)`。当 `sync == NULL` 时，BSP 层的 DMA 函数会启动 DMA 后跳过等待直接 cleanup，**数据可能未完成传输就被返回**。

**影响**: 如果任何调用方构造 `dma_sync == NULL` 的 `hc165_t`，读取数据不可靠。当前 `Key_Scan_Task` 传非空 sync 不受影响，但这是一个接口契约 Bug。

**修复**:

```c
// 74hc165.c — hc165_read() 开头加判断
bsp_status_e hc165_read(hc165_t *dev, uint8_t *buf)
{
    /* sync 为 NULL 时走轮询（遵守头文件契约） */
    if (dev->dma_sync == NULL) {
        return hc165_read_polling(dev, buf);
    }

    /* 原有 DMA 流程不变 */
    dev->gpio_ops->write(dev->pl_pin, GPIO_LOW);
    // ...
}
```

---

## P1 — 健壮性隐患（本周内修复）

### FIX-07. SPI DMA 等待无超时 — 任务可永久阻塞

**文件**:
- `Bsp/stm32f4/spi/bsp_spi.c:460-464,521-525` — `sync->wait(sync->handle)`
- `Apps/key_scan_app/key_scan_app.c:27` — `xSemaphoreTake(..., portMAX_DELAY)`
- `Bsp/stm32f4/stm32f4xx_it.c:168-180` — ISR 只处理 TC

**问题**:
1. DMA 传输后 `xSemaphoreTake(portMAX_DELAY)` 无超时，如果中断不来（ISR 未链接、DMA 错误、硬件异常），任务永久阻塞
2. ISR 只处理了 TC（Transfer Complete），没有处理 TE（Transfer Error）/ FE/DME 等 DMA 错误标志，错误时上层永远等不到通知

**影响**: Key_Scan_Task 卡死后，按键扫描停止工作。如果是安全关键功能（紧急停止按钮），可能造成安全事故。

**修复**（改动 `spi_dma_sync_t` 签名，当前只有一个使用方，范围可控）:

**Step 1** — 修改接口签名，增加 timeout 参数：

```c
// bsp_spi_interface.h
typedef struct {
    void *handle;
    bool (*wait)(void *handle, uint32_t timeout_ms);       // 返回 true=收到通知, false=超时
    void (*notify_from_isr)(void *handle);
} spi_dma_sync_t;
```

**Step 2** — Apps 侧适配：

```c
// key_scan_app.c
static bool spi_dma_wait(void* handle, uint32_t timeout_ms) {
    return xSemaphoreTake((SemaphoreHandle_t)handle, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
```

**Step 3** — BSP SPI DMA 函数检查返回值：

```c
// bsp_spi.c — spi_receive_multi_data_dma 中
if (sync && sync->wait) {
    bool ok = sync->wait(sync->handle, 100);  // 100ms 超时
    if (!ok) {
        spi_dma_cleanup();
        return BSP_STAT_TIME_OUT;
    }
}
```

**Step 4** — ISR 补充 DMA 错误处理：

```c
// stm32f4xx_it.c
void KEY_SCAN_SPI_RX_DMA_IRQHandler(void)
{
    /* 传输完成 */
    if (DMA_GetITStatus(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC)) {
        DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC);
        g_spi_dma_isr_count++;
        if (g_spi_dma_sync_ptr && g_spi_dma_sync_ptr->notify_from_isr)
            g_spi_dma_sync_ptr->notify_from_isr(g_spi_dma_sync_ptr->handle);
    }
    /* 传输错误 — 也通知等待任务，避免死锁 */
    if (DMA_GetITStatus(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TEIF0)) {
        DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, DMA_IT_TEIF0);
        if (g_spi_dma_sync_ptr && g_spi_dma_sync_ptr->notify_from_isr)
            g_spi_dma_sync_ptr->notify_from_isr(g_spi_dma_sync_ptr->handle);
    }
}
```

---

### FIX-08. USART 发送函数无超时保护

**文件**: `Bsp/stm32f4/usart/bsp_usart.c` — 所有 send 函数

**问题**: SPI polling 版本有 `SPI_TIME_OUT` 保护，而 USART 的 `send_byte`/`send_string`/`send_hex` 全部是无限 `while` 循环等待 `TXE`/`TC` 标志。如果 USART 外设异常（时钟未使能、总线错误），系统死锁。

**影响**: USART 硬件异常时系统卡死。SPI 有超时而 USART 没有，防御性编程不一致。

**修复**: 添加超时宏，所有 busy-wait 循环加超时保护：

```c
// bsp_usart.h
#define USART_TIME_OUT  0x1000

// bsp_usart.c — send_byte 示例
static bsp_status_e stm32f4_usart_send_byte(uasrt_id_e id, uint8_t ch)
{
    uint32_t timeout = USART_TIME_OUT;
    switch (id) {
    case USART_ID_DEBUG:
        USART_SendData(DEBUG_USART, ch);
        while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET) {
            if (timeout-- == 0) return BSP_STAT_TIME_OUT;
        }
        break;
    // ... 其他 case 同理
    }
    return BSP_STAT_TRUE;
}
```

`send_string`、`send_hex`、`_write()` 中所有 `while` 循环同理修改。

---

### FIX-09. `configASSERT` 宏 if 半截问题

**文件**: `FreeRTOS/inc/FreeRTOSConfig.h:84`

**问题**: 宏展开为裸 `if` 语句，在 `if-else` 上下文中会导致 else 悬空。

```c
// 当前代码（BUG）
#define configASSERT(x) if((x)==0) vAssertCalled(__FILE__,__LINE__)
```

**影响**: 虽然当前代码中 `configASSERT` 都不在 `if-else` 中使用，但这是一个经典的宏陷阱，未来随时可能踩中。

**修复**（1 行）:

```c
#define configASSERT(x) do { if((x)==0) vAssertCalled(__FILE__,__LINE__); } while(0)
```

---

### FIX-10. `xSemaphoreCreateBinary()` 返回值未检查

**文件**: `Apps/key_scan_app/key_scan_app.c:42`

**问题**: 如果创建失败（FreeRTOS heap 不足），`s_spi_dma_sem` 为 NULL，后续 `xSemaphoreGiveFromISR(NULL, ...)` 导致未定义行为。

**影响**: 在内存紧张场景下可能导致 HardFault。

**修复**（1 行）:

```c
s_spi_dma_sem = xSemaphoreCreateBinary();
configASSERT(s_spi_dma_sem != NULL);  // ← 添加此行
```

---

### FIX-11. `_write()` 放在 BSP 驱动文件中 — printf 支持建立在巧合之上

**文件**:
- `Bsp/stm32f4/usart/bsp_usart.c:457-469` — 强定义 `_write()`
- `Core/src/syscalls.c:80-90` — weak `_write()`

**问题**: 强符号 `_write` 能被链接器提取，**仅仅因为** `g_stm32f4_usart_driver_` 也定义在同一个 `.o` 中。如果拆分编译单元或条件编译移除 USART 驱动，`bsp_usart.o` 不再被提取 → `_write` 回退到 weak 版本 → `printf` 直接 HardFault。同时 `_write()` 直接调 SPL API（`USART_SendData`），绕过了 BSP 抽象层。

**影响**: printf 支持脆弱，依赖编译单元布局的巧合。

**修复**:

**Step 1** — 从 `bsp_usart.c` 中删除 `_write()` 函数。

**Step 2** — 在 `Core/src/syscalls.c` 中实现强 `_write()`，走 BSP 抽象：

```c
// syscalls.c
#include "bsp_interface.h"

int _write(int file, char* ptr, int len)
{
    if (g_board_hw_bsp_ && g_board_hw_bsp_->usart_ops) {
        for (int i = 0; i < len; i++) {
            g_board_hw_bsp_->usart_ops->usart_send_byte(USART_ID_DEBUG, (uint8_t)ptr[i]);
        }
    }
    return len;
}
```

注意：`syscalls.c` 通过 `Bsp_Interface` 只能看到 `usart_ops_t` 接口（不依赖 SPL），CMake 编译隔离保证这一点。需确认 `syscalls.c` 的 target 能看到 `bsp_interface.h`（当前 Core/ 目录的 target link 了 Bsp_Driver，可以传播）。

---

### FIX-12. `Bsp_Init()` 错误累积 `|=` 语义不严谨

**文件**: `Bsp/bsp_interface.c:44-49`

**问题**: `BSP_STAT_CHOOSE_ERROR_TARGET=1` 和 `BSP_STAT_INVALID_PARAMS=2` 做 OR 后得到 `3`，不等于任何已定义错误码。调用方无法区分具体哪个步骤失败。

```c
// 当前代码
status |= g_board_hw_bsp_->platform_init();    // 返回 -1, 0, 1, 2, 4 之一
status |= g_board_hw_bsp_->usart_ops->init();  // | 操作混合非 bit-flag 枚举值
status |= g_board_hw_bsp_->systick_ops->init();
```

**影响**: 初始化失败时无法定位具体故障环节。

**修复**（短路模式，第一个错误就返回）：

```c
bsp_status_e Bsp_Init(void)
{
    g_board_hw_bsp_ = &BSP_DRIVER_INTERFACE;

    bsp_status_e status;

    status = g_board_hw_bsp_->platform_init();
    if (status != BSP_STAT_TRUE) return status;

    status = g_board_hw_bsp_->usart_ops->init(USART_ID_DEBUG);
    if (status != BSP_STAT_TRUE) return status;

    // systick 初始化已移除（见 FIX-05）

    return BSP_STAT_TRUE;
}
```

---

### FIX-13. `configCHECK_FOR_STACK_OVERFLOW` 关闭

**文件**: `FreeRTOS/inc/FreeRTOSConfig.h:223`

**问题**: 开发阶段栈溢出检测关闭，栈溢出是最常见的嵌入式 Bug 之一，关闭后只能通过 HardFault 间接发现。

**影响**: 栈溢出时无诊断信息，只能看到 HardFault 死循环，难以定位根因。

**修复**:

```c
// FreeRTOSConfig.h
#define configCHECK_FOR_STACK_OVERFLOW  2   // 最严格检测

// 同时需要在某处实现 hook（例如 app_init.c 或单独文件）
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while (1);
}
```

---

### FIX-14. DMA 停止等待无超时

**文件**: `Bsp/stm32f4/spi/bsp_spi.c:340-341` 和 `Bsp/stm32f4/spi/bsp_spi.c:377-378`

**问题**: DMA 流停止后等待完全停止的循环没有超时保护。

```c
while (DMA_GetCmdStatus(KEY_SCAN_SPI_TX_DMA_STREAM) != DISABLE)
    ;  // ← 无超时，DMA 硬件异常时死循环
```

**影响**: DMA 总线错误时系统卡死。

**修复**:

```c
uint32_t dma_timeout = SPI_TIME_OUT;
while (DMA_GetCmdStatus(KEY_SCAN_SPI_TX_DMA_STREAM) != DISABLE) {
    if (dma_timeout-- == 0) break;  // 超时后强制继续
}
```

TX 和 RX 两处都要修改。

---

## P2 — 架构与可维护性（计划中修复）

### FIX-15. USART 驱动大量 Copy-Paste 重复代码

**文件**: `Bsp/stm32f4/usart/bsp_usart.c`（~480 行）

**问题**: `usart_gpio_config`、`usart_base_config`、`send_byte`、`send_string`、`send_hex`、`send_array` 每个函数都包含 3 段几乎相同的 `switch-case`（DEBUG/BLT/FINGER），差异仅在寄存器基地址和引脚号。是项目中最严重的 DRY 违反。

**影响**: 新增 USART 外设需要修改 6+ 个函数、复制粘贴 ~100 行代码，极易遗漏或出错。

**修复**: 引入配置描述符表，所有函数共享一条路径：

```c
// bsp_usart.c 顶部
typedef struct {
    USART_TypeDef      *inst;
    uint32_t            clk;
    volatile uint32_t  *clk_reg;
    uint32_t            baud;
    GPIO_TypeDef       *tx_port;  uint16_t tx_pin;  uint8_t tx_pinsrc;  uint8_t tx_af;  uint32_t tx_clk;
    GPIO_TypeDef       *rx_port;  uint16_t rx_pin;  uint8_t rx_pinsrc;  uint8_t rx_af;  uint32_t rx_clk;
} usart_hw_config_t;

static const usart_hw_config_t s_usart_cfg[USART_ID_MAX] = {
    [USART_ID_DEBUG] = { USART3, RCC_APB1Periph_USART3, &RCC->APB1ENR, 115200,
                         DEBUG_USART_TX_PORT, DEBUG_USART_TX_PIN,  DEBUG_USART_TX_PINSRC,  DEBUG_USART_TX_AF,  DEBUG_USART_TX_CLK,
                         DEBUG_USART_RX_PORT, DEBUG_USART_RX_PIN,  DEBUG_USART_RX_PINSRC,  DEBUG_USART_RX_AF,  DEBUG_USART_RX_CLK },
    [USART_ID_BLT]   = { /* ... */ },
    [USART_ID_FINGER] = { /* ... */ },
};

// 所有函数缩减为：
static bsp_status_e stm32f4_usart_send_byte(uasrt_id_e id, uint8_t ch)
{
    if (id >= USART_ID_MAX) return BSP_STAT_CHOOSE_ERROR_TARGET;
    const usart_hw_config_t *cfg = &s_usart_cfg[id];
    USART_SendData(cfg->inst, ch);
    uint32_t timeout = USART_TIME_OUT;
    while (USART_GetFlagStatus(cfg->inst, USART_FLAG_TXE) == RESET) {
        if (timeout-- == 0) return BSP_STAT_TIME_OUT;
    }
    return BSP_STAT_TRUE;
}
```

预计从 ~480 行减到 ~200 行，且新增 USART 只需加一行配置。

> 注意：SPL 的时钟使能函数（如 `RCC_APB1PeriphClockCmd`）的参数形式需要特殊处理，可以封装一个 `usart_clock_enable()` 辅助函数。

---

### FIX-16. SPI 驱动同样存在 Copy-Paste 预警

**文件**: `Bsp/stm32f4/spi/bsp_spi.c`

**问题**: 当前只有 `SPI_ID_KEY_SACN` 一个 case，但如果未来扩展 LCD SPI、LED Array SPI，会重蹈 USART 的覆辙。

**影响**: 技术债积累。

**修复**: 与 FIX-15 同理，引入 `spi_hw_config_t` 配置表。

---

### FIX-17. SPI DMA 全局单实例模型

**文件**: `Bsp/stm32f4/spi/bsp_spi.c:24`

**问题**: `g_spi_dma_sync_ptr` 是全局单例，同一时刻只能有一个 SPI DMA 事务。多设备场景下 sync 指针冲突。

**影响**: 扩展多 SPI 设备时必须重构。

**修复**:

```c
// 改为按 spi_id_e 索引的数组
static const spi_dma_sync_t *g_spi_dma_sync_ptrs[SPI_ID_MAX] = { NULL };

// DMA 函数中
g_spi_dma_sync_ptrs[id] = sync;

// ISR 中根据 stream 反查 id（或直接用当前 id）
```

---

### FIX-18. ISR handler 硬件细节暴露在 `stm32f4xx_it.c` 中

**文件**: `Bsp/stm32f4/stm32f4xx_it.c:168-180`

**问题**: ISR 文件直接访问 `DMA_GetITStatus`、`DMA_ClearITPendingBit`、`g_spi_dma_sync_ptr` 等 SPI 驱动内部细节。未来增加更多外设中断后，ISR 文件会变成各种硬件细节的堆砌。

**影响**: 违反职责分离，ISR 文件应该只做"路由"。

**修复**: 在 `bsp_spi.c` 中暴露 handler 函数：

```c
// bsp_spi.c
void bsp_spi_dma_isr_handler(void)
{
    if (DMA_GetITStatus(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC)) {
        DMA_ClearITPendingBit(KEY_SCAN_SPI_RX_DMA_STREAM, KEY_SCAN_SPI_RX_DMA_IT_TC);
        g_spi_dma_isr_count++;
        if (g_spi_dma_sync_ptr && g_spi_dma_sync_ptr->notify_from_isr)
            g_spi_dma_sync_ptr->notify_from_isr(g_spi_dma_sync_ptr->handle);
    }
}

// bsp_spi.h
extern void bsp_spi_dma_isr_handler(void);

// stm32f4xx_it.c
extern void bsp_spi_dma_isr_handler(void);
void KEY_SCAN_SPI_RX_DMA_IRQHandler(void) { bsp_spi_dma_isr_handler(); }
```

---

### FIX-19. 公共 API 拼写错误

**文件**:
- `Bsp/common/bsp_usart_interface.h:15` — `uasrt_id_e` → `usart_id_e`
- `Bsp/common/bsp_spi_interface.h:12` — `SPI_ID_KEY_SACN` → `SPI_ID_KEY_SCAN`
- `Bsp/stm32f4/usart/bsp_usart.c:196` — `stm32f4_uasrt_init` → `stm32f4_usart_init`

**问题**: 拼写错误已进入公共接口，传播到所有使用点。

**影响**: 越晚修代价越大，影响代码可读性和专业度。

**修复**: 全局重命名。使用 IDE 的 Rename Symbol 功能或 `sed`：
1. `uasrt_id_e` → `usart_id_e`（约 20+ 处）
2. `SPI_ID_KEY_SACN` → `SPI_ID_KEY_SCAN`（约 10+ 处）
3. `stm32f4_uasrt_init` → `stm32f4_usart_init`（2 处）

---

### FIX-20. `bsp_status_e` 枚举跳值

**文件**: `Bsp/common/bsp_common_def.h`

**问题**: 枚举从 `2` 直接跳到 `4`，`3` 遗漏。

```c
BSP_STAT_CHOOSE_ERROR_TARGET(1),
BSP_STAT_INVALID_PARAMS(2),
BSP_STAT_TIME_OUT(4)   // ← 跳过 3
```

**影响**: 如果有人用 `switch-case` 处理错误码，遗漏值 3 会被 `default` 吞掉；或者误以为这些是 bit flags。

**修复**: 连续编号或改为 bit flags（配合 FIX-12 的错误处理策略调整）：

```c
// 方案 A: 连续编号
typedef enum {
    BSP_STAT_ERROR               = -1,
    BSP_STAT_TRUE                = 0,
    BSP_STAT_CHOOSE_ERROR_TARGET = 1,
    BSP_STAT_INVALID_PARAMS      = 2,
    BSP_STAT_BUSY                = 3,   // ← 补充遗漏值
    BSP_STAT_TIME_OUT            = 4,
} bsp_status_e;

// 方案 B: bit flags（配合 |= 累积模式）
typedef enum {
    BSP_STAT_TRUE                = 0,
    BSP_STAT_CHOOSE_ERROR_TARGET = (1 << 0),  // 1
    BSP_STAT_INVALID_PARAMS      = (1 << 1),  // 2
    BSP_STAT_TIME_OUT            = (1 << 2),  // 4
    BSP_STAT_ERROR               = 0xFF,
} bsp_status_e;
```

---

### FIX-21. `g_spi_dma_sync_ptr` 的 const 被强制丢弃

**文件**: `Bsp/stm32f4/spi/bsp_spi.c:442,503`

**问题**: `(spi_dma_sync_t *)sync` 丢弃了 `const` 限定符。`sync` 参数声明为 `const` 是正确的（BSP 不应修改同步对象），但全局指针是非 const。

**影响**: 编译器无法帮你检查 ISR 中是否意外修改了 sync 对象。

**修复**:

```c
// bsp_spi.c
static const spi_dma_sync_t *g_spi_dma_sync_ptr = NULL;  // 加 const

// 赋值时不再需要强转
g_spi_dma_sync_ptr = sync;  // const T* = const T*，类型匹配
```

---

### FIX-22. ISR 文件中 FreeRTOS handler 注释不明确

**文件**: `Bsp/stm32f4/stm32f4xx_it.c:101-142`

**问题**: `SVC_Handler`、`PendSV_Handler`、`SysTick_Handler` 被注释掉，但没有说明原因。新开发者可能取消注释导致重复定义链接错误。

**修复**: 添加明确注释：

```c
/* FreeRTOS port.c (RVDS/ARM_CM4F/port.c) 通过 FreeRTOSConfig.h 的宏映射
 * 定义了以下三个 handler：
 *   #define vPortSVCHandler   SVC_Handler
 *   #define xPortPendSVHandler PendSV_Handler
 *   #define vPortSysTickHandler SysTick_Handler
 * 此处不能重复定义，否则链接时报 multiple definition 错误。
 * 如需切换到 bare-metal（无 RTOS），取消注释以下三段即可。
 */
```

---

### FIX-23. SysTick 函数 Doxygen 注释完全错误

**文件**: `Bsp/stm32f4/systick/bsp_systick.c:50-52,89-92,125-129`

**问题**: `delay_us`、`delay_ms`、`delay_s` 三个函数的 `@param` 注释是从 USART 函数复制来的：

```c
 * @param  id:串口设备号    ← 应该是 systick 设备号
 * @param  ch:要发送的字节   ← 延时函数没有 ch 参数
```

**影响**: 误导代码阅读者。

**修复**:

```c
/**
 * @brief  微秒级延时
 * @param  id:SysTick 设备号（未使用，保留接口一致性）
 * @param  us:延时微秒数
 * @retval bsp_status_e: BSP_STAT_TRUE 成功
 */
static bsp_status_e stm32f4_systick_delay_us(systick_id_e id, uint32_t us)
```

`delay_ms` 和 `delay_s` 同理修正。

---

### FIX-24. FreeRTOSConfig.h 包含 `<stdio.h>`

**文件**: `FreeRTOS/inc/FreeRTOSConfig.h:73`

**问题**: 头文件中包含 `<stdio.h>` 会将标准库依赖传播给所有包含 `FreeRTOSConfig.h` 的编译单元（几乎全部 `.c` 文件）。

**影响**: 编译时间增加，违背最小依赖原则。

**修复**: 将 `configASSERT` 的 printf 输出改为自定义弱函数：

```c
// FreeRTOSConfig.h — 移除 #include <stdio.h>
#define configASSERT(x) do { \
    if((x)==0) { \
        extern void vAssertCalled(const char*, int); \
        vAssertCalled(__FILE__, __LINE__); \
    } \
} while(0)

// 在 app_init.c 或单独文件中实现
#include <stdio.h>
void vAssertCalled(const char *file, int line) {
    printf("ASSERT: %s:%d\r\n", file, line);
    while (1);
}
```

---

### FIX-25. FreeRTOS 堆大小偏小

**文件**: `FreeRTOS/inc/FreeRTOSConfig.h:186`

**问题**: `configTOTAL_HEAP_SIZE = 20KB`，STM32F405 有 128KB SRAM，堆只占 15%。当前 2 个任务 × 256 words = 2KB 栈 + TCB + 信号量，20KB 够用但余量不大。

**影响**: 后续增加蓝牙接收缓冲区、指纹协议栈等可能堆不足。

**修复**: 调到 32KB（128KB 的 25%，仍有充足空间给 .bss/.data 和栈）：

```c
#define configTOTAL_HEAP_SIZE  ((size_t)(32*1024))
```

---

### FIX-26. 任务创建放在临界区内

**文件**: `Apps/app_init.c:34-52`

**问题**: `xTaskCreate` 涉及 `pvPortMalloc` + 栈清零，在临界区内执行会长时间屏蔽中断。当前只有 2 个任务问题不大，但不是好实践。

**影响**: 中断延迟增大。

**修复**: `vTaskStartScheduler()` 之前创建任务不需要临界区（调度器未启动，无上下文切换）：

```c
int32_t AppTaskCreate(void)
{
    configASSERT(BSP_STAT_TRUE == Bsp_Init());
    configASSERT(NULL != g_board_hw_bsp_);

    int32_t status = APP_TASK_SUCCESS;

    // 调度器未启动，不需要 taskENTER_CRITICAL()
    if (xTaskCreate(Key_Scan_Task, "Key_Scan_Task", 256, NULL, 4, NULL) != pdPASS)
        status = APP_TASK_FAIL;

    if (xTaskCreate(Led_Status_Task, "Led_Status_Task", 256, NULL, 2, NULL) != pdPASS)
        status = APP_TASK_FAIL;

    if (status == APP_TASK_SUCCESS)
        vTaskStartScheduler();

    return status;
}
```

---

## P3 — 编码风格与细节（有空时修复）

### FIX-27. 类型不一致：`u8`/`u16`/`u32` vs `uint8_t`/`uint16_t`/`uint32_t`

**文件**: `Bsp/stm32f4/systick/bsp_systick.c`

**问题**: 接口头文件用 `<stdint.h>` 类型，实现用 SPL 的 `u8`/`u16`/`u32`。不一致增加阅读成本，MISRA-C 要求使用 `<stdint.h>`。

**修复**: `bsp_systick.c` 中统一改为 `uint8_t`/`uint16_t`/`uint32_t`。

---

### FIX-28. HardFault Handler 缺少调试信息

**文件**: `Bsp/stm32f4/stm32f4xx_it.c:54-60`

**问题**: 当前只做 `while(1)` 死循环，无法定位故障原因。

**修复**:

```c
void HardFault_Handler(void)
{
    printf("HardFault! HFSR=%08lX CFSR=%08lX BFAR=%08lX\r\n",
           SCB->HFSR, SCB->CFSR, SCB->BFAR);
    while (1);
}
```

---

### FIX-29. CCMRAM 64KB 完全未使用

**文件**: `STM32F405RGT6_FLASH.ld:65`

**问题**: 链接脚本定义了 64KB CCMRAM 但未实际使用。CCMRAM 不支持 DMA 访问，但可用于 FreeRTOS 堆或大缓冲区。

**修复**: 将 FreeRTOS 堆（`ucHeap`）放到 CCMRAM section：

```c
// FreeRTOS/port/MemMang/heap_4.c
#if defined(__CC_ARM)
__attribute__((section("ccmram")))
#elif defined(__GNUC__)
__attribute__((section(".ccmram")))
#endif
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
```

---

### FIX-30. 链接脚本丢弃 libc/libm/libgcc 可能有问题

**文件**: `STM32F405RGT6_FLASH.ld:218-223`

**问题**: 全部丢弃 libc/libm/libgcc，但项目使用了 `-u _printf_float`（依赖 libgcc 的浮点格式化）和可能的 `memcpy`/`memset`（编译器隐式生成调用）。

**修复**: 去掉 discard 段，让 `--gc-sections` 自动裁剪未使用函数：

```c
// 删除或注释掉
// /DISCARD/ :
// {
//     libc.a ( * )
//     libm.a ( * )
//     libgcc.a ( * )
// }
```

---

### FIX-31. 低优先级零散改善（汇总表）

| 编号 | 文件 | 问题 | 修复 |
|------|------|------|------|
| L1 | `led_status_app.c` | `portTickType` 旧写法 | 改为 `TickType_t` |
| L2 | `led_status_app.c:36` | `vTaskDelayUntil(..., 300)` 硬编码 tick 数 | 改为 `pdMS_TO_TICKS(300)` |
| L3 | `app_init.c:37-46` | 任务优先级写死数字 | 用 `KEY_SCAN_TASK_PRI` 宏（已有定义） |
| L4 | `led_status_app.h:23` | `extern TaskHandle_t` 声明但无定义 | 删除声明或补定义 |
| L5 | `led_status_app.h:5-8` | include 了不需要的 `queue.h`/`semphr.h` | 只保留 `FreeRTOS.h` 和 `task.h` |
| L6 | 多处 CMakeLists.txt | `aux_source_directory()` 不够显式 | 改为 `target_sources()` 显式列出 |
| L7 | `bsp_usart_interface.h` | `send_string` 的 `char* str` 缺 `const` | 改为 `const char* str` |
| L8 | `bsp_gpio_interface.h` | `GPIO_TOGGLE` 混在状态枚举里 | 分离为动作枚举或接受现状 |
| L9 | `led.c`/`74hc165.c` | `init()` 不返回 `bsp_status_e` | `hc165_init`/`led_init` 应转发 `ops->init()` 的返回值 |
| L10 | `key_scan_app.c` | `printf` 在 10ms 周期任务中使用 | 调试阶段可接受，正式版做 log 层 |
| L11 | `bsp_spi.c:25` | `g_spi_dma_isr_count` 只累加不消费 | 要么移除，要么配套实现健康监测 |
| L12 | `hc165_init` 7 个参数 | 参数过多 | 改为只传 `hc165_t *dev`，调用方预填结构体 |

---

## 修复执行顺序建议

```
第一批（1-2 天，P0 全部）:
  FIX-01  send_array 溢出            (1 行)
  FIX-02  send_string 空字符串       (6 行 × 3 case)
  FIX-03  delay_s 溢出               (1 行)
  FIX-04  SPI 超时计数器复用          (6 行 × 2 函数)
  FIX-05  SysTick 移除               (3 文件各删 1-2 行)
  FIX-06  74HC165 NULL sync 判断     (3 行)

第二批（3-5 天，P1 全部）:
  FIX-07  SPI DMA wait 超时          (接口 + 实现 + ISR，~30 行)
  FIX-08  USART 超时保护             (~20 行)
  FIX-09  configASSERT 宏            (1 行)
  FIX-10  信号量创建检查             (1 行)
  FIX-11  _write 位置                (2 文件，~15 行)
  FIX-12  Bsp_Init 短路              (~10 行)
  FIX-13  栈溢出检测                 (2 行)
  FIX-14  DMA 停止超时               (~8 行)

第三批（1-2 周，P2 选择性）:
  FIX-15  USART 配置表重构           (大改动，~200 行重写)
  FIX-17  SPI DMA 多设备安全         (~15 行)
  FIX-18  ISR 封装                  (~10 行)
  FIX-19  拼写错误全局修正           (sed 批量)
  FIX-20~26 其余 P2 项

第四批（有空时，P3）:
  FIX-27~31 按需执行
```

---

## 审查中确认的健康指标（无需修改）

| 设计 | 评价 |
|------|------|
| CMake target 级编译隔离 | `Bsp_Interface` 看不到 `stm32f4xx.h`，`Component` 看不到 SPL 和 FreeRTOS，隔离真实有效 |
| Component 层零泄漏 | `led` 和 `74hc165` 零个平台头、零个 RTOS 头 |
| `spi_dma_sync_t` 同步注入 | BSP 不知道 FreeRTOS 存在，却能使用 RTOS 同步 — 专业的依赖倒置 |
| `Bsp_ISR OBJECT` 处理弱符号 | 理解链接器行为，不是靠运气 |
| `board_hw_bsp_t` 聚合 + 全局单例注入 | 换平台只换一个描述符 |
| `gpio_pin_e` 逻辑引脚抽象 | 上层不知道 PA15/PB1，只知道 LED/HC165_PL |
| 依赖倒置方向 | Apps → Component → BSP_Interface ← BSP_Driver，方向正确 |

---

> **文档版本**: v1.0 — 2026-06-09
> **合并来源**: `architecture-review-20250609.md` (Sonnet) + `architecture-review-opus-20260609.md` (Opus 4.6) + 本会话逐行审查
