# RU-005-fp16-mb KNN 加速 Phase 1: cdist compute_mode + FP16 Autocast + MB FP16

> 任务编号: RU-005-fp16-mb
> 创建日期: 2026-04-13
> 优先级: P0 (主线杠杆, RU-004 FAISS 失败后的 Phase 1 方案)
> 执行人: DogWind (XYC_Windsurf on 5060)
> 预估时间: 2-3 小时
> 依赖: RU-004 ✅ (baseline_cdist.json 锁定)
> 参考调研: `/tmp/knn_acceleration_research.md` (Napoleon 已 WebSearch 验证)

---

## Q1 做什么

组合 3 个有官方支撑的技术, 把 GPU cdist 从 232.9ms 降到 **≤40ms** (5.8x 加速):

1. **cdist compute_mode='use_mm_for_euclid_dist'** (官方 PyTorch 参数, 预估 ~2x)
2. **FP16 Autocast** (Tensor Core 硬件加速, V100 官方 8x, 5060 Ti 保守 3-6x)
3. **Memory Bank FP16 存储** (带宽减半, 预估 1.3x)

组合保守估计: 5-10x → 232ms 到 25-45ms ✅

**变量隔离**: 只改 KNN 这一步, 其他代码冻结。

---

## Q2 看到什么 (前置资源)

### 必需资源 (已确认存在)
- **baseline_cdist.json**: `D:\RevUnsup\docs\baseline_cdist.json` (RU-004 锁定的)
- **参考脚本**: `D:\sanity_profile.py` (已有 warmup + sync + 分段计时)
- **你 RU-004 改好的 chunked matmul 代码** (在你的工作目录, 1.25x 已验证)
- **模型**: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\`
- **6 张测试图** (必须用同一套, 不准换):
  1. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\2.jpg`
  2. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\5.jpg`
  3. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\034-169-301(1)-temp5.jpg`
  4. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\273-263-201(1)-temp5.jpg`
  5. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\274-244-202(1)-temp2.jpg`
  6. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\276-176-301(1)-temp5.jpg`

### 工具 (已有, 不需要新装)
- PyTorch 2.11.0 + cu128 (已用)
- `torch.cuda.amp.autocast` (PyTorch 内置, 不需要 pip install)
- anomalib2.2 conda 环境 (RU-004 用的那个)

---

## Q3 记得什么 (必读上下文)

### 必读 — Napoleon 的调研文档
- Phase 1 的 3 个技术都有官方支撑 (见任务描述顶部)
- 为什么不用 FAISS-CPU: RU-004 验证变慢 8.6x (GPU 起点下不适用)
- 为什么不用 PCA 降维: ReConPatch 论文验证 128→64 会明显掉精度

### 必读 — RU-004 的结果
- `docs/baseline_cdist.json` (GPU cdist baseline)
- `docs/exp_001_faiss.json` (FAISS 失败记录, 留作对照)
- `docs/exp_001_report.md` (chunked matmul 1.25x 的方法)

### 关键参考数据
- cdist mean = 232.93ms (std 1.42ms, 非常稳)
- 总推理 mean = 294ms
- cdist 占 79.2%
- Memory Bank: Patch[0]=17891×128, Patch[1]=6586×128
- 精度偏差 baseline 要求 < 5%

### 已知陷阱 (继续记住)
1. ⚠️ `thresh = pow(thresh, 2.0)` — 只影响阈值判断, 不影响 cdist 本身
2. ⚠️ Tensor Core 要求 dim 是 8/16 倍数 → 当前 128 ✅ 是 16 倍数, 天然适配
3. ⚠️ FP16 数值范围 [-65504, 65504], 欧氏距离的平方可能溢出 — 需要测试

---

## Q4 不能做什么 (红线)

### 算法/数据红线
- ❌ **不换测试图** — 严格用 Q2 列的 6 张, 跟 RU-004 同一套
- ❌ **不跑 best-of-N** — mean ± std, 每张图 5 次
- ❌ **不改 modelConfig.yaml / 不重训模型**
- ❌ **不做 PCA 降维** (论文反证, 不要走这条路)
- ❌ **不装 FAISS-GPU 或 KeOps** (Windows 风险已验证)

### 代码红线
- ❌ **不改除 cdist (及其 FP16/compute_mode) 之外的代码**
- ✅ 允许改的地方只有:
  - `bank = bank.half()` (MB FP16)
  - `with autocast(dtype=torch.float16): dist = torch.cdist(query, bank, compute_mode='use_mm_for_euclid_dist')`
  - 其他保持 FP32

### Git 红线
- ❌ 不删别人文件
- ❌ 不改 baseline_cdist.json
- ❌ 不 force push
- ✅ 只新增: docs/exp_002_fp16.json + docs/exp_002_report.md

---

## Q5 怎么知道对了 (验收)

### 验收 1: 产出 `docs/exp_002_fp16.json`

同 RU-004 的 JSON 格式, 6 张图 × 5 次:
```json
{
  "date": "2026-04-13",
  "based_on": "baseline_cdist.json",
  "config": {
    "compute_mode": "use_mm_for_euclid_dist",
    "autocast": "fp16",
    "memory_bank_dtype": "fp16"
  },
  "runs_per_image": 5,
  "images": [
    {
      "name": "2.jpg",
      "times_ms": [...5 次...],
      "mean_ms": X,
      "std_ms": Y,
      "knn_mean_ms": Z,
      "anomaly_score": (跟 baseline 对比),
      "ret": 1 或 0
    },
    ...6 张...
  ],
  "summary": {
    "total_mean_ms": X,
    "knn_mean_ms": Z,
    "speedup_knn": "232.93 / Z 倍",
    "speedup_total": "294 / X 倍",
    "max_score_deviation_pct": N,
    "all_ret_match": true,
    "precision_pass": N < 5
  }
}
```

### 验收 2: 精度对齐 (反 reward hacking)
- 每张图的 `anomaly_score` 跟 baseline 对比
- 偏差 < 5% 才算合格
- `ret` 必须跟 baseline 完全一致
- 如果偏差 > 5% → 走 L 灯塔

### 验收 3: 产出 `docs/exp_002_report.md` (20-30 行)
包含:
- 改动的 3 行代码 (贴出来)
- 加速比 (knn 和 total)
- 精度对齐结果
- 对比 RU-004 的 chunked matmul (哪个快)
- 通过/不通过 判定

### 交付方式
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs/exp_002_fp16.json docs/exp_002_report.md
git diff --cached --stat   # 自检: 2 个新增, 无删除
git commit -m "RU-005: FP16+autocast+mb_fp16 KNN 加速 (232ms -> Xms, Y倍)"
git push
```

