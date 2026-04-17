# RU-020 三方对比报告

> 2026-04-17 · 机器 PC-20260115QQCJ · RTX 5060 Ti 16GB (sm_120) · LibTorch 2.8.0+cu129

## 测试条件
- 硬件: RTX 5060 Ti 16GB (sm_120)
- LibTorch 2.8.0+cu129 / CUDA 12.9 / VS2022 Release x64
- 模型: 客户原 model.ckpt + model.mb (57MB, FP16)
- 测试图: V2 六图 (2.jpg, 5.jpg, 034, 273, 274, 276) — 2 张 1969×2152 + 4 张 1009×777
- 每次测试: 预热 3 次 + 测 5 次取平均

## 三方推理耗时

| 版本 | 描述 | 平均 ms | 相对 B 加速 | 状态 |
|------|------|---------|-------------|------|
| **A** | 客户未加速 FP32 (decrypted_infer_demo.cpp) | **N/A (crash)** | — | ❌ 与 FP16 模型不兼容 |
| **B** | 客户自加速 FP16 kHalf (L0-v2) | **83.24** | 1.00x (基准) | ✅ |
| **B'** | B + cudnn.benchmark + TF32 + FP16reduction | **84.46** | 0.99x (-1.5%) | ✅ 无效 |
| **C** | B' + max_pool_4x (forward 后降维) | **26.64** | **3.12x** | ✅ |

### 各图明细 (ms)

| 图 | 尺寸 | B | B' | C |
|---|------|---|----|---|
| 2.jpg | 1969×2152 | 92.9 | 94.8 | 39.5 |
| 5.jpg | 1969×2152 | 93.1 | 94.6 | 36.5 |
| 034-temp5 | 1009×777 | 79.3 | 78.5 | 21.3 |
| 273-temp5 | 1009×777 | 78.1 | 78.5 | 21.1 |
| 274-temp2 | 1009×777 | 77.8 | 80.4 | 20.7 |
| 276-temp5 | 1009×777 | 78.2 | 80.0 | 20.7 |
| **Overall** | — | **83.24** | **84.46** | **26.64** |

## 结论与分析

### A: Crash — FP32 demo 不能加载 FP16 模型
老的 `decrypted_infer_demo.cpp` 对 `tensor_image` 送入 FP32，但客户的 model.ckpt 已是 FP16 (kHalf 权重)，forward 时类型不匹配导致 warmup 首次推理 crash (0xC0000409 或 silent exit)。
这正是客户自己迁移到 FP16 版本的原因。要比较 A，需要一个 FP32 版本的 model.ckpt，本次无此资源。

### B→B': cudnn.benchmark 无效 (0.99x)
- cudnn.benchmark 通过重编译首次 forward 选最快 kernel,但 RTX 5060 Ti + LibTorch 2.8 的 cuDNN 默认策略已经选到最优
- TF32/FP16 reduction 对 KNN 主导的推理无贡献 (KNN 用 cdist 循环)
- **建议: 生产环境可移除,无负面影响**

### B→C: max_pool_4x 带来 3.12x 加速 ✅✅
- `anomaly_map` shape [1, C, H, W] → max_pool(kernel=4) → [1, C, H/4, W/4]
- patch 数从 H·W 降到 H·W/16 → reshape 后 KNN 查询向量 N₁ 降为 1/16
- `cdist(N₁×C, N₂×C)` 计算量直接降 16 倍
- 实测: **83.24 → 26.64 ms**, 节省 68% 推理时间
- ret_code = 1 与 B 一致, anomaly_score = 1.0 与 B 一致 (饱和), 未引入 crash

## 精度说明

**本实验仅验证推理速度和 ret_code 一致性**。由于 eval_main 的 `score = countNonZero(mask) / (H·W)` 在当前 MB 规模下饱和 (全 1.0), 本地无法区分 good/bad。**精度需客户在产线真实数据上做 AUROC 对照测试**。

Python 侧 RU-005/006/007 实验证明 max_pool_4x 在独立 Python 环境精度损失 <5%, 但 C++ DLL 集成后建议独立验证。

## 风险提示 ⚠️

max_pool_4x 的数学正确性:
- anomaly_map patches 空间降采样 4x → patch 定位分辨率降低 4x
- 对 **小缺陷 (< 4 patch)** 可能漏检
- 对大缺陷 (片状缺陷) 检出应无影响

建议产线部署时:
1. 先用 C 版本跑一批真实图，对比 B 版本 ret_code 差异
2. 若 good/bad AUROC 回退 < 3%，可全面切换到 C
3. 若回退较大，可退回 B' (无显著加速但代码稳)

## 建议使用路径

1. **追求速度**: 用 C 版本 (26.64 ms, 3.12x 加速)
2. **稳定优先**: 继续用 B 版本 (83.24 ms, 已验证)
3. **不推荐**: B' 版本 (无加速，多了 3 行代码没必要)
