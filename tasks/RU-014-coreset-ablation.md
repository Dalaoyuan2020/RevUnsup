# RU-014 Coreset 激进采样消融 — v2 审核后修正版 (DogWind)

> 2026-04-17 · 目标: 验证 coreset_sampling_ratio=0.01 让 MB 从 712MB 降 → 实测(预计 50-200MB 区间),推理 745ms → 实测(预计 100-400ms 区间)
> 单变量对照实验,subagent 审核 ⚠️ 有条件通过后修正版

---

## 📌 v2 vs v1 变更说明 (审核修正)

审核发现 3 必修 + 2 个坑, v2 已全部修正:

1. **预期改现实** — 原 "→100ms" 改 "100-400ms 区间" (论文实测 10%→1% 只快 23%,faiss O(sqrt(N)) 非线性)
2. **加 Step 0** — 先验证 yaml coreset_sampling_ratio 生效路径,不能凭印象假设
3. **失败预案升级** — 精度崩先退 **0.03** 而非 **0.1**,resnet18 + 1% 未有论文直接证据
4. **训完必做 3 步** — dir model.mb → AUROC 回归 → 阈值重标定
5. **标注不确定性** — 论文消融基于 WideResNet50,我们用 resnet18,精度外推风险

---

## 🧠 背景 + 证据

### RU-011-v2 发现 (底基)
- 训练 39 图 × dataAugIters=10 → model.mb **712 MB**
- 推理 745 ms (KNN 主导)
- Good 9/9 全误报 (score 饱和)

### SSH + 代码双证据
- `train.py` 循环: 每图 × 10 次 forward 累积特征 → 最后一次 coreset 采 10%
- Smoke (3 图) MB = 57.4MB, 跟客户 57MB 一致
- Full (39 图) MB = 712MB
- **实测 13 倍图数 ↔ 12.4 倍 MB**,特征-MB 近线性

### 论文证据 (Roth 2022 CVPR + amazon-science 官方 repo)

**精度**:
| 配置 | Image AUROC | Pixel AUROC | PRO |
|------|-------------|-------------|-----|
| PatchCore-100% | 99.1 | 98.0 | 93.3 |
| PatchCore-10% (我们当前) | 99.0 | 98.1 | 93.5 |
| **PatchCore-1% (目标)** | **99.0** | **98.0** | **93.1** |

**推理时间 (faiss-on-gpu)**:
- 100% (no subsample): 0.6 s/图
- 10%: 0.22 s/图 (比 100% 快 2.7x, **非 10x**)
- 1%: 0.17 s/图 (比 10% 再快 23%)

**关键: faiss 索引 O(sqrt(N))/O(log N),MB 缩小 10 倍不代表推理快 10 倍**。

**官方默认**:
- baseline: `-p 0.1` (我们当前)
- Ensemble: `-p 0.01` ← **官方把 1% 当标配量级**

**风险标注**:
- 论文消融全部基于 WideResNet50 backbone
- 我们用 **resnet18**,特征分布更稀疏,1% coreset 精度保留性**未验证**
- 小样本 (39 图 × 10 aug) 场景下 1% 稳定性也未必完全等同 MVTec 标准量级

---

## 🎯 任务目标

**单变量改动: coreset_sampling_ratio 0.1 → 0.01,其他全不变**

成功判据 (调整后更现实):
- MB: 712 MB → **50-200 MB 区间都算成功** (10x 线性是乐观,featureSpan 可能非线性)
- 推理: 745 ms → **100-400 ms 区间都算成功** (faiss 次线性)
- 精度: Good 正确 ≥ 7/9 (容忍小回退), Bad 检测不降

---

## 📋 执行步骤 (v2 新增 Step 0 + 阈值重标定)

### Step 0: 验证 coreset_sampling_ratio 生效路径 (15 分钟,**必做**)

**目标**: 确认改 yaml 真的会改 coreset,不是代码硬编码 override。

```powershell
# 找到真正的 train.py
cd D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\train

# grep 关键字
findstr /N "coreset_sampling_ratio" train.py torch_model.py *.py

# 期望看到至少一行 `model.coreset_sampling_ratio = data['coreset_sampling_ratio']` 或类似
# 如果找不到,说明 yaml 路径有问题,停报告
```

**验证 smoke**:
```powershell
# 先把 yaml 里 coreset_sampling_ratio 改 0.01
# 跑 3 图 × 2 epoch smoke
# 检查输出的 model.mb 大小

# smoke MB 如果从之前 57MB → 5-6MB, 说明 yaml 生效了 ✅
# smoke MB 如果还是 57MB, 说明 yaml 没生效 ❌ 停报告
```

**如果找不到 grep 匹配 / smoke 验证失败**: 停,报告,不要硬推 Step 1。

