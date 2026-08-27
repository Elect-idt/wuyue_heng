# wuyue_heng 架构审查报告 — Opus 4.6 复审 (2026-06-09)

> 本文档由 Claude Opus 4.6 主会话亲自读代码后形成，非子代理结论。
> 之前已有一份 Sonnet 分析报告（`architecture-review-20250609.md`），本文在部分问题上做了补充和修正。

## 总体判断

框架大方向正确，CMake 编译隔离真实有效，Component 层零平台/RTOS泄漏，ISR 弱符号问题已专业处理。
目前最大短板不是架构路线，而是 **接口契约一致性、异常路径健壮性、SysTick 所有权边界** 还需要收口。

| 维度 | 评分 |
|---|---:|
| 总体架构方向 | 8 / 10 |
| CMake 分层隔离 | 8 / 10 |
| BSP/Component/Apps 分层 | 7.5 / 10 |
| 接口设计成熟度 | 6.5 / 10 |
| 运行健壮性 | 6 / 10 |
| 长期维护性 | 6.5 / 10 |

---

## 高优先级问题（建议尽快修复）

### H1. SysTick 作为公共 BSP ops 暴露 — 与 FreeRTOS 所有权冲突

**涉及文件**:
- `Bsp/bsp_interface.h:35` — `const systick_ops_t *systick_ops;`
- `Bsp/bsp_interface.c:49` — `systick_ops->init(SYSTICK_ID_DEFAULT, SYSCLK_MHZ)`
- `Bsp/stm32f4/systick/bsp_systick.c:65-80,104-119` — 直接操作 `SysTick->LOAD/VAL/CTRL`
- `Bsp/stm32f4/stm32f4_bsp.c:25` — `.systick_ops = &g_stm32f4_systick_driver_`

**问题**: `SysTick` 在 FreeRTOS 系统里属于 RTOS port 独占资源（`FreeRTOSConfig.h:304` 映射了 `vPortSysTickHandler → SysTick_Handler`）。当前 `systick_ops_t` 的 delay 函数会直接覆写 SysTick 寄存器，破坏 RTOS tick。

上层（Apps/Component）目前没有实际调用 `delay_us/delay_ms`，但 `board_hw_bsp_t` 公开暴露了它，容易误导后续维护者。

代码自身也承认：`bsp_systick.c:23` — "freertos会重新初始化systick，所以这个驱动意义不大"。

**修复方案**:
1. 从 `board_hw_bsp_t` 中移除 `systick_ops` 字段
2. 从 `Bsp_Init()` 中移除 `systick_ops->init(...)` 调用
3. 从 `g_stm32f4_bsp_` 初始化中移除 `.systick_ops`
4. 保留 `bsp_systick.c` 和 `bsp_systick_interface.h` 文件但标记为 deprecated，或限制为仅启动前可用
5. 如需微秒延时，未来改用 DWT cycle counter 或 TIMx

**风险**: 低。上层无调用点，纯接口裁剪。

---

### H2. SPI DMA 等待无 timeout — 任务可能永久阻塞

**涉及文件**:
- `Bsp/stm32f4/spi/bsp_spi.c:460-464,521-525` — `sync->wait(sync->handle)`
- `Apps/key_scan_app/key_scan_app.c:27` — `xSemaphoreTake(..., portMAX_DELAY)`
- `Bsp/stm32f4/stm32f4xx_it.c:168-180` — ISR 只处理 TC

**问题**: DMA 传输启动后，如果中断不来（配置错误、ISR 未链接、DMA error），`xSemaphoreTake(portMAX_DELAY)` 会永久阻塞，Key_Scan_Task 永远卡住。

ISR 也只处理了 TC（Transfer Complete），没有处理 TE/FE/DME 等 DMA 错误。一旦 DMA 错误，上层永远等不到通知。

**修复方案**:
1. `spi_dma_sync_t` 的 `wait` 签名改为带 timeout：`bool (*wait)(void *handle, uint32_t timeout_ms)`
2. Apps 侧 `spi_dma_wait` 改为 `xSemaphoreTake(handle, pdMS_TO_TICKS(timeout_ms))`
3. `spi_receive_multi_data_dma` / `spi_send_multi_data_dma` 检查 wait 返回值，返回 `BSP_STAT_TIME_OUT`
4. ISR 补充 DMA 错误标志清理（至少 TE），错误时也通知等待任务
5. DMA 错误后在 cleanup 中 reset DMA stream 状态

