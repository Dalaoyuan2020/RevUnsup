# RU-008 Step 2: TensorRT 模型转换验证报告

## 日期
2026-04-14

## 结果概览
- [x] torch_tensorrt.compile FP32 成功 ❌
- [ ] TRT FP32 forward 成功 ❌
- [ ] torch_tensorrt.compile FP16 成功
- [ ] TRT FP16 forward 成功

## 耗时对比 (单次 forward, 1 张 dummy 图)

| 后端 | 耗时 (ms) | 加速 vs PyTorch |
|------|-----------|------------------|
| PyTorch (baseline) | 30.98 | 1.00x |
| TensorRT FP32 | FAILED | - |
| TensorRT FP16 | NOT TESTED | - |

## 编译时间
- FP32 首次 compile: FAILED
- FP16 首次 compile: 未测试

## 警告或错误

### 关键错误
```
RuntimeError: [Error thrown at core/partitioning/shape_analysis.cpp:312]
Unable to process subgraph input type of at::kLong/at::kDouble,
try to compile model with truncate_long_and_double enabled
```

### 错误分析
- model.ckpt 包含 TorchScript 模型，内部有 Long/Double 数据类型
- TensorRT 不原生支持 Long/Double 类型
- 错误提示建议启用 `truncate_long_and_double` 参数

### 其他警告（非阻塞）
```
[TRT] Functionality provided through tensorrt.plugin module is experimental
[Torch-TensorRT] Unable to read CUDA capable devices. Return status: 801
Unable to import quantization op (modelopt library not installed)
triton not found (optional dependency)
```

## 脚本输出完整 log
```
[04/14/2026-23:06:37] [TRT] [W] Functionality provided through tensorrt.plugin module is experimental.
WARNING: [Torch-TensorRT] - Unable to read CUDA capable devices. Return status: 801
Unable to import quantization op. Please install modelopt library (https://github.com/NVIDIA/TensorRT-Model-Optimizer?tab=readme-ov-file#installation) to add support for compiling quantized models
Unable to import quantize op. Please install modelopt library (https://github.com/NVIDIA/TensorRT-Model-Optimizer?tab=readme-ov-file#installation) to add support for compiling quantized models
W0414 23:06:38.566000 64712 Softwares\Anaconda\envs\anomalib_trt\Lib\site-packages\torch\utils\flop_counter.py:29] triton not found; flop counting will not work for triton kernels
Traceback (most recent call last):
  File "D:\trt_test.py", line 46, in <module>
    trt_model_fp32 = torch_tensorrt.compile(
                     ^^^^^^^^^^^^^^^^^^^^^^^
  File "D:\Softwares\Anaconda\envs\anomalib_trt\Lib\site-packages\torch_tensorrt\_compile.py, line 245, in compile
    compiled_ts_module: torch.jit.ScriptModule = torchscript_compile(
                                                 ^^^^^^^^^^^^^^^^^^^^
  File "D:\Softwares\Anaconda\envs\anomalib_trt\Lib\site-packages\torch_tensorrt\ts\_compiler.py, line 158, in compile
    compiled_cpp_mod = _C.compile_graph(module._c, _parse_compile_spec(spec))
                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
RuntimeError: [Error thrown at core/partitioning/shape_analysis.cpp:312] Unable to process subgraph input type of at::kLong/at::kDouble, try to compile model with truncate_long_and_double enabled

Loading model from D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt...
Model loaded.
Creating dummy input shape=(1, 3, 2048, 2048)...

=== PyTorch baseline ===
PyTorch avg: 30.98 ms  (range 29.18-31.80)

=== TensorRT FP32 compile ===
This may take 2-10 minutes on first run. Please wait...
FP32 compile FAILED: [Error thrown at core/partitioning/shape_analysis.cpp:312] Unable to process subgraph input type of at::kLong/at::kDouble, try to compile model with truncate_long_and_double enabled


==================================================
SUMMARY
==================================================
PyTorch:         30.98 ms
TensorRT FP32:   FAILED
TensorRT FP16:   NOT TESTED OR FAILED

DONE
```

## 结论
- **转换路径**: ❌ 当前不可行
- **失败原因**: model.ckpt 包含 Long/Double 类型，TensorRT 不原生支持
- **建议解决方案**:
  1. 尝试启用 `truncate_long_and_double=True` 参数
  2. 或改用 ONNX 中间格式导出（可能解决类型兼容性问题）
  3. 或检查 model.ckpt 的导出方式，避免 Long/Double 类型

## 下一步建议
**需要羊爸爸决策**:
- 选项 A: 修改 D:\trt_test.py，添加 `truncate_long_and_double=True` 参数重试
- 选项 B: 改用 ONNX 导出路径 → TensorRT
- 选项 C: 放弃 TensorRT，回退到 Phase 3B (GPU 预处理)

## Artifacts
- 测试脚本: D:\trt_test.py
- 报告: D:\RevUnsup\docs\exp_006_trt_conversion.md
