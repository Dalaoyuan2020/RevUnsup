# L0 Baseline 交接 — 给 Napoleon

## 日期
2026-04-15

## 状态
- [x] dumpbin 导出查询: 看到 creatModel / anomalyDetMatin / deinitModel / deinitAllModel (4个全在)
- [x] 分支判断: A (完整) — 但需用解密源码替换加密文件才能编译出有效DLL
- [x] main.cpp 写了并编译通过
- [x] 5 图 × 5 次 baseline 测完

## 关键数字

### 编译环境
- VS 2022 Community (MSVC 14.44.35207, v143)
- Windows SDK 10.0.26100.0
- LibTorch 2.8.0+cu129
- GPU: RTX 5060 Ti (sm_120)

### 模型初始化
- creatModel 耗时: 630 ms
- Warmup 首次: 2481 ms (CUDA 预热), 后续 80-96 ms

### Baseline 耗时 (5图 × 5次, 预热后)

| Image | Size | Run1 | Run2 | Run3 | Run4 | Run5 | Avg | Std |
|-------|------|------|------|------|------|------|-----|-----|
| 0.png | 730x410 | 78.5 | 79.8 | 78.1 | 77.4 | 78.9 | **78.5** | 0.8 |
| 5.jpg | 1969x2152 | 98.5 | 97.6 | 95.3 | 94.4 | 97.0 | **96.6** | 1.5 |
| 6.jpg | 1087x847 | 82.0 | 80.6 | 83.1 | 81.9 | 81.1 | **81.8** | 0.9 |
| 7.jpg | 1081x841 | 80.9 | 83.5 | 81.1 | 82.1 | 81.9 | **81.9** | 0.9 |
| 8.jpg | 1081x854 | 79.8 | 81.2 | 81.8 | 81.0 | 81.3 | **81.0** | 0.7 |

- **5图平均推理耗时: 84.0 ms**
- **单图最快: 77.4 ms (0.png Run4)**
- **单图最慢: 98.5 ms (5.jpg Run1, 大图 1969x2152)**
- **标准差极低 (0.7-1.5 ms), 非常稳定**

### 重要观察
1. 所有 5 张图 ret=1 (检测到异常), score=1.0 (非零像素比例100%)
2. 大图 5.jpg (1969x2152) 比小图慢约 15ms, 主要是 resize + 预处理开销
3. 4 张 ~1000px 图平均 80.8 ms, 非常一致
4. 客户 FP16 已生效 (输入 kHalf, MB kFloat16, 输出 kFloat32)

## 编译过程遇到的问题 (已解决)

| 问题 | 解决方案 |
|------|----------|
| 路径硬编码 `D:\wej_AI_5.0\` | 全局替换为 `D:\XYC_Dog_Agent\...` |
| Windows SDK 10.0.22621.0 缺失 | 改为本机 10.0.26100.0 |
| 工程含 9 个 .cpp 但只有 1 个在我们工作区 | 精简为仅 test_ad_infer_new.cpp |
| 源文件 E-SafeNet 加密 | 用 EncryCode.md 第 2-1043 行解密源码替换 |
| LTCG 优化掉导出函数 | 关闭 WholeProgramOptimization |

## 产物
- `cpp_patch/L0_baseline/x64/Release/xyc_all_AI.dll` — 客户原版 DLL (解密源码编译)
- `cpp_patch/L0_baseline/x64/Release/test_main.exe` — 测试 harness
- `cpp_patch/L0_baseline/test_ad_infer_new.cpp` — 解密源码 (1042行)
- `cpp_patch/L0_baseline/test_main.cpp` — 测试程序
- `cpp_patch/L0_baseline/test_ad_infer_new.vcxproj.original` — 客户原始工程文件
- `cpp_patch/diffs/L0_vcxproj_path_fix.diff` — 工程文件修改 diff
- `docs/handover_L0_export.md` — dumpbin 完整输出

## Napoleon 可查收的
1. `docs/handover_L0_baseline.md` (本文件)
2. `docs/handover_L0_export.md` (dumpbin 导出)
3. `cpp_patch/L0_baseline/` (源码 + 编译工程)
4. `cpp_patch/diffs/L0_vcxproj_path_fix.diff` (防拷打证据)

## 下一步
建议进入 L0.5 预处理修正 (诊断客户 FP16 bug: uint8→kHalf→归一化 顺序问题)

---

已提交, handover-ready, Napoleon 可查收 docs/handover_L0_baseline.md
