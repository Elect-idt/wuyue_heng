---
name: reference-ccmram-vs-sram
description: STM32F405 CCMRAM vs SRAM 的区别、性能、DMA 限制、用法（参考知识）
metadata: 
  node_type: memory
  type: reference
  originSessionId: 19eb04a2-f6aa-4be0-a562-ef941e55d7fa
---

# STM32F405 CCMRAM vs SRAM

## 物理布局
- **SRAM**:   `0x20000000`, 128KB —— 连在 AHB 总线矩阵，CPU/DMA/外设都能访问
- **CCMRAM**: `0x10000000`,  64KB —— 紧耦合内存，**只有 Cortex-M4 内核能访问**

## 核心区别（最重要的点）
⚠️ **CCMRAM 是 CPU 私有的，DMA 和任何外设都访问不到它。**
- DMA 写 CCMRAM 地址 → 数据丢失 / 总线错误 / HardFault
- 当前项目链接脚本已定义 `.ccmram` 段但未使用（`CCMRAM: 0 B / 64KB`）

## 性能对比
| 特性 | SRAM | CCMRAM |
|------|------|--------|
| CPU 访问 | 普通（与 DMA 抢总线） | 最快（无竞争，0 等待） |
| DMA 访问 | ✅ 可以 | ❌ 不可以 |
| 外设访问 | ✅ 可以 | ❌ 不可以 |

## 能放什么 / 不能放什么
- ✅ **FreeRTOS 堆**（纯 CPU 内存管理，最经典用法）
- ✅ **任务栈**（CPU 压栈出栈）
- ✅ **DSP/算法查找表、中间数组**（CPU 专用加速）
- ❌ **SPI/USART/ADC DMA 缓冲区**（DMA 访问不到！）
- ❌ **任何外设共享数据**

## 用法（链接段 + attribute）
```c
// 单个变量
uint8_t fast_buf[256] __attribute__((section(".ccmram")));

// FreeRTOS 堆（heap_4.c）
static uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".ccmram")));
```
链接脚本 `STM32F405RGT6_FLASH.ld` 已有 `.ccmram` 段定义（`:176-185`），`>CCMRAM AT>FLASH`。

## 何时启用（本项目判断）
- **当前不用**：SRAM 才用 18%（23768/131072），没压力
- **未来启用信号**：DMA 缓冲吃紧时，把 FreeRTOS 堆挪到 CCMRAM，SRAM 全给 DMA
- **硬实时场景**：高优先级 ISR 数据、算法加速

## 关联
- 用户决定 FIX-29 暂不启用 CCMRAM（2026-06-10）
- 链接脚本位置：`STM32F405RGT6_FLASH.ld`
- DMA 同步机制见 [[project-p0-fixes-completed]]
