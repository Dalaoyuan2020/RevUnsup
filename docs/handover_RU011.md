# RU-011 ChipROI02 训评交接 — 给 Napoleon

## 日期
2026-04-16

## 状态
- [x] 摸清 train.exe 参数
- [x] 构建训练工作区 (39 张)
- [x] 训练完成 (28.5 min, CPU, sm_120 不兼容)
- [x] 模型三件套验证 (ckpt 44.8MB + mb 42.5MB + yaml)
- [x] 准备 18 张评估图 (9+3+6)
- [x] 编译 eval harness
- [x] 旧模型管道验证通过
- [❌] **新模型 crash** (0xC0000409 stack buffer overrun)

## 关键发现

### 训练
- `train.exe --phase train --dataset_path <parent> --category <name>`
- 目录: `{dataset_path}/{category}/train/*.png` (直接放图, 不含 good/)
- 输出在: `{dataset_path}/{category}/model/` (不在 --project_path!)
- sm_120 不兼容 → CPU 模式 → 28.5 分钟
- forwardSize 默认 256×256 (因未指定 --modelInputW/H)

### 评估
- **新模型**: creatModel ✅ → anomalyDetMatin ❌ crash (0xC0000409)
- **旧模型 (验证用)**: 18 张全 ret=1, good 0/9 (模型不适配 ChipROI02)

## 阻塞点
**新模型与 LibTorch 2.8.0 DLL 不兼容**。train.exe 打包的旧 PyTorch 生成的 TorchScript 格式可能不被 LibTorch 2.8 正确执行。

## 建议 Napoleon 判断
1. **方案 A**: 用 Python 重新训练 (anomalib + PyTorch 2.11), 生成兼容 TorchScript
2. **方案 B**: 用 Python 做推理评估 (跳过 C++ DLL)
3. **方案 C**: 重训时加 --modelInputW 1000 --modelInputH 1000 试试

## 产物
- `docs/exp_011_chiproi02_eval.md` — 详细报告
- `docs/handover_RU011.md` — 本文件
- `cpp_patch/RU011_eval/eval_main.cpp` — 评估 harness
- `cpp_patch/RU011_train_output/Model/` — 新模型 (本地, 不 push)

等 Napoleon 判断方向。
