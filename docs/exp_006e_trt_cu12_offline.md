# RU-008 Step 2e: cu12 离线装 + TRT 转换

## 日期
2026-04-15

## 环境
### 装包命令
```
# Step 1: libs + bindings (--no-deps --no-index)
pip install --no-deps --no-index tensorrt_cu12_libs-10.16.1.11-py3-none-win_amd64.whl tensorrt_cu12_bindings-10.16.1.11-cp312-none-win_amd64.whl
# Successfully installed tensorrt-cu12-bindings-10.16.1.11 tensorrt-cu12-libs-10.16.1.11

# Step 2: 占位符包 tar.gz (--no-deps --no-build-isolation, 用 env 里已有的 setuptools)
pip install --no-deps --no-build-isolation tensorrt_cu12-10.16.1.11.tar.gz
# Successfully installed tensorrt_cu12-10.16.1.11
```

### pip list | findstr tensorrt (安装后)
```
tensorrt_cu12          10.16.1.11
tensorrt_cu12_bindings 10.16.1.11
tensorrt_cu12_libs     10.16.1.11
torch_tensorrt         2.11.0
```

### import 验证
```
TRT: 10.16.1.11
T-TRT: 2.11.0
CUDA: True
Device: NVIDIA GeForce RTX 5060 Ti
```

## 结果
- [x] 离线装成功 ✅
- [x] import tensorrt 成功 ✅
- [x] import torch_tensorrt 成功 ✅
- [ ] FP32 compile 成功 ❌
- [ ] FP32 forward 成功
- [ ] FP16 compile 成功
- [ ] FP16 forward 成功

## 耗时
| 后端 | ms | 加速 |
|------|----|------|
| PyTorch | 29.44 | 1.00x |
| TRT FP32 | FAILED | - |
| TRT FP16 | NOT TESTED | - |

## 编译时间
- FP32: 秒级崩溃（未完成）
- FP16: 未到达

## 进展与新错误

### 进展（相比 Step 2b）
cu12 libs 换装后，类型截断全部通过：
```
WARNING: Truncating intermediate graph input type from at::kLong to at::kInt (x3)
WARNING: Unable to process input type of at::kLong, truncate type to at::kInt in scalar_to_tensor_util (x2)
WARNING: Truncating weight (constant in the graph) from Int64 to Int32 (x3)
```

### 新错误
```
RuntimeError: [Error thrown at core/runtime/runtime.cpp:107]
Expected (cudaGetDevice(reinterpret_cast<int*>(&device)) == cudaSuccess) to be true but got false
Unable to get current device (runtime.get_current_device)
```

### 错误分析
| 对比 | Step 2b (cu13) | Step 2e (cu12) |
|------|----------------|----------------|
| 类型截断 | ❌ 卡在 Long/Double | ✅ 通过 |
| CUDA 初始化 | Error 35 (driver mismatch) | `cudaGetDevice` 失败 |
| TRT 能建立 builder | ❌ | ❌ |

两个版本（cu12/cu13）都在不同阶段失败：
- cu13: TRT builder 构造时失败（error 35: driver mismatch）
- cu12: 更晚一步，进入 runtime，但 `cudaGetDevice` 返回 false

**根本矛盾**：torch_tensorrt 2.11.0 / TRT 10.16.1.11 的 CUDA context 与 PyTorch 的 CUDA context 相互隔离。PyTorch cu128 用自带的 CUDA 12.8 runtime；TRT libs 尝试独立初始化另一套 CUDA context，但失败。

这是 **torch_tensorrt `ir='torchscript'` 路径** 的根本限制。

## 完整 log
```
Loading model from D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt...
Model loaded.
Creating dummy input shape=(1, 3, 2048, 2048)...

=== PyTorch baseline ===
PyTorch avg: 29.44 ms  (range 28.28-30.54)

=== TensorRT FP32 compile ===
This may take 2-10 minutes on first run. Please wait...

WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Truncating intermediate graph input type from at::kLong to at::kInt
WARNING: [Torch-TensorRT] - Unable to process input type of at::kLong, truncate type to at::kInt in scalar_to_tensor_util
WARNING: [Torch-TensorRT] - Unable to process input type of at::kLong, truncate type to at::kInt in scalar_to_tensor_util
WARNING: [Torch-TensorRT] - Truncating weight (constant in the graph) from Int64 to Int32
WARNING: [Torch-TensorRT] - Truncating weight (constant in the graph) from Int64 to Int32
WARNING: [Torch-TensorRT] - Truncating weight (constant in the graph) from Int64 to Int32

FP32 compile FAILED: [Error thrown at core/runtime/runtime.cpp:107]
Expected (cudaGetDevice(reinterpret_cast<int*>(&device)) == cudaSuccess) to be true but got false
Unable to get current device (runtime.get_current_device)

==================================================
SUMMARY
==================================================
PyTorch:         29.44 ms
TensorRT FP32:   FAILED
TensorRT FP16:   NOT TESTED OR FAILED

DONE
```

## 结论
- **转换**: ❌ torch_tensorrt `ir='torchscript'` 路径失败
- **根本原因**: TRT 内部 CUDA runtime 与 PyTorch cu128 CUDA context 相互隔离，`cudaGetDevice` 无法在 TRT 侧获取当前 GPU

## 下一步建议（报告羊爸爸，等决策）

| 选项 | 描述 |
|------|------|
| **Plan C** | ONNX 导出 → `onnxruntime-gpu` TensorRT EP（绕开 torch_tensorrt 完全） |
| **Plan D** | 放弃 TRT，转 Phase 3B GPU 预处理 |
| **Plan E** | 调查 `ir='dynamo'` 路径（torch_tensorrt 新 API，不走 torchscript） |

## Artifacts
- 测试脚本: D:\trt_test.py
- 报告: D:\RevUnsup\docs\exp_006e_trt_cu12_offline.md