**风险**: 中。改了 `spi_dma_sync_t` 的 `wait` 签名会影响所有使用方，但当前只有 `key_scan_app` 一个使用方，改动范围可控。

---

### H3. 74HC165 注释说 NULL sync 则轮询，但实现无条件走 DMA

**涉及文件**:
- `Component/74hc165/74hc165.h:12` — `const spi_dma_sync_t *dma_sync; // NULL 则用轮询`
- `Component/74hc165/74hc165.c:27-28` — `spi_receive_multi_data_dma(..., dev->dma_sync)` 无条件调用

**问题**: 头文件契约声明 `dma_sync == NULL` 时应走轮询，但 `hc165_read()` 无条件调用 DMA 版 API。当 `sync == NULL` 时，实际行为是"启动 DMA 后不等待就 cleanup"，数据可能未完成就被返回。

**修复方案**: 在 `hc165_read()` 开头加判断：

```c
if (dev->dma_sync == NULL) {
    return hc165_read_polling(dev, buf);
}
```

已有 `hc165_read_polling()` 可直接复用。

**风险**: 极低。现有 `Key_Scan_Task` 传的是非空 sync，不受影响。

---

### H4. SPI polling 超时计数器复用 bug

**涉及文件**:
- `Bsp/stm32f4/spi/bsp_spi.c:225-250` — `spi_send_byte`
- `Bsp/stm32f4/spi/bsp_spi.c:289-315` — `spi_receive_byte`

**问题**: 同一个 `stm32f4_timeout` 变量被多个 while 循环复用，第二个循环继承第一个消耗后的剩余值，可能导致假超时。

**修复方案**: 每个 `while` 循环前独立重置 `stm32f4_timeout = SPI_TIME_OUT`。

**风险**: 极低。纯 bug 修复。

---

### H5. Apps `xSemaphoreCreateBinary()` 返回值未检查

**涉及文件**:
- `Apps/key_scan_app/key_scan_app.c:42` — `s_spi_dma_sem = xSemaphoreCreateBinary();`

**问题**: 如果创建失败（heap 不足），后续 `s_spi_dma_sync.handle = s_spi_dma_sem` 会是 NULL，ISR 中 `xSemaphoreGiveFromISR(NULL, ...)` 会导致未定义行为。

**修复方案**: 创建后加 `configASSERT(s_spi_dma_sem != NULL);`

**风险**: 极低。

---

## 中优先级问题（建议后续修复）

### M1. `_write()` 放在 `bsp_usart.c` 中边界不纯

**涉及文件**:
- `Bsp/stm32f4/usart/bsp_usart.c:456-469` — 强定义 `_write()`
- `Core/src/syscalls.c:80-90` — weak `_write()`

**问题**: `_write()` 是 libc syscall 层 glue，不是 USART 驱动的一部分。当前强符号能工作是因为 `bsp_usart.o` 和 `g_stm32f4_usart_driver_` 在同一编译单元，链接器会把整个 `.o` 拉出来。如果拆分编译单元，`_write()` 可能不再被提取。

**建议**: 将 `_write()` 移到 `Core/src/syscalls.c`，内部调用 `g_board_hw_bsp_->usart_ops->usart_send_byte()`。

---

### M2. SPI DMA 全局 `g_spi_dma_sync_ptr` 是单实例模型

**涉及文件**:
- `Bsp/stm32f4/spi/bsp_spi.c:24` — `spi_dma_sync_t *g_spi_dma_sync_ptr`

**问题**: 全局只有一个 sync 指针，同一时刻只能有一个 SPI DMA 事务。当前单设备可用，多设备扩展时会冲突。

**建议**: 改为按 `spi_id_e` 索引的数组 `g_spi_dma_sync_ptrs[SPI_ID_MAX]`，ISR 根据 stream 找对应 context。

---

### M3. `Bsp_Init()` 用 `|=` 累积错误码，语义不严谨

**涉及文件**:
- `Bsp/bsp_interface.c:44-49`

**问题**: `BSP_STAT_CHOOSE_ERROR_TARGET=1` 和 `BSP_STAT_INVALID_PARAMS=2` 做 OR 后得到 `3`，不等于任何已定义错误码。

