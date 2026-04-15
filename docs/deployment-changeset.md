# RevUnsup 部署改动清单

> 版本：v1.0 | 日期：2026-04-15 | 作者：DogWind
> 用途：**打包前核对 / 实机部署最小改动说明**
> 原则：封装版推理 DLL (`anomalyDet.dll`) **不动**，所有优化在 Python 推理层 (`anomalib2.2` env) 实施。

---

## 目录

1. [改动概览（一张表）](#1-改动概览)
2. [详细改动说明](#2-详细改动说明)
   - 2.1 [KNN 加速：FP16 Chunked Matmul（RU-005）](#21-knn-加速fp16-chunked-matmulru-005)
   - 2.2 [Feature Pooling：max_pool_4x（RU-006）](#22-feature-poolingmax_pool_4xru-006)
   - 2.3 [零成本 Flag（RU-007）](#23-零成本-flagru-007)
3. [对原封装版的改动（结论：无）](#3-对原封装版的改动结论无)
4. [部署所需文件清单（打包索引）](#4-部署所需文件清单打包索引)
5. [环境差异说明](#5-环境差异说明)
6. [实机测试步骤](#6-实机测试步骤)
7. [已放弃/未完成的方向](#7-已放弃未完成的方向)

---

## 1. 改动概览

| # | 改动内容 | 所在文件/位置 | 改动类型 | 性能收益 | 风险 |
|---|----------|--------------|---------|---------|------|
| A | FP16 Chunked Matmul KNN | `ru007_experiment.py` `knn_fp16_prealloc()` | 新增函数 | KNN 367ms→4.9ms（74x） | 低（FP16 精度损失可忽略） |
| B | max_pool_4x 特征池化 | `ru007_experiment.py` Forward 后一行 | 新增一行 `F.max_pool2d(features, kernel_size=4)` | 总推理 480ms→71.3ms | 中（轻微定位精度损失，实测可接受） |
| C | `cudnn.benchmark = True` | `ru007_experiment.py` 第14行 | 新增一行（文件顶部） | ~1ms 额外优化 | 极低 |
| D | `channels_last` 内存格式 | `ru007_experiment.py` 模型加载 + patch 处理 | 新增2行 | 对 ResNet18 无明显效果 | 极低 |
| E | `torch.compile` | `ru007_experiment.py` `load_all()` 函数 | try-except 包裹（失败自动跳过） | 实测无显著收益 | 极低（失败会 fallback） |

**净效果**：总推理时间 **480ms → 65.5ms（7.3x）**，主要来自改动 A + B。

---

## 2. 详细改动说明

### 2.1 KNN 加速：FP16 Chunked Matmul（RU-005）

**原逻辑**（封装版 DLL 内部 C++）：
```cpp
// demo.cpp (infer 版) nearest_neighbors()
// 分块迭代 + torch::cdist → 最近邻搜索
// 耗时: 367ms/图
```

**新逻辑**（Python，`ru007_experiment.py` 第35-44行）：
```python
def knn_fp16_prealloc(feat_fp16, mb_fp16, b_sq, result_buf, chunk=256):
    N = feat_fp16.shape[0]
    for i in range(0, N, chunk):
        end = min(i + chunk, N)
        c = feat_fp16[i:end]
        a_sq = (c * c).sum(dim=1, keepdim=True)
        d = a_sq + b_sq.unsqueeze(0) - 2.0 * torch.mm(c, mb_fp16.T)
        d.clamp_(min=0)
        result_buf[i:end], _ = d.min(dim=1, keepdim=True)
```

**关键变化**：
- `torch.cdist` (float32) → 手写 matmul 展开 (float16)，利用 RTX 5060 Ti Tensor Core
- Memory Bank 预转 FP16 并驻留 GPU（`load_all()` 中预处理）
- `b_sq`（Memory Bank 各行平方和）预计算一次，推理时复用
- 分块大小 `chunk=256`（大 MB）或 `chunk=512`（小 MB），防 OOM

**部署时需注意**：
- Memory Bank (`model.mb`) 加载后需在 `load_all()` 中转 FP16：
  ```python
  mb_fp16_list.append(emb.to(device, dtype=torch.float16))
  b_sq_list.append((emb_fp16 * emb_fp16).sum(dim=1))
  ```

---

### 2.2 Feature Pooling：max_pool_4x（RU-006）

**原逻辑**（封装版）：
- backbone 输出 `features` 形状：`[1, C, H/8, W/8]`，如 `[1, 512, 256, 256]` = 65,536 个特征点
- 全部 65,536 个点都送入 KNN 搜索

**新逻辑**（`ru007_experiment.py` 第137行）：
```python
features = F.max_pool2d(features, kernel_size=4)
# 65,536 → 4,096 个特征点（减少 16x）
```

**插入位置**：`features = model(patch)` 紧接其后，KNN 之前

**关键参数**：
- `kernel_size=4`（冠军配置，RU-006 扫描 6 种配置后选定）
- `kernel_size=2`：精度更好但速度较慢
- `kernel_size=8`：更快但定位精度下降明显，不推荐

**精度影响**：
- 热力图空间分辨率从 256×256 降为 64×64（上采样回原图时平滑）
- 实测 6 张图异常判定结论（0/1）不变，max_score 变化 < 5%

---

### 2.3 零成本 Flag（RU-007）

三个 flag，**一行代码**，对模型文件不改动：

#### Flag 1: `cudnn.benchmark = True`
```python
# ru007_experiment.py 第14行（import 之后，main 之前）
torch.backends.cudnn.benchmark = True
```
- cuDNN 自动选择最优卷积算法（第一次 forward 略慢，之后最优）
- 仅在输入尺寸固定时有效（本项目 patch 固定 2048×2048 ✅）

#### Flag 2: `channels_last` 内存格式
```python
# load_all() 内，model.eval() 之后
model = model.to(memory_format=torch.channels_last)

# run_image() 内，patch 归一化之后
patch = patch.to(memory_format=torch.channels_last)
```
- NHWC 格式，ResNet 等 CNN 在 Ampere/Blackwell 架构上略有提升
- 实测对本模型效果不显著（Forward 无明显变化）

#### Flag 3: `torch.compile`
```python
# load_all() 内，try-except 包裹（失败自动 fallback）
try:
    model = torch.compile(model, mode='reduce-overhead')
except Exception as e:
    print(f"torch.compile FAILED: {e} -> continuing without")
```
- 实测无显著收益（TorchScript 模型 compile 效果有限）
- **失败时自动跳过，不影响推理**

---

## 3. 对原封装版的改动（结论：无）

| 文件 | 改动 |
|------|------|
| `anomalyDet.dll` | ❌ 未改动，不需要重新编译 |
| `model.ckpt` | ❌ 未改动，原始 TorchScript 模型 |
| `model.mb` | ❌ 未改动，原始 Memory Bank |
| `modelConfig.yaml` | ❌ 未改动 |
| `ADetect.exe` | ❌ 未改动 |
| `接口说明.h` | ❌ 未改动 |
| 训练代码 (`train.py` 等) | ❌ 未改动 |

**所有优化均在 Python 推理层（`anomalib2.2` env）实施，与封装版 DLL 完全独立。**

---

## 4. 部署所需文件清单（打包索引）

### 4.1 核心推理脚本（新增）

| 文件 | 路径 | 说明 | 是否新建 |
|------|------|------|---------|
| `ru007_experiment.py` | `D:\ru007_experiment.py` | **主推理脚本**（含全部优化）| ✅ 新建 |
| `trt_test.py` | `D:\trt_test.py` | TRT 验证测试脚本（非生产用）| ✅ 新建 |

### 4.2 原始模型文件（不改动，原路径复制）

| 文件 | 原始路径 | 说明 |
|------|----------|------|
| `model.ckpt` | `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt` | TorchScript backbone+head |
| `model.mb` | `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.mb` | Memory Bank（含 cut_index） |
| `modelConfig.yaml` | `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\modelConfig.yaml` | 推理配置 |

### 4.3 Python 环境（`anomalib2.2`）

| 包 | 版本 | 说明 |
|----|------|------|
| Python | 3.x | Conda env `anomalib2.2` |
| PyTorch | 2.11.0+cu128 | 支持 sm_120 (RTX 5060 Ti) |
| torchvision | 配套版本 | — |
| numpy | — | — |
| opencv-python | >=4.5 | `cv2.imread` |

导出 env：
```powershell
conda activate anomalib2.2
pip freeze > requirements_anomalib2.2.txt
```

### 4.4 实验报告（文档，不影响运行）

| 文件 | 路径 | 说明 |
|------|------|------|
| `exp_003_report.md` | `D:\RevUnsup\docs\exp_003_report.md` | RU-006 Feature Pooling 扫描报告 |
| `exp_004_report.md` | `D:\RevUnsup\docs\exp_004_report.md` | RU-007 零成本 flag 报告 |
| `exp_005_trt_install.md` | `D:\RevUnsup\docs\exp_005_trt_install.md` | TRT 环境安装记录 |
| `exp_006_trt_conversion.md` | `D:\RevUnsup\docs\exp_006_trt_conversion.md` | TRT 转换失败（Long/Double）|
| `exp_006b_trt_conversion_truncate.md` | `D:\RevUnsup\docs\exp_006b_trt_conversion_truncate.md` | TRT 重试（cu13 CUDA 35）|
| `exp_006c_trt_cu12.md` | `D:\RevUnsup\docs\exp_006c_trt_cu12.md` | TRT cu12 在线安装失败 |
| `exp_006e_trt_cu12_offline.md` | `D:\RevUnsup\docs\exp_006e_trt_cu12_offline.md` | TRT cu12 离线安装（cudaGetDevice 失败）|
| `phase3c-progress-report.html` | `D:\RevUnsup\docs\phase3c-progress-report.html` | 本次汇报 HTML |
| `deployment-changeset.md` | `D:\RevUnsup\docs\deployment-changeset.md` | **本文件** |

---

## 5. 环境差异说明

| 项目 | 开发机 (5060) | 目标实机 | 注意事项 |
|------|--------------|---------|---------|
| GPU | RTX 5060 Ti (sm_120 Blackwell) | 未知 | 必须使用 PyTorch ≥ 2.6 + CUDA 12.x 才能支持 Blackwell |
| PyTorch | 2.11.0+cu128 | 需 ≥ 2.6+cu12x | sm_120 支持从 PyTorch 2.6 开始 |
| CUDA Driver | 12.9 | 需 ≥ 12.0 | — |
| `anomalyDet.dll` | CUDA 11.7 (sm_50~sm_90) | 同左 | **RTX 5060 Ti 上 DLL 不可用**，需走 Python 推理 |
| 封装版 DLL 路径 | `D:\XYC_Dog_Agent\...` | 可变 | `MODEL_DIR` 变量改一下即可 |

**关键结论**：

> `anomalyDet.dll` 使用 LibTorch CUDA 11.7，**不支持 RTX 5060 Ti (sm_120)**，实机若也是 5060 Ti 则必须走本文的 Python 推理路线。如果实机是旧卡（RTX 30xx/40xx），DLL 可正常运行，Python 优化版作为对比参考。

---

## 6. 实机测试步骤

### 最小改动部署清单

**Step 1** — 把以下文件复制到实机：
```
ru007_experiment.py          ← 主推理脚本
model/model.ckpt             ← 不改动
model/model.mb               ← 不改动
model/modelConfig.yaml       ← 不改动
```

**Step 2** — 修改 `ru007_experiment.py` 中的路径（仅需改 2 个变量）：
```python
# 第16行
MODEL_DIR = r"<实机模型目录>"   # 改为实机路径

# 第20-27行
IMAGES = [r"<测试图路径1>", ...]  # 改为实机测试图
```

**Step 3** — 安装 Python 环境：
```powershell
conda create -n anomalib2.2 python=3.x
conda activate anomalib2.2
pip install torch==2.11.0+cu128 torchvision --index-url https://download.pytorch.org/whl/cu128
pip install opencv-python numpy
```

**Step 4** — 运行验证：
```powershell
conda activate anomalib2.2
python D:\ru007_experiment.py
```

**Step 5** — 对比封装版 DLL（如实机 GPU 支持）：
```powershell
ADetect.exe   # 对比 DLL 结果与 Python 推理结论是否一致
```

### 验收指标

| 指标 | 期望值 | 说明 |
|------|--------|------|
| 总推理耗时 (warmup 后) | ≤ 80ms | 5060 Ti 实测 65.5ms |
| KNN 耗时 | ≤ 10ms | 5060 Ti 实测 4.9ms |
| 异常判定 ret (0/1) | 与原 DLL 一致 | 主要验证点 |
| max_score | 误差 < 10% | FP16 精度损失 |
| GPU 显存 | ≤ 8GB | 5060 Ti 峰值 ~6.4GB |

---

## 7. 已放弃/未完成的方向

| 方向 | 状态 | 原因 |
|------|------|------|
| TensorRT `ir='torchscript'` | ❌ 放弃（当前） | CUDA context 隔离，`cudaGetDevice` 在 TRT runtime 内失败 |
| `tensorrt_cu13_libs` | ❌ 放弃 | CUDA error 35，cu13 与 CUDA 12.9 驱动不匹配 |
| `tensorrt_cu12_libs` 在线安装 | ❌ 放弃 | `pypi.nvidia.com` 网络不可达 |
| FAISS-GPU KNN | ❌ 放弃 | 安装复杂，CPU 版比 GPU cdist 慢 8.6x |
| torch.compile | ⚠ 无收益 | TorchScript 模型 compile 效果有限，保留 try-except |
| channels_last | ⚠ 无收益 | ResNet18 在本配置下无明显提升，保留代码 |
| TRT `ir='dynamo'` | ⏸ 待探索 | 可能绕开 context 隔离，成本低（改 2 行） |
| ONNX + onnxruntime-gpu | ⏸ 待探索 | 绕开 torch_tensorrt，清华源可安装 |

---

*DogWind | 2026-04-15 | RevUnsup Phase 3 打包说明*
