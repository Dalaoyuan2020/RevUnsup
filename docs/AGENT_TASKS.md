# Agent 任务分工

## 依赖关系

```
Agent 1 (Codex) ──checkpoint──→ Agent 2 (Claude Code) ──ONNX──→ Agent 3 (OpenCode)
```

Agent 1 和 Agent 3 可并行启动：Agent 3 先用公开 MVTec 数据集的 PatchCore ONNX 搭框架。

---

## Agent 1: Python 训练流程 — 分配给 Codex

**为什么给 Codex**: Codex 擅长批量生成完整的 Python 脚本，训练流程逻辑清晰、上下文集中，适合一次性生成。

### 任务清单

1. **`src/roi_alignment.py`** — 实现所有 TODO 函数
   - 模板匹配 (cv2.matchTemplate)
   - 偏移估计 + 仿射变换对齐
   - ROI mask 应用 + 白底裁切
   - 输出 256x256 图像

2. **`scripts/rebuild_segmented_dataset.py`** — 批量 ROI 裁切
   - 遍历原始图像目录
   - 调用 roi_alignment 处理每张图
   - 输出 good/bad 分类目录 + metrics

3. **`scripts/prepare_dataset.py`** — 整理成 Anomalib 格式
   - 输入: good/bad 裁切目录
   - 输出: train/good, test/good, test/defect
   - 按 8:2 拆分 good 样本

4. **`scripts/train_patchcore.py`** — PatchCore 训练
   - 使用 Anomalib API
   - 参数对齐: resnet18, 256x256, coreset_sampling_ratio=0.1, num_neighbors=1
   - 保存 checkpoint 到 checkpoints/

5. **`scripts/extract_thresholds.py`** — 阈值提取
   - 从 model.ckpt 提取 image_threshold, pixel_threshold
   - 输出 JSON

### 交付物
- 可运行的 5 个脚本
- 训练产出的 model.ckpt
- thresholds.json

### Prompt（直接喂给 Codex）

```
你是一个 Python 计算机视觉工程师。请在 RevUnsup 仓库中实现以下文件，所有 TODO 都要填充完整可运行的代码。

项目背景：基于 Anomalib PatchCore 的无监督缺陷检测。
核心参数：backbone=resnet18, image_size=256, coreset_sampling_ratio=0.1, num_neighbors=1, batch_size=1, task=segmentation

需要实现的文件：
1. src/roi_alignment.py — 模板匹配+对齐+ROI裁切，所有函数补完
2. scripts/rebuild_segmented_dataset.py — 批量ROI裁切脚本，CLI入口
3. scripts/prepare_dataset.py — 整理成 Anomalib Folder 格式
4. scripts/train_patchcore.py — 调用 anomalib API 训练 PatchCore
5. scripts/extract_thresholds.py — 从 checkpoint 提取阈值

依赖：anomalib>=1.0.0, torch>=2.0, opencv-python, numpy
数据格式：Anomalib Folder 风格 (train/good, test/good, test/defect)
每个脚本必须有 argparse CLI 入口，可独立运行。
不要修改 configs/patchcore_default.yaml 的参数。
```

---

## Agent 2: 推理 + ONNX 导出 — 分配给 Claude Code

**为什么给 Claude Code**: 推理入口和 ONNX 导出涉及 Anomalib 内部结构理解、memory bank 拆分等技巧性工作，Claude Code 的推理能力更适合处理这类需要深度理解的任务。

### 任务清单

1. **`src/infer_service.py`** — 实现 PatchCoreInference 类
   - load_model(): 加载 checkpoint + 阈值
   - predict(): 返回 anomaly_score, heatmap, mask, is_defect
   - predict_batch(): 批量推理
   - visualize(): 三联图输出

2. **`src/export_onnx.py`** — ONNX 导出（拆分式）
   - export_feature_extractor(): ResNet18 → ONNX
   - export_memory_bank(): coreset → .npy
   - export_deploy_config(): 阈值+参数 → JSON

3. **`scripts/visualize_patchcore.py`** — 推理可视化
   - 调用 infer_service 生成三联图
   - 输出 CSV 统计报告

4. **精度验证** — 确认 Python 推理结果正确
   - 用 MVTec 公开数据测试
   - 记录 Python 推理耗时作为基线

