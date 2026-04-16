# RU-011 ChipROI02 精度验证报告

> 2026-04-16 · DogWind 执行

---

## 环境

- 数据集: ChipROI02 (39 train + 9 good + 3 real bad + 6 synth bad = 18 test)
- 训练: `train.exe` (PyInstaller 打包, PyTorch ~1.x, **sm_120 不兼容**)
- 推理 DLL: L0-v2 编译版 `xyc_all_AI.dll` (LibTorch 2.8.0+cu129)
- GPU: RTX 5060 Ti 16GB (sm_120 Blackwell)

---

## 训练阶段

### train.exe 用法
```
train.exe --phase train --dataset_path <parent_dir> --category <name> --project_path <output>
```
- 目录结构要求: `{dataset_path}/{category}/train/*.png` (图片直接放 train/, 不含 good/ 子目录)
- 输出位置: `{dataset_path}/{category}/model/` (在数据集目录内, 不在 --project_path)

### 训练结果
| 指标 | 值 |
|------|-----|
| 训练用时 | **28.5 分钟** (CPU 模式, sm_120 不兼容) |
| CPU 占用 | ~1500s / 峰值 4.2GB RAM |
| 输出 model.ckpt | 44.8 MB |
| 输出 model.mb | 42.5 MB |
| 输出 modelConfig.yaml | 757 B |

### 警告
```
NVIDIA GeForce RTX 5060 Ti with CUDA capability sm_120 is not compatible
with the current PyTorch installation. (supports sm_37..sm_86)

random_projection.py:90: RuntimeWarning: divide by zero encountered in scalar divide
```

### 新 vs 旧模型对比
| 文件 | 新 (ChipROI02) | 旧 (test_ad_infer_new) | 说明 |
|------|----------------|----------------------|------|
| model.ckpt | 44.8 MB | 22.5 MB | 新模型 2x 大 |
| model.mb | 42.5 MB | 57.2 MB | 新 memory bank 较小 |
| forwardSize | **256×256** | 1000×1000 | 未指定 --modelInputW/H, 用了默认值 |

---

## 评估阶段

### ❌ 新模型: Crash

新模型 creatModel 成功加载 (返回 0), 但第一次 `anomalyDetMatin` 调用时 crash:
```
Exit code: -1073740791 (0xC0000409 = STATUS_STACK_BUFFER_OVERRUN)
```

**可能原因**:
1. **PyTorch 版本不匹配**: train.exe 打包的 PyTorch (~1.x, sm_86) 生成的 TorchScript 模型与 LibTorch 2.8.0 不完全兼容
2. **forwardSize=256×256**: 小尺寸 patch 可能触发 DLL 内部 patching 逻辑的边界条件
3. **model.ckpt 大小异常**: 44.8MB (2x 旧模型) 可能包含不同的网络结构

### ✅ 旧模型 (管道验证): 成功但无意义

用旧模型 (非 ChipROI02) 跑 18 张 ChipROI02 图, 验证 eval harness 工作正常:

#### Good (9 张, 应 ret=0)
| 图 | Avg ms | ret | score | 正确? |
|----|--------|-----|-------|-------|
| 000_*_crop_001.png | 77.2 | 1 | 1.0 | ❌ |
| 000_*_crop_002.png | 76.6 | 1 | 1.0 | ❌ |
| 000_*_crop_003.png | 79.5 | 1 | 1.0 | ❌ |
| 001_*_crop_001.png | 77.0 | 1 | 1.0 | ❌ |
| 001_*_crop_002.png | 77.0 | 1 | 1.0 | ❌ |
| 001_*_crop_003.png | 77.2 | 1 | 1.0 | ❌ |
| 002_*_crop_001.png | 77.7 | 1 | 1.0 | ❌ |
| 002_*_crop_002.png | 77.1 | 1 | 1.0 | ❌ |
| 002_*_crop_003.png | 77.2 | 1 | 1.0 | ❌ |

#### Real Bad (3 张, 应 ret=1)
| 图 | Avg ms | ret | score | 正确? |
|----|--------|-----|-------|-------|
| 006_*_crop_001.png | 76.9 | 1 | 1.0 | ✅ |
| 006_*_crop_002.png | 78.0 | 1 | 1.0 | ✅ |
| 006_*_crop_003.png | 77.4 | 1 | 1.0 | ✅ |

#### Synthetic Bad (6 张, 应 ret=1)
| 图 | Avg ms | ret | score | 正确? |
|----|--------|-----|-------|-------|
| Gemini_*_ctn0kh*.png | 76.7 | 1 | 1.0 | ✅ |
| Gemini_*_g8dwun*.png | 77.4 | 1 | 1.0 | ✅ |
| Gemini_*_ne6g80*.png | 77.0 | 1 | 1.0 | ✅ |
| Gemini_*_qz7ne1*.png | 77.5 | 1 | 1.0 | ✅ |
| Gemini_*_ts9a68*.png | 77.4 | 1 | 1.0 | ✅ |
| Gemini_*_u4vk23*.png | 76.8 | 1 | 1.0 | ✅ |

#### 判对率 (旧模型, 无意义参考)
| 类别 | 正确 | 说明 |
|------|------|------|
| Good | **0/9** | 全错 (旧模型对 ChipROI02 不适用) |
| Real Bad | **3/3** | 碰巧全对 (所有图都 ret=1) |
| Synth Bad | **6/6** | 碰巧全对 |
| **总计** | **9/18** | 等于随机 |

---

## 结论

| 管道 | 状态 | 说明 |
|------|------|------|
| 训练管道 | ✅ 跑通 | train.exe 成功输出三件套, 但用 CPU 模式 (28.5 分钟) |
| 评估管道 | ⚠️ 部分 | eval harness 正常, 但新模型与 DLL 不兼容 crash |
| 精度评估 | ❌ 未完成 | 需要兼容模型才能做有意义的精度评估 |

---

## 🔧 遗留问题 + 建议

### P0: 新模型 crash 解决方案
| 方案 | 可行性 | 风险 |
|------|--------|------|
| **A: 用 Python 推理替代 DLL** | 高 | Python 环境 PyTorch 2.11+cu128 能加载 LibTorch 2.8 的模型但不一定能加载 train.exe 的模型 |
| **B: 用 Python 重新训练** | 高 | 跳过 train.exe, 直接用 anomalib 训练, 生成兼容 LibTorch 2.8 的 TorchScript |
| **C: 重训指定 forwardSize=1000** | 中 | 可能解决边界条件 crash, 但不解决 TorchScript 版本问题 |
| **D: 降级 LibTorch 到 train.exe 对应版本** | 低 | 需要完全重编译 DLL, 工作量大 |

### P1: train.exe sm_120 不兼容
- train.exe 打包的 PyTorch 只支持到 sm_86
- RTX 5060 Ti 是 sm_120, 训练只能 CPU 模式
- 后续训练建议用 Python 环境 (PyTorch 2.11+cu128 支持 sm_120)

### P2: 全图都 ret=1 score=1.0 的根因
- 与 L0-v2 baseline 同样的问题: 阈值 0.1 (内部 pow=0.01) 可能太低
- 旧模型用于不同数据集的图像, 自然全判异常

---

## 📦 产物

- `cpp_patch/RU011_train_output/Model/` — 新训练模型三件套 (本地保留, 不 push)
- `cpp_patch/RU011_eval/eval_main.cpp` — 评估 harness 源码
- `cpp_patch/RU011_eval/test_images/` — 18 张评估图 (本地保留, 不 push)
- `docs/exp_011_chiproi02_eval.md` — 本报告
- `docs/handover_RU011.md` — 交接文档
