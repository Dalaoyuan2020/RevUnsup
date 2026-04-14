# RU-008-phase3c-step1-env Phase 3C Step 1: 新建独立 TensorRT 环境

> 任务编号: RU-008-phase3c-step1-env (本次只做 Step 1)
> 创建日期: 2026-04-14
> 优先级: P0
> 执行人: DogWind (XYC_Windsurf on 5060)
> 预估时间: 30-60 分钟
> 依赖: RU-007 ✅ (65.5ms, Phase 3A 天花板到头)

---

## ⚠️ 本次只做一件事

**新建独立 conda env 装 TensorRT, 验证能不能装通。**

- ✅ 不动 anomalib2.2 (现有工作环境完全零影响)
- ✅ 不改代码 (推理代码零改动)
- ✅ 不跑实验 (只验证环境)
- ✅ 装不通就删 (一条命令回到起点)

**本次不做**: 改推理代码 / 导 ONNX / 跑 TensorRT 实验 (这些留给 Step 2/3/4)

---

## Q1 做什么

1. 在 5060 上新建独立 conda env `anomalib_trt`
2. 装 PyTorch + torch-tensorrt + tensorrt
3. 验证: python -c "import torch, torch_tensorrt, tensorrt; print(versions)"
4. **物理隔离**: anomalib2.2 完全不触碰
5. 产出安装报告 (含成功 / 失败的具体步骤)

**不是** 完整 TensorRT 落地, 只是 **验证路径能走通**。

---

## Q2 看到什么 (前置资源)

### 已知环境
- **现有工作 env**: `anomalib2.2` (在 D:\Softwares\Anaconda\envs\anomalib2.2\)
  - Python 3.12
  - PyTorch 2.11.0+cu128
  - RU-004/005/006/007 都在这里跑
  - **本次绝对不碰**
- **GPU**: RTX 5060 Ti (Blackwell sm_120, CUDA 12.8)
- **系统**: Windows

### 新 env 目标配置
- `anomalib_trt` (独立, 新建)
- Python 3.12 (跟现有一致, 兼容性更好)
- PyTorch 跟 anomalib2.2 匹配 (2.11+cu128) 或稍低版本 (看 TensorRT 要求)
- torch-tensorrt (最新版)
- tensorrt / tensorrt-rtx (根据 Windows + Blackwell 适配)

