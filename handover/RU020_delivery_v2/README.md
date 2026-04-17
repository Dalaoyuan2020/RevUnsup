# RU-020 交付包 v2

> 2026-04-17 · DogWind · PC-20260115QQCJ

## 一句话结论

**max_pool_4x (C 版本) 相比客户当前 FP16 版本 (B) 带来 3.12x 加速 (83.24 → 26.64 ms)，ret_code 一致，无 crash。**

## 内容

```
1_dll_A_fp32/          A 版本: FP32 老 demo (⚠️ 与 FP16 模型不兼容, 记录)
1_dll_B_fp16/          B 版本: 客户现用 FP16 kHalf (L0-v2 基线)
1_dll_Bp_cudnn/        B' 版本: B + cudnn.benchmark + TF32 (实测无加速)
1_dll_C_accelerated/   C 版本: B' + max_pool_4x (主力交付, 3.12x 加速)
2_source/              4 个版本源码
3_diff/                每步改动 diff
4_benchmark/           三方对比 + 原始 output
5_compile_guide/       VS2022 编译步骤
```

## 三方速度对比 (V2 六图)

| 版本 | 平均 ms | vs B |
|------|---------|------|
| A (FP32 demo) | crash (不兼容 FP16 模型) | — |
| **B** (FP16 现用) | **83.24** | 1.00x |
| B' (+cudnn) | 84.46 | 0.99x |
| **C** (+max_pool_4x) | **26.64** | **3.12x** ✅ |

## 推荐使用

1. **主力部署**: C 版本 (速度优先，ret_code 与 B 一致)
2. **精度验证**: 客户产线数据跑 AUROC 对照 C vs B
3. **回退方案**: 若 C 精度回退过大，B' 可作为 no-op 稳妥版本

## 运行时依赖

DLL 需要以下运行时 DLL 在 PATH 或同目录 (从 LibTorch 2.8.0+cu129 copy):
- c10.dll / c10_cuda.dll / torch.dll / torch_cpu.dll / torch_cuda.dll
- cudart64_12.dll / cublas64_12.dll / cublasLt64_12.dll / cudnn*_9.dll
- opencv_world451.dll / zlibwapi.dll / libiomp5md.dll
- 完整列表见 `D:\RevUnsup\cpp_patch\L0_baseline\x64\Release\*.dll`

## 编译

见 `5_compile_guide/how-to-compile.md`

---

Napoleon 可查收 `docs/handover_RU020.md` 和本 README