### 交付物
- 可运行的推理服务
- feature_extractor.onnx + memory_bank.npy + deploy_config.json
- Python 推理耗时基线报告

### Prompt（直接喂给 Claude Code）

```
你是一个深度学习部署工程师。请在 RevUnsup 仓库中实现推理和 ONNX 导出模块。

项目背景：PatchCore (Anomalib) 无监督缺陷检测，需要从 Python checkpoint 导出到可被 C++ 加载的格式。

关键难点：PatchCore 的 memory bank (coreset embeddings) 不是标准网络层，不能直接导进 ONNX。
解决方案：拆分导出：
- feature_extractor.onnx: 只导出 ResNet18 特征提取部分
- memory_bank.npy: 单独保存 coreset embeddings
- deploy_config.json: 保存阈值和配置参数

需要实现的文件：
1. src/infer_service.py — PatchCoreInference 类，所有方法补完
2. src/export_onnx.py — 拆分式导出，三个函数补完
3. scripts/visualize_patchcore.py — 推理可视化脚本

Anomalib checkpoint 结构参考：
- model.backbone: ResNet18 feature extractor
- model.memory_bank: tensor, shape=[N, C] coreset embeddings
- image_threshold: NormalizationCallback 保存的阈值
- pixel_threshold: 同上

输入图像预处理：resize(256,256) → normalize(ImageNet mean/std) → CHW → batch
ONNX opset=14, 输入名="input", 输出名="features"
```

---

## Agent 3: C++ 部署 + DLL — 分配给 OpenCode

**为什么给 OpenCode**: C++/CUDA 工程、CMake 配置、DLL 封装、C# P/Invoke 这类系统级编程是 OpenCode 的强项。

### 任务清单

1. **`deploy/cpp_inference/patchcore_engine.cpp`** — 核心推理引擎
   - init(): 加载 ONNX Runtime session + memory bank + config
   - infer(): 预处理 → 特征提取 → KNN搜索 → 异常分数 → 热力图
   - release(): 释放资源
   - GPU 加速 (CUDA EP)

2. **`deploy/dll_wrapper/api.cpp`** — DLL 封装
   - 实现 AIAD_Init / AIAD_Infer / AIAD_Release
   - C ABI 导出

3. **`deploy/csharp_ui/MainForm.cs`** — C# 测试界面
   - WinForms 极简界面
   - P/Invoke 调用 DLL
   - 显示推理结果

4. **`deploy/cpp_inference/CMakeLists.txt`** — 完善 CMake 配置
   - 适配 Windows + CUDA + ONNX Runtime

5. **性能压测**
   - RTX 2080 上测推理延迟
   - 目标 ≤100ms

### 交付物
- 可编译的 C++ 推理引擎
- anomalyDet.dll
- C# 测试界面
- 性能测试报告

### Prompt（直接喂给 OpenCode）

```
你是一个 C++ 工业部署工程师。请在 RevUnsup/deploy/ 目录下实现 PatchCore 的 C++ 推理引擎和 DLL 封装。

部署架构（拆分式加载）：
- feature_extractor.onnx: ONNX Runtime 加载，GPU 推理 (CUDA EP)
- memory_bank.npy: 加载到内存，用于 KNN 最近邻搜索
- deploy_config.json: 阈值和参数

推理流程：
1. 图像预处理: imread → resize(256,256) → float32 → normalize(ImageNet) → NCHW
2. 特征提取: ONNX Runtime 推理 feature_extractor.onnx
3. KNN 搜索: 提取的特征 patch 与 memory bank 做欧氏距离最近邻
4. 异常分数: 最近邻距离的最大值作为 image-level score
5. 热力图: 每个 patch 的距离 reshape 成空间图，上采样到 256x256
6. 掩膜: 热力图 > pixel_threshold 的区域

需要实现：
1. deploy/cpp_inference/patchcore_engine.cpp — 填充所有 TODO
2. deploy/dll_wrapper/api.cpp — 填充所有 TODO
3. deploy/csharp_ui/MainForm.cs — 完整 WinForms 界面

环境：Windows, CUDA 11.6+, ONNX Runtime 1.16+, OpenCV 4.8+, VS2022
目标GPU：RTX 2080 (8GB), 推理延迟 ≤100ms
DLL 接口对齐旧版：AIAD_Init / AIAD_Infer / AIAD_Release
```
