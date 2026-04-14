# RU-007-phase3a Forward 零成本加速组合

> 任务编号: RU-007-phase3a
> 创建日期: 2026-04-14
> 优先级: P0
> 执行人: DogWind (XYC_Windsurf on 5060)
> 预估时间: 1-1.5 小时
> 依赖: RU-006 ✅ (max_pool_4x 8.8ms KNN, 总 71.3ms)
> 目标: 总推理 71.3ms → ≤ 40ms (降 44%+)

---

## Q1 做什么

在 RU-006 的 max_pool_4x 代码基础上, 叠加 3 个零成本 flag:

1. **torch.compile(mode='reduce-overhead')** — [PyTorch 官方]
2. **torch.backends.cudnn.benchmark = True** — [PyTorch 官方]
3. **channels_last 内存格式** — [PyTorch 官方]

全部有官方文档支撑, 改动 < 10 行代码, 不装新库。

预估: Forward 41ms → 22-30ms, 总推理 71.3ms → 50-55ms (1.3-1.5x)

**变量隔离**: 不改 KNN, 不改 max_pool_4x, 不改预处理, 不测精度。

---

## Q2 看到什么 (前置资源)

### 必需资源 (已确认存在, 都在 5060 本地)
- **RU-006 代码**: 你 max_pool_4x 的推理脚本 (FP16 + chunked matmul + pool)
- **baseline**: `D:\RevUnsup\docs\baseline_cdist.json` (294ms total, RU-004 锁定)
- **参考数据 (1x 基线)**:
  - RU-006 `docs/exp_003_fp_scan.json` 的 `max_pool_4x` 配置: KNN 8.8ms, 总 71.3ms
- **模型**: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\`
- **6 张测试图** (严格复用, 不换):
  1. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\2.jpg`
  2. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\5.jpg`
  3. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\034-169-301(1)-temp5.jpg`
  4. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\273-263-201(1)-temp5.jpg`
  5. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\274-244-202(1)-temp2.jpg`
  6. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\276-176-301(1)-temp5.jpg`

### 工具 (已有)
- PyTorch 2.11.0+cu128 ✅ (已兼容 Blackwell sm_120)
- torch.compile (PyTorch 2 内置)
- 不需要 pip install 任何东西

---

## Q3 记得什么 (必读上下文)

### 必读
- `tasks/RU-006-fp-pooling-scan.md` (上次任务)
- `docs/exp_003_report.md` (RU-006 结果, max_pool_4x 最快)
- Napoleon 的调研文档 (见 /tmp/forward_acceleration_research.md, 如果你本地没有, 看 git log 可以找到)

### 3 个方法的关键点

**torch.compile** [直接·PyTorch 官方]
- 'reduce-overhead' 模式用 CUDA graphs 减少 CPU 开销
- ResNet18 on Titan V 实测 25-30% 加速
- ⚠️ 首次跑慢几秒 (JIT 编译), 需要 warmup 3+ 次再测

**cudnn.benchmark = True** [直接·PyTorch 官方]
- 输入固定时自动选最快 kernel
- 我们的 patch 尺寸固定 (2048×2048) ✅
- 通用 1.1-1.3x

**channels_last** [直接·PyTorch 官方]
- NHWC 内存格式, 对 Conv/BN 友好
- 跟 FP16 Tensor Core 配合更好
- 我们已经用 FP16 ✅

### 历史数字 (验证用)

| 任务 | KNN | 总 | 关键技术 |
|---|---|---|---|
| RU-004 baseline | 232.9ms | 294ms | GPU torch.cdist |
| RU-004 chunked | 185.8ms | 247ms | FP32 chunked matmul |
| RU-005 | 71.4ms | 134ms | FP16 + Tensor Core |
| RU-006 max_pool_4x | **8.8ms** | **71.3ms** | + pool query reduce |
| RU-007 (本次) 目标 | — | ≤ 40ms | + 3 个 flag |

---

## Q4 不能做什么 (红线)

### 代码红线
- ❌ **不改 RU-006 的 FP16 chunked matmul 代码** (已经最优)
- ❌ **不改 max_pool_4x 配置** (RU-006 的冠军配置, 保留)
- ❌ **不改模型本身 / 不改 backbone**
- ❌ **不动预处理** (留给 Phase 3B)
- ✅ 只加 3 处:
  - 开头: `torch.backends.cudnn.benchmark = True`
  - 模型加载后: `model = model.to(memory_format=torch.channels_last)` 和 `model = torch.compile(model, mode='reduce-overhead')`
  - 输入: `input.to(memory_format=torch.channels_last)`

### 测试红线
- ❌ **不换测试图** (同 6 张)
- ❌ **不跑 best-of-N** (mean ± std)
- ❌ **不测精度** (按用户决定, 只测速度)
- ✅ warmup: 跑 **5 次** 再开始计时 (torch.compile 需要, 不能像 RU-005/006 那样跑 1 次 warmup)

### Git 红线
- ❌ 不删别人文件 (2026-04-09 事故教训还记得吧?)
- ❌ 不改已有的 baseline/exp_001/exp_002/exp_003 文件
- ❌ 不 force push
- ✅ 只新增: docs/exp_004_compile.json + docs/exp_004_report.md

---

## Q5 怎么知道对了 (验收)

### 验收 1: 产出 `docs/exp_004_compile.json`

同之前的格式, 6 张图 × 5 次:
```json
{
  "date": "2026-04-14",
  "based_on": "exp_003_fp_scan.json (max_pool_4x config)",
  "config": {
    "base": "RU-006 max_pool_4x (FP16 + chunked matmul + max_pool kernel=4)",
    "added_flags": {
      "cudnn_benchmark": true,
      "channels_last": true,
      "torch_compile": "mode=reduce-overhead",
      "warmup_iters": 5
    }
  },
  "runs_per_image": 5,
  "images": [
    {"name": "2.jpg", "times_ms": [...], "mean_ms": X, "std_ms": Y,
     "forward_mean_ms": Z, "knn_mean_ms": W, ...},
    ...6 张...
  ],
  "summary": {
    "total_mean_ms": X,
    "forward_mean_ms": Y,
    "knn_mean_ms": Z,
    "speedup_total_vs_RU006": "71.3 / X 倍",
    "speedup_forward_vs_RU006": "41 / Y 倍",
    "target_40ms": true/false
  }
}
```

### 验收 2: 分段计时要完整
- Forward (backbone) 单独计时 — 这是核心指标
- KNN 单独计时 — 应该跟 RU-006 的 8.8ms 接近 (不应该变)
- 预处理 + 后处理 — 应该跟 RU-006 接近
- 总推理 wall time

### 验收 3: 产出 `docs/exp_004_report.md` (15-25 行)
包含:
- 改动的 3 行代码
- Forward 加速比 (对比 RU-006 的 41ms)
- 总推理加速比 (对比 71.3ms)
- 是否达到 40ms 目标
- 每个 flag 的独立效果 (如果 torch.compile 失败了, 其他 2 个应该还在)

### 交付方式
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs\exp_004_compile.json docs\exp_004_report.md
git diff --cached --stat    # 自检: 2 个新增, 无删除
git commit -m "RU-007: Phase 3A 零成本组合 (compile+cudnn+channels_last)"
git push
```

