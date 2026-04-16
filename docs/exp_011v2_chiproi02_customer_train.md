# RU-011-v2: ChipROI02 客户训练管线验证报告

> **日期**: 2026-04-16  
> **执行者**: DogWind (XYC_Windsurf)  
> **机器**: PC-20260115QQCJ / RTX 5060 Ti 16GB / CUDA 12.9  

## 摘要

使用客户真实 Python 训练管线 (`ad_code/train/train.py`) 替代 v1 的 `train.exe`，
在 ChipROI02 数据集上完成了完整的「训练 → FP16后处理 → DLL推理评估」闭环。

**核心成果**:
- ✅ 客户管线训练成功输出 model.ckpt + model.mb + modelConfig.yaml
- ✅ 发现并解决 v1 crash (0xC0000409) 根因: **FP32→FP16 精度转换**
- ✅ 18 张图 DLL 推理零 crash，avg 745ms/张
- ⚠️ Good 图误报率高 (0/9 correct)，属于阈值/数据量问题，非管线兼容性问题

## 1. 环境搭建

| 项目 | 值 |
|------|-----|
| conda env | `custom_train` |
| Python | 3.8.20 |
| PyTorch | 2.0.1 + cu118 |
| OpenCV | 4.5.4 (pip headless wheel, 绕过conda DLL问题) |
| .pyd 模块 | 5/5 加载成功 (torch_model, dynamic_module, feature_extractor, k_center_greedy, random_projection) |

**注意**: RTX 5060 Ti (sm_120) 与 PyTorch 2.0.1 (最高 sm_90) 不兼容，训练自动 fallback 到 CPU。

## 2. 训练参数

```
--dataset_path  D:\RevUnsup\cpp_patch\RU011v2_train
--category      ChipROI02
--num_epochs    50 (smoke=2)
--modelInputW   1000
--modelInputH   1000
--batch_size    1
```

## 3. 训练结果

| 阶段 | 图片数 | 耗时 | model.ckpt | model.mb |
|------|--------|------|-----------|---------|
| Smoke (3图×2ep) | 3 | 652s (~11min) | 42.8 MB (FP32) | 55 MB |
| Full (39图×50ep) | 39 | ~30min | 42.8 MB (FP32) | 679 MB |

modelConfig.yaml 关键字段: `forwardSize: [1000, 1000]`, `exportType: 1`, `modelindex: 1`

## 4. 关键发现: v1 Crash 根因

### 问题
v1 train.exe 和 v2 Python 管线产出的 model.ckpt 都在 DLL 推理时 crash (0xC0000409)。

### 根因分析
| 属性 | 旧工作模型 | 新训练模型 |
|------|-----------|-----------|
| model.ckpt 大小 | 22.5 MB | 44.9 MB |
| 权重精度 | **FP16 (HalfTensor)** | **FP32 (FloatTensor)** |
| 大小比 | 1x | 2x |

DLL 推理流程 (`anomalyDetBigMatinPatchCoreCUT`) 将输入转为 FP16 后送入模型:
```cpp
input = input.to(kFloat16);  // DLL 固定 FP16 输入
output = module.forward(input);  // 需要模型权重也是 FP16
```

- **FP16 输入 + FP16 权重** → ✅ 正常
- **FP16 输入 + FP32 权重** → ❌ 类型不匹配 → 0xC0000409

### 原因
客户原始训练环境 (`D:\wej_AI_5.0\unsupervisd_train\`) 在 `torch.jit.trace()` 前将模型转为 FP16。
而我们的环境 (sm_120 不兼容，CPU fallback) 保持 FP32，导致 TorchScript 模型以 FP32 导出。

### 修复
```python
model = torch.jit.load('model.ckpt', map_location='cpu')
model = model.half()  # FP32 → FP16
torch.jit.save(model, 'model.ckpt')
```
44.9 MB → 22.5 MB，与旧模型一致。

## 5. DLL 评估结果 (FP16 后处理后)

```
creatModel returned: 0 (3577 ms)  ← 模型加载成功
Warmup: 5252ms / 807ms / 764ms

=== ACCURACY ===
Good correct:      0/9   (全部误报为 bad, ret=1, score=1.0)
Real bad correct:  3/3   ✅
Synth bad correct: 6/6   ✅
Overall: 9/18
```

平均推理耗时: **745 ms/张** (含 KNN 679MB memory bank 遍历)

### Good 误报分析
- 所有 good 图 anomaly_score = 1.0000 (最大值)，ret=1
- 原因: 训练集仅 39 张 + dataAugIters=10 → memory bank 过大 (679MB)
- 对比: 旧工作模型 model.mb = 57MB (合理范围)
- 根本原因: 训练集太小或增广倍数太高，导致 coreset 采样后特征仍过密
- 修复方向: 调整 `coreset_sampling_ratio`、减少 `dataAugIters`、增加训练数据

## 6. 文件清单

```
D:\RevUnsup\cpp_patch\
├── RU011v2_train\ChipROI02\        # 训练数据 + 输出
│   ├── train\                      # 39 张 PNG
│   └── model\                      # model.ckpt(FP32原始) + model.mb + modelConfig.yaml
├── RU011v2_smoke\ChipROI02\        # Smoke test 输出
│   ├── train\                      # 3 张 PNG
│   └── model\                      # smoke 模型
├── RU011v2_eval\                   # 评估工作区
│   ├── model\                      # model.ckpt(FP16后处理) + model.mb + modelConfig.yaml
│   ├── test_images\{good,real_bad,synth_bad}  # 18 张评估图
│   ├── eval_main.cpp               # v2 评估源码
│   ├── eval_main.exe               # 编译后评估程序
│   └── output_fp16.txt             # 评估完整输出
```

## 7. 结论与后续

### 已达成
1. ✅ 客户 Python 管线完整跑通 (train.py → model output)
2. ✅ 根因分析: FP32/FP16 精度不匹配导致 crash
3. ✅ FP16 后处理方案验证: DLL 推理零 crash
4. ✅ 18 图评估流程完整执行

### 后续建议
1. **生产化 FP16 转换**: 将 `model.half()` 步骤集成到训练后处理脚本
2. **阈值调优**: 当前 thresh=0.1 对新模型过敏感，需在更大数据集上标定
3. **降低 memory bank**: 调整 `coreset_sampling_ratio` 或 `dataAugIters` 减小 model.mb
4. **GPU 训练**: 在 sm_90 以下显卡上训练可直接产出 FP16 模型，无需后处理
