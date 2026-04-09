---
type: concept
created: 2026-04-09
updated: 2026-04-09
tags: [核心, 阶段二, L4]
related: [[Harness是什么]] [[五层架构]] [[多Agent协作]] [[A2A交接协议]]
---

# 5Q+1L Harness

## 一句话总结
**阶段二 (2026-04-XX) 设计的 Harness 最小骨架: 用 5 道防线控制 Agent 的"世界", 而不是控制 Agent 怎么写代码。**

## 来源
Napoleon 在阶段一失败后提出的方案。
关键洞察 (原文):
> "阶段 1 的失败不是因为谁能力不行, 而是因为三个人从头到尾没有在同一个世界里工作。阶段 2 的设计: 先建共同的世界, 再让每个人在这个世界里干活。"

## 5Q+1L 定义

| | 含义 | 实现 |
|---|---|---|
| **Q1** 做什么 | 任务 | Prompt 任务清单 |
| **Q2** 看到什么 | 环境 | scripts/env_check.py + 共享数据集 |
| **Q3** 记得什么 | 知识 | Prompt 技术约束 + 中文注释 + lighthouse.yaml |
| **Q4** 不能做什么 | 边界 | 交叉验收机制 (上游 PASS 才能开工) |
| **Q5** 怎么知道对了 | 验证 | tests/test_agent*.py 自动化测试 |
| **L** 灯塔 | 备选 | configs/lighthouse.yaml (决策记录 + 备选方案 + 预留端口) |

## 对应到 [[五层架构]]

| 5Q+1L | 五层位置 |
|---|---|
| Q1 (做什么) | L2 编排 |
| Q2 (看到什么) | L4 通讯 (可见) |
| Q3 (记得什么) | L4 通讯 (可达) |
| Q4 (不能做什么) | L4 通讯 (可信) |
| Q5 (怎么知道对了) | L3 评估 (收敛) + L4 (可标) |
| L 灯塔 | L5 协议封装 |

## 阶段二的执行顺序

```
第 0 步: 准备 Harness
       env_check.py + 验收数据集 + 测试脚本 + lighthouse.yaml
       ↓
第 1 步: Agent 1 + Agent 3 并行启动
       Agent 1 训练 / Agent 3 先搭 C++ 框架
       ↓
       Agent 1 交付 → 跑 test_agent1.py → PASS?
       ↓
第 2 步: Agent 2 启动
       先验 Agent 1 交付物 → OK → 开始推理+导出
       ↓
       Agent 2 交付 → 跑 test_agent2.py → PASS?
       ↓
第 3 步: Agent 3 接入真实 ONNX
       先验 Agent 2 交付物 → OK → 完成 C++ 推理+DLL
       ↓
       Agent 3 交付 → 跑 test_agent3.py → PASS?
       ↓
第 4 步: 全链路门禁
       跑 integration_test.py → ALL PASS → ✅ 交付
                              → FAIL → 定位责任人 → 修复 → 重跑
```

## 仓库已落地的 6 个文件 (重要!)

这些文件【早就在 RevUnsup 仓库里】, 但 DogWind/OldMajor_Codex 都没用过:
- `scripts/env_check.py`
- `configs/acceptance_criteria.yaml`
- `configs/lighthouse.yaml`
- `tests/test_agent1.py`
- `tests/test_agent2.py`
- `tests/test_agent3.py`
- `tests/integration_test.py`

**Claude 的失败教训**: 我之前重新发明了 [[A2A交接协议]] 等等, 没看到这 6 个文件早就把 L4 设计好了。

## Karpathy LLM Wiki vs 5Q+1L

| 维度 | LLM Wiki | 5Q+1L |
|---|---|---|
| 关注点 | 知识组织 | Agent 协作 |
| 单位 | wiki 节点 | Agent 任务 |
| 核心机制 | ingest/query/lint | env_check/acceptance/test |

两者互补:
- LLM Wiki 解决【硬盘→内存→缓存】的知识流动
- 5Q+1L 解决【多 Agent 在同一世界协作】

## 相关
- [[Harness是什么]] — 5Q+1L 是 Harness 的最小落地
- [[五层架构]] — 5Q+1L 跨越 L2/L3/L4/L5
- [[多Agent协作]] — 5Q+1L 的服务对象
- [[A2A交接协议]] — 我误以为新创造的, 其实只是 5Q+1L Q4 的子集
- [[2026-04-09事故_DogWind删文件]] — 没用 5Q+1L 导致的事故

## 来源
- Notion 项目卡: RevUnsup 项目主页, "阶段 2 落地方案" 章节
- docs/configs/ tests/ 目录的实际文件
