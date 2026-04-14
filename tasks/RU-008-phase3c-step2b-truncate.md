# RU-008 Step 2b: TensorRT 转换重试 (加 truncate_long_and_double)

> 任务编号: RU-008-phase3c-step2b-truncate
> 创建日期: 2026-04-14
> 优先级: P0
> 执行人: DogWind
> 预估时间: 20-40 分钟
> 依赖: RU-008 Step 2 ❌ (失败于 Long/Double 类型)

---

## ⚠️ 两件事

### 第一件: 先 push 未上传的 commit

Step 2 的 commit `d933370` 因网络失败还在本地, 先 push:

```powershell
cd D:\RevUnsup
git push
```

如果网络还是不行, 等 2 分钟再试。
如果怎么都 push 不上去, 报告网络问题, 跳到第二件事也 OK。

### 第二件: 改 1 行代码, 重跑 Step 2 的脚本

在 `D:\trt_test.py` 里加一个参数 `truncate_long_and_double=True`, 其他都不变。

---

## Q1 做什么 (只改 2 处)

### 改动 1: FP32 compile 加参数
找到 `trt_model_fp32 = torch_tensorrt.compile(...)` 这段, 加一个参数:

```python
# 原:
trt_model_fp32 = torch_tensorrt.compile(
    model,
    ir='torchscript',
    inputs=[torch_tensorrt.Input(INPUT_SHAPE, dtype=torch.float32)],
    enabled_precisions={torch.float32},
)

# 改成:
trt_model_fp32 = torch_tensorrt.compile(
    model,
    ir='torchscript',
    inputs=[torch_tensorrt.Input(INPUT_SHAPE, dtype=torch.float32)],
    enabled_precisions={torch.float32},
    truncate_long_and_double=True,   # ← 新增这一行
)
```

### 改动 2: FP16 compile 同样加参数
找到 `trt_model_fp16 = torch_tensorrt.compile(...)`, 一样加:

```python
trt_model_fp16 = torch_tensorrt.compile(
    model,
    ir='torchscript',
    inputs=[torch_tensorrt.Input(INPUT_SHAPE, dtype=torch.float16)],
    enabled_precisions={torch.float16, torch.float32},
    truncate_long_and_double=True,   # ← 新增这一行
)
```

**其他都不改**。

---

## Q2 为什么这样改

TensorRT 不支持 Long/Double 类型 (Step 2 错误)。
`truncate_long_and_double=True` 告诉 torch_tensorrt:
- Long → Int32 (截断)
- Double → Float32 (截断)

对 PatchCore 来说, 这个截断**安全**:
- Int32 表示 ±21 亿, Memory Bank 才 2 万, 不会溢出
- Double → Float32 精度损失几乎无感

这是 **torch_tensorrt 官方文档明确推荐的 workaround**。

---

## Q3 怎么跑

```powershell
conda activate anomalib_trt
python D:\trt_test.py
```

(同 Step 2, 只是脚本内容改了 2 处)

---

## Q4 不能做什么

- ❌ 不改其他代码 (只加 2 个 truncate_long_and_double=True)
- ❌ 不动 anomalib2.2
- ❌ 不动 D:\RevUnsup 代码
- ❌ 不测 6 图完整 baseline (只 1 张 dummy)

---

## Q5 产出 (跟 Step 2 一样, 只换文件名)

### 文件: `D:\RevUnsup\docs\exp_006b_trt_conversion_truncate.md`

模板:
```markdown
# RU-008 Step 2b: TensorRT 转换重试 (truncate_long_and_double)

## 日期
2026-04-14

## 结果
- [x]/[ ] FP32 compile 成功
- [x]/[ ] FP32 forward 成功
- [x]/[ ] FP16 compile 成功
- [x]/[ ] FP16 forward 成功

## 耗时对比
| 后端 | 耗时 (ms) | 加速 vs PyTorch |
|------|-----------|------------------|
| PyTorch | 30.98 | 1.00x |
| TRT FP32 | X.X | Y.Y x |
| TRT FP16 | Z.Z | W.W x |

## 编译时间
- FP32 compile: N 秒
- FP16 compile: M 秒

## 警告
(如果有, 贴)

## 完整 log
```
(粘 stdout)
```

## 结论
- 转换: ✅ / ❌
- 下一步建议:
  - 成功 → Step 3 (集成到推理 pipeline)
  - 失败 → 记录具体 error, 考虑换 ONNX 中间格式
```

### Git 提交
```powershell
cd D:\RevUnsup
git pull --ff-only
git add docs\exp_006b_trt_conversion_truncate.md
git diff --cached --stat    # 只有 1 个新增
git commit -m "RU-008 Step 2b: TensorRT truncate retry (SUCCESS/FAILED)"
git push
```

---

## L 灯塔

### Plan A: truncate 生效, FP32 成功
→ 继续试 FP16 (同样加参数)
→ 写报告, 推 Step 3

### Plan B: FP32 成功但 FP16 失败
→ 记录, 报告 FP32 的数字
→ FP16 留给 Step 3 再调

### Plan C: 加 truncate 还是失败 (新 error)
→ 贴完整 error 到报告
→ 下一步建议走 ONNX 路径 (Step 2c)
→ 不自己修

### Plan D: FP32 通了但 forward 崩
→ 记录具体错误
→ 可能是 input shape 问题或其他

---

## 时间预算

- 改脚本 (加 2 行): 1 分钟
- 运行 + 等编译: 15-30 分钟 (2 个 compile 各 ~5-10 min)
- 写报告: 10 分钟

**合计 20-40 分钟**

---

*任务 v1.0 / Step 2b: 极小改动, 只加 truncate 参数重试*