### Step 1: 复用现有 env + 工作区 (10 分钟)

不重建 env。DogWind 之前已建 `custom_train` env 跑通训练。

```powershell
conda activate custom_train
python -c "import torch_model; print('pyd ok')"

mkdir D:\RevUnsup\cpp_patch\RU014_train\ChipROI02
mkdir D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model
xcopy "D:\RevUnsup\cpp_patch\RU011v2_train\ChipROI02\train" ^
      "D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\train\" /E /I /Y
```

### Step 2: 改 yaml (核心 1 行改动, 5 分钟)

```powershell
cd D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\train

# 备份
copy modelConfig.yaml modelConfig.yaml.RU014_backup

# 改
powershell -Command "(Get-Content modelConfig.yaml) -replace 'coreset_sampling_ratio: 0.1', 'coreset_sampling_ratio: 0.01' | Set-Content modelConfig.yaml"

# 验证
findstr /C:"coreset_sampling_ratio" modelConfig.yaml
```

### Step 3: 执行完整训练 (30-40 分钟, CPU 模式)

Step 0 验证过了才能跑 full。

```powershell
cd D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\train
conda activate custom_train

python train.py ^
  --phase train ^
  --dataset_path D:\RevUnsup\cpp_patch\RU014_train ^
  --category ChipROI02 ^
  --modelInputW 1000 ^
  --modelInputH 1000 ^
  --num_epochs 50 ^
  --model_dir D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\model.ckpt
```

### Step 4: 验证 MB 大小 (5 分钟)

```powershell
dir D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model
```

**判据**:
- MB 在 50-200 MB → ✅ 继续
- MB 仍 700+ MB → ❌ 回去查 Step 0 (yaml 没生效)
- MB 极小 <10 MB → ⚠️ 可能 coreset 崩了,报告

### Step 5: FP16 后处理 + 喂回 DLL 评估 (30 分钟)

```powershell
python -c "import torch; m=torch.jit.load(r'D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\model.ckpt').half(); torch.jit.save(m, r'D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\model.ckpt')"

mkdir D:\RevUnsup\cpp_patch\RU014_eval
xcopy "D:\RevUnsup\cpp_patch\RU011v2_eval\*" "D:\RevUnsup\cpp_patch\RU014_eval\" /E /I /Y

del D:\RevUnsup\cpp_patch\RU014_eval\model\model.ckpt
del D:\RevUnsup\cpp_patch\RU014_eval\model\model.mb
copy "D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\model.ckpt" "D:\RevUnsup\cpp_patch\RU014_eval\model\"
copy "D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\model.mb" "D:\RevUnsup\cpp_patch\RU014_eval\model\"
copy "D:\RevUnsup\cpp_patch\RU014_train\ChipROI02\model\modelConfig.yaml" "D:\RevUnsup\cpp_patch\RU014_eval\model\"

cd D:\RevUnsup\cpp_patch\RU014_eval
eval_main.exe > output_coreset_001.txt 2>&1
```

记录 18 图推理耗时 + ret + score。

### Step 6: 还原客户 yaml (强制, 2 分钟)

```powershell
cd D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\train
copy /Y modelConfig.yaml.RU014_backup modelConfig.yaml
findstr /C:"coreset_sampling_ratio" modelConfig.yaml
# 期望: 0.1
del modelConfig.yaml.RU014_backup
```

### Step 7: (新增) 阈值重标定探查 (20 分钟)

coreset 从 10% 降到 1%, 异常分数分布会变 → 老阈值 0.01 可能不再合适。

**做法**:
- 跑 18 图收集所有 score 值
- 看 good 和 bad 的 score 分布
- 理论最佳阈值 = good_max 和 bad_min 之间
- 如果发现老阈值导致全误报, 建议新阈值

记在报告里, **不真改客户配置**, 只提供参考。

### Step 8: 写对比报告 (30 分钟)

`docs/exp_014_coreset_ablation.md`:

```markdown
# RU-014 Coreset 1% 消融报告 (v2 修正版)

## Step 0 yaml 生效验证
- grep 结果: ...
- smoke MB: ... (是否从 57MB 缩到 ~5-6MB)
- 结论: yaml 生效 ✅/❌

## MB 大小对比

| 实验 | 图数 | coreset ratio | model.mb | 缩减比 |
|------|------|---------------|----------|--------|
| RU-011-v2 | 39 | 0.1 | 712 MB | 1x |
| RU-014 full | 39 | 0.01 | ? MB | ?x |
| 客户原模型 (参考) | ? | 0.1 声称 | 57 MB | — |

## 推理时间对比 (同 18 图集)

| 实验 | 平均推理 ms | 相对 RU-011-v2 加速 |
|------|------------|---------------------|
| RU-011-v2 (底基) | 745 | 1.0x |
| RU-014 | ? | ?x |

**预期**: 100-400 ms 区间都算成功 (faiss 次线性)

## 精度对比

| 类别 | RU-011-v2 正确率 | RU-014 正确率 |
|------|------------------|---------------|
| Good (9 张) | 0/9 | ?/9 |
| Real Bad (3 张) | 3/3 | ?/3 |
| Synth Bad (6 张) | 6/6 | ?/6 |
| 整体 | 9/18 | ?/18 |

## Step 7 阈值分布观察
- Good score 范围: ...
- Bad score 范围: ...
- 建议阈值: ...

## 结论
- MB 缩减是否符合预期: ✅/❌
- 速度目标达成: ✅/❌
- 精度未显著回退: ✅/❌
- 下一步建议: (参考 RU-014-b featureSpan 消融?)
```

---

## 🛑 硬性纪律

1. ✅ **只改 1 个参数** (coreset_sampling_ratio),其他全不动
2. ✅ **Step 0 不过关不推进 Step 3**
3. ✅ 训练后**必须还原客户 yaml**
4. ❌ 不删 RU-011-v2 目录 (底基对比需要)
5. ❌ 不改客户 ad_code/train 的任何 .py 文件
6. ✅ 数据支撑每个结论,**不编**

---

## ⚠️ 失败分支树

| 失败信号 | 阶段 | 应对 |
|---------|------|------|
| Step 0 grep 找不到 coreset_sampling_ratio | Step 0 | 停,报告 train.py 实际路径,让 Napoleon 定位 |
| Step 0 smoke MB 仍 57MB | Step 0 | yaml 没生效,查代码硬编码,停报告 |
| Step 4 MB 仍 700+ MB | Step 4 | Step 0 判断失误或代码中途 override,停报告 |
| Step 4 MB 变很小 <10 MB | Step 4 | coreset 算法边缘崩,记录数据,继续 Step 5 评估精度 |
| Step 5 DLL crash | Step 5 | FP16 后处理漏了,重跑 .half() |
| Good 全误报 (9/9 错) | Step 5 | **先退 0.03** (不是 0.1) 再试;记录这次数据 |
| 精度良好但速度没降 | Step 5 | faiss 次线性的锅,接受或研究索引结构 |
| 训练超 45 分钟 | Step 3 | Ctrl+C,报告 |

**核心规则**: 精度崩 → **先退 0.03**,再崩 → 退 0.05,再崩 → 退 0.1。**不要一次跳回 0.1**,保持信息量。

---

## ⏱️ 时间预算 (v2 含 Step 0 + 7)

- Step 0 yaml 验证: 15 分钟
- Step 1 复用 env: 10 分钟
- Step 2 改 yaml: 5 分钟
- Step 3 full 训练: 30-40 分钟
- Step 4 MB 验证: 5 分钟
- Step 5 FP16 + 评估: 30 分钟
- Step 6 还原 yaml: 2 分钟
- Step 7 阈值观察: 20 分钟
- Step 8 报告: 30 分钟

**合计 2.5-3 小时**

---

## 📦 交接协议

### Git commit
```
git add docs\exp_014_coreset_ablation.md docs\handover_RU014.md
git commit -m "RU-014 v2: Coreset 1% ablation with Step 0 validation (handover-ready)"
git push
```

### 报告末尾
```
已 commit <hash>, handover-ready, Napoleon 可查收 docs/handover_RU014.md
```

---

## 证据等级标注 (给 Napoleon 审核)

| 推论 | 证据级别 | 来源 |
|------|---------|------|
| coreset 1% 精度近似 10% (MVTec) | A | Roth 2022 Table 5 + amazon-science 官方 Ensemble `-p 0.01` |
| coreset 1% 在 resnet18 下精度 | **B (外推)** | 论文消融基于 WR50,未直接验证 |
| MB 大小 × KNN 时间非严格线性 | A | 论文数字 100%→10%→1% 是 0.6/0.22/0.17s (非 10x) |
| yaml coreset_sampling_ratio 生效 | **B (需 Step 0 验证)** | 假设基于 anomalib 默认行为,客户定制代码需验证 |
| 1% 在 dataAug × 10 增广下稳定性 | C (推测) | 无直接证据,论文都是独立图 |

---

## 📋 v2 vs v1 改动清单

- ✅ 新增 Step 0 yaml 生效验证
- ✅ 新增 Step 7 阈值分布观察
- ✅ 预期调整 100→100-400ms 区间
- ✅ 失败预案: 先退 0.03 不是 0.1
- ✅ 证据等级标注 (A/B/C)
- ✅ resnet18 backbone 风险明确标注

---

开始吧 DogWind。**Step 0 不过关绝不推进**,稳扎稳打。
