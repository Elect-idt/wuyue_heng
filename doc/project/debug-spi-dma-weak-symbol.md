# SPI DMA 中断调试记录：弱符号与静态库链接陷阱

## 问题现象

74HC165 按键扫描任务（`Key_Scan_Task`）初始化完成后，while 循环内的所有 printf 均无输出，LED 状态任务（`Led_Status_Task`）也停止打印。串口仅输出：

```
[HC165] init OK, 3 chips, period 10ms
```

## 调试过程

### 第一阶段：定位卡死点

在 `key_scan_app.c` 的 while 循环中加逐步 printf：

```c
printf("[HC165] loop #%lu enter\r\n", ++loop_cnt);
printf("[HC165] before hc165_read\r\n");
bsp_status_e status = hc165_read(&s_hc165, s_key_data);
printf("[HC165] after hc165_read, status=%d\r\n", status);
```

**串口输出：**

```
[HC165] init OK, 3 chips, period 10ms
[HC165] loop #1 enter
[HC165] before hc165_read
```

**结论：** 卡死在 `hc165_read()` 内部，该函数调用 `spi_receive_multi_data_dma()` 启动 DMA 并等待信号量。

### 第二阶段：排除 DMA Channel 配置错误

检查 `bsp_spi.h` 发现 DMA Channel 配置错误：

```c
// 错误：Channel 3 对应 SPI1，不是 SPI2
#define KEY_SCAN_SPI_RX_DMA_CHANNEL        DMA_Channel_3
#define KEY_SCAN_SPI_TX_DMA_CHANNEL        DMA_Channel_3
```

**修正为：**

```c
// SPI2_RX = DMA1_Stream3 Channel 0, SPI2_TX = DMA1_Stream4 Channel 0
#define KEY_SCAN_SPI_RX_DMA_CHANNEL        DMA_Channel_0
#define KEY_SCAN_SPI_TX_DMA_CHANNEL        DMA_Channel_0
```

同时修复 `spi_tx_dma_config()` 的复制粘贴 bug（用了 RX 的 Channel 宏）。

**结果：** 修正后仍然只打印 init OK，问题未解决。

### 第三阶段：排除 SPI 硬件问题

临时改用 polling 版本 `hc165_read_polling()`：

```c
bsp_status_e status = hc165_read_polling(&s_hc165, s_key_data);
```

**串口输出：**

```
[HC165] init OK, 3 chips, period 10ms
[HC165] before read
[HC165] after read, st=0 00 FF 00
[HC165] before read
[HC165] after read, st=0 00 FF 00
...
```

**结论：** SPI 硬件正常，数据读取成功。问题锁定在 DMA 传输或 DMA 中断。

### 第四阶段：DMA 逐步 printf 定位 crash 点

在 `spi_receive_multi_data_dma()` 中每一步加 printf：

```c
printf("[DMA] 1.clk\r\n");
KEY_SCAN_SPI_DMA_CLK_INIT(...);
printf("[DMA] 2.tx_cfg\r\n");
spi_tx_dma_config(...);
// ... 每步都加 printf
printf("[DMA] 6b.tx_en\r\n");
DMA_Cmd(KEY_SCAN_SPI_TX_DMA_STREAM, ENABLE);
printf("[DMA] 7.wait\r\n");
```

**串口输出：**

```
[DMA] 1.clk
[DMA] 2.tx_cfg
[DMA] 3.rx_cfg
[DMA] 4.spi_dma_en
[DMA] 5.tc_it
[DMA] 6a.rx_en
[DMA] 6b.tx_en
```

**结论：** DMA 启动后立即 crash，连 `7.wait` 都没打印。DMA 一使能 ~2µs 就传输完成并触发中断。

### 第五阶段：分离 DMA 传输与 DMA 中断

**测试 1 — 关闭 DMA 中断，纯轮询：**

```c
// 跳过 DMA_ITConfig(TC)
// 改用轮询等待
while (DMA_GetFlagStatus(..., DMA_FLAG_TCIF3) == RESET);
```

**串口输出：**

```
[DMA] 7.poll
[DMA] 8.poll_done isr=0
```

**结论：** DMA 传输本身完全正常！问题在 DMA 中断触发时。

**测试 2 — 开中断但 ISR 极简化（只计数，不调 FreeRTOS）：**

```c
void DMA1_Stream3_IRQHandler(void) {
    if (DMA_GetITStatus(...)) {
        DMA_ClearITPendingBit(...);
        g_spi_dma_isr_count++;
        // 不调 xSemaphoreGiveFromISR
    }
}
```

**串口输出：**

```
[DMA] 6b.tx_en
（无后续输出）
```

**结论：** ISR 函数体根本没执行！中断向量指向的不是我们写的 ISR。

### 第六阶段：确认根因

