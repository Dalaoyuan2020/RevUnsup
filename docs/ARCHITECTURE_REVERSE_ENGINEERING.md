# anomalyDet 架构逆向与复现说明

## 1. 结论先行

`C:\Users\Administrator\Downloads\anomalyDet\anomalyDet` 不是源码仓库，而是一个已经打包完成的异常检测交付物，结构上可以拆成两部分：

1. `train/`
   这是旧版训练侧产物，核心是一个封装后的 Python 训练程序 `train.exe`，配套 `_internal/` 运行时、Anomalib/PyTorch 依赖、配置文件和示例数据。
2. `infer/`
   这是旧版部署侧产物，核心是 `anomalyDet.dll + AIAD_SDK.cs`，配套 `model.ckpt`、`model.mb`、`modelConfig.yaml` 以及推理所需 CUDA / Torch 动态库。

可以确认的旧版技术路线是：

- 模型：`PatchCore`
- Backbone：`resnet18`
- 数据格式：`MVTec / Folder 风格`
- 任务类型：`segmentation`
- 输入尺寸：`256 x 256`
- 训练 / 测试 batch size：`1`
- `coreset_sampling_ratio = 0.1`
- `num_neighbors = 1`
- 推理交付同时保留：
  - `model.ckpt`：训练检查点
  - `model.mb`：部署侧模型二进制
  - `modelConfig.yaml`：部署参数

这说明旧版本质上是：

`ROI 分割数据准备 -> PatchCore 训练 -> checkpoint 阈值提取 -> 可视化/热力图 -> DLL/SDK 部署`

## 2. 旧版 anomalyDet 的可确认架构

### 2.1 训练侧

从 `train/_internal/con1.yaml` 可以确认：

- 数据集根目录：`D:/DataSet/`
- 数据集格式：`mvtec`
- 类别名：`Train-20230228223349`
- `image_size: [256, 256]`
- `task: segmentation`
- `train_batch_size: 1`
- `test_batch_size: 1`
- `backbone: resnet18`
- `layers: [layer2, layer3]`
- `coreset_sampling_ratio: 0.1`
- `num_neighbors: 1`
- `weight_file: weights/model.ckpt`

从 `infer/model/modelConfig.yaml` 还能补充出旧版部署参数：

- `forwardSize: [2048, 2048]`
- `maxTrainSize / maxTrainSize1 / maxTrainSize2`
- `poolSize`
- `scoreRatio`
- `Thresh / Thresh1 / choosedThreshFinal`
- 颜色增强、旋转增强等训练超参

因此旧版训练侧并不是“神秘黑盒模型”，而是“Anomalib PatchCore + 一套工程参数封装”。

### 2.2 推理侧

`infer/` 目录包含：

- `anomalyDet.dll`
- `AIAD_SDK.cs`
- `model/model.ckpt`
- `model/model.mb`
- `model/modelConfig.yaml`
- Torch / CUDA 运行库

这说明旧版部署结构是：

1. 外部系统通过 `AIAD_SDK.cs` 调用 `anomalyDet.dll`
2. DLL 读取 `model.mb` 或 `model.ckpt + modelConfig.yaml`
3. 返回异常分数、异常判定、可能还包括像素级热力图能力

也就是说，旧版是“训练与部署分离”的双产物架构，而不是单一 Python 脚本。

## 3. 现有工程与旧版的映射关系

当前仓库中真正可复现旧版架构的是：

- 数据重建：`scripts/rebuild_segmented_dataset.py`
- 标准数据集整理：`scripts/prepare_result0208_dataset.py`
- 模型训练：`scripts/train_patchcore.py`
- 阈值提取：`scripts/extract_thresholds.py`
- 可视化输出：`scripts/visualize_patchcore.py`
- 总编排：`scripts/run_acceptance_pipeline.py`
- ROI 对齐与掩膜裁切核心：`src/roi_alignment.py`
- 调试图输出：`src/debug_utils.py`
- 结果解释：`src/explainability.py`

旧版与现版的一一映射如下：

| 旧版模块 | 现版对应 |
|---|---|
| `train.exe` | `scripts/train_patchcore.py` |
| `train/_internal/con1.yaml` | `config/model_config.yaml` + 脚本参数 |
| 原始数据切 ROI | `scripts/rebuild_segmented_dataset.py` + `src/roi_alignment.py` |
| 数据整理成 mvtec/folder | `scripts/prepare_result0208_dataset.py` |
| `model.ckpt` | `models/patchcore_runs/.../model.ckpt` |
| 训练后阈值固化 | `scripts/extract_thresholds.py` |
| 推理热图输出 | `scripts/visualize_patchcore.py` |
| 一键主流程 | `scripts/run_acceptance_pipeline.py` |
| 交付式部署 DLL | 当前仓库还未复刻 |

## 4. 当前仓库中“有效信息前置”后的真实架构

如果把旧版黑盒拆开，真正需要复现的不是 `exe/dll` 外壳，而是下面这条主链路：

### Layer A. ROI 重建层

职责：

- 选择模板图
- 对目标图做模板匹配
- 估计偏移
- 图像对齐
- 应用 ROI mask
- 输出白底 core crop

对应实现：

- `src/roi_alignment.py`
- `scripts/rebuild_segmented_dataset.py`

### Layer B. 标准数据集层

职责：

- 将 `good/bad` 裁切结果整理成 Anomalib 可直接使用的目录
- 形成 `train/good`, `test/good`, `test/defect`
- 在必要时引入合成缺陷图做测试补位

对应实现：

- `scripts/prepare_result0208_dataset.py`

### Layer C. PatchCore 训练层

职责：

