# L0' FP16 诊断交接 — 给 Napoleon

## 日期
2026-04-15

## 状态
- [x] 建 L0' 工作区
- [x] 写诊断程序 (两条预处理路径 + backbone 对比)
- [x] 编译通过 (解决 CUDA /INCLUDE 符号问题)
- [x] 5 图 × 两路径诊断完成
- [x] backbone 输出对比完成 (Step 4 可选项也做了)

## 关键数字

### 输入 (预处理后)
- **worst max_diff: 0.003** (minor, 在 0.001-0.01 之间)
- **worst mean_diff: 0.0005**

### Backbone 输出
- **worst max_diff: 0.056** (> 0.01, bug confirmed at backbone level)
- **worst mean_diff: 0.0002**
- **放大倍数: 19-45x** (从输入到backbone)
- **relative error mean: 0.2-0.5%**

### 核心结论
**FP16 预处理 bug 存在但不严重**。预处理层差异 minor (0.003)，经 backbone 放大到 0.05。但 backbone mean relative error 仅 0.2-0.5%，**不太可能是 5/5 全误报的唯一原因**。

建议优先调查：① 测试图是否本身就是异常样本；② 阈值 0.1 (内部 pow=0.01) 是否过低。

## 编译额外发现
- `torch::cuda::is_available()` 需要 `/INCLUDE:"?ignore_this_library_placeholder@@YAHXZ"` 链接标志才能在 Windows 上正确检测 CUDA。L0 的 vcxproj 已有此标志，独立编译时容易遗漏。

## 产物
- `cpp_patch/L0_prime_diag/diag_main.cpp` — 诊断源码
- `docs/exp_009_L0_prime_fp16_diag.md` — 详细诊断报告 (含表格)
- `docs/handover_L0_prime.md` — 本文件

## Napoleon 可查收的
1. `docs/handover_L0_prime.md` (本文件)
2. `docs/exp_009_L0_prime_fp16_diag.md` (诊断数据)
3. `cpp_patch/L0_prime_diag/` (诊断源码)

## 下一步建议
| 方案 | 说明 |
|------|------|
| **L0.5 预处理修正** | 可做，消除 0.05 backbone 偏差，但可能不是 5/5 误报根因 |
| **阈值敏感性测试** | 用 thresh=0.5, 1.0, 2.0 各跑一次看 ret 变化 |
| **正常样本测试** | 找已知 OK 图片测试，确认 baseline 是否也全 ret=1 |
| **直接进 L1 加速** | 如果确认测试图就是异常图，精度问题不存在 |

等 Napoleon 判断方向。

---

已提交, handover-ready, Napoleon 可查收 docs/handover_L0_prime.md
