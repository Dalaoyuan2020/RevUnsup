---
type: entity
created: 2026-04-09
updated: 2026-04-09
tags: [Agent, L1]
related: [[OldMajor_Codex]] [[Napoleon]] [[A2A交接协议]] [[2026-04-09事故_DogWind删文件]]
---

# DogWind (XYC_Windsurf)

## 一句话总结
**鑫业城 5060 上跑的 Windsurf Agent, 角色: 代码分析师 (只读不写)**

## 基本信息

- **完整名**: XYC_Windsurf (鑫业城_Windsurf)
- **简写**: DogWind
- **设备**: 鑫业城 5060 (xyc, 工作机, 不入猪圈)
- **编译器**: Windsurf
- **角色**: 代码分析师, 经验考古的同族兄弟
- **创建日期**: 2026-04-08
- **人格文件**: `.persona/PERSONA.md` + `.persona/skills/`

## 已完成的任务

| 任务 | 状态 | 产出 |
|---|---|---|
| RU-001: 封装版代码地图 | ✅ DONE | `docs/封装版代码地图.md` (8 题全答完, 第 8 题挖出 5 个金矿) |
| RU-001 解密附件 | ✅ DONE | `docs/decrypted_infer_demo.cpp` 等 3 份 (羊爸爸手动粘贴) |
| RU-003: 杠杆清单 + 阈值协议 | ✅ DONE | `docs/杠杆清单.md` + `docs/阈值校准协议.md` |

## 关键发现 (来自 RU-001)

DogWind 在封装版里挖出 3 个金矿, 后来都进了 wiki:
- [[KNN瓶颈]] — `nearest_neighbors` 分块搜索是耗时大头
- [[阈值陷阱]] — `pow(thresh, 2.0)` 偷偷平方
- [[隐藏可调参数]] — `modelInputW/H` 等被硬编码

## 5 条铁律 (人格文件里)

1. 不假装跑通了
2. 不绕过加密 (等羊爸爸手动粘贴)
3. 不修改代码 (只读, 除非任务明确允许)
4. 不写没用的话
5. 每次任务后留痕

## 角色定位 (跟 [[OldMajor_Codex]] 互补)

| | DogWind | OldMajor_Codex |
|---|---|---|
| 设备 | 鑫业城 5060 | 309 老 2080 |
| 编译器 | Windsurf | Codex |
| 对象 | 甲方封装版 + 源码 | 旧版预实验 |
| 视角 | 看现在 | 挖历史 |
| 产出 | 代码地图 + 金矿 | 经验清单 + 数字链 |

**不重叠原则**: DogWind 不动 Old Major 的旧实验, OldMajor 不动鑫业城的代码。

## ⚠️ 已知问题

### 2026-04-09 事故
DogWind 在 push RU-002+RU-003 时, 一次删了 6 个其他 Agent 的文件。
详见 [[2026-04-09事故_DogWind删文件]]
教训: L4 通讯协议没建立, DogWind 不知道哪些是别人的文件。

直接催生了 [[A2A交接协议]] 和 [[5Q+1L Harness]] 的强化。

## 下一步

按 [[Harness是什么]] 的精神, DogWind 不再被【任务驱动】, 而是被 [[5Q+1L Harness]] 框架驱动:
- 启动时跑 `scripts/env_check.py` (Q2)
- 读 `configs/acceptance_criteria.yaml` (Q5)
- 完成后跑 `tests/test_agent*.py` (Q5)
- 不写新代码, 只在已有 6 文件框架内跑

## 相关
- [[OldMajor_Codex]] — 同族兄弟
- [[Napoleon]] — 上级 (L2 编排)
- [[A2A交接协议]] — DogWind 必读
- [[5Q+1L Harness]] — DogWind 应该按这个框架工作
- [[2026-04-09事故_DogWind删文件]] — DogWind 的负面教材
- [[KNN瓶颈]] [[阈值陷阱]] [[隐藏可调参数]] — DogWind 的金矿

## 来源
- 硬盘: `.persona/PERSONA.md`
- 硬盘: `docs/封装版代码地图.md`
- 硬盘: `docs/杠杆清单.md`
- 硬盘: `docs/阈值校准协议.md`
