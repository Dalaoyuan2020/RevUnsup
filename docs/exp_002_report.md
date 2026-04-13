# RU-005 实验报告: FP16 Tensor Core + Chunked Matmul KNN 加速

> 日期: 2026-04-13 | 执行人: DogWind | GPU: RTX 5060 Ti (sm_120) | PyTorch 2.11.0+cu128

## 结论

| 指标 | Baseline (RU-004) | RU-005 FP16 matmul | 变化 |
|------|:-:|:-:|:-:|
| **KNN 耗时** | 232.9 ms | **71.4 ms** | **3.26× 加速** ✅ |
| **总推理** | 294.0 ms | **133.6 ms** | **2.20× 加速** |
| 最大精度偏差 | — | **0.10%** | < 5% ✅ |
| ret 一致性 | — | 6/6 ✅ | |
| 目标 ≤40ms | — | ❌ 差 31ms | 需 Phase 2 |

## 改动的 3 行代码

```python
# ① MB 启动时量化为 FP16 (做一次)
mb_fp16 = bank.to('cuda').half()
b_sq = (mb_fp16 * mb_fp16).sum(dim=1)  # 预计算 ||b||²

# ② 推理循环: features cast FP16, 分块 matmul 走 Tensor Core
ff_fp16 = features.half()
dists_sq = a_sq + b_sq - 2.0 * torch.mm(chunk_fp16, mb_fp16.T)  # Tensor Core FP16 GEMM

# ③ 结果 cast 回 FP32 做阈值判断
top_scores = result_buf.float().sqrt()
```

## 6 张图详细数据

| 图片 | BL cdist (ms) | FP16 KNN (ms) | 加速 | 精度偏差 | ret |
|------|:-:|:-:|:-:|:-:|:-:|
| 2.jpg | 233.0 | 71.8 | 3.24× | 0.10% | ✅ |
| 5.jpg | 230.8 | 71.7 | 3.22× | 0.02% | ✅ |
| 034-169-301(1)-temp5.jpg | 231.6 | 71.6 | 3.23× | 0.04% | ✅ |
| 273-263-201(1)-temp5.jpg | 233.0 | 71.2 | 3.27× | 0.01% | ✅ |
| 274-244-202(1)-temp2.jpg | 234.9 | 71.2 | 3.30× | 0.09% | ✅ |
| 276-176-301(1)-temp5.jpg | 234.3 | 71.0 | 3.30× | 0.01% | ✅ |

## 对比 RU-004 chunked matmul (FP32)

| | RU-004 FP32 matmul | RU-005 FP16 matmul | 提升 |
|-|:-:|:-:|:-:|
| KNN | 185.8 ms | **71.4 ms** | **2.60×** |
| 技术 | 分块 matmul FP32 | 分块 matmul FP16 + Tensor Core | +FP16 |

FP16 的 2.6× 提速来自 RTX 5060 Ti Tensor Core 的 FP16 GEMM 加速。

## 为什么 cdist compute_mode 无效

诊断发现 `torch.cdist(compute_mode='use_mm_for_euclid_dist')` 会创建完整 65536×17891 距离矩阵 (4.7GB FP32 / 2.3GB FP16)，导致显存爆炸反而变慢 15×。**手动分块 matmul** 每次只处理 [256, 17891] = 18MB，完美适配 GPU L2 cache。

## 优化细节

- **Hybrid chunk**: P0(17891向量)=256, P1(6586向量)=512
- **Pre-alloc output**: 避免 list.append + torch.cat 开销
- **b_sq 预计算**: 每次推理省 2 次全量 sum 操作
- **显存**: 仅 738MB (baseline cdist 需 6394MB)

## 下一步 (Phase 2 建议)

KNN 已从 232.9ms→71.4ms，剩余瓶颈是 **65536 个查询点** (256×256 feature map)。
→ **S1 缩图** (2048→1024): 查询点降 4×，预估 KNN ≤ 20ms，总 ≤ 80ms
