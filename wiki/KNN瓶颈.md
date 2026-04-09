---
type: fact
created: 2026-04-09
updated: 2026-04-09
tags: [性能, L1, 关键发现]
related: [[封装版.exe]] [[旧版预实验代码]] [[选FAISS_IVFFlat]] [[OldMajor_Codex]] [[DogWind]]
---

# KNN 是确定性瓶颈

## 一句话总结
**PatchCore 推理的耗时大头就是 KNN 搜索。旧版已经验证: 5600ms → 83.3ms (FAISS IVFFlat 67x 加速)。**

## 双重证据 (横向收敛)

### 证据 1: [[DogWind]] 在 [[封装版.exe]] 里看到的
封装版的 `nearest_neighbors` 函数有【分块搜索】机制:
```cpp
float spanlen1 = ceil(float(maxSearchNaxFeatrueSize1) / float(embedding_size));
int iterTimes = ceil(float(mb_size) / spanlen1);
```
- 控制参数: `maxSearchNaxFeatrueSize1` (在 modelConfig.yaml, 默认 1.4e9)
- 这是【防 OOM 的优化】, 也是【耗时的隐藏瓶颈】

### 证据 2: [[OldMajor_Codex]] 在 [[旧版预实验代码]] 里跑出的完整数字链
| 阶段 | 推理时间 | 加速 |
|---|---|---|
| 单线程 CPU 暴力搜索 | 5600 ms | 1x |
| OpenMP 多线程 | 1427 ms | 3.9x |
| FAISS FlatL2 | 258.1 ms | 21.7x |
| **FAISS IVFFlat** | **83.3 ms** | **67.2x** |

证据来源:
- `H:\RevUnsup-main\RECEIPT.md`: "推理时间 ~5500ms, 主要耗时在 KNN 搜索"
- `H:\RevUnsup-main\src\faiss_inference.py`: 注释明确写"使用 FAISS-CPU 加速 KNN 搜索"
- `H:\RevUnsup-main\results_faiss\faiss_performance.json`: IVFFlat 模式 83.3 ms 总加速 67.2x

## 关键参数

旧版验证的 FAISS IVFFlat 配置:
- `nlist = 100`
- `nprobe = 10`

## 意义

这是【确定性结论】, 不需要再探索:
1. 优化方向已知 (KNN)
2. 答案已知 (FAISS IVFFlat)
3. 可复用资产已有 (旧版的 src/faiss_inference.py)

[[Harness是什么]] 的优化循环可以直接跳到这一步, 不需要从零探索。

## ⚠️ 但要注意

旧版的 83.3 ms 是【selftest 6 张图 + 21811x384 memory bank】的数字。
鑫业城业务数据上的实际加速比可能不一样, 因为:
- memory bank 大小不同
- 图分辨率不同
- batch 不同

所以 [[业务可分性问题]] 之外, 还需要在 5060 上重新跑一次确认。

## 相关
- [[封装版.exe]] — DogWind 在这里看到 nearest_neighbors 分块机制
- [[旧版预实验代码]] — OldMajor_Codex 在这里挖出完整数字链
- [[选FAISS_IVFFlat]] — 基于本节点的决策
- [[隐藏可调参数]] — modelInputW/H 等也是 KNN 之外的杠杆
- [[业务可分性问题]] — 提速容易, 提精度更难

## 来源
- 硬盘: `docs/封装版代码地图.md` §8 最可疑 #4
- 硬盘: `docs/预实验经验清单.md` §3 发现 1 + §7 复用 #1
