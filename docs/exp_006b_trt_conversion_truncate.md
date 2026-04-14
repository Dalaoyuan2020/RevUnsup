# RU-008 Step 2b: TensorRT 转换重试 (加 truncate)

## 日期
2026-04-14

## 改动
在 D:\trt_test.py 的 2 个 torch_tensorrt.compile() 里各加一行:
`truncate_long_and_double=True`

## 结果
- [ ] FP32 compile 成功 ❌
- [ ] FP32 forward 成功 ❌
- [ ] FP16 compile 成功
- [ ] FP16 forward 成功

## 耗时对比
| 后端 | 耗时 (ms) | 加速 vs PyTorch |
|------|-----------|------------------|
| PyTorch | 30.98 (昨天测) | 1.00x |
| TRT FP32 | FAILED | - |
| TRT FP16 | NOT TESTED | - |

## 编译时间
- FP32 首次 compile: FAILED (秒级崩溃)
- FP16 首次 compile: 未到达

## 警告与错误

### 进度
`truncate_long_and_double=True` **通过了**类型检查：
```
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
```

### 新的根本错误
```
WARNING: [Torch-TensorRT] - Unable to read CUDA capable devices. Return status: 801
ERROR: [Torch-TensorRT TorchScript Conversion Context] - Unable to determine GPU memory usage
ERROR: [Torch-TensorRT TorchScript Conversion Context] - createInferBuilder: Error Code 6: API Usage Error
  (CUDA initialization failure with error: 35.
  In `anonymous-namespace'::ensureCudaInitialized::<lambda_1>::operator () at builder.cpp:1406)
ERROR: [Torch-TensorRT TorchScript Conversion Context] - Error Code 1: Cuda Runtime
  (In nvinfer1::catchCudaError at checkMacros.cpp:229)
```

## 根因分析

| 错误 | 含义 |
|------|------|
| `CUDA error 35` | `CUDA_ERROR_SYSTEM_DRIVER_MISMATCH` |
| `Unable to read CUDA capable devices` | TRT 无法枚举 GPU |

**根本问题**: TensorRT 安装的是 `cu13` (CUDA 13.x) 版本的 libs (`tensorrt_cu13_libs`)，但系统驱动是 CUDA 12.9。TRT 内部的 CUDA context 初始化与驱动版本不匹配，导致 error 35。

注意：`torch.cuda` 正常（用 cu128），因为 PyTorch 走的是自带 CUDA 12.8 运行时；但 TensorRT libs 自己尝试初始化 CUDA 时失败了。

## 完整 log
```
[04/14/2026-23:28:35] [TRT] [W] Functionality provided through tensorrt.plugin module is experimental.
WARNING: [Torch-TensorRT] - Unable to read CUDA capable devices. Return status: 801
Unable to import quantization op. (modelopt not installed)
Unable to import quantize op. (modelopt not installed)
triton not found (optional)
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT TorchScript Conversion Context] - Unable to determine GPU memory usage
ERROR: [Torch-TensorRT TorchScript Conversion Context] - createInferBuilder: Error Code 6: API Usage Error
  (CUDA initialization failure with error: 35.)
ERROR: [Torch-TensorRT TorchScript Conversion Context] - Error Code 1: Cuda Runtime
```

## 结论
- **转换**: ❌ 不可行（当前 env 配置）
- **原因**: `tensorrt_cu13_libs` 与系统 CUDA 12.9 驱动不匹配，TRT 内部 CUDA 初始化失败

## 下一步建议 (需羊爸爸决策)

| 选项 | 描述 | 风险 |
|------|------|------|
| **A**: 改装 cu12 版 TRT libs | 用 `tensorrt-cu12-libs` 直接安装 .whl | 需要能访问 pypi.nvidia.com |
| **B**: 手动下载 TRT Windows 安装包 | 从 NVIDIA 官网下载 TRT 10.x for CUDA 12 | 需要 NVIDIA 账号 |
| **C**: 走 ONNX 路径 | torch → ONNX → TensorRT (onnxruntime-gpu) | 绕过 torch_tensorrt 的 CUDA 冲突 |
| **D**: 放弃 TensorRT | 回退到 Phase 3B (GPU 预处理) | 不能利用 TRT Tensor Core |

**我的推荐**: 选项 C (ONNX)，因为：
- onnxruntime-gpu 用 CUDA 12.x 运行时，与系统兼容
- 不依赖 tensorrt_cu13_libs
- 可以用 TensorRT execution provider

## Artifacts
- 测试脚本: D:\trt_test.py (已加 truncate_long_and_double=True)
- 报告: D:\RevUnsup\docs\exp_006b_trt_conversion_truncate.md