### 参考文档
- [Torch-TensorRT 安装指南](https://docs.pytorch.org/TensorRT/getting_started/installation.html)
- [Torch-TensorRT-RTX (Blackwell 专用)](https://docs.pytorch.org/TensorRT/getting_started/tensorrt_rtx.html)
- [Windows TensorRT 安装经验](https://medium.com/@peiyuan67/ai-inference-clear-steps-to-install-tensorrt-on-windows-d13f49d2f18b)

---

## Q3 记得什么 (必读上下文)

### 为什么要隔离
- 直接在 anomalib2.2 装 TensorRT, pip 可能自动升级 PyTorch / CUDA 依赖
- 升级后, 现有 RU-005/006/007 的代码可能崩
- 重装 anomalib2.2 要几十分钟, 还可能装不回来
- **独立 env 是唯一安全做法**

### Blackwell 特殊性
- RTX 5060 Ti 是 sm_120 (2025 年新架构)
- 需要 CUDA 12.8+ 支持
- TensorRT 需要对应版本 (可能要 TensorRT 10.5+ 或 TensorRT for RTX)
- PyTorch 2.11+ 才原生支持 Blackwell

### 开关模式设计 (本次不实现, 但要记得)
未来完整方案:
```
环境变量 BACKBONE_BACKEND=pytorch|tensorrt
默认 pytorch (现有行为零变化)
```
本次只是把 env 搭好, 代码层开关留给 Step 2。

---

## Q4 不能做什么 (红线)

### 环境红线 (最重要!)
- ❌ **不能 pip install 到 anomalib2.2 env** — 永远不要 `conda activate anomalib2.2 && pip install tensorrt`
- ❌ **不能升级 anomalib2.2 的 PyTorch / CUDA 依赖**
- ❌ **不能修改 anomalib2.2 env 的任何文件**
- ✅ **只在 anomalib_trt 里操作**
- ✅ 操作前 `conda activate anomalib_trt` 确认环境

### 代码红线
- ❌ **本次不改推理代码** (留给 Step 2)
- ❌ **不改 RU-005/006/007 的代码**
- ✅ 只可以新建: 一个 `D:\test_trt.py` 测试脚本 (验证 import 成功, 可选)

### Git 红线
- ❌ 不删别人文件
- ❌ 不改 baseline / exp_* 数据
- ✅ 只新增: `docs/exp_005_trt_install.md`

---

## Q5 怎么知道对了 (验收)

### 验收 1: 新 env 成功建立
```powershell
conda env list
# 应该看到:
#   anomalib2.2        D:\Softwares\Anaconda\envs\anomalib2.2
#   anomalib_trt       D:\Softwares\Anaconda\envs\anomalib_trt  ← 新的
```

### 验收 2: 关键库都能 import

```powershell
conda activate anomalib_trt
python -c "import torch; print('torch:', torch.__version__, 'cuda:', torch.version.cuda, 'available:', torch.cuda.is_available())"
python -c "import torch_tensorrt; print('torch_tensorrt:', torch_tensorrt.__version__)"
python -c "import tensorrt; print('tensorrt:', tensorrt.__version__)"
```

应该全部输出版本号, 没有 ImportError。

### 验收 3: 最小 GPU 识别测试

```powershell
python -c "
import torch
x = torch.randn(100, 100, device='cuda')
y = x @ x.T
print('GPU matmul OK:', y.shape, y.device)
print('Capability:', torch.cuda.get_device_capability())  # 应该是 (12, 0) 代表 Blackwell
"
```

### 验收 4: anomalib2.2 零影响

```powershell
conda activate anomalib2.2
cd D:\sanity_profile.py 的目录
# 跑一次 RU-007 的代码, 应该跟之前 65.5ms 一致
python sanity_profile.py  # 或你 RU-007 的脚本
```

如果现在 anomalib2.2 下跑 RU-007 的数字变了 → 证明污染了 → 回滚

### 验收 5: 产出报告 `docs/exp_005_trt_install.md`

```markdown
# RU-008 Step 1: TensorRT 环境安装报告

## 结果
- [x] / [ ] 新 env anomalib_trt 建成
- [x] / [ ] torch / torch_tensorrt / tensorrt 都能 import
- [x] / [ ] GPU capability (12, 0) ✅ Blackwell
- [x] / [ ] anomalib2.2 零影响 (重跑 RU-007 数字一致)

## 安装的版本
- Python: 3.X
- PyTorch: X.X.X+cuXXX
- torch-tensorrt: X.X.X
- tensorrt: X.X.X

## 安装命令 (贴完整的)
```bash
conda create -n anomalib_trt python=3.12 -y
conda activate anomalib_trt
pip install torch==X.X.X ...
pip install torch-tensorrt ...
pip install tensorrt ...
```

## 遇到的问题
- (如果有, 列出具体错误 + 解决方法)

## 下一步建议
- 继续 Step 2 (代码加开关)? ✅
- 或者 路径不通, 建议回退?
```

### 交付
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs\exp_005_trt_install.md
git diff --cached --stat   # 只 1 个新增
git commit -m "RU-008 Step 1: TensorRT env install (SUCCESS/FAILED)"
git push
```

---

## L 灯塔 (意外处理)

### Plan A (主路径)
按 Q5 全通过 → 报告 SUCCESS, 进 Step 2

### Plan B: pip 冲突报错
例: "Cannot install torch==X.X.X because it conflicts with ..."
→ 查 torch-tensorrt 对应的 torch 版本, 换版本重试
→ 记录在报告里

### Plan C: tensorrt 装不上 (Windows 没 wheel)
尝试 2 个替代:
1. `pip install tensorrt-cu12` 或 `tensorrt-rtx`
2. 从 NVIDIA 官网下 TensorRT SDK 手动装
如果都不行 → 报告 "Windows + Blackwell + TensorRT 当前无可用路径"
→ **干净删除 env**: `conda env remove -n anomalib_trt`
→ 这是合法结论, 证明 3C 路径受阻, 接受 65.5ms

### Plan D: 装完 import 报错
例: DLL load failed, CUDA version mismatch
→ 记录完整错误
→ 不要在 anomalib_trt 里继续挣扎, 直接删 env 重来
→ 或者报告 + 等 Napoleon 调研

### Plan E: 装着装着破坏了 anomalib2.2 (最严重, 最不希望发生)
如果【发现 anomalib2.2 受影响】:
→ **立即停止所有 pip 操作**
→ 用之前备份的 `D:\anomalib2.2_backup.txt` 记录恢复步骤
→ commit blocker issue, 报告羊爸爸

**预防最重要**: 每次 pip install 前, 检查 `conda info --envs` 确认当前是 anomalib_trt, 不是 anomalib2.2!

---

## 安装策略 (推荐顺序)

### 稳妥版 (推荐)
```powershell
# 0. 先备份 anomalib2.2 规格
conda activate anomalib2.2
pip freeze > D:\anomalib2.2_pipfreeze_backup.txt
conda list --export > D:\anomalib2.2_conda_backup.txt
conda deactivate

# 1. 建新 env
conda create -n anomalib_trt python=3.12 -y
conda activate anomalib_trt

# 2. 确认现在不是 anomalib2.2
conda info --envs
# active env 应该是 anomalib_trt, 不是 anomalib2.2!

# 3. 装 PyTorch (用 cu128 对齐 anomalib2.2)
pip install torch==2.11.0+cu128 --extra-index-url https://download.pytorch.org/whl/cu128

# 4. 装 TensorRT (二选一, 试 pip 优先)
pip install tensorrt

# 如果 pip 没有 wheel, 试:
# pip install tensorrt-cu12

# 5. 装 torch-tensorrt
pip install torch-tensorrt

# 6. 验证
python -c "import torch, torch_tensorrt, tensorrt; print('all OK')"
```

### 激进版 (如果稳妥版报错)
先建 env, 然后一条命令:
```powershell
conda create -n anomalib_trt python=3.12 -y
conda activate anomalib_trt
pip install torch torch-tensorrt tensorrt --extra-index-url https://download.pytorch.org/whl/cu128
# 让 pip 自动解决版本依赖
```

---

## 时间预算

- 备份 + 新建 env: 5 分钟
- pip install: 10-20 分钟 (第一次下载慢)
- 验证 + 报告: 10 分钟
- 如果遇到冲突排查: +10-20 分钟

**合计 30-60 分钟**

---

## 这次任务的意义

这是 Phase 3C 的【侦察兵】, 不是完整方案:
- 成功装通 → 证明路径可行, 可以继续 Step 2
- 装不通 → 证明路径受阻, 接受 65.5ms 是合理结论

**成功/失败都是有价值的结果**, 30 分钟换一个明确答案。

---

*任务 v1.0 / 2026-04-14 / 侦察性任务, 只装 env 不动代码*
