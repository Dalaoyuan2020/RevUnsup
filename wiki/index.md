---
type: meta
created: 2026-04-09
updated: 2026-04-09
---

# wiki/index.md — RevUnsup 知识地图

> 主索引。所有 wiki 节点都列在这里, 按 type 分组。
> Agent 启动后第一件事就是读这个文件, 找到自己需要的节点。

---

## 🔵 概念 (concept)

- [[五层架构]] — Napoleon 定义的 L1-L5 项目分层
- [[5Q+1L Harness]] — 阶段二的 5 道防线设计 (env_check / 验收 / 灯塔)
- [[Harness是什么]] — 不是项目管理, 是自动迭代框架
- [[硬盘内存缓存模型]] — 知识的三层存储编排原则
- [[多Agent协作]] — 同层内多 IDE 互相监督, 不是跨设备
- [[反过度工程化]] — 从最简单方案开始, 验证后才加复杂度

## 🟢 事实 (fact)

- [[KNN瓶颈]] — 5600→83.3 ms (旧版预实验数据链)
- [[阈值陷阱]] — 封装版 pow(thresh, 2.0) 内部平方
- [[业务可分性问题]] — Good/Bad 分数重叠是真天花板
- [[隐藏可调参数]] — 封装版硬编码 modelInputW/H 等

## 🟠 实体 (entity)

- [[DogWind]] — 鑫业城 5060 上的 Windsurf Agent, 代码分析师
- [[OldMajor_Codex]] — 309 老 2080 上的 Codex Agent, 经验考古学家
- [[Napoleon]] — Mac mini 主控, L2 编排者
- [[设备地图]] — 三只猪 (Napoleon/Snowball/Old Major) + 鑫业城 5060
- [[封装版.exe]] — 甲方提供的工业部署形态
- [[源码版]] — 加密的源码, 需要白名单软件读
- [[旧版预实验代码]] — Old Major 上的 H:\\RevUnsup-main

## 🔴 事件 (event)

- [[2026-04-09事故_DogWind删文件]] — DogWind push 误删 6 个文件
- [[2026-04-09 RU-001完成]] — DogWind 完成封装版代码地图
- [[2026-04-09 OM-001完成]] — OldMajor_Codex 完成预实验经验清单

## 🟣 决策 (decision)

- [[选PatchCore]] — RevUnsup 的核心算法
- [[选FAISS_IVFFlat]] — KNN 加速方案 (旧版已验证 67x)
- [[A2A交接协议]] — 防止 Agent 互相覆盖工作
- [[采用LLM_Wiki模式]] — 知识组织方式 (Karpathy 风格)
- [[暂缓L5]] — 当前优先 L1+L2+L3, 不急 L5

---

## 各层架构对应的节点

### L1 Worker (执行)
[[DogWind]] [[OldMajor_Codex]] [[封装版.exe]] [[源码版]] [[旧版预实验代码]]

### L2 主脑 (编排)
[[Napoleon]] [[五层架构]] [[Harness是什么]] [[硬盘内存缓存模型]]

### L3 评估 (收敛)
[[KNN瓶颈]] [[阈值陷阱]] [[业务可分性问题]]

### L4 通讯 (Agent 互相说话)
[[A2A交接协议]] [[5Q+1L Harness]] [[多Agent协作]] [[2026-04-09事故_DogWind删文件]]

### L5 协议封装 (暂缓)
[[采用LLM_Wiki模式]]

---

## 当前活跃工作 (优先级倒序)

🔥 P0 — 立刻
- 让代码真正跑起来 (基于阶段二 6 个文件)
- 建立 [[硬盘内存缓存模型]] 落地: 这个 wiki 就是内存层

🟡 P1 — 这周
- 把现有 docs/ 内容继续 ingest 到 wiki
- 第一次 LINT 检查

🟢 P2 — 后续
- 第三个 Agent (待定)

---

## 节点统计

| Type | 数量 | 待补 |
|---|---|---|
| concept | 6 | 5Q+1L 详情, 多 Agent 协作机制 |
| fact | 4 | (待跑出 baseline 数字后补充) |
| entity | 7 | 第三个 Agent 待定 |
| event | 3 | 后续大事件继续记录 |
| decision | 5 | 后续决策记录 |
| **合计** | **25** | (本批为最小可用集) |

---

## 如何贡献新节点

参考 [[WIKI.md]] 的"三大操作 → INGEST"章节。
简短版:
1. 创建 `wiki/<节点名>.md` (按格式)
2. 在本文件 (index.md) 的对应分组加一行
3. 在 `wiki/log.md` append 一行
4. 找 2-5 个相关节点, 双向加 `related`

---

*v0.1 / 2026-04-09 / 第一批 25 个节点*
