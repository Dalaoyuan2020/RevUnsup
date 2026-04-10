# RU-002-sanity 封装版 Sanity Check - 2026-04-10

## 环境
- 设备: 鑫业城 5060 (Windows, RTX 5060 Ti 16GB, CUDA 12.9 driver)
- 封装版路径: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\` (DLL) + `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\ad_code\infer\` (EXE)
- 模型文件夹: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\` (model.ckpt 43MB + model.mb 12MB + modelConfig.yaml)
- 测试图: `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\test\2.jpg` (1969x2152, 芯片 ROI 彩色图)

## 调用
- 方式: Python TorchScript 等效推理 (Plan C 变体)
- 原因: ADetect.exe 主路径失败 (硬编码路径 + DLL WinError 1114)，根因是 **RTX 5060 Ti (sm_120 Blackwell) 不兼容封装版 LibTorch (CUDA 11.7, 最高 sm_90)**
- 解法: 用 `anomalib2.2` conda 环境 (PyTorch 2.11.0+cu128, 支持 sm_120)，直接加载 model.ckpt + model.mb 复现 DLL 推理逻辑
- 命令: `D:\Softwares\Anaconda\envs\anomalib2.2\python.exe D:\sanity_check_run.py`
- 传入 thresh: 0.1 (实际生效: 0.01, pow(0.1, 2)=0.01)

## 结果
- 崩溃: ✅ No
- return value: 1 (检测到缺陷)
- 单图推理时间: 3690 ms
- resultPicOut 非零像素数 (>150): 0 (热力图整体偏暗，未做 gammaCorrection)
- resultPicOut 尺寸: 1969x2152
- Max anomaly score: 1.093

## 跟 OM-001 参考对比
- 旧版单线程 CPU baseline: 5600 ms
- 本次 (GPU, RTX 5060 Ti): 3690 ms
- 量级: 合理 (GPU 加速约 1.5x, 主要瓶颈在 nearest_neighbors cdist)

## 观察
- **BLOCKER 发现**: 封装版 DLL/EXE 无法在 RTX 5060 Ti 上运行 — LibTorch CUDA 11.7 不支持 sm_120。后续必须用 Python 推理或重编译 DLL
- Memory Bank 只有 3 个 patch embeddings (1x2 grid), 说明这个模型的 coreset 比较小
- `nonzero(>150)=0` 是因为 Python 脚本没做 gammaCorrection(1.2), 实际 DLL 输出会更亮
- 模型加载 213ms + MB 加载 41ms, 推理 3690ms, 瓶颈在推理循环 (cdist)

## 下一步建议
1. **P0**: 确认是否需要重编译 DLL (用新版 LibTorch + CUDA 12.x), 或转为纯 Python 推理路线
2. FAISS 替换 cdist 是最大杠杆 (RU-003 杠杆清单 A1)
3. 完整 baseline (RU-002 Phase 2) 用 Python 等效脚本跑多张图
