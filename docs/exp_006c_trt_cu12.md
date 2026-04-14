# RU-008 Step 2c: cu12 TRT libs 重试

## 日期
2026-04-14

## 环境改动
- **卸载 (成功)**:
  - tensorrt 10.16.1.11
  - tensorrt_cu13 10.16.1.11
  - tensorrt_cu13_libs 10.16.1.11
  - tensorrt_cu13_bindings 10.16.1.11
- **安装尝试**: `tensorrt-cu12==10.16.1.11 tensorrt-cu12-libs==10.16.1.11 tensorrt-cu12-bindings==10.16.1.11`
- **安装结果**: ❌ 超时失败

## 结果
- [ ] import 成功 ❌ (安装未完成)
- [ ] FP32 compile 成功
- [ ] FP32 forward 成功
- [ ] FP16 compile 成功
- [ ] FP16 forward 成功

## 失败原因

### 安装卡住
`tensorrt-cu12-libs` 是 placeholder 包，metadata 构建阶段需要从 `pypi.nvidia.com` 下载 ~1.9GB 的真实 wheel。
等待 ~15 分钟无进展，符合任务文档的停止条件（"还不行就停下报告"）。

### pip 安装 log
```
Looking in indexes: https://pypi.tuna.tsinghua.edu.cn/simple
Collecting tensorrt-cu12==10.16.1.11
  Downloading tensorrt_cu12-10.16.1.11.tar.gz (17 kB)
  Installing build dependencies: finished with status 'done'
  Getting requirements to build wheel: finished with status 'done'
  Preparing metadata (pyproject.toml): finished with status 'done'
Collecting tensorrt-cu12-libs==10.16.1.11
  Downloading tensorrt_cu12_libs-10.16.1.11.tar.gz (15 kB)
  Installing build dependencies: finished with status 'done'
  Getting requirements to build wheel: finished with status 'done'
  Preparing metadata (pyproject.toml): started
  <--- STUCK HERE ~15 min, killed --->
```

### 根本障碍
NVIDIA pip 包结构：
- `tensorrt_cu12_libs-10.16.1.11.tar.gz` 本身只有 ~15KB（placeholder）
- 实际 DLL 文件需在 metadata 阶段从 `https://pypi.nvidia.com/` 拉取 ~1.9GB
- 系统网络无法访问 `pypi.nvidia.com`，导致永远卡住

## 结论
- **转换**: ❌ Step 2c 无法完成（网络障碍）
- **cu12 libs 安装路径被网络封锁**

## 下一步建议（走 Plan C: ONNX 路径）

ONNX 路径不依赖 TRT libs 的 placeholder 机制：
1. `torch.onnx.export(model, ...)` → 导出 `.onnx`
2. `onnxruntime-gpu` (CUDA 12.x 运行时，清华源可以下载完整 whl)
3. `ort.InferenceSession(..., providers=['CUDAExecutionProvider'])`

此路径完全绕开 `pypi.nvidia.com`，只需清华源即可安装。

## Artifacts
- 报告: D:\RevUnsup\docs\exp_006c_trt_cu12.md
