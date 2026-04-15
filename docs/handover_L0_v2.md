# L0-v2 六图 Baseline 交接 — 给 Napoleon

## 日期
2026-04-16

## 状态
- [x] 建 V2 测试图副本 (6 张, 5.2MB)
- [x] 改 test_main.cpp 图片路径
- [x] 重编译 test_main.exe
- [x] 跑 6图×5次 baseline
- [x] Python vs C++ 对比表

## 关键数字

| 指标 | 数值 |
|------|------|
| 6 图平均推理 | **86.5 ms** |
| 大图 (2000px) | 95-96 ms |
| 中图 (1000px) | 81-84 ms |
| vs Python baseline | **3.40x 加速** |
| ret 对齐 | 6/6 一致 (全 ret=1) |
| 标准差 | < 3ms |

## 对比旧 L0 (5 张客户图)
- 旧 L0: 84.0ms 平均, 5 张 (730-1969px)
- 新 L0-v2: 86.5ms 平均, 6 张 (1009-1969px)
- 差异 +2.5ms 主要因为 V2 没有小图 (730px), 大图占比更高

## 重要发现
1. 6 张图全部 ret=1 score=1.0 — Python 侧也是，确认都是异常样本
2. C++ 客户原版代码 (未优化) 已比 Python 快 3.4x
3. FP16 预处理 bug 对 ret 结果无影响

## 产物
- `cpp_patch/test_images_v2/` — V2 六图副本
- `cpp_patch/L0_baseline/test_main.cpp` — V2 路径版
- `docs/exp_010_L0_v2_baseline.md` — 详细报告
- `docs/handover_L0_v2.md` — 本文件

## 下一步建议
| 方案 | 说明 |
|------|------|
| L0.5 预处理修正 | 改 kHalf 顺序，消除 backbone 0.05 偏差 |
| L1 cudnn.benchmark | 1 行改动，预期 -5~15% |
| L3 cdist 一次性 | 替换循环分块 nearest_neighbors，预期 -10~20ms |

等 Napoleon 判断方向。

---

已提交, handover-ready, Napoleon 可查收 docs/handover_L0_v2.md
