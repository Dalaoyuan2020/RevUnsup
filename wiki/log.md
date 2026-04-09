---
type: meta
created: 2026-04-09
updated: 2026-04-09
---

# wiki/log.md — append-only 时间线

> 谁什么时候做了什么。只 append, 不修改, 不删除。
> 所有 INGEST/LINT/重大决策都在这里留一条。

---

## 2026-04-09

- **15:30** — 创建 wiki/ 目录, 起草 [[WIKI.md]] schema (三层存储 + 三操作 + 节点格式)
- **15:30** — 创建 wiki/index.md (主索引), 列出第一批 25 个节点
- **15:30** — 创建 wiki/log.md (本文件)
- **15:30** — 第一批 INGEST: 核心 concept/fact/entity/event/decision 节点 (从现有 docs/ 提炼)
- **早些时候** — 决策: [[采用LLM_Wiki模式]] (Karpathy gist + NickSpisak/SamurAIGPT 参考)
- **早些时候** — 事件: [[2026-04-09事故_DogWind删文件]] (DogWind push 删 6 文件, 已修复)
- **早些时候** — 完成: [[2026-04-09 RU-001完成]] (DogWind 封装版代码地图)
- **早些时候** — 完成: [[2026-04-09 OM-001完成]] (OldMajor_Codex 预实验经验清单)

## 2026-04-08

- 创建 [[DogWind]] 人格 (XYC_Windsurf), 喂饭执行 RU-001
- 创建 [[OldMajor_Codex]] 人格 (Codex on 2080), 喂饭执行 OM-001

## 2026-04-XX (历史, 阶段二)

- 阶段二 [[5Q+1L Harness]] 设计完成, 6 个文件 push 到仓库:
  - scripts/env_check.py
  - configs/acceptance_criteria.yaml
  - configs/lighthouse.yaml
  - tests/test_agent1.py / test_agent2.py / test_agent3.py
  - tests/integration_test.py

## 2026-03-XX (历史, 阶段一)

- 三个 Milestone 跑通但失败
- 失败教训: "三个人没有在同一个世界里工作" (Napoleon 总结)
- 这个教训直接催生了阶段二的 5Q+1L 设计

---

## INGEST 操作日志格式

```
- HH:MM — INGEST [[节点名]] (type=xxx) | 来源: <硬盘文件或对话>
```

## LINT 操作日志格式

```
- HH:MM — LINT 全量 | 修复: <什么>; 合并: <什么>; 删除: <什么>
```

## 重大决策日志格式

```
- HH:MM — DECISION [[决策名]] | 选 X 不选 Y, 理由: <一句话>
```

---

*append-only · 不修改 · 不删除*
