# RU-004-knn-faiss 第一个杠杆实验: FAISS 替换 cdist

> 任务编号: RU-004-knn-faiss
> 创建日期: 2026-04-13
> 优先级: P0 (主线阻塞, 是五层架构 L1 Worker 层的第一次真正落地)
> 执行人: DogWind (XYC_Windsurf on 5060)
> 预估时间: 2-3 小时 (含 baseline 锁定)
> 依赖: RU-001 ✅ + OM-001 ✅ + RU-002-sanity ✅
> 参考: sanity_profile.py (D:\sanity_profile.py, 已有 warmup+sync+分段计时)

---

## Q1 做什么

用 FAISS IVFFlat 替换 anomalyDet 推理流程里的 `torch.cdist`, 把 KNN 搜索耗时从 **~367ms** 压到 **≤ 40ms** (9x+ 加速), 同时保证精度偏差 < 5%。

**核心原则**: 变量隔离 — 只改 cdist 这一步, 其他(预处理/forward/后处理)全部冻结。

---

## Q2 看到什么 (前置资源)

### 必需资源 (已确认存在)
- **参考脚本**: `D:\sanity_profile.py` (你上次自己写的, 质量很高)
- **模型**: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\` (真实业务模型, Memory Bank ~24477 embeddings)
- **6 张测试图** (完整列表, 不准换):
  1. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\2.jpg` (封装版自带)
  2. `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\5.jpg` (封装版自带)
  3. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\034-169-301(1)-temp5.jpg`
  4. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\273-263-201(1)-temp5.jpg`
  5. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\274-244-202(1)-temp2.jpg`
  6. `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\276-176-301(1)-temp5.jpg`

### 工具
- Python anomalib2.2 conda 环境 (你 sanity check 用的那个)
- FAISS 需要 pip install:
  ```powershell
  D:\Softwares\Anaconda\envs\anomalib2.2\python.exe -m pip install faiss-cpu
  ```
  ⚠️ **只装 faiss-cpu, 不要装 faiss-gpu** (OM-001 验证过 Windows 上是黑洞)

### 资源缺失时的动作
- 任何路径找不到 → `git commit -am "blocker: RU-004 缺 <资源>"` + push + 微信告诉羊爸爸
- FAISS 安装失败 → 同上, 不要自己搞 conda 乱装

---

## Q3 记得什么 (必读上下文)

### 必读文档
- `docs/封装版代码地图.md` (你自己写的 RU-001)
- `docs/杠杆清单.md` (RU-003, S 级杠杆 A1 就是 FAISS)
- `docs/预实验经验清单.md` (OM-001, §7 复用 #1 是 FAISS 方案)
- `docs/阈值校准协议.md` (pow(thresh,2) 陷阱)

### 已知陷阱 (别再踩)
1. ⚠️ `thresh = pow(thresh, 2.0)` — 只影响阈值判断, 不影响 KNN 搜索本身
2. ⚠️ 输出是灰度热力图, 不是二值 — 精度对齐时用连续值对比, 不是 mask
3. ⚠️ modelInputW/H 硬编码 — baseline 阶段不碰

### 关键参数 (来自 OM-001 验证)
- FAISS IVFFlat: `nlist=100, nprobe=10` (67x 加速的配置)
- Memory Bank 当前规模: Patch[0]=17891×128, Patch[1]=6586×128
- 因为规模只是 OM-001 的 37%, 加速比可能压缩到 10-20x (仍然够)

---

## Q4 不能做什么 (红线)

### 算法/数据红线
- ❌ **不换测试图** — 就是这 6 张, 全跑, 不能挑
- ❌ **不跑多次取最快值** — 要 mean ± std, 不是 best-of-N
- ❌ **不改 modelConfig.yaml**
- ❌ **不改除 cdist 之外的任何代码** (预处理/forward/后处理冻结)
- ❌ **不用 FAISS-GPU** — OM-001 验证 Windows 是黑洞, 只用 faiss-cpu
- ❌ **不改 baseline_cdist.json** 一旦写入就锁定

### Git 红线 (2026-04-09 事故教训)
- ❌ **不删除你没创建的文件**
- ❌ **不改其他 Agent 的产出** (.persona/, wiki/, 其他 tasks/)
- ❌ **不 force push**
- ✅ 提交前 `git diff --cached --stat` 自检, 确认只有新增: docs/baseline_cdist.json + docs/exp_001_faiss.json + docs/exp_001_report.md

---

## Q5 怎么知道对了 (验收)

完成判定 (3 条全满足):

### 验收 1: Baseline 锁定
产出 `docs/baseline_cdist.json`:
```json
{
  "date": "2026-04-13",
  "config": {"cdist": "torch.cdist", "thresh_input": 0.1, "thresh_eff": 0.01},
  "memory_bank": {"patch0_shape": [17891, 128], "patch1_shape": [6586, 128]},
  "runs_per_image": 5,
  "images": [
    {"name": "2.jpg", "times_ms": [480.1, 478.3, 481.7, 479.2, 480.5], "mean_ms": 479.96, "std_ms": 1.2, "cdist_mean_ms": 367.3, "anomaly_score": 1.093, "ret": 1},
    ...共 6 张...
  ],
  "summary": {
    "total_mean_ms": X,
    "total_std_ms": Y,
    "cdist_mean_ms": Z,
    "cdist_pct": "76.5%"
  },
  "locked": true
}
```
**锁定**: 此文件一旦写入, 后续任何实验不许改。

### 验收 2: FAISS 实验结果
产出 `docs/exp_001_faiss.json` (同样 30 次数据):
```json
{
  "date": "2026-04-13",
  "based_on": "baseline_cdist.json",
  "config": {"cdist": "faiss.IndexIVFFlat", "nlist": 100, "nprobe": 10},
  "runs_per_image": 5,
  "images": [
    {"name": "2.jpg", "times_ms": [...], "mean_ms": X, "std_ms": Y, "knn_mean_ms": Z, "anomaly_score": 1.091, "ret": 1},
    ...共 6 张...
  ],
  "summary": {
    "total_mean_ms": X,
    "knn_mean_ms": Z,
    "speedup_knn": "367.3 / Z 倍",
    "speedup_total": "..."
  }
}
```

### 验收 3: 精度对齐
对每张图, 计算 `abs(baseline_score - faiss_score) / baseline_score`:
- **每张图都 < 5%** 才算合格
- 任何一张 > 5% → FAISS 配置不合格, 要走 Plan B (nprobe=20) 或 Plan C (FlatL2)
- ret (0/1) 必须一致

### 产出 `docs/exp_001_report.md` (20 行以内)
简短报告, 含:
- baseline vs faiss 的 KNN 耗时对比表 (6 张图)
- 整体加速比 (mean)
- 精度偏差的最大值
- 通过/不通过 判定

### 交付方式
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs/baseline_cdist.json docs/exp_001_faiss.json docs/exp_001_report.md
git diff --cached --stat  # 自检, 只有 3 个新增
git commit -m "RU-004: KNN FAISS 加速 (367ms → Xms, Y倍)"
git push
```

