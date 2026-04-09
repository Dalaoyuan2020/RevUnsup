# RevUnsup — 无监督缺陷检测复现与部署

基于 PatchCore 的无监督缺陷检测系统：从逆向分析到算法复现到 C++ 工业部署。

## 项目背景

复现企业级 anomalyDet 交付物的完整能力链路：

```
原图 → 模板对齐 → ROI裁切 → PatchCore训练 → 阈值提取 → 热力图推理 → DLL/SDK部署
```

逆向分析文档：[docs/ARCHITECTURE_REVERSE_ENGINEERING.md](./docs/ARCHITECTURE_REVERSE_ENGINEERING.md)

## 技术栈

| 层 | 技术 |
|---|---|
| 算法 | PatchCore (Anomalib), ResNet18 |
| 训练 | Python 3.10+, PyTorch, Anomalib |
| 导出 | ONNX |
| 部署 | C++17, ONNX Runtime / TensorRT, CUDA 11.6+ |
| 封装 | DLL (C ABI) + C# SDK |
| 界面 | C# WinForms |
| 目标 | RTX 2080 推理 ≤100ms |

## 核心参数（对齐旧版）

```yaml
model: PatchCore
backbone: resnet18
layers: [layer2, layer3]
image_size: [256, 256]
task: segmentation
coreset_sampling_ratio: 0.1
num_neighbors: 1
batch_size: 1
```

## 项目结构

```
RevUnsup/
├── src/                           # Python 核心源码
│   ├── roi_alignment.py           # ROI 模板对齐+裁切
│   ├── infer_service.py           # 统一推理入口
│   ├── export_onnx.py             # ONNX 导出（含 memory bank）
│   └── utils.py                   # 通用工具
├── scripts/                       # 流程脚本
│   ├── rebuild_segmented_dataset.py  # 原图→ROI裁切数据集
│   ├── prepare_dataset.py            # 整理成 Anomalib 格式
│   ├── train_patchcore.py            # PatchCore 训练
│   ├── extract_thresholds.py         # 阈值提取
│   ├── visualize_patchcore.py        # 推理可视化
│   └── run_pipeline.py               # 一键全流程
├── deploy/                        # C++ 部署层
│   ├── cpp_inference/             # C++ 推理引擎
│   │   ├── CMakeLists.txt
│   │   ├── patchcore_engine.h
│   │   ├── patchcore_engine.cpp
│   │   └── main.cpp
│   ├── dll_wrapper/               # DLL 封装
│   │   ├── api.h
│   │   └── api.cpp
│   └── csharp_ui/                 # C# 测试界面
│       └── MainForm.cs
├── configs/                       # 配置文件
│   └── patchcore_default.yaml
├── tests/                         # 测试
├── data/                          # 数据（不入库）
├── checkpoints/                   # 模型产物（不入库）
└── docs/                          # 文档
    ├── ARCHITECTURE_REVERSE_ENGINEERING.md
    └── AGENT_TASKS.md
```

## 快速开始

```bash
# 1. 安装依赖
pip install -r requirements.txt

# 2. 训练
python scripts/train_patchcore.py --dataset-root data/datasets --category ChipROI01

# 3. 推理
python src/infer_service.py --checkpoint checkpoints/model.ckpt --input test.png

# 4. 导出 ONNX
python src/export_onnx.py --checkpoint checkpoints/model.ckpt --output deploy/model.onnx

# 5. C++ 部署
cd deploy/cpp_inference && cmake -B build && cmake --build build
./build/patchcore_infer --model ../model.onnx --image test.png
```

## Agent 分工

| Agent | 负责人 | 任务 |
|---|---|---|
| Agent 1 - Python 训练流程 | Codex | ROI裁切 + 数据集 + 训练 + 阈值 |
| Agent 2 - 推理导出 | Claude Code | 统一推理入口 + ONNX导出 + 验证 |
| Agent 3 - C++ 部署 | OpenCode | C++引擎 + DLL + C#界面 |

详见 [docs/AGENT_TASKS.md](./docs/AGENT_TASKS.md)

## v0.2 — Harness 自动化升级（2026-04-09）

从【三Agent手动分工版】升级到【Harness编排自动版】。

新加入的层：
- **L2 编排层**：博士生看面板做决策，不亲自整理数据/不写脚本
- **L3 评估层**：4维评价(数据集/模型/打包文件/评价方式) — 见 [docs/L3-evaluation-spec.md](./docs/L3-evaluation-spec.md)
- **L4 监督层**：硕士生作为状态守门人，看面板/标异常/写周报
- **Dashboard**：[dashboard/](./dashboard/) 下的4个静态HTML，三端可视化

任务分配优先级（劳动力协议）：
```
① Agent (默认首选)
② 硕士生 (Agent搞不定的兜底)
③ 博士生 (战略决策/评价体系/编排逻辑)
```

不可外包的事（必须博士生做）：
- 战略决策（论文方向、实验设计）
- 评价体系本身的设计（什么算"提升"）
- 编排逻辑（下一步做什么）

详见 [docs/L3-evaluation-spec.md](./docs/L3-evaluation-spec.md) 的 §6.1 任务分配优先级。

### Dashboard 入口

```
dashboard/
├── index.html         # 首页(三角架构图)
├── orchestrator.html  # 博士生编排端
├── worker.html        # Worker执行端
└── supervisor.html    # 硕士生监督端
```

本地直接打开 `dashboard/index.html` 即可预览。

## License

MIT
