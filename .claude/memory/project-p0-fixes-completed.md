---
name: project-p0-fixes-completed
description: 2026-06-09/10 P0+P1+P2 修复完成，P3 待办，详细修复清单在 doc/architecture-fix-plan-20260609.md
metadata: 
  node_type: memory
  type: project
  originSessionId: 19eb04a2-f6aa-4be0-a562-ef941e55d7fa
---

# 修复进度（截至 2026-06-10）

## 已完成

### P0（6 条，全部完成）
- FIX-01~06: 见 `doc/architecture-fix-plan-20260609.md`

### P1（完成 6 条，跳过 1 条）
- FIX-07: SPI DMA wait 超时 + ISR DMA 错误处理 + cleanup 关中断使能 + DMA停止等待超时
- FIX-08: USART 超时保护 → **用户决定跳过**（波特率不确定）
- FIX-09~14: 见 `doc/architecture-fix-plan-20260609.md`

### P2（完成 6 条，跳过 2 条）
- FIX-15: USART 配置表重构（`usart_hw_config_t`，`base_clk_cmd` 函数指针放描述符）→ 480 行→230 行
- FIX-19: 拼写错误全局修正（`uasrt`→`usart`、`SACN`→`SCAN`）sed 批量
- FIX-20: `bsp_status_e` 枚举连续编号（TIME_OUT 4→3）
- FIX-21: 已完成（const 修复）
- FIX-22: ISR 三个 FreeRTOS handler 注释合并统一说明
- FIX-23: SysTick Doxygen 注释修正
- FIX-26: 任务创建移除临界区（调度器未启动不需要）
- FIX-24: **跳过**（保留 FreeRTOSConfig.h 的 `#include <stdio.h>`）
- FIX-25: **跳过**（堆保持 20KB，暂不调到 32KB）
- FIX-18: ISR handler 封装 + stm32f4xx_it.c/h 移至 `Bsp/stm32f4/isr/`（P1 阶段提前做）

### 未做（FIX-16/17，SPI 只有 1 设备，价值低）
- FIX-16: SPI 配置表（`spi_hw_config_t`）— 仅 1 个 SPI 设备，暂不做
- FIX-17: SPI DMA 多设备数组 — 仅 1 个 SPI 设备，暂不做

## 待修复（P3）

详细代码方案见: `doc/architecture-fix-plan-20260609.md`
- FIX-27: 类型统一 `u8`/`u16`/`u32` → `uint8_t`/`uint16_t`/`uint32_t`（仅 bsp_systick.c）
- FIX-28: HardFault Handler 加调试信息打印
- FIX-29: CCMRAM 64KB 利用（FreeRTOS 堆放 CCMRAM）
- FIX-30: 链接脚本 discard 段审查
- FIX-31: 零散编码风格（L1~L12 见文档）

## 编译状态
- 零错误零警告，Flash: 31384 B (5.99%), RAM: 23768 B (18.13%)

## 关键设计决策
- **USART 重构模式**: `usart_hw_config_t` 配置表 + `base_clk_cmd` 函数指针（APB1/APB2 统一）。新增外设可套用此模式（SPI 暂不需要）
- **SysTick 延时替代**: 用 TIM 基础定时器（非 DWT），用户明确选择
- **USART 超时**: 不加，用户认为波特率不确定时超时值难选
- **ISR 目录隔离**: `stm32f4xx_it.c/h` 在 `Bsp/stm32f4/isr/`，只依赖 StdPeriph_Driver
- **栈溢出检测**: configCHECK_FOR_STACK_OVERFLOW=2，hook 里 printf 任务名（注意 printf 本身耗栈）

## 下一步
下次说"继续 P3 修复"，读此 memory + `doc/architecture-fix-plan-20260609.md` 从 FIX-27 开始。

## Why: 确保下次对话直接知道修复进度和下一步。
## How to apply: 下次会话说"继续 P3 修复"开始。
