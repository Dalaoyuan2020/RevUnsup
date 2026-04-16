# RU-011-v2 交接文档

> **状态**: handover-ready  
> **日期**: 2026-04-16  
> **执行者**: DogWind  

## 交接要点

### 1. 客户训练管线已验证可用
- 环境: `custom_train` conda env (Python 3.8.20 + PyTorch 2.0.1 + cu118)
- 命令: `python train.py --dataset_path <DIR> --category <NAME> --modelInputW 1000 --modelInputH 1000 --num_epochs 50`
- 工作目录必须是 `ad_code/train/` (因为 pyd 模块和 YAML 配置在此)
- con1.yaml 会被训练回写，建议先备份

### 2. FP16 后处理是必须步骤
客户管线在 CPU 上训练时产出 FP32 模型，而 DLL 期望 FP16。必须在训练后执行:
```python
import torch
m = torch.jit.load('model.ckpt', map_location='cpu')
m = m.half()
torch.jit.save(m, 'model.ckpt')
```

### 3. 已知问题
- Good 图误报率 100% (9/9)：训练数据量不足 + memory bank 过大 (679MB vs 正常 57MB)
- 训练在 5060 Ti 上自动 fallback 到 CPU (sm_120 不兼容 PyTorch 2.0.1)
- pip SSL 全局故障，需用 PowerShell 下载 wheel 或用 conda 安装

### 4. 文件位置
- 报告: `docs/exp_011v2_chiproi02_customer_train.md`
- 训练输出: `cpp_patch/RU011v2_train/ChipROI02/model/`
- 评估输出: `cpp_patch/RU011v2_eval/output_fp16.txt`
- 评估源码: `cpp_patch/RU011v2_eval/eval_main.cpp`

### 5. Napoleon 需关注
1. FP16 转换应集成到客户交付的训练后处理流程
2. 阈值调优需在客户环境 + 完整数据集上进行
3. v1 crash 根因已确认，可关闭 RU-011 v1 遗留问题
