# RU-002-sanity 封装版 Sanity Check

> 任务编号: RU-002-sanity
> 创建日期: 2026-04-10
> 优先级: P0 (会议前必须有)
> 执行人: DogWind (XYC_Windsurf on 鑫业城 5060)
> 预估时间: 30-60 分钟
> 依赖: RU-001 ✅ (封装版代码地图)

---

## Q1 做什么

在 5060 鑫业城电脑上, 用已有的封装版 .exe/.dll 跑通 **1 张测试图**, 证明整条推理链路不崩溃。
**不追求完整 baseline, 不追求精度, 不调参**。

---

## Q2 看到什么

### 必需资源 (在 5060 本地找)

1. **封装版入口** (RU-001 已确认存在):
   - `ADetect.exe` (C++ 测试用可执行)
   - `anomalyDet.dll` (C++ DLL 库)
   - 位置附近: `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\` 或类似路径
   - 具体在 RU-001 封装版代码地图里有写

2. **模型文件夹** (必须一整套):
   - `model.ckpt` — TorchScript 模型
   - `model.mb` — Memory Bank
   - `modelConfig.yaml` — 配置文件

3. **测试图**:
   - 封装版代码自带的测试样本 (应该在 code/code/ 附近的某个 test/ 或 image/ 目录)
   - 或项目里自带的 `D:\\` 开头的示例图 (参见 main 函数里的硬编码路径)
   - 任意一张能跑即可, 最好是正常样本 (good case), 不容易分心

### 资源缺失时的动作

如果上面任何一个**必需资源**找不到, 立即停下:

```bash
# 用 coding-to-coding 的 agent-issue (暂时 v0.1 占位, 可直接微信告诉羊爸爸)
echo "需要 <资源名>: 在 5060 上找不到" >> issues/缺失资源.md
git add issues/缺失资源.md
git commit -m "blocker: RU-002-sanity 缺 <资源名>"
git push
```

然后**停下等羊爸爸回复**, 不要:
- ❌ 自己瞎找替代品
- ❌ 用 selftest 的模型顶上 (业务数据上会得到错误结论)
- ❌ 自己生成假测试图

---

## Q3 记得什么

### 必读文档 (执行前必须读)

1. **`docs/封装版代码地图.md`** (RU-001, 你自己写的) — 知道 anomalyDetMatin 的调用签名
2. **`docs/decrypted_infer_demo.cpp`** — 看 main 函数里 demo 调用的模板
3. **`docs/阈值校准协议.md`** — 知道 pow(thresh, 2.0) 陷阱
4. **`docs/杠杆清单.md`** — 虽然 sanity check 不用杠杆, 但知道它们存在

### 已知陷阱 (⚠️ 必须记住)

1. **`thresh = pow(thresh, 2.0)`** ⚠️
   - 在 `anomalyDetMatin` 函数第一行
   - 你传 0.1, 实际生效是 0.01
   - Sanity check 建议: 用封装版 main 函数的默认值, 不要手动设 thresh

2. **输出是灰度热力图, 不是二值 mask** ⚠️
   - `resultPicOut` 是 0-255 连续值
   - 记录"非零像素数"时, 先做一次 threshold (比如 > 150) 再数
   - 否则数字会很大, 没意义

3. **`modelInputW/H` 硬编码为原图尺寸** ⚠️
   - 暴露的 4 参数接口内部硬编码
   - Sanity check **不要碰**, 用原图尺寸跑

### 参考数据 (量级对比用)

- OM-001 里的旧版单线程 baseline: **5600 ms** (21811x384 memory bank)
- 你跑出的时间可以和这个比一下, 数量级应该接近 (可能更快因为 GPU)
- 但不要追求"跑赢", 只是量级对比

---

## Q4 不能做什么

### 算法边界
- ❌ **不跑完整测试集** — 只跑 1 张
- ❌ **不调任何参数** — 用默认 thresh, 默认 overlap, 默认 cutSize
- ❌ **不修改封装版源码** — 只调用, 不改
- ❌ **不尝试优化** — 这是 baseline, 不是实验

### 文件边界 (防 2026-04-09 事故重演)
- ❌ **不改 `.persona/` 下任何文件** (那是 Agent 人格, 你只改自己的)
- ❌ **不改 `wiki/`** (Napoleon 个人目录在别的地方, 不在本仓库)
- ❌ **不改 `docs/` 下已有文件** (RU-001/OM-001 等产出)
- ❌ **不改 `tasks/` 下其他任务文件**
- ❌ **不改 `configs/` `tests/` `scripts/`**
- ✅ **只新增**: `docs/sanity-check.md` (你的产出)

### Git 边界
- ❌ **不 force push**
- ❌ **不删除任何文件** (你没创建的文件)
- ✅ 提交前 `git diff` 检查, 确认只有 `docs/sanity-check.md` 一个新增

### 加密边界
- ❌ **不绕过任何加密保护** — 如果遇到加密文件, 停下 issue

---

## Q5 怎么知道对了

完成判定 (**所有 6 条都要满足**):

- [ ] 找到并确认 3 个前置资源 (exe/模型/图)
- [ ] ADetect.exe 或等效调用运行不崩溃 (无 crash, 无 exception)
- [ ] 记录 5 个关键数字:
  - [ ] 崩溃状态: ✅No / ❌Yes
  - [ ] return value (0/1/-1)
  - [ ] 单图推理时间 (ms)
  - [ ] resultPicOut 阈值后的非零像素数 (threshold > 150)
  - [ ] 调用时传入的 thresh 值 + 内部实际生效值 (pow 平方后)
- [ ] 产出 `docs/sanity-check.md` (20 行以内报告)
- [ ] Git 只新增这一个文件 (用 `git diff --stat` 自检)
- [ ] 最后手动 commit push (DogWind 会走 git 命令)

### 产出物格式 (docs/sanity-check.md)

```markdown
# RU-002-sanity 封装版 Sanity Check - 2026-04-10