---

## L 灯塔 (主路径不通时)

### 主路径
FAISS IVFFlat, `nlist=100, nprobe=10` (OM-001 验证)

### Plan B: 精度掉 > 5%
→ 改 `nprobe=20` (更精确, 但慢一点)
→ 重跑 Step 2, 输出 `exp_001b_faiss_nprobe20.json`

### Plan C: Plan B 还不行
→ 退回 FAISS FlatL2 (精确但只有 20x 加速, 不做近似)
→ 应该能保证精度 100% 对齐

### Plan D: FAISS 本身装不上
→ 先别搞 conda 环境
→ commit 一个 issue 报告停下: 具体报错 + 你试了什么
→ 等羊爸爸/Napoleon 决定

### Plan E: 跑着跑着 OOM / 崩
→ 停下, 记录具体错误
→ 不要瞎改 batch size / memory
→ 报告

---

## 这个任务的更深意义

这不只是一次性的 "跑个 FAISS"。

按【杠杆实验 5 步标准流程】, 这个任务是:
- Step 1 锁 baseline (验收 1)
- Step 2 实施杠杆 (FAISS 替换 cdist)
- Step 3 对比数字 (验收 2)
- Step 4 精度对齐 (验收 3)
- Step 5 Napoleon 独立验证 (你 push 后我 SSH 过去跑一遍)

做完这一轮, 我们就有了:
- 【一本可复用的实验账本格式】(baseline_*.json + exp_*.json + report.md)
- 【防 reward hacking 的机制】(锁定 baseline + 精度对齐)
- 【下一个杠杆的模板】(降采样 / 量化 / TensorRT 都走同样 5 步)

这是论文里 L3 评估层的第一个实战。

---

## 备注

- 不追求完美, 追求诚实
- 跑不通任何一步都立即停下报告, 不要硬上
- 完成后等 Napoleon SSH 过去独立验证 (Step 5)
- 时间预算 2-3 小时, 超时也没关系, 但要说明超时原因

---

*任务文档 v1.0 / 2026-04-13 / 按 5Q+1L 模板 / 第一个真正的杠杆实验*
