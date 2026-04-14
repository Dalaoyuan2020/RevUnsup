# RU-008 Step 2: TensorRT 模型转换验证 (最小可用)

> 任务编号: RU-008-phase3c-step2-conversion
> 创建日期: 2026-04-14
> 优先级: P0
> 执行人: DogWind
> 预估时间: 1-2 小时
> 依赖: RU-008 Step 1 ✅ (anomalib_trt env 装通了)

---

## ⚠️ 本次只做 1 件事

**验证能不能把 model.ckpt 转成 TensorRT engine 并跑通 1 次推理。**

- ✅ 只在 anomalib_trt env 里操作
- ✅ 不改 anomalib2.2 的任何代码
- ✅ 不改 D:\RevUnsup 的推理代码 (sanity_profile.py 等保持)
- ✅ 只新建一个【独立测试脚本】 `D:\trt_test.py`
- ✅ 不做完整 baseline (6 图 × 5 次), 只验证"能转通"

**本次不做**: 完整速度测试 / 精度对齐 / 集成到 RevUnsup (留给 Step 3/4)

---

## Q1 做什么 (极简目标)

在 **anomalib_trt** 环境里跑一个独立脚本, 完成:

1. 加载 `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt` (TorchScript)
2. 用 `torch_tensorrt.compile()` 把它转成 TensorRT engine
3. 喂 1 张 dummy 输入, 跑通一次 forward
4. 记录 TensorRT 单次耗时, 对比 PyTorch 单次耗时
5. 写报告

目标: **证明转换路径能走通**, 不追求性能数字完美。

---

## Q2 看到什么 (前置资源)

### 环境 (Step 1 已建)
- `anomalib_trt` env (Python 3.12, PyTorch 2.11+cu128, TensorRT 10.16, torch-tensorrt 2.11)
- 激活方式: `conda activate anomalib_trt`

### 模型
- `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt` (TorchScript)
- `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.mb` (Memory Bank)
- `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\modelConfig.yaml`

### Backbone Input Shape (重要!)
从 `sanity_profile.py` (RU-002-sanity/RU-005/006 的原始脚本) 可以看到:
- 每个 patch 尺寸: **2048 × 2048**
- 输入格式: `[1, 3, 2048, 2048]` FP32
- 或 FP16 版本

---

## Q3 记得什么 (关键上下文)

### torch_tensorrt.compile 的典型用法

基本形式:
```python
import torch
import torch_tensorrt

model = torch.jit.load('model.ckpt', map_location='cuda').eval()
example_input = torch.randn(1, 3, 2048, 2048, device='cuda')

trt_model = torch_tensorrt.compile(
    model,
    ir='torchscript',
    inputs=[torch_tensorrt.Input(example_input.shape, dtype=torch.float32)],
    enabled_precisions={torch.float32},  # 先 FP32, 简单
)
```

### FP16 优化 (如果 FP32 通了再试)
```python
trt_model = torch_tensorrt.compile(
    model,
    ir='torchscript',
    inputs=[torch_tensorrt.Input(example_input.shape, dtype=torch.float16)],
    enabled_precisions={torch.float16},  # FP16 更快
)
```

### 首次编译会很慢
- TensorRT 首次 compile 要 2-10 分钟 (JIT 优化)
- 这是正常的, 不是卡住
- 第二次调用会很快 (cache)

---

## Q4 不能做什么 (红线)

### 环境红线
- ❌ 不在 anomalib2.2 里操作
- ❌ 不改 anomalib2.2 的任何东西
- ❌ 不改 anomalib_trt 的版本配置 (就用 Step 1 装的版本)

### 代码红线
- ❌ **不改 D:\RevUnsup 的任何推理代码** (sanity_profile.py 等都保持)
- ❌ 不导入 anomalib2.2 的代码到 anomalib_trt
- ✅ 只新建: `D:\trt_test.py` (独立测试脚本)