## 环境
- 设备: 鑫业城 5060 (Windows)
- 封装版路径: <实际路径>
- 模型文件夹: <实际路径>
- 测试图: <实际路径>

## 调用
- 方式: <ADetect.exe 直接跑 / main 函数模板改写 / ctypes 调 DLL>
- 命令/代码: <具体>
- 传入 thresh: <值> (实际生效: <pow 平方后>)

## 结果
- 崩溃: ✅ No / ❌ Yes
- return value: <0/1/-1>
- 单图推理时间: <X> ms
- resultPicOut 非零像素数 (>150): <N>
- resultPicOut 尺寸: <W>x<H>

## 跟 OM-001 参考对比
- 旧版单线程 CPU baseline: 5600 ms
- 本次: <X> ms
- 量级: <是否合理>

## 观察 (选填, 但鼓励)
- <任何你觉得值得记录的发现>
- <是否触发了 RU-001 的任何陷阱>

## 下一步建议 (选填)
- <基于这次跑通, 下一个最值得做的实验是什么>
```

### 交付方式

```bash
cd /path/to/RevUnsup
git pull --ff-only  # 再同步一次, 确认没新内容
git add docs/sanity-check.md
git diff --cached --stat  # 确认只有 1 个新增, 没有误删别的
git commit -m "RU-002-sanity: 封装版单图 sanity check 完成"
git push
```

---

## L 灯塔 (主路径不通时)

### 主路径
```
直接命令行跑: ADetect.exe <测试图路径>
```

### Plan B: ADetect.exe 不接受命令行参数

根据 RU-001 的发现, `main.cpp` 里有写死的 `cv::imwrite("D://4.jpg", resultPicOut)`, 说明它可能是 hardcode 输入路径。

做法:
- 看 `docs/decrypted_infer_demo.cpp` 的 main 函数
- 写一个**最小的 C++ wrapper** (照 main 函数的模板, 只改输入图路径)
- 编译运行
- 要求: 不修改原 dll/exe, 只写新 wrapper

### Plan C: C++ 环境编不过 / 缺 OpenCV 头文件

- 不要 `apt install` / `choco install` 自己装环境
- 改用 **Python ctypes**:
  ```python
  import ctypes
  dll = ctypes.WinDLL("anomalyDet.dll")
  # 看 anomalyDetMatin 签名, 构造参数
  # 调用 + 计时
  ```
- 优点: 不用编译
- 缺点: 需要处理 cv::Mat 的 Python 侧构造 (可能要 numpy 配合)

### Plan D: Python ctypes 也搞不定 / 环境问题

- **停下来**
- 创建 `issues/RU-002-sanity-blocker.md`, 内容:
  ```markdown
  # Blocker: RU-002-sanity 无法启动
  
  试过:
  1. ADetect.exe 直接跑 → <具体报错>
  2. C++ wrapper → <具体报错>
  3. Python ctypes → <具体报错>
  
  请羊爸爸决定下一步。
  ```
- commit push 这个 issue
- 等待

### Plan E (最终兜底)

- 微信直接告诉羊爸爸 + 贴报错截图
- 不要瞎改, 不要瞎试, 不要自己装依赖

---

## 备注

### 这是会议前的紧急任务
- 羊爸爸下午有会议, 需要一个"能跑通"的演示素材
- 所以这个任务不追求完美, 只追求"真的跑过"
- 跑通了 → 会议上可以讲"我们已经到 sanity check 阶段"
- 跑不通 → 会议上可以讲"我们正在解决 X 问题" (Plan D 的 blocker 报告就是素材)

### Napoleon 会复核
- 你跑完后, 先 push
- 然后微信通知羊爸爸 "RU-002-sanity 完成, 产出在 docs/sanity-check.md"
- 羊爸爸 Napoleon 会看一眼再带进会议

### 下一步 (不在本任务范围)
- RU-002 (完整版): 跑 baseline, 多张图, 精度评估
- RU-004: 第一个真正的杠杆实验

---

*v1.0 / 2026-04-10 / 按 5Q+1L 模板 / RevUnsup 主线*
