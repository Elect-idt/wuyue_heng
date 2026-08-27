---
name: project-status
description: 2026-08-16 审查清单全部清零（P0~P3+P1+P2），框架就绪待新驱动开发；硬件待办（Q7'补偿等）
metadata: 
  node_type: memory
  type: project
  originSessionId: 19eb04a2-f6aa-4be0-a562-ef941e55d7fa
---

# wuyue_heng 项目状态（新会话从这里开始）

## 当前状态快照（2026-08-16 收工）
- **代码**：全绿编译（零错误零警告，Flash 6.35%），最新提交 4e1d677 已推送 GitHub
- **硬件验证**：⚠ 仅旧版按键扫描上板跑通过；**2026-08-16 的全部重构
  （SPI V3.0 配置表、P1/P2 修复、预填描述符）只过了编译，未上板**。
  继续开发前建议先烧录验证按键读数正常（见"下一步"第 2 条）
- **架构**：审查清单全部清零（P0~P3+P1+P2），框架进入可复制扩展状态
- 本文件是进度+决策中枢；架构规则看 CLAUDE.md（自动加载）；
  CMake/CCMRAM 深层知识看本目录两个 reference 文件

## 新器件/驱动开发 SOP（入口指针，详细规则以源头为准）
1. 架构约定与惯例 → `CLAUDE.md`（配置表模式、预填描述符、事务锁、优先级登记）
2. SPI 设备三步接入法 → `Bsp/stm32f4/spi/bsp_spi.c` 头注释
3. 照抄范例 → SPI：`bsp_spi.c` 的 `spi_hw_config_t`；器件：`Component/74hc165/74hc165.h`
4. 新任务三件事：优先级进 `Apps/common/app_common_def.h`、源文件手动加
   `Apps/CMakeLists.txt`、（如有钩子）进 `Apps/common/app_hooks.c`

---

# 历史进度归档（截至 2026-08-16，P0~P3 全部完成）

## P0~P2（2026-06-09/10 完成）
- P0 全部、P1 除 FIX-08（用户跳过）、P2 大部分：见 `doc/project/architecture-fix-plan-20260609.md`

## P3（2026-08-16 收尾）
- FIX-27: 已确认 bsp_systick.c 无 u8/u16/u32 残留（无需改）
- FIX-28: HardFault 打印 HFSR/CFSR/MMFAR/BFAR/LR（已完成）
- FIX-29: FreeRTOS 堆 48KB 放 CCMRAM（commit 86a8562）
- FIX-30: 链接脚本 /DISCARD/ 已移除（.ld 内有注释说明）
- FIX-31: L1~L7、L9、L11 已完成；L8 接受现状（GPIO_TOGGLE 留在状态枚举）；
  L10（log 层）、L12（hc165_init 7 参数重构）有意推迟
- L3: 任务优先级宏移入 key_scan_app.h / led_status_app.h，app_init.c 引用宏
- L5: 任务头文件瘦身（删 queue.h/semphr.h/bsp_interface.h，.c 自行 include）
- L6: 全部 CMakeLists 弃用 aux_source_directory，显式列源文件
- L7: usart_send_string/send_array 参数加 const
- L9: led_init/hc165_init 返回 bsp_status_e，调用方 configASSERT

## 硬件相关（未提交/待硬件确认）
- key_scan_app.c 有 Q7\(反相)错接补偿（奇数索引取反），板子改 Q7->SER 接线后应删除
- KEY_ACTIVE_LEVEL_BITMAP：chip1 active-high，chip0/2 active-low
- doc/project/硬件待修改.txt 记录硬件待改事项

## 2026-08-16 Opus 复审 + P1 修复（同日完成）
- Opus 全局架构复审结论：健康度 8/10，无 P0，技术债集中在错误路径/扩展路径
- P1 已全部修复：
  - P1-1: configASSERT -> 真函数 vAssertCalled（app_hooks.c），打印后关中断停车；
    FreeRTOSConfig.h 不再 include stdio.h
  - P1-2: SPI DMA 使能 TE 中断（DMA_IT_TC|DMA_IT_TE），cleanup 对称关闭+清标志，
    KEY_SCAN_SPI_RX_DMA_IT_TE 宏进 bsp_spi.h
  - P1-3: DMA 超时 -> cleanup **之后**排空信号量陈旧令牌（关键：cleanup 前排空
    关不死竞态窗口，用户指出后修正方案）
  - P1-4: g_spi_dma_sync_ptr 单例 -> s_spi_dma_sync_ptrs[SPI_ID_MAX] 按 id 索引；
    bsp_spi_dma_isr_handler(spi_id_e id) 带 id 路由；Bsp_ISR 链接 bsp_common_interface
  - P1-5: USART 4 处忙等加 USART_TIME_OUT（bsp_usart.h），inner send_byte 检查返回值
  - P2-9: app_common_def.h 的 FromISR 优先级注释纠错（正确规则：NVIC 数值 0~4
    禁止调 FromISR，DMA 中断 6 合法）
  - P2-10: 核实为误报（Core/src 实际就是小写，Linux 构建无碍）
