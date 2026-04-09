# WIKI.md — RevUnsup 知识架构 Schema

> **这是给 LLM/Agent 读的 schema 文档**, 类似 Karpathy LLM Wiki 模式里的 AGENTS.md。
> 任何 Agent (DogWind / OldMajor_Codex / Claude / 后来的) 启动时, 必须先读本文件, 才知道怎么在 wiki 里找东西、加东西、维护东西。
>
> 灵感来源: [Karpathy LLM Wiki](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f)
> 创建日期: 2026-04-09

---

## 知识架构的【三层存储】比喻

```
💾 硬盘  = Raw sources (原始资料, immutable)
   位置: docs/, .persona/, configs/, 以及 RU-001/OM-001 等任务输出
   特点: 不变, 大量, 不常访问

🧠 内存  = Wiki nodes (LLM 维护的活跃知识)
   位置: wiki/
   特点: 双向链接, 简短, 高频访问, 持续维护

⚡ 缓存  = LLM 当前对话的上下文
   位置: 每次启动时被注入的几个 wiki 节点
   特点: 最快, 最小, 任务完成后释放
```

**编排原则**: 不要把硬盘的所有东西都塞进缓存。从硬盘 → 加载到内存 → 提取到缓存。

---

## wiki/ 目录约定 (扁平结构)

```
wiki/
├── index.md           ← 主索引 (像 Wikipedia 主页, 列所有节点)
├── log.md             ← append-only 时间线 (谁什么时候做了什么)
├── <节点1>.md          ← 各类节点扁平存放
├── <节点2>.md
└── ...
```

**为什么扁平**: 双向链接的本质是网络, 不是树。不分子目录, 用 frontmatter 的 `type` 字段来分类。

---

## 节点类型 (frontmatter `type` 字段)

| type | 含义 | 例子 |
|---|---|---|
| `concept` | 抽象概念/方法论 | 五层架构, 5Q+1L Harness, Harness是什么 |
| `fact` | 具体数字/状态 | KNN瓶颈数据, 阈值陷阱, 业务可分性 |
| `entity` | 具体的人/Agent/设备/文件 | DogWind, OldMajor_Codex, Napoleon Mac mini |
| `event` | 发生的事情 | 2026-04-09 DogWind 删文件事故 |
| `decision` | 选择和理由 | 选 PatchCore, 选 FAISS IVFFlat, 选 A 方案 Wiki |

---

## 节点文件格式 (强制)

```markdown
---
type: concept|fact|entity|event|decision
created: YYYY-MM-DD
updated: YYYY-MM-DD
tags: [tag1, tag2]
related: [[节点A]] [[节点B]]
---

# 节点标题

## 一句话总结
(必填, 一句话说清楚这个节点是什么)

## 正文
(用大白话, 50-200 行以内)

## 相关
- [[节点A]] — 为什么相关
- [[节点B]] — 为什么相关

## 来源
- 硬盘文件: docs/xxx.md (或者 RU-XXX 任务的产出)
- 对话: 微信/chat 某次讨论
```

---

## 三大操作 (Karpathy 模式)

### 1. INGEST (吸收新知识)
触发: 有新资料/对话/产出时
动作:
- 判断这是 concept/fact/entity/event/decision 中的哪种
- 找一个简短中文名 (用空格或下划线)
- 创建 `wiki/<名字>.md`
- 在 `wiki/index.md` 加一行
- 在 `wiki/log.md` append 一行 "YYYY-MM-DD: ingest <节点名>"
- 找出 2-5 个相关节点, 在 frontmatter `related` 里加 [[双向链接]]
- 同时去更新那些相关节点的 `related`, 加上自己

### 2. QUERY (回答问题)
触发: 用户/Agent 问问题时
动作:
- 先读 `wiki/index.md`
- 找出相关的 3-5 个节点 (按 type + tags + 关键词)
- 把这些节点 ingested 到当前对话上下文 (缓存)
- 基于这些节点回答, **不是从硬盘原文回答**
- 如果发现 wiki 里没有相关节点, 触发 INGEST

### 3. LINT (维护)
触发: 定期 (一周一次?) 或感觉知识混乱时
动作:
- 检查所有节点的 `related` 是否双向 (A 链接 B, B 也要链接 A)
- 检查孤立节点 (没人链接的)
- 检查重复节点 (两个名字描述同一件事 → 合并)
- 检查过期节点 (内容已经被新事实推翻)
- 检查 `index.md` 是否完整列出所有节点
- 检查 `log.md` 是否有重要事件被遗漏

---

## 写作风格 (强制)

1. **大白话** — 不要文档腔, 当成跟同事聊天
2. **短** — 单个节点 50-200 行, 超过就拆
3. **一句话总结必填** — 这是这个节点的"slogan"
4. **双向链接** — 任何提到其他节点的地方都用 `[[节点名]]`
5. **来源必填** — 说清楚这个知识是从哪里来的, 方便溯源
6. **不重复硬盘内容** — wiki 节点是【提炼】, 不是【复制】

---

## Agent 使用指南

### Agent 启动时 (强制)
```
1. 读 WIKI.md (本文件)
2. 读 wiki/index.md (看有哪些节点)
3. 根据当前任务, 读 3-5 个相关节点
4. 开始干活
```

### Agent 完成任务后 (强制)
```
1. 决定: 这次产出有没有新知识值得 ingest 到 wiki?
2. 如果有: 执行 INGEST 操作
3. 在 wiki/log.md append 一条
```

---

## 当前状态 (2026-04-09)

- ✅ wiki/ 目录已建
- ✅ wiki/index.md 已建 (空索引等待填充)
- ✅ wiki/log.md 已建
- 🔄 第一批节点正在 ingest (从现有 docs/ 提炼)
- ⏳ Agent 还没有读过本 schema, 需要在下一次启动时强制读

---

## 演进规则

- 本 schema (WIKI.md) 不要轻易改
- 真要改, 在 wiki/log.md 记录"schema 更新: <修改内容>"
- 所有 wiki 节点的格式 follow 本 schema
- 不符合 schema 的节点 → LINT 操作时修复

---

*v0.1 / 2026-04-09 / 创建于 DogWind 删文件事故之后, 用于建立"共同的世界"*