**建议**: 改为短路模式，第一个非零就返回；或把错误码改为真正的 bit flags。

---

### M4. 公共 API 拼写错误

**涉及文件**:
- `Bsp/common/bsp_usart_interface.h:15` — `uasrt_id_e` → 应为 `usart_id_e`
- `Bsp/common/bsp_spi_interface.h:12` — `SPI_ID_KEY_SACN` → 应为 `SPI_ID_KEY_SCAN`
- `Bsp/stm32f4/usart/bsp_usart.c:196` — `stm32f4_uasrt_init` → 应为 `stm32f4_usart_init`

**问题**: 拼写错误已进入公共接口，越晚修代价越大。

---

### M5. `g_spi_dma_sync_ptr` 的 const 被强制丢弃

**涉及文件**:
- `Bsp/stm32f4/spi/bsp_spi.c:442,503` — `(spi_dma_sync_t *)sync` 丢弃 const

**建议**: 将 `g_spi_dma_sync_ptr` 改为 `const spi_dma_sync_t *`。

---

### M6. ISR 注释不够明确

**涉及文件**:
- `Bsp/stm32f4/stm32f4xx_it.c:131-142` — `SysTick_Handler` 被注释但没说明原因

**建议**: 在注释处明确写明 "FreeRTOS port.c 定义了 SysTick_Handler/SVC_Handler/PendSV_Handler，此处不能重复定义"。

---

## 低优先级问题（可选修复）

### L1. `portTickType` 旧写法
- `Apps/led_status_app/led_status_app.c:28`
- `Apps/key_scan_app/key_scan_app.c:38`
- 建议改为 `TickType_t`

### L2. LED 任务周期未用 `pdMS_TO_TICKS`
- `Apps/led_status_app/led_status_app.c:36` — `vTaskDelayUntil(..., 300)`
- 建议改为 `pdMS_TO_TICKS(300)`

### L3. 任务优先级宏定义了但创建时写死数字
- `Apps/key_scan_app/key_scan_app.c:17` 定义 `KEY_SCAN_TASK_PRI 4`
- `Apps/app_init.c:37-46` 创建时写死 `4` 和 `2`
- 建议统一用宏

### L4. `led_status_app.h:23` extern `TaskHandle_t` 无定义
- 声明了 `Led_Status_Task_Handle` 但全工程无定义

### L5. 头文件 include 过重
- `Apps/led_status_app/led_status_app.h:5-8` include 了 `FreeRTOS.h/task.h/queue.h/semphr.h`
- 实际只需要 `FreeRTOS.h` 和 `task.h`

### L6. `aux_source_directory()` 不够显式
- 多处 CMakeLists.txt 使用，建议改为显式 `target_sources()`

### L7. USART 接口混入格式化层
- `usart_send_string` / `usart_send_hex` 更适合 debug/log 层，不是稳定驱动抽象
- `send_string` 的 `char* str` 参数缺 `const`

### L8. `GPIO_TOGGLE` 混在状态枚举里
- `GPIO_LOW/GPIO_HIGH` 是状态，`GPIO_TOGGLE` 是动作
- 不是严重问题，但语义不纯

### L9. `led_init()` / `hc165_init()` 不返回初始化状态
- 内部调用的 `ops->init()` 返回值被丢弃

### L10. `printf` 在 10ms 周期任务中使用
- 格式化开销大、阻塞发送、多任务无互斥
- 调试阶段可接受，正式版建议做 log 层

---

## 值得肯定的架构亮点

| 设计 | 评价 |
|---|---|
| CMake target 级编译隔离 | `Bsp_Interface` 看不到 `stm32f4xx.h`，`Component` 看不到 SPL 和 FreeRTOS，隔离真实有效 |
| Component 层零泄漏 | `led` 和 `74hc165` 零个平台头、零个 RTOS 头 |
| `spi_dma_sync_t` 同步注入 | BSP 不知道 FreeRTOS 存在，却能使用 RTOS 同步 — 专业的依赖倒置 |
| `Bsp_ISR OBJECT` 处理弱符号 | 理解链接器行为，不是靠运气 |
| `board_hw_bsp_t` 聚合 + 全局单例注入 | 换平台只换一个描述符 |
| `gpio_pin_e` 逻辑引脚抽象 | 上层不知道 PA15/PB1，只知道 LED/HC165_PL |