- 读取 Folder/MVTec 风格数据
- 初始化 `Patchcore(backbone=resnet18, coreset_sampling_ratio=0.1)`
- 用 `Engine.fit()` 训练并导出 checkpoint

对应实现：

- `scripts/train_patchcore.py`

### Layer D. 阈值恢复层

职责：

- 从 `model.ckpt` 中提取 `image_threshold` / `pixel_threshold`
- 同时保留 raw threshold 与 normalized threshold

对应实现：

- `scripts/extract_thresholds.py`

### Layer E. 推理表达层

职责：

- 载入 checkpoint
- 对测试集推理
- 输出热力图、三联图、CSV、解释文本
- 计算 ROI 内/外重叠关系，辅助识别疑似误报

对应实现：

- `scripts/visualize_patchcore.py`
- `src/explainability.py`

### Layer F. 验收编排层

职责：

- 串起数据准备、训练、阈值提取、推理、报告
- 形成一次完整可复现运行

对应实现：

- `scripts/run_acceptance_pipeline.py`

## 5. 为什么 `src/pipeline.py + layer0~5` 不是当前应复现的主线

仓库里的 `src/pipeline.py` 和 `layer0_input.py` 到 `layer5_output.py` 是早期 6 层骨架设计，但从当前文档和脚本边界可以确认：

- 它们目前不是实际主入口
- 还没有真正接入完整训练/推理逻辑
- 如果直接往这套骨架上继续补功能，容易再次回到“边做边补丁”的状态

所以复现旧版架构时，应优先复现“稳定主流程脚本链”，而不是优先复活占位层。

## 6. 复现架构的最小可执行方案

如果你的目标是“把 old anomalyDet 的能力复刻出来”，建议按下面顺序落地：

### 第一步：复现 ROI 裁切层

运行：

```powershell
python scripts/rebuild_segmented_dataset.py `
  --source-root <原始芯片目录> `
  --roi-root <ROI掩膜目录> `
  --output-root data/segmented_rebuild
```

输出应为每个 `ChipROIxx` 的：

- `good/`
- `bad/`
- `metrics.json`
- `metrics.csv`
- `processing_report.txt`

### 第二步：整理成训练数据集

运行：

```powershell
python scripts/prepare_result0208_dataset.py `
  --source-root data/segmented_rebuild `
  --target-root data/datasets/result0208
```

输出目录应为：

```text
data/datasets/result0208/
  ChipROI01/
    train/good
    test/good
    test/defect
```

### 第三步：训练 PatchCore

旧版最接近参数建议：

- `backbone = resnet18`
- `coreset_sampling_ratio = 0.1`
- `image-size = 256`
- `batch-size = 1`
- `epochs = 1`

运行示例：

```powershell
python scripts/train_patchcore.py `
  --dataset-root data/datasets/result0208 `
  --results-root models/patchcore_runs `
  --category ChipROI01 `
  --image-size 256 `
  --train-batch-size 1 `
  --eval-batch-size 1 `
  --device gpu `
  --backbone resnet18 `
  --coreset-sampling-ratio 0.1 `
  --max-epochs 1
```

### 第四步：提取阈值

```powershell
python scripts/extract_thresholds.py `
  --results-root models/patchcore_runs `
  --output-path models/thresholds.json `
  --categories ChipROI01
```

### 第五步：生成推理结果

```powershell
python scripts/visualize_patchcore.py `
  --dataset-root data/datasets/result0208 `
  --results-root models/patchcore_runs `
  --output-root data/results/patchcore `
  --category ChipROI01 `
  --device gpu `
  --image-size 256 `
  --subset defect `
  --thresholds-json models/thresholds.json `
  --three-panel
```

### 第六步：用一键总控跑完整流程

```powershell
python scripts/run_acceptance_pipeline.py --device gpu --epochs 1
```

## 7. 如果要 1:1 逼近旧版 anomalyDet，还缺什么

当前仓库已经复现了“训练与推理逻辑主体”，但还没有完全复刻旧版交付形态。差异主要有三项：

1. 缺少部署侧封装
   旧版是 `DLL + SDK`，现版还是 Python 脚本。
2. 缺少 `model.mb` 的导出链
   当前主线保留的是 `model.ckpt`，还没有构造与旧版一致的部署模型格式。
3. 缺少统一推理 API
   旧版应当存在固定函数接口供 C# / 上位机调用，现版主要面向离线脚本。

所以如果你的目标是“工程能力复现”，当前仓库已经基本具备。
如果你的目标是“交付形态复现”，下一步要做的是：

- 新建一个统一 `infer_service.py` 或部署导出模块
- 固化输入输出协议
- 再决定导出成 Python API、CLI、HTTP 服务，还是 C# 可调用 DLL

## 8. 推荐的复现落地路线

最稳妥的路线不是直接逆向 `train.exe` / `anomalyDet.dll`，而是：

1. 先以现有仓库脚本链复刻旧版核心算法流程
2. 用旧版配置参数校准训练与推理行为
3. 跑通 `ChipROI01/02/05/06` 等重点 ROI
4. 最后再补部署包装层

这样做的原因是：

- 旧版源码并不完整可见
- 但核心算法与参数已经足够被恢复
- 当前仓库已经把最重要的“有效信息”拆出来了

## 9. 一句话总结

你要复现的本质不是 `exe + dll` 本身，而是这条主架构：

`原图 -> 模板对齐 -> ROI 掩膜裁切 -> Folder/MVTec 数据集 -> PatchCore(resnet18) 训练 -> checkpoint 阈值提取 -> 热图/掩膜推理输出 -> 部署封装`

当前 `xinye-defect-detection` 已经复现了这条链路的大部分主体，后续只需要继续补“统一部署入口”即可逼近旧版完整交付形态。