---

## L 灯塔 (主路径不通时)

### 主路径
3 组合同时启用:
- compute_mode='use_mm_for_euclid_dist'
- autocast(dtype=torch.float16)
- bank 存 FP16

### Plan B: 精度掉 > 5%
可能是 FP16 溢出 (欧氏距离的平方过大)。
→ 先试只开 compute_mode (不开 autocast), 看能不能到 100ms 级
→ 或者 autocast 用 bfloat16 代替 float16 (精度更好, 速度略慢)

### Plan C: FP16 溢出
用 bfloat16 (数值范围更大, 精度跟 FP32 差距更小):
```python
with autocast(dtype=torch.bfloat16):
    dist = torch.cdist(query, bank_bf16)
```

### Plan D: Tensor Core 没激活 (加速比 < 2x)
- 检查 dim / batch 是否对齐 8/16 倍数 (应该 OK)
- 检查 PyTorch 2.11 + sm_120 是否正确识别 Tensor Core
- 跑 `torch.cuda.get_device_capability()` 确认是 (12,0) Blackwell

### Plan E: 整个方案都不行
- 停下, commit 一个 blocker issue
- 贴具体错误, 报告羊爸爸
- 等 Phase 2 决策 (Feature Pooling 或其他)

---

## 实现路线 (明确代码层)

### Step 1: 修改推理函数 (只改 3 行)

在你 RU-004 改好的 chunked matmul 代码里, 核心改动:

```python
# 原 RU-004 chunked matmul:
# bank 是 FP32 存储
# query 是 FP32
# 手动 chunked matmul

# ↓ RU-005 改动 ↓

# 1. Memory Bank 量化 (只做一次, 加载时)
bank_fp16 = bank.to('cuda').half()

# 2. 推理循环里加 autocast
from torch.cuda.amp import autocast

for patch in patches:
    features = backbone(patch)  # FP32
    
    with autocast(dtype=torch.float16):
        # compute_mode 在 autocast 里会自动走 matmul 路径
        # 如果用 cdist (不用你的 chunked 版本), 加 compute_mode 参数
        dist = torch.cdist(features, bank_fp16, 
                           compute_mode='use_mm_for_euclid_dist')
    
    # 后续需要 FP32 精度时 cast 回来
    dist_fp32 = dist.float()
    # ... 阈值判断, anomaly map ...
```

### Step 2: 跑 6 张图 × 5 次, 记录 mean/std
- 按 `sanity_profile.py` 的分段计时方式
- 需要 `torch.cuda.synchronize()` + warmup (你之前加过)

### Step 3: 精度对齐
- 每张图的 anomaly_score 跟 baseline_cdist.json 的同一张图对比
- 偏差 = abs(new - baseline) / baseline
- 必须 < 5%

### Step 4: 写 report.md

---

## 备注

- 这次测试**只证明 Phase 1 方案有效**, 不做 Phase 2 (Feature Pooling)
- 如果 Phase 1 到 40ms 内, 任务完成, Phase 2 留灯塔
- 如果 Phase 1 到 50-100ms, Napoleon 会跟羊爸爸讨论是否进 Phase 2
- 如果 Phase 1 失败 (< 2x), 走 Plan D 或 E, 报告根因

---

*任务文档 v1.0 / 2026-04-13 / 基于 Napoleon knowledge-grounded 调研 / 第二个杠杆实验*