最终定位：`stm32f4xx_it.c` 被编译进 `libBsp_Driver.a`（STATIC 库），但链接器从不提取它。原因链：

1. 启动文件 `startup_stm32f40_41xxx.s` 定义了弱符号：
   ```asm
   .weak      DMA1_Stream3_IRQHandler
   .thumb_set DMA1_Stream3_IRQHandler, Default_Handler
   ```
2. 向量表引用 `DMA1_Stream3_IRQHandler` → 被弱符号（`Default_Handler` 死循环）解析
3. GNU 链接器处理 `.a` 时**只提取能解析未定义符号的 .o 文件**
4. `stm32f4xx_it.o` 中全是弱符号覆盖（`HardFault_Handler`、`DMA1_Stream3_IRQHandler` 等），没有任何非弱引用指向它
5. 链接器**从不提取** `stm32f4xx_it.o`
6. DMA 中断触发 → 跳转 `Default_Handler`（死循环）→ 系统卡死

## 修复方案

将 `stm32f4xx_it.c` 从 STATIC 库中移出，改为 OBJECT 库直接注入可执行文件：

**`Bsp/CMakeLists.txt`：**

```cmake
# 从平台源文件列表中排除 ISR 文件
list(FILTER Bsp_Platform_Src EXCLUDE REGEX "stm32f4xx_it\\.c$")

# 创建 OBJECT 库，不打包进 .a
add_library(Bsp_ISR OBJECT
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/stm32f4xx_it.c
)
target_include_directories(Bsp_ISR PRIVATE
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/gpio
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/usart
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/systick
    ${CMAKE_SOURCE_DIR}/Bsp/stm32f4/spi
)
target_link_libraries(Bsp_ISR PRIVATE StdPeriph_Driver bsp_common_interface)
```

**`CMakeLists.txt`（顶层）：**

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/src/main.c
    ${CMAKE_SOURCE_DIR}/Core/src/syscalls.c
    ${CMAKE_SOURCE_DIR}/Core/src/sysmem.c
    $<TARGET_OBJECTS:Bsp_ISR>    # 直接链接，绕过静态库按需提取
)
```

## 经验总结

### 1. 弱符号 + 静态库 = 陷阱

GNU 链接器对 STATIC 库（`.a`）采用**按需提取**策略：只有当某个 `.o` 能解析当前未定义的符号时才提取它。弱符号（`.weak`）已经算"已定义"，不构成"未定义引用"，因此链接器不会为覆盖弱符号而提取 `.o`。

这是**纯 CMake/链接器层面的问题**，与代码逻辑无关，极难通过代码审查发现。

### 2. 调试策略：二分法 + 隔离变量

本次调试的关键方法：

| 步骤 | 操作 | 目的 |
|------|------|------|
| 1 | 加逐步 printf | 定位卡死函数 |
| 2 | 改用 polling | 排除硬件问题 |
| 3 | DMA 内部逐步 printf | 定位 crash 点到 DMA_Cmd 之后 |
| 4 | 关中断 + 轮询 | 分离 DMA 传输与 DMA 中断 |
| 5 | ISR 极简化 | 分离 ISR 入口与 ISR 内容 |
| 6 | 检查 CMake/链接 | 确认弱符号链接陷阱 |

每一步**只改变一个变量**，通过串口输出判断该变量是否是根因。

### 3. ISR 文件的链接规则

**规则：** 所有覆盖启动文件弱符号的 ISR handler 文件，必须通过以下方式之一直接链接到可执行文件：

- `OBJECT` 库 + `$<TARGET_OBJECTS:...>` 注入可执行文件（本项目采用）
- 直接加入 `add_executable` 的源文件列表
- `--whole-archive` 强制提取整个静态库

绝不能放在普通 `STATIC` 库中依赖链接器自动提取。

### 4. 两个独立 Bug 的巧合

本次调试中实际修了两个独立 Bug：

1. **DMA Channel 错误**：`DMA_Channel_3`（SPI1）→ `DMA_Channel_0`（SPI2），以及 `spi_tx_dma_config` 中误用 RX Channel 宏的复制粘贴 Bug
2. **弱符号链接陷阱**：`stm32f4xx_it.o` 从未被链接器提取

Bug 1 修完后系统仍卡死（因为 Bug 2 导致中断向量指向死循环），两个问题叠加才表现为"DMA 完全不工作"。

### 5. 调试输出要在架构允许的范围内

- `bsp_spi.c` 属于 Bsp 驱动层，不应该 include `<stdio.h>`
- `key_scan_app.c` 属于 Apps 层，不能 include `bsp_spi.h`（平台私有头文件）
- 临时调试 printf 放在正确位置：App 层用 `extern` 声明 Bsp 层变量（链接器解析），不破坏头文件依赖
