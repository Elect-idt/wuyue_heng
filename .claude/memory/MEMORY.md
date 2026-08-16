# wuyue_heng 项目记忆索引

> 项目相关知识（跟随仓库走，可 git 追踪、团队共享）。
> 架构规则/编码约定见项目根目录的 `CLAUDE.md`（每次会话自动加载），本目录只放按需查阅的参考资料和进度。

## 参考资料
- [reference-cmake-linking.md](reference-cmake-linking.md) — CMake 链接深层机制：OBJECT 库原理、弱符号+静态库陷阱、编译vs链接、$<TARGET_OBJECTS:> vs target_link_libraries（排错时查）
- [reference-ccmram-vs-sram.md](reference-ccmram-vs-sram.md) — CCMRAM vs SRAM：CCMRAM 是 CPU 私有内存 DMA 够不着；启用时机与用法（用到才查）

## 项目进度
- [project-p0-fixes-completed.md](project-p0-fixes-completed.md) - 审查清单全部清零（P0~P3+P1+P2，2026-08-16）；含硬件待办（Q7 错接补偿、active level 位图）与新器件开发惯例入口
