# RU-014 Coreset 1% 消融 — 交接文档

> **日期**: 2026-04-17  
> **执行者**: DogWind  
> **状态**: 完成，结论：coreset ratio 改动在客户 pipeline 中效果有限  

---

## 一句话结论

**coreset_sampling_ratio 0.1→0.01 仅让 MB 从 679→553 MB（-19%），推理从 745→734 ms（-1.5%）。客户代码的分块 coreset 策略稀释了 ratio 参数的效果。**

---

## 环境

| 项 | 值 |
|---|---|
| conda env | `custom_train` (Python 3.8) |
| PyTorch | 2.0.1+cu118 (CPU fallback) |
| 训练机 | PC-20260115QQCJ / RTX 5060 Ti |
| 训练时间 | 92.5 min (CPU mode) |

---

## 文件位置

| 文件 | 路径 |
|------|------|
| 训练输出 | `D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\` |
| 评估工作区 | `D:\RevUnsup\cpp_patch\RU014_eval\` |
| 评估结果 | `D:\RevUnsup\cpp_patch\RU014_eval\output_coreset_001.txt` |
| 实验报告 | `D:\RevUnsup\docs\exp_014_coreset_ablation.md` |
| Smoke 测试 | `D:\RevUnsup\cpp_patch\RU014_smoke\` |

---

## 关键数据

| 指标 | RU-011-v2 (ratio=0.1) | RU-014 (ratio=0.01) | 变化 |
|------|----------------------|---------------------|------|
| model.mb | 679 MB | 553 MB | -19% |
| 推理 ms | 745 | 734 | -1.5% |
| Good correct | 0/9 | 0/9 | 不变 |
| Bad correct | 9/9 | 9/9 | 不变 |
| 训练时间 | ~30 min | 92.5 min | +3x |

---

## 为什么效果不如预期

论文 (Roth 2022) 的 coreset 是对全部特征一次性采样 10%→1% = 10x 缩减。

客户代码的实现是：
1. 特征按 `maxTrainSize` 分块
2. 每块独立做 coreset 采样
3. 所有块的结果拼接

分块后每块样本少，coreset 有最小采样数 → **实际缩减比远低于 ratio 倒数**。

---

## 客户 yaml 状态

```
coreset_sampling_ratio: 0.1  ← 已还原 ✅
```

---

## 下一步建议

1. **RU-014-b: dataAugIters 消融** — 10→3→1，直接减少源特征量，比改 coreset ratio 更有效
2. **RU-014-c: MB 后处理剪裁** — Python 脚本截取 MB 前 N 行，免训练，快速验证 MB 大小对推理速度的影响
3. **不建议继续降 coreset ratio** — 分块稀释严重，0.001 会有数值问题

---

Napoleon 可查收 `docs/exp_014_coreset_ablation.md`