### 测试红线
- ❌ 不做 6 图 × 5 次完整 baseline (本次只要 1 张图验证能跑)
- ❌ 不做精度对齐 (Step 4 才做)
- ❌ 不改 memory bank / KNN 部分 (只转 backbone)

### Git 红线
- ❌ 不删别人文件
- ✅ 只新增: `docs/exp_006_trt_conversion.md` + 可选 `scripts/trt_test.py`

---

## Q5 怎么知道对了 (验收)

### 必须完成的 3 件
1. [x] 脚本能运行完成 (不崩)
2. [x] TensorRT engine 成功 compile (可能要等几分钟)
3. [x] engine 能跑 1 次推理, 输出 shape 跟 PyTorch 一致

### 加分项 (有余力再做)
4. [ ] PyTorch vs TensorRT 跑 10 次对比耗时
5. [ ] 尝试 FP16 (先 FP32 通了再上 FP16)

### 产出报告 `docs/exp_006_trt_conversion.md`

格式:
```markdown
# RU-008 Step 2: TensorRT 模型转换验证报告

## 结果
- [x]/[ ] 脚本运行完成
- [x]/[ ] torch_tensorrt.compile 成功
- [x]/[ ] FP32 forward 成功
- [x]/[ ] FP16 forward 成功 (可选)

## 耗时对比 (单次 forward, 1 张图)
- PyTorch: X ms
- TensorRT FP32: Y ms (Z 倍加速)
- TensorRT FP16: W ms (V 倍加速, 如果跑了)

## 编译时间
- 首次 compile 用了: N 分钟

## 警告或错误
- (贴具体 error message, 如果有)

## 下一步
- 成功 → 进 Step 3 (集成到推理 pipeline)
- 部分成功 → 分析哪些操作 fallback 到 PyTorch (Torch-TensorRT 的 auto-fallback)
- 失败 → 记录具体错误, 可能需要换导出方式 (ONNX 中间格式)
```

### 交付
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs\exp_006_trt_conversion.md
git diff --cached --stat
git commit -m "RU-008 Step 2: TensorRT conversion test (SUCCESS/PARTIAL/FAILED)"
git push
```

---

## L 灯塔 (意外处理)

### Plan A: 完整 FP32 compile 成功
→ 试 FP16, 成功就太好了
→ 记录耗时, 进 Step 3

### Plan B: FP32 compile 失败
可能原因: TorchScript 模型里某个算子不支持
→ 看 Error 信息, 记录具体是哪个算子
→ 尝试 `ir='dynamo'` 替代 `ir='torchscript'`
→ 如果还不行, 写报告说 "原 TorchScript 路径不通, 建议 Step 3 用 ONNX 中间格式"

### Plan C: torch_tensorrt 本身 import 失败
可能是 Step 1 的版本混合问题爆发了
→ 记录完整 error
→ 报告羊爸爸, 等决策

### Plan D: compile 成功但 forward 报错
可能是 input shape 不对
→ 检查 sanity_profile.py 里 backbone 的真实 input shape
→ 调整后重试

### Plan E: 任何其他意外
→ 停下, 不硬上
→ commit blocker issue, 报告具体错误

---

## 建议的完整脚本 (DogWind 可直接复制)

`D:\trt_test.py`:

```python
"""
RU-008 Step 2: TensorRT conversion test
必须在 anomalib_trt env 里跑: conda activate anomalib_trt && python trt_test.py
"""
import time
import torch
import torch_tensorrt

# ==== 1. 加载原模型 ====
MODEL_PATH = r"D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\model.ckpt"
print(f"Loading model from {MODEL_PATH}...")
model = torch.jit.load(MODEL_PATH, map_location='cuda').eval()
print("Model loaded.")

# ==== 2. 准备 dummy input ====
# 注意: backbone input shape 可能是 [1, 3, 2048, 2048]
# 如果不对, 报错后调整这里
INPUT_SHAPE = (1, 3, 2048, 2048)
print(f"Creating dummy input shape={INPUT_SHAPE}...")
example_input = torch.randn(*INPUT_SHAPE, device='cuda')