- P2 待办（2026-08-16 已做 P2-6/7/8，仅剩 P2-11 用户考虑中）：
  - P2-6+7 已完成：bsp_spi.c V3.0 重构为 spi_hw_config_t 配置描述符表
    （照 FIX-15 USART 模式），7 处 switch-case 收敛为查表；方向校验死代码删除
    （stm32f4_spi_base_config 数组一并移除）。新增 SPI 设备三步：bsp_spi.h 加宏
    + s_spi_cfg[] 加一行 + isr 路由加一条
  - P2-8 已完成：usart_ops_t 加 usart_receive_byte（轮询+超时）；协议级 RX
    （IDLE+DMA + rx_notify 注入，照 spi_dma_sync_t 模式）已在接口头注释占位，
    蓝牙/指纹协议开发时实现
  - P2-11 已完成（2026-08-16）：hc165 + led 统一改"预填描述符"构造
    （C99 指定初始化器预填 struct，init 只做校验+硬件 init）；bsp_lock_t
    事务互斥类型已落 Bsp/common，hc165_t.lock 字段就位（当前 NULL，出现
    第二个总线消费者时 Apps 创建互斥量注入同一实例即可）；hc165_read/
    read_polling 已包事务锁（无锁时零开销直通）。惯例已写入 CLAUDE.md
    （含"出现运行时状态升级为 cfg 结构体"的判据）
  - **Opus 审查清单至此全部清零（P0~P3 + P1 + P2 全部完成）**
- 同 SPI 多任务互斥设计已定（用户确认方向）：Apps 创建互斥量 -> 注入 Component
  （hc165_t 加 mutex 字段包住整个器件事务），BSP 不掺和；等出现第二个消费者时实现

## 2026-08-16 会话补充（P3 收尾后的结构调整）
- 新建 `Apps/common/`：`app_common_def.h`（任务优先级集中定义，对应 bsp_common_def.h 命名）
  + `app_hooks.c`（FreeRTOS 应用钩子统一放这，勿散落到任务文件）
- `vApplicationStackOverflowHook` 从 app_init.c 移到 app_hooks.c
  -> 触发归档提取时机陷阱，用 `App_Hooks` OBJECT 库注入 elf 解决
  （详见 reference-cmake-linking.md 的实战记录）
- 任务头文件已瘦身为纯函数声明；.c 自行 include FreeRTOS/bsp 头
- 新增任务时：优先级登记到 app_common_def.h；新钩子加到 app_hooks.c；
  源文件手动加进 Apps/CMakeLists.txt 的 App_Task_Src

## 编译状态（2026-08-16 终版，P2-11 后）
- 零错误零警告，Flash: 33272 B (6.35%)，RAM: 3280 B (2.50%)，CCMRAM: 48KB/64KB（堆）

## 会话零散决策（2026-08-16，已固化到代码注释/CLAUDE.md，此处仅索引）
- 74HC165 PL 低电平 80ns 规格：间接调用开销(150~600ns)裕量充足，不加延时；
  失效条件（改直接 BSRR 写/换平台）已注释在 74hc165.c
- GPIO/DMA 时钟使能故意不进 spi_hw_config_t：F4 全家族焊死 AHB1 无设备差异，
  配置表只放"随设备变"的字段；SPI 本体 APB1/APB2 差异已由 base_clk_cmd 覆盖

## 下一步（2026-08-17 起）
1. ~~提交本版~~ 已完成：4e1d677 已推送（2026-08-16 深夜，经 rebase 解决孪生提交冲突）
2. 硬件验证按键读数：确认 Q7' 补偿方向和 KEY_ACTIVE_LEVEL_BITMAP 配置；
   板子改 Q7->SER 接线后删除 hc165_raw_to_keys 的补偿步（代码有 TODO 注释）
3. 小模型开发新驱动（LCD/蓝牙等）：框架已备好——SPI/USART 配置表模式、
   Component 预填描述符惯例、CLAUDE.md 成文约定，照 bsp_spi.c/74hc165.h 抄即可
4. 触发式待办：新 SPI 设备进场用三步接入法（见 bsp_spi.c 头注释）；
   usart 协议级 RX（IDLE+DMA+rx_notify 注入）在蓝牙/指纹开发时实现；
   **组件需要多实例**（如通用按键矩阵驱动多组 74HC165）→ 套虚表/实例分离
   +create/destroy 模板，见 doc/knowledge/c-oop-static-vs-dynamic.md（BSP 层保持
   const 注册表不动）

## Why: 确保下次对话直接知道修复进度，避免重复检查。
## How to apply: 审查修复全部完成；下次会话从"下一步"清单或新需求继续。
