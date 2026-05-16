用户准备提交代码，请按以下步骤依次检查和更新：

## 第1步：检查本次改动

运行 `git status` 和 `git diff --stat` 了解本次改动了哪些文件和内容。

## 第2步：更新 CLAUDE.md

根据本次改动，检查 `CLAUDE.md` 是否需要更新：
- 目录结构是否有变化（新增/移动/删除文件）
- CMake target 架构是否有变化
- 编码约定是否有新增
- 文档索引是否需要补充

只在有变化时才修改，不要无意义地重写。

## 第3步：更新 .claude/MEMORY.md

根据本次改动，检查 `.claude/MEMORY.md` 是否需要更新：
- 新增的外设驱动状态
- 新增的设计模式或架构决策
- 关键约定的变化

只在有变化时才修改。如果涉及新的技术主题，考虑创建独立的知识文档（如 `.claude/xxx-knowledge.md`），并在 MEMORY.md 中加索引。

## 第4步：更新 auto memory

检查 `C:\Users\Administrator\.claude\projects\J--ArmsApprentice-wuyue-heng-2-software-wuyue-heng\memory\MEMORY.md` 是否需要同步更新。

## 第5步：更新 README.md

根据本次改动，检查 `README.md` 是否需要更新：
- 外设驱动状态表
- 目录结构
- 架构说明

只在有变化时才修改。

## 第6步：生成 commit message

按以下格式生成 commit message，直接输出到终端让用户复制：

```
ENH:简要描述本次改动

WHY:为什么要做这次改动

WHAT:具体做了什么
1、xxx
2、xxx
```

参考最近的 git log 格式，以 ENH: 或 FIX: 开头。不要自动执行 git commit，让用户自己操作。
