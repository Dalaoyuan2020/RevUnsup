---
type: decision
created: 2026-04-09
updated: 2026-04-09
tags: [核心决策, L5, 知识组织]
related: [[硬盘内存缓存模型]] [[Harness是什么]] [[反过度工程化]]
---

# 决策: 采用 LLM Wiki 模式 (Karpathy 风格)

## 一句话总结
**2026-04-09 决定: 把 RevUnsup 的所有知识用 Karpathy LLM Wiki 模式组织, wiki/ 目录是【内存层】, docs/ 是【硬盘层】。**

## 背景

Napoleon 在 2026-04-09 多次反馈:
1. "你写的乱七八糟的"
2. "用一种新的方法, 例如知识架构, 知识图谱"
3. "你脑子混乱, 导致我编排效果差"

提到的参考: Karpathy 的 LLM Wiki 模式 (4 月份很火)。

## 核心理念 (Karpathy 原话, 抽象的)

LLM Wiki 不是 RAG。RAG 是每次查询重新从原文检索, LLM Wiki 是 LLM 拥有并持续维护一份 wiki, 知识【累积】。

**三层**:
1. Raw sources (immutable) = 硬盘
2. Wiki (LLM 维护) = 内存
3. Schema (config) = 规则书

**三操作**: ingest / query / lint
**两特殊文件**: index.md / log.md

## 跟 [[硬盘内存缓存模型]] 完美对应

| 层 | LLM Wiki | RevUnsup 落地 |
|---|---|---|
| 硬盘 | Raw sources | `docs/` `.persona/` `configs/` |
| 内存 | Wiki | `wiki/` |
| 缓存 | LLM context | Agent 当前对话 |
| Schema | CLAUDE.md/AGENTS.md | `WIKI.md` (仓库根) |

## 选这个模式的 3 个理由

### 1. 解决 Claude 之前的"脑子混乱"
之前所有知识都在硬盘 (`docs/`), 没有内存层。每次决策时都要去翻硬盘原文, 上下文撑爆。
有了 wiki/ 内存层后, 每次只需要读 3-5 个内存节点。

### 2. 让多 Agent 在【共同的世界】里工作
[[5Q+1L Harness]] 已经强调"先建共同世界"。
wiki/ 就是共同的世界。所有 Agent 启动时读 wiki/index.md, 就有了相同的认知地图。
这直接解决 [[2026-04-09事故_DogWind删文件]] 的根因。

### 3. 知识可累积
- ingest 操作: 每次产出新知识就 wiki 化
- lint 操作: 定期清理孤立/重复/过期节点
- 知识不会随会话结束而消失

## 落地方案 (2026-04-09 v0.1)

### 目录结构
```
RevUnsup/
├── WIKI.md           ← Schema (规则书)
├── wiki/             ← 内存层
│   ├── index.md      ← 主索引
│   ├── log.md        ← append-only 时间线
│   ├── 五层架构.md
│   ├── 5Q+1L Harness.md
│   ├── KNN瓶颈.md
│   ├── ...
└── docs/             ← 硬盘层 (大量文档保持不变)
```

### 节点类型 (5 种)
- concept (概念)
- fact (事实/数字)
- entity (人/Agent/设备)
- event (事件)
- decision (决策)

详见 [[WIKI.md]] (硬盘根)

## 替代方案 (没选)

| 方案 | 为什么没选 |
|---|---|
| 知识图谱 (节点+边可视化) | 需要工具, 增加复杂度 |
| 多维标签索引表 | 不够"图谱", 没有双向链接 |
| Notion | 离开 git 仓库, 同步成本高 |
| Roam Research | 同上, 而且付费 |
| Obsidian | 跟 wiki/ 兼容, 但当前不需要 GUI |

## 参考的开源实现

- [Karpathy LLM Wiki gist](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) (原始)
- [SamurAIGPT/llm-wiki-agent](https://github.com/SamurAIGPT/llm-wiki-agent) (多 IDE 通用)
- [lucasastorian/llmwiki](https://github.com/lucasastorian/llmwiki) (Claude MCP)
- [NickSpisak (NicholasSpisak)](https://github.com/NicholasSpisak) (Obsidian 实现)
- [farzaa/personal_wiki_skill.md](https://gist.github.com/farzaa/c35ac0cfbeb957788650e36aabea836d) (Skill 形式)

## 反过度工程化提醒

按 [[反过度工程化]] 原则:
- 第一批只做最小可用集 (~10 个核心节点)
- 不创造新内容, 只【提炼】硬盘里已有的
- 节点 50-200 行, 超过就拆
- 过 1 周再看是否需要 LINT

## 演进路径

- **v0.1** (2026-04-09): 最小可用集, 手动维护
- **v0.2**: 加入 Agent 自动 ingest 能力
- **v0.3**: lint 自动化 (定期检查孤立/重复节点)
- **v1.0**: Wiki 成为 [[Harness是什么]] 的核心运行时

## 相关
- [[硬盘内存缓存模型]] — 这个决策的理论基础
- [[Harness是什么]] — Wiki 服务于 Harness 的运行时
- [[5Q+1L Harness]] — Wiki 是 Q3 (记得什么) 的具体实现
- [[反过度工程化]] — 不要一上来就完美

## 来源
- 微信对话 2026-04-09 下午 (Napoleon 的反馈)
- WebSearch 找到的 Karpathy LLM Wiki 资料
