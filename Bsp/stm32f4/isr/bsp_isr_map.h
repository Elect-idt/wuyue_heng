#ifndef __BSP_ISR_MAP_H_
#define __BSP_ISR_MAP_H_

/**
 ******************************************************************************
 * @file    bsp_isr_map.h
 * @brief   平台中断路由映射表（「逻辑功能名 → 物理 IRQHandler 符号」）
 ******************************************************************************
 * @attention
 *
 * 由 stm32f4xx_it.c（ISR 路由层）包含，单点维护「外设功能 → IRQHandler」
 * 的硬件绑定。
 *
 * 为什么独立成一个头：
 *   stm32f4xx_it.c 只需「这个功能对应哪个 IRQHandler 符号」来做路由，
 *   不应为此 include 整个 bsp_spi.h——那会让 ISR 层连带看到 GPIO_ResetBits、
 *   SPI_Init 等 SPL 调用宏与 GPIO/CLK 配置，破坏 Bsp_ISR 的编译隔离。
 *   本头只含纯符号映射，不引入任何 SPL 依赖。
 *
 *   改 DMA/SPI 通道只需改这里一处，路由层与驱动层自动同步。
 ******************************************************************************
 */

/* SPI 键盘扫描（SPI2）：仅 RX DMA 开中断（DMA1_Stream3）。
 * TX DMA 不开中断——全双工下 RX 收完即代表 TX 也发完，用 RX TC 通知即可。 */
#define KEY_SCAN_SPI_RX_DMA_IRQHandler     DMA1_Stream3_IRQHandler

/* WS2812 RGB LED（SPI3）：发送同步同样用 RX DMA TC 通知（DMA1_Stream0），
 * 路由到 bsp_spi_dma_isr_handler(SPI_ID_WS2812_LED) */
#define WS2812_LED_SPI_RX_DMA_IRQHandler     DMA1_Stream0_IRQHandler

#endif /* __BSP_ISR_MAP_H_ */
