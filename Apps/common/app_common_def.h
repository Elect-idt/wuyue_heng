#ifndef __APP_COMMON_DEF_H_
#define __APP_COMMON_DEF_H_

/* Apps 层公共配置宏（任务优先级等全局分配型定义统一放这里，
 * 命名对应 Bsp/common/bsp_common_def.h。
 * 优先级集中定义的原因：各任务相对高低、是否冲突，只有放一处才看得清。）
 *
 * FreeRTOS 优先级注意事项：
 *   - 任务优先级：数值越大越高，0 最低（空闲任务），configMAX_PRIORITIES 为上限
 *   - 中断优先级（NVIC）数值越小越紧急，与任务优先级方向相反！
 *   - 数值 0~4（即优先级高于 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5）
 *     的 ISR 禁止调用 FromISR 系列 API，否则内核崩溃。当前 SPI DMA 中断
 *     抢占优先级为 6，合法（务必不要"优化"到 4 以下）
 */

/* 按键扫描任务：10ms 周期 DMA 读取，高优先级保证节拍稳定 */
#define KEY_SCAN_TASK_PRI 4

/* LED 灯效任务：20ms 周期刷新，低于按键扫描（节拍更硬），高于心跳闪烁 */
#define LED_RGB_DISPLAY_TASK_PRI 3

/* LED 状态任务：300ms 心跳闪烁，低优先级即可 */
#define LED_STATUS_TASK_PRI 2

#endif /* __APP_COMMON_DEF_H_ */