---

## L 灯塔 (主路径不通时)

### Plan A (主路径)
3 个 flag 同时开, warmup 5 次后测

### Plan B: torch.compile 在 Blackwell 崩
可能原因: sm_120 的 torch.compile 稳定性
→ 去掉 `torch.compile(...)`, 保留 cudnn + channels_last
→ 单独测这 2 个的组合效果
→ 报告 torch.compile 失败原因

### Plan C: channels_last 某个算子不支持
会看到 UserWarning 说 fallback 到 contiguous
→ 去掉 channels_last, 只保留 torch.compile + cudnn
→ 单独测这个组合

### Plan D: 3 个都失败
→ 退回 RU-006 代码, 报告所有失败原因
→ 这本身也是有价值的发现 (Blackwell + PyTorch 2.11 的零成本优化不可用)

### Plan E: 加速 < 10%
→ 说明零成本方案天花板很低
→ 如实报告, 进 Phase 3B/3C 讨论

---

## 实现路线 (明确代码)

### Step 1: 读 RU-006 你自己的代码
找到你 RU-006 跑 max_pool_4x 的脚本, 这是基础。

### Step 2: 加 3 处改动

```python
import torch
import torch.nn.functional as F
from torch.cuda.amp import autocast

# === 改动 1: 程序开头 ===
torch.backends.cudnn.benchmark = True

# === 改动 2: 模型加载后 ===
model = torch.jit.load(model_path, map_location=device)
model.eval()
# 新增: channels_last
model = model.to(memory_format=torch.channels_last)
# 新增: compile (注意: 第一次跑会卡几秒编译)
model = torch.compile(model, mode='reduce-overhead')

# === 改动 3: 输入也转 channels_last ===
# 在预处理后, 喂入模型前:
tensor_image = tensor_image.to(memory_format=torch.channels_last)
```

### Step 3: Warmup 加到 5 次
```python
# 跑 6 张图之前, 用第 1 张图做 5 次 warmup
print("Warming up torch.compile (5 iters)...")
for i in range(5):
    _ = inference(dummy_image)
    torch.cuda.synchronize()
print("Warmup done.")
```

### Step 4: 跑 6 张 × 5 次, 按 RU-006 格式记录

### Step 5: 写 report

---

## 备注

- 这是 Phase 3A, 如果到了 40ms 就完美
- 如果只到 50-55ms, Phase 3B (GPU 预处理) 是下一步
- 如果到了 30-40ms, Phase 3C (TensorRT) 不需要做
- 精度不测 (用户决定, 甲方上线自测)

---

*任务 v1.0 / 2026-04-14 / 零成本 Forward 加速组合*
