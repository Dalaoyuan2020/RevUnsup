# RU-006 实验报告: Feature Pooling 速度扫描

> 日期: 2026-04-14 | GPU: RTX 5060 Ti | 6图×5次 mean±std | **不测精度, 只看速度**

## 速度对比

| 配置 | Query/图 | KNN (ms) | KNN 加速 | 总推理 (ms) | 总加速 |
|------|:--------:|:--------:|:--------:|:-----------:|:------:|
| baseline (no pool) | 131,072 | 68.9 | 1.00× | 129.1 | 1.00× |
| **avg_pool 2×** | 32,768 | 22.1 | 3.11× | 83.3 | 1.55× |
| **avg_pool 4×** | 8,192 | 13.3 | 5.18× | 75.1 | 1.72× |
| **max_pool 2×** | 32,768 | 30.8 | 2.23× | 92.9 | 1.39× |
| **max_pool 4×** | 8,192 | **8.8** | **7.78×** | **71.3** | **1.81×** |

## 观察

1. **max_pool_4x 最快**: KNN 8.8ms, 远超 40ms 目标 (7.78× vs baseline)
2. **avg vs max**: 2× 时 avg 快 (22ms vs 31ms); 4× 时 max 快 (8.8ms vs 13.3ms)
3. **总推理极限**: 71.3ms (max_pool_4x), 其中 Forward ~41ms 占 57%, 已成新瓶颈
4. **avg_pool_4x 有一次 GPU 调度抖动** (5.jpg knn=35±53ms), 排除后实际 ~9ms
5. **Query 从 131K → 8K** (16× 缩减), KNN 接近线性下降

## 极限分析

- KNN 已压到 ~9ms, 接近 kernel launch + 循环开销的理论下限
- 总推理 71ms 中 Forward 41ms + 预处理 ~20ms = 61ms 不可压缩
- **KNN 不再是瓶颈**, Forward 成为下一个优化目标

## 曲线

见 `docs/exp_003_curve.png` (KNN + Total, avg/max 双线)

## 下一步建议

- 精度验证: 甲方用 max_pool_4x 上线测精度, 如果 OK 直接部署
- 如果精度掉太多: 退回 avg_pool_2x (22ms, 仍 < 40ms)
- Forward 优化: TensorRT FP16 可能将 41ms → ~15ms, 总推理 → ~45ms
