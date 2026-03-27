# Agent 任务分工

## 依赖关系

```
Agent 1 (Codex) ──checkpoint──→ Agent 2 (Claude Code) ──ONNX──→ Agent 3 (OpenCode)
```

Agent 1 和 Agent 3 可并行启动：Agent 3 先用公开 MVTec 数据集的 PatchCore ONNX 搭框架。

## ⚠️ 重要：本项目目标是学习，不是交付产品

项目负责人是一位正在学习工业视觉部署全链路的工程师。所有 Agent 的输出必须让他能看懂、能验证、能学到东西。具体要求已写入每个 Agent 的 Prompt 中。

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
- **LEARN_AGENT1.md**（给项目负责人的学习说明）

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

=== 额外要求（必须遵守）===

1. 中文注释：每个函数开头用中文写一段注释，说明"这个函数在整个流程中扮演什么角色"，不是解释代码语法，而是解释它在做什么业务动作。例如：
   # 这个函数把原始大图和模板图做匹配，算出偏移量
   # 就像拿一张标准照片去比对当前照片歪了多少

2. 验证步骤：每个脚本末尾加一段 if __name__ == "__main__" 的验证代码，用 print 输出关键中间结果。例如训练脚本结束后打印：
   print("✅ 训练完成")
   print(f"  checkpoint 保存在: {checkpoint_path}")
   print(f"  checkpoint 文件大小: {size} MB")
   print(f"  训练耗时: {time} 秒")

3. 学习文档：完成所有代码后，额外生成一个 docs/LEARN_AGENT1.md，内容是：
   - 用大白话（中文）解释你写的每个文件在干什么
   - 用一张流程图（文字版）画出这5个脚本的执行顺序和数据流向
   - 列出"如果要自己从零写，最容易踩的3个坑"
   - 列出"运行这些脚本后，怎么判断结果是正确的"（具体看哪个数字、哪个文件）
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
- **LEARN_AGENT2.md**（给项目负责人的学习说明）

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

=== 额外要求（必须遵守）===

1. 中文注释：在以下关键节点加中文注释，解释"为什么这样做"而不是"做了什么"：
   - 为什么 memory bank 不能塞进 ONNX（在 export_onnx.py 顶部解释）
   - 为什么要分成三个文件导出（在 export_all 函数里解释）
   - 推理时 ONNX 负责什么、Python 自己负责什么（在 predict 函数里标注）
   - 热力图是怎么从一堆距离数字变成一张图的（在生成热力图的地方解释）

2. 验证步骤：
   - infer_service.py 的 predict() 运行后打印：
     print(f"✅ 推理完成: {image_path}")
     print(f"  异常分数: {score:.4f} (阈值: {threshold:.4f})")
     print(f"  判定结果: {'❌ 缺陷' if is_defect else '✅ 正常'}")
     print(f"  推理耗时: {time_ms:.1f} ms")
   - export_onnx.py 导出后打印：
     print(f"✅ 导出完成")
     print(f"  ONNX 文件: {onnx_path} ({onnx_size:.1f} MB)")
     print(f"  Memory Bank: {mb_path} (shape={shape}, {mb_size:.1f} MB)")
     print(f"  配置文件: {config_path}")
   - 导出后自动做一次验证：用同一张图分别跑 Python 推理和 ONNX 推理，打印两个分数的差值，确认一致性

3. 学习文档：完成所有代码后，额外生成 docs/LEARN_AGENT2.md，内容是：
   - 用大白话（中文）解释"为什么不能把整个 PatchCore 直接导出成一个 ONNX"
   - 画一张数据流图（文字版）：一张图片从输入到最终判定"有没有缺陷"，中间经过了哪些步骤，每一步的输入输出分别是什么形状的数据
   - 对比表格：Python 推理 vs ONNX 推理 vs 未来的 C++ 推理，三者的区别是什么
   - 列出"导出 ONNX 时最容易翻车的 3 个坑"
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
- **LEARN_AGENT3.md**（给项目负责人的学习说明）

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

=== 额外要求（必须遵守）===

1. 中文注释：在以下位置加详细中文注释，面向一个懂 Python 但刚接触 C++ 部署的读者：
   - patchcore_engine.cpp 顶部：用中文解释"C++ 推理引擎在整个系统中的角色"，以及"它和 Python 推理相比，做的事情完全一样，只是换了个语言执行"
   - init() 函数：每加载一个文件（onnx/npy/json），注释说明"这个文件是谁生成的、里面装的是什么"
   - infer() 函数：把 6 步推理流程的每一步都用注释标出来，格式为：
     // === 第1步：图像预处理 ===
     // 把图片从 BGR 转成 RGB，缩放到 256x256，除以 255 变成 0~1 的小数
     // 再减去 ImageNet 均值、除以标准差（和 Python 训练时的预处理保持一致）
   - api.cpp：解释"为什么要封装成 DLL"和"C# 是怎么调用这个 DLL 的"
   - KNN 搜索部分：解释"为什么这一步不在 ONNX 里，要自己写"

2. 验证步骤：
   - main.cpp 运行后打印：
     printf("✅ 引擎初始化成功\n");
     printf("  ONNX 模型: %s\n", onnx_path);
     printf("  Memory Bank: %d 个特征向量, 每个 %d 维\n", n_samples, n_dims);
     printf("  GPU: %s\n", gpu_name);
     printf("✅ 推理完成: %s\n", image_path);
     printf("  异常分数: %.4f (阈值: %.4f)\n", score, threshold);
     printf("  判定结果: %s\n", is_defect ? "❌ 缺陷" : "✅ 正常");
     printf("  推理耗时: %.1f ms\n", time_ms);
     printf("    其中特征提取: %.1f ms\n", feat_time);
     printf("    其中KNN搜索: %.1f ms\n", knn_time);
     printf("    其中后处理: %.1f ms\n", post_time);
   - 耗时要分步计时，这样能看出瓶颈在哪

3. 学习文档：完成所有代码后，额外生成 docs/LEARN_AGENT3.md，内容是：
   - 用大白话（中文）解释"C++ 部署到底在干什么，和 Python 推理有什么区别"
   - 画一张调用链路图（文字版）：从 C# 界面点击"检测"按钮开始，数据经过了哪些层（C# → DLL → C++ → ONNX Runtime → CUDA → GPU → 结果返回），每一层做了什么
   - 交付物清单表格：最终给客户的文件夹里有哪些文件，每个文件是什么、谁生成的、能不能删
   - 列出"C++ 部署最常见的 5 个报错"及对应的排查方法（比如 DLL 找不到、CUDA 版本不匹配等）
   - 性能优化指南：如果推理超过 100ms，按什么顺序排查和优化
```
