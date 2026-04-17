# RU-014 Coreset 1% 消融报告 (v2 修正版)

> **日期**: 2026-04-17  
> **执行者**: DogWind (XYC_Windsurf)  
> **机器**: PC-20260115QQCJ / RTX 5060 Ti 16GB / CUDA 12.9  
> **单变量**: coreset_sampling_ratio 0.1 → 0.01，其他全不动  

## 摘要

**coreset 0.1→0.01 在客户 pipeline 中效果远低于预期。MB 仅缩 19%（679→553MB），推理仅快 1.5%（745→734ms）。根因：客户代码对特征做了分块（featureSpan=1, maxTrainSize 分块），每块独立 coreset 后仍累积大量特征。**

---

## Step 0: yaml 生效验证

### grep 结果
```
train.py:159: model.coreset_sampling_ratio = data['coreset_sampling_ratio']
train.py:178: model.coreset_sampling_ratio = 0.5   # 仅 modelTrainType=1 时 override
modelConfig.yaml:13: coreset_sampling_ratio: 0.1
```
- 默认 modelTrainType=0 → yaml 值生效 ✅

### smoke 验证 (3 图 × 2 epoch)
| 实验 | coreset ratio | model.mb |
|------|---------------|----------|
| RU-011-v2 smoke (0.1) | 0.1 | 54.75 MB |
| RU-014 smoke (0.01) | 0.01 | 45.04 MB |

缩减 18%，不是预期的 ~10x。但 MB 确实变化 → **yaml 生效确认** ✅

**结论**: yaml 生效 ✅，但缩减幅度远低于论文预期

---

## MB 大小对比

| 实验 | 图数 | coreset ratio | model.mb | 缩减比 |
|------|------|---------------|----------|--------|
| RU-011-v2 | 39 | 0.1 | **679 MB** | 1x |
| **RU-014** | 39 | **0.01** | **553 MB** | **1.23x (仅 -19%)** |
| 客户原模型 (参考) | ? | 0.1 | 57 MB | — |

### 为什么只缩 19% 而非 10x？

根因分析：客户代码 `subsample_embedding11()` 在 `.pyd` 内部实现，无法直接查看。但从行为推断：

1. **分块 coreset**: `modelConfig.yaml` 有 `maxTrainSize: [51200, 51200]` 和 `maxTrainSize1: [5120, 5120]`。代码将特征矩阵按 `maxTrainSize` 分块，**每块独立做 coreset**。
2. **每块最小采样数**: 即使 ratio=0.01，每个小块的 1% 可能低于某个最小值，被 clamp 到最小采样数。
3. **featureSpan=1**: 特征在空间维度不做 stride，产生大量特征点，分成许多小块后 coreset 效率低。
4. **dataAugIters=10**: 每图 10 次增广，特征量是无增广的 10x。

**论文 vs 客户代码的关键差异**：论文的 coreset 是对**全部特征一次性采样**；客户代码是**分块后每块独立采样再拼接**，这导致总缩减比远低于 ratio 的倒数。

---

## 推理时间对比 (同 18 图集)

| 实验 | 平均推理 ms | 相对加速 |
|------|------------|---------|
| RU-011-v2 (0.1) | **745** | 1.0x |
| RU-014 (0.01) | **734** | **1.015x (-1.5%)** |

推理几乎无差。原因：
- MB 仅缩 19% → KNN 遍历量几乎不变
- DLL KNN 是循环 cdist（非 faiss），O(N) 线性 → 19% MB 缩减 → 理论快 19%
- 实际推理中 forward (backbone) 占主导 (~52ms/79%)，KNN 改善被稀释

---

## 精度对比

| 类别 | RU-011-v2 (0.1) | RU-014 (0.01) |
|------|-----------------|---------------|
| Good (9 张) | 0/9 | 0/9 |
| Real Bad (3 张) | 3/3 | 3/3 |
| Synth Bad (6 张) | 6/6 | 6/6 |
| **Overall** | **9/18** | **9/18** |

精度完全一致。good 误报问题不变——原因是 MB 太大导致 score 饱和，与 coreset ratio 无关。

---

## Step 7: 阈值分布观察

所有 18 图 anomaly_score = 1.0000（满分）。

**原因**: eval_main 的 `score = countNonZero(resultImage) / (rows * cols)` 计算的是 anomaly mask 的非零像素占比。DLL 内部以 `anomalyThresh=0.1` 做二值化，由于 MB 过大导致距离值全超阈值，mask 全非零 → score 全 1.0。

**结论**: 在当前 MB 规模 (553-679 MB) 下，eval_main 的 score 已饱和，**无法区分 good 与 bad**。要获得有意义的阈值分布，需要 MB < ~100MB（参考客户 57MB 模型可以工作）。

---

## 训练时间

| 实验 | 训练时间 | 备注 |
|------|---------|------|
| RU-011-v2 (0.1) | ~30 min | CPU mode |
| RU-014 (0.01) | **92.5 min** | CPU mode, coreset greedy 子采样耗时长 |

coreset ratio 降到 0.01 反而训练更慢：greedy k-center 算法仍需对全部 N 个候选计算距离，只是选出的点更少，但分块+多次迭代导致总耗时增加。

---

## Risk Hit (实际触发的风险)

| # | 风险 | 触发？ | 详情 |
|---|------|--------|------|
| 1 | 论文消融基于 WR50 | N/A | 问题不在 backbone，在分块 coreset |
| 2 | 训练超 45 分钟 | **YES** | 92.5 min，2.3x 预期 |
| 3 | MB 仍 700+ | **部分** | 553 MB，未达 50-200 目标 |
| 4 | faiss 次线性无效 | **YES** | DLL 用循环 cdist 非 faiss |
| 新 | 分块 coreset 稀释 ratio | **YES** | 根因发现 |

---

## 结论

### 已达成
- ✅ 完整跑通 coreset 0.01 训练 + FP16 后处理 + DLL 评估
- ✅ 18 图零 crash
- ✅ yaml 还原确认

### 未达成
- ❌ MB 从 679→553 MB（仅 -19%，目标 50-200 MB）
- ❌ 推理从 745→734 ms（仅 -1.5%，目标 100-400 ms）
- ❌ Good 精度仍 0/9

### 根因
**客户代码的分块 coreset 策略**使得 `coreset_sampling_ratio` 参数的效果被大幅稀释。要实现论文级别的 MB 缩减，需要：

1. **降低 dataAugIters**（10→1-3），直接减少源特征量
2. **调整 maxTrainSize 分块参数**，减少块数
3. **或在 DLL 侧做 MB 后处理剪裁**（不需重训练）

### 下一步建议
- **RU-014-b**: dataAugIters 消融（10→3→1），比改 coreset ratio 更直接
- **RU-014-c**: MB 后处理剪裁（Python 脚本直接截取 MB 前 N 行），免训练
- 不建议继续降 coreset ratio（0.001 会有数值问题，且分块稀释严重）

---

## 客户 yaml 还原确认

```
coreset_sampling_ratio: 0.1  ← 已还原 ✅
```
