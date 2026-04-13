# RU-006-fp-pooling-scan Feature Pooling 速度扫描

> 任务编号: RU-006-fp-pooling-scan
> 创建日期: 2026-04-14
> 优先级: P0
> 执行人: DogWind (XYC_Windsurf on 5060)
> 预估时间: 1-1.5 小时
> 依赖: RU-005 ✅ (FP16 chunked matmul 已跑通 71.4ms)

---

## Q1 做什么

在 RU-005 的 FP16 代码基础上, 加 Feature Pooling 层, 拉【速度-pool 类型-缩小比例】曲线。
**不测精度** (甲方上线自测)。

验证 2 种 pool × 2 种比例 = 4 个配置, 加 RU-005 基线 = 5 个数据点。

---

## Q2 看到什么 (前置资源)

### 必需资源 (已确认存在)
- **RU-005 代码**: 你上次改好的 FP16 + chunked matmul 脚本
- **baseline**: `D:\RevUnsup\docs\baseline_cdist.json` (RU-004 锁定, 作为 0 倍基线)
- **RU-005 数据**: `D:\RevUnsup\docs\exp_002_fp16.json` (71.4ms 作为 1x 基线)
- **模型**: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\`
- **6 张测试图**: 严格复用 RU-004/005 的同一套 (tasks/RU-005 Q2 有完整路径)

### 工具
- PyTorch 2.11 (已有)
- `torch.nn.functional.avg_pool2d` / `max_pool2d` (PyTorch 内置, 不需要装任何东西)
- matplotlib (如果没装: `pip install matplotlib`)

---

## Q3 记得什么 (必读上下文)

### 必读
- `tasks/RU-005-fp16-mb.md` (上次任务, 你的代码基础)
- `docs/exp_002_report.md` (RU-005 结果, 71.4ms KNN)
- 本任务整体设计思路: 不动 backbone input, 只在 backbone 输出后加 pool

### 关键前提
- RU-005 已经验证: FP16 chunked matmul = 71.4ms KNN (3.26x)
- RU-006 是在这个基础上, 【加】一个 pool 步骤
- 不替换任何已有代码, 只多加一行 `features = pool(features)`

### 已知数字 (作为参考)
- Query 当前: 每 patch 65536 个 (256×256 feature map)
- pool(2) 后: 每 patch 16384 (128×128 feature map, 4x less)
- pool(4) 后: 每 patch 4096 (64×64 feature map, 16x less)
- 理论 KNN 加速: 4x / 16x

---

## Q4 不能做什么 (红线)

### 算法红线
- ❌ **不改 backbone input** (S1 缩图太激进, 不做)
- ❌ **不改 modelConfig.yaml / 不重训**
- ❌ **不测精度** (只测速度, 精度甲方上线自测)
- ❌ **不换测试图** (严格同一套 6 张)
- ❌ **不跑 best-of-N** (mean ± std)

### 代码红线
- ❌ **不改 RU-005 已有的 FP16 / chunked matmul 代码** (保持)
- ✅ 只允许加 1 行: `features = F.avg_pool2d(features, N)` 或 `max_pool2d`
- 其他代码都冻结

### Git 红线
- ❌ 不删别人文件
- ❌ 不改 baseline_cdist.json / exp_001 / exp_002
- ❌ 不 force push
- ✅ 只新增: docs/exp_003_fp_scan.json + docs/exp_003_report.md + docs/exp_003_curve.png (可选)

---

## Q5 怎么知道对了 (验收)

### 验收 1: 4 个配置全跑完 + 基线作对照

产出 `docs/exp_003_fp_scan.json`:

```json
{
  "date": "2026-04-14",
  "based_on": "exp_002_fp16.json",
  "runs_per_image": 5,
  "configs": [
    {
      "name": "baseline_no_pool",
      "pool_type": null,
      "pool_kernel": 1,
      "patches_per_image": 65536,
      "images": [
        {"name": "2.jpg", "times_ms": [...], "knn_mean_ms": 71.4, ...},
        ...6 张...
      ],
      "summary": {"total_mean_ms": X, "knn_mean_ms": Y}
    },
    {
      "name": "avg_pool_2x",
      "pool_type": "avg",
      "pool_kernel": 2,
      "patches_per_image": 16384,
      "images": [...6 张 × 5 次...],
      "summary": {...}
    },
    {
      "name": "avg_pool_4x",
      ...
    },
    {
      "name": "max_pool_2x",
      ...
    },
    {
      "name": "max_pool_4x",
      ...
    }
  ]
}
```

### 验收 2: 速度对比曲线

产出 `docs/exp_003_curve.png`:
- X 轴: 缩小比例 (1x, 4x, 16x)
- Y 轴: KNN 耗时 (ms)
- 2 条线: avg_pool 和 max_pool
- 标注每个点的具体数字

简易 matplotlib 代码 (可参考):
```python
import matplotlib.pyplot as plt
plt.plot([1, 4, 16], [71.4, avg_2x_knn, avg_4x_knn], 'o-', label='avg_pool')
plt.plot([1, 4, 16], [71.4, max_2x_knn, max_4x_knn], 's-', label='max_pool')
plt.xlabel('Query reduction (×)')
plt.ylabel('KNN time (ms)')
plt.xscale('log')
plt.legend(); plt.grid(True)
plt.savefig('docs/exp_003_curve.png', dpi=100)
```

### 验收 3: 文字报告

产出 `docs/exp_003_report.md` (15-25 行):
- 5 个配置的速度对比表
- avg vs max 的速度差异观察
- 极限加速比 (max reduction 的 KNN 数字)
- 下一步建议 (空间还够吗? 要继续降维吗?)

---

## 交付方式

```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs\exp_003_fp_scan.json docs\exp_003_report.md docs\exp_003_curve.png
git diff --cached --stat   # 自检, 2-3 个新增无删除
git commit -m "RU-006: FP pooling speed scan (2x/4x × avg/max)"
git push
```

---

## L 灯塔

### Plan A (主路径): 4 个配置全跑
预期都能跑完, 1-1.5 小时

### Plan B: pool 后输出形状不对, backbone 报错
可能原因: 分块 patch 的 feature map 维度跟 pool kernel 不整除
→ 调整 pool 的 `stride` 和 `padding`, 或者用 `F.adaptive_avg_pool2d((H//N, W//N))`
→ 详见: https://pytorch.org/docs/stable/generated/torch.nn.functional.adaptive_avg_pool2d.html

### Plan C: 部分配置崩溃
只提交跑通的配置, 其他标 "failed, reason: ..."
至少交付 2 个配置的数据, 仍然能看速度趋势

### Plan D: matplotlib 没装 / 画不出图
跳过 curve.png, 用 markdown 表格代替
不是阻塞点

### Plan E: 整体跑不起来
commit 一个 blocker issue, 报告具体报错
不要硬改

---

## 备注

- 这是【不测精度】的速度扫描, 精度甲方上线自测
- 产出曲线后, 你写报告讲一下【极限在哪】
- 如果最快的配置到了 20ms 内, 一个天花板的知识
- 如果最快的还是 > 40ms, 说明 pool 不够, 需要更激进的方案 (但不做)

---

*任务 v1.0 / 2026-04-14 / 速度扫描, 不测精度*