# ==== 3. PyTorch baseline 耗时 ====
print("\n=== PyTorch baseline ===")
# warmup
with torch.no_grad():
    for _ in range(3):
        _ = model(example_input)
torch.cuda.synchronize()

# 测 10 次
times_pt = []
with torch.no_grad():
    for i in range(10):
        torch.cuda.synchronize()
        t0 = time.perf_counter()
        _ = model(example_input)
        torch.cuda.synchronize()
        t1 = time.perf_counter()
        times_pt.append((t1 - t0) * 1000)
print(f"PyTorch avg: {sum(times_pt)/len(times_pt):.2f} ms  (std {max(times_pt)-min(times_pt):.2f})")

# ==== 4. TensorRT compile (FP32 先) ====
print("\n=== TensorRT FP32 compile ===")
print("This may take 2-10 minutes on first run...")
t0 = time.perf_counter()
try:
    trt_model_fp32 = torch_tensorrt.compile(
        model,
        ir='torchscript',
        inputs=[torch_tensorrt.Input(INPUT_SHAPE, dtype=torch.float32)],
        enabled_precisions={torch.float32},
    )
    compile_time = time.perf_counter() - t0
    print(f"FP32 compile OK in {compile_time:.1f}s")
    
    # ==== 5. TRT FP32 forward ====
    print("\n=== TensorRT FP32 forward ===")
    with torch.no_grad():
        for _ in range(3):
            _ = trt_model_fp32(example_input)
    torch.cuda.synchronize()
    
    times_trt = []
    with torch.no_grad():
        for i in range(10):
            torch.cuda.synchronize()
            t0 = time.perf_counter()
            _ = trt_model_fp32(example_input)
            torch.cuda.synchronize()
            t1 = time.perf_counter()
            times_trt.append((t1 - t0) * 1000)
    print(f"TRT FP32 avg: {sum(times_trt)/len(times_trt):.2f} ms")
    print(f"Speedup: {sum(times_pt)/sum(times_trt):.2f}x")

except Exception as e:
    print(f"FP32 compile FAILED: {e}")
    print("Trying ir='dynamo'...")
    # TODO: 试 dynamo, 如果还失败就报告

# ==== 6. TRT FP16 (可选) ====
print("\n=== TensorRT FP16 compile (optional) ===")
try:
    trt_model_fp16 = torch_tensorrt.compile(
        model,
        ir='torchscript',
        inputs=[torch_tensorrt.Input(INPUT_SHAPE, dtype=torch.float16)],
        enabled_precisions={torch.float16, torch.float32},
    )
    example_fp16 = example_input.half()
    with torch.no_grad():
        for _ in range(3):
            _ = trt_model_fp16(example_fp16)
    torch.cuda.synchronize()
    
    times_fp16 = []
    with torch.no_grad():
        for i in range(10):
            torch.cuda.synchronize()
            t0 = time.perf_counter()
            _ = trt_model_fp16(example_fp16)
            torch.cuda.synchronize()
            t1 = time.perf_counter()
            times_fp16.append((t1 - t0) * 1000)
    print(f"TRT FP16 avg: {sum(times_fp16)/len(times_fp16):.2f} ms")
    print(f"Speedup vs PyTorch: {sum(times_pt)/sum(times_fp16):.2f}x")
except Exception as e:
    print(f"FP16 compile FAILED (非阻塞): {e}")

print("\n=== DONE ===")
```

---

## 时间预算

- 环境激活 + 写脚本: 5 分钟
- PyTorch baseline 10 次: 1 分钟
- TRT FP32 compile: 2-10 分钟 (首次)
- TRT FP32 forward 10 次: 1 分钟
- TRT FP16 compile (可选): 2-10 分钟
- 写报告: 15 分钟

**合计 30-60 分钟** (大部分是等 TRT compile)

---

*任务 v1.0 / 2026-04-14 / Step 2: 最小可用验证, 不集成不完整测*
