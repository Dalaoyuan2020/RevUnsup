# RU-020 加速迁移 — 交接文档

> 2026-04-17 · DogWind · 为 15:00 汇报交付

## 一句话结论

**max_pool_4x (C 版本) 带来 3.12x 加速 (83.24 → 26.64 ms), ret_code 与现用 B 版本完全一致, 无 crash。**

## 交付清单

| 文件 | 路径 |
|------|------|
| 完整交付包 | `D:\RevUnsup\handover\RU020_delivery_v2\` |
| 实验报告 | `docs/exp_020_3way_comparison.md` |
| 编译手册 | `handover\RU020_delivery_v2\5_compile_guide\how-to-compile.md` |
| diff 文件 | `handover\RU020_delivery_v2\3_diff\*.diff` |
| C 源码 | `cpp_patch\RU020_C_maxpool\test_ad_infer_new.cpp` |
| B' 源码 | `cpp_patch\RU020_Bp_cudnn\test_ad_infer_new.cpp` |
| A 源码 | `cpp_patch\RU020_A_fp32\test_ad_infer_new.cpp` |

## 三方速度 (V2 六图, 预热 3 + 测 5 取平均)

| 版本 | 平均 ms | 相对 B |
|------|---------|--------|
| A (FP32 demo) | crash — 与 FP16 模型不兼容 | — |
| **B** (FP16 现用) | **83.24** | 1.00x |
| B' (cudnn+TF32) | 84.46 | 0.99x (实测无加速) |
| **C** (max_pool_4x) | **26.64** | **3.12x** ✅ |

## 关键发现

1. **cudnn.benchmark 在 LibTorch 2.8 + RTX 5060 Ti 已默认启用**, B' 3 行加速代码在本场景无效
2. **max_pool_4x 是主力加速手段**: 1 行代码, patch 数降 16 倍, KNN cdist 相应降速
3. **A 版本 crash** 验证了客户之前必须迁移到 FP16 kHalf 的原因
4. **精度未本地验证**: eval_main 的 score 在大 MB 下饱和 (全 1.0), 需客户产线数据做 AUROC

## 建议客户采用路径

1. 先拷贝 `1_dll_C_accelerated\xyc_all_AI.dll` 到产线替换
2. 跑一批真实 good+bad 图, 对比 ret_code 和误检率 vs B 版本
3. 若精度回退 < 3%: 全面部署 C
4. 若回退较大: 回退到 B (稳妥) 或 B' (无效但无副作用)

## 风险

- **max_pool_4x 丢分辨率**: 小缺陷 (< 4 patch) 可能漏检
- **cuDNN benchmark 首次慢**: 如客户产线 warmup < 3 次, 会体验到首次 460ms
- **A 没测到**: 没有 FP32 版本的 model.ckpt; 若客户有需要, 给他 FP32 ckpt 再测

## 本次未完成

- A 版本 bench crash, 只交付 DLL + 源码, 未交付数字
- C 版本精度未在产线数据验证 (本地 V2 6 图 score 全饱和无法区分)

---

Napoleon 可查收 `docs/handover_RU020.md` + `handover/RU020_delivery_v2/README.md`
