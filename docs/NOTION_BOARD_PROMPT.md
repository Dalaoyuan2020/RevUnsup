# Notion 项目看板 Prompt

把以下内容喂给 Notion AI，让它自动生成看板：

---

请帮我创建一个项目管理看板，项目名称："RevUnsup — 无监督缺陷检测复现与部署"

## 看板结构

使用 Board View（看板视图），列分为以下状态：
- Backlog（待办）
- In Progress（进行中）
- Review（待验证）
- Done（完成）
- Blocked（阻塞）

## 属性字段

每张卡片包含以下属性：
- **负责 Agent**: Select — 选项为 "Codex", "Claude Code", "OpenCode", "人工"
- **优先级**: Select — P0(必须), P1(重要), P2(可选)
- **预估耗时**: Text
- **依赖项**: Relation（关联其他卡片）
- **交付物**: Text
- **标签**: Multi-select — "Python", "C++", "C#", "文档", "测试"

## 卡片内容（请全部创建）

### Milestone 1: Python 训练流程 (Agent: Codex)

1. **实现 ROI 对齐裁切模块**
   - 负责: Codex | P0 | 3h
   - 文件: src/roi_alignment.py
   - 交付: 可运行的模板匹配+裁切函数
   - 标签: Python

2. **实现数据集重建脚本**
   - 负责: Codex | P0 | 2h
   - 文件: scripts/rebuild_segmented_dataset.py
   - 依赖: ROI对齐模块
   - 标签: Python

3. **实现数据集格式化脚本**
   - 负责: Codex | P0 | 1h
   - 文件: scripts/prepare_dataset.py
   - 依赖: 数据集重建
   - 标签: Python

4. **实现 PatchCore 训练脚本**
   - 负责: Codex | P0 | 3h
   - 文件: scripts/train_patchcore.py
   - 依赖: 数据集格式化
   - 交付: model.ckpt
   - 标签: Python

5. **实现阈值提取脚本**
   - 负责: Codex | P1 | 1h
   - 文件: scripts/extract_thresholds.py
   - 依赖: 训练完成
   - 交付: thresholds.json
   - 标签: Python

### Milestone 2: 推理 + 导出 (Agent: Claude Code)

6. **实现统一推理入口**
   - 负责: Claude Code | P0 | 4h
   - 文件: src/infer_service.py
   - 依赖: checkpoint (来自 Codex)
   - 交付: 可运行的推理服务
   - 标签: Python

7. **实现 ONNX 拆分导出**
   - 负责: Claude Code | P0 | 4h
   - 文件: src/export_onnx.py
   - 依赖: 推理入口验证通过
   - 交付: feature_extractor.onnx + memory_bank.npy + deploy_config.json
   - 标签: Python

8. **实现推理可视化脚本**
   - 负责: Claude Code | P1 | 2h
   - 文件: scripts/visualize_patchcore.py
   - 依赖: 推理入口
   - 标签: Python

9. **Python 推理精度验证**
   - 负责: Claude Code | P0 | 2h
   - 交付: 精度验证报告 + 耗时基线
   - 标签: 测试

### Milestone 3: C++ 部署 (Agent: OpenCode)

10. **实现 C++ 推理引擎**
    - 负责: OpenCode | P0 | 8h
    - 文件: deploy/cpp_inference/patchcore_engine.cpp
    - 依赖: ONNX 导出 (来自 Claude Code)
    - 交付: 可编译运行的推理引擎
    - 标签: C++

11. **实现 DLL 封装**
    - 负责: OpenCode | P0 | 4h
    - 文件: deploy/dll_wrapper/api.cpp
    - 依赖: C++ 推理引擎
    - 交付: anomalyDet.dll
    - 标签: C++

12. **实现 C# 测试界面**
    - 负责: OpenCode | P1 | 4h
    - 文件: deploy/csharp_ui/MainForm.cs
    - 依赖: DLL 封装
    - 交付: 可运行的 WinForms 程序
    - 标签: C#

13. **性能压测 RTX 2080**
    - 负责: OpenCode | P0 | 2h
    - 目标: 推理延迟 ≤100ms
    - 依赖: C++ 推理引擎
    - 标签: 测试

### Milestone 4: 集成 (Agent: 人工)

14. **全流程端到端测试**
    - 负责: 人工 | P0 | 4h
    - 依赖: 所有 Agent 交付
    - 标签: 测试

15. **实现一键 Pipeline 脚本**
    - 负责: 人工 | P1 | 2h
    - 文件: scripts/run_pipeline.py
    - 标签: Python

## 额外视图

请同时创建：
- **Timeline View**（时间线视图）— 按 Milestone 分组，展示依赖关系
- **Table View**（表格视图）— 按负责 Agent 分组，方便看每个 AI 的工作量
