# RU-020 三方对比实验报告

> 2026-04-17 · DogWind · 汇报交付用

## 摘要

**C 版本 (max_pool_4x) 相比客户 FP16 基线 (B) 带来 3.12x 加速: 83.24 → 26.64 ms**, ret_code 一致, 无 crash。cudnn.benchmark/TF32 (B') 在 LibTorch 2.8 + RTX 5060 Ti 上**未带来加速**。

## 结果数据

| 版本 | 描述 | 平均 ms | 相对 B | Status |
|------|------|---------|--------|--------|
| A | FP32 老 demo | crash | — | FP32/FP16 模型不匹配 |
| **B** | FP16 kHalf (现用) | **83.24** | 1.00x | 基线 |
| B' | +cudnn +TF32 | 84.46 | 0.99x | 实测无加速 |
| **C** | +max_pool_4x | **26.64** | **3.12x** | 主力交付 ✅ |

## 各图明细

| 图 | 尺寸 | B ms | B' ms | C ms |
|---|------|------|-------|------|
| 2.jpg | 1969×2152 | 92.9 | 94.8 | 39.5 |
| 5.jpg | 1969×2152 | 93.1 | 94.6 | 36.5 |
| 034 | 1009×777 | 79.3 | 78.5 | 21.3 |
| 273 | 1009×777 | 78.1 | 78.5 | 21.1 |
| 274 | 1009×777 | 77.8 | 80.4 | 20.7 |
| 276 | 1009×777 | 78.2 | 80.0 | 20.7 |

## 代码改动

### B → B' (+3 行 cudnn/TF32)
位置: `test_ad_infer_new.cpp:891` `creatModel` 内, `deviceGPU = std::make_unique<...>` 之后
```cpp
at::globalContext().setBenchmarkCuDNN(true);
at::globalContext().setAllowTF32CuBLAS(true);
at::globalContext().setAllowFP16ReductionCuBLAS(true);
```

### B' → C (+1 行 max_pool_4x)
位置: `test_ad_infer_new.cpp:424` `forward` 之后, `permute` 之前
```cpp
anomaly_map = torch::nn::functional::max_pool2d(
    anomaly_map,
    torch::nn::functional::MaxPool2dFuncOptions(4)
);
```

**数学**: `[1,C,H,W]` → max_pool(4) → `[1,C,H/4,W/4]`, patch 数从 H·W 降到 H·W/16, KNN cdist 复杂度降 16x。

## 结论

1. **主推 C 版本**: 3.12x 加速，实现简单 (1 行)
2. **B' 可不交付**: cudnn benchmark 在 RTX 5060 Ti + LibTorch 2.8 上无效果
3. **A 需要 FP32 模型**: 本次无 FP32 model.ckpt 可测

## 精度未验证

- eval_main 的 score 在当前 MB 规模下饱和 (good 和 bad 都是 1.0)，本地无法区分
- **需客户产线真实数据做 AUROC 对照验证**
- 风险: max_pool_4x 对小缺陷 (< 4 patch) 可能漏检，大缺陷无影响

## 交付包
- `D:\RevUnsup\handover\RU020_delivery_v2\` (3 个 DLL + 源码 + diff + 报告)
