# RU-009 客户新代码全面摸底报告

## 日期
2026-04-15

---

## 1. 客户自己的部署说明 (5060推理库说明.txt 全文)

> 注: 客户文件名实际为 `5060推理库说明.txt` (不是"部署说明")，内容是 DLL 接口文档而非部署步骤。

```
xyc_all_AI中，5060推理所有函数说明

1、libtorch推理图像分割函数说明：
目前使用方案中的5060的deeplabv3+的模型，1748*1748图像使用fp16推理需要416ms左右
初始化函数 model_path模型路径，模型为：xyc_seg_libtorch.pt
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号
size输入宽高（目前只能是1728，如需改变需要pytorch训练代码调整尺寸重新生成模型）,
extern "C" __declspec(dllexport) int xyc_seg_lib_init(const char* model_path, int gpu_id, int size = 1748) 
推理函数 cv::Mat & input 输入图像, cv::Mat & output 输出图像, int size输入宽高（目前只能是1728，如需改变需要重新生成模型）
extern "C" __declspec(dllexport) int xyc_seg_lib_run(cv::Mat & input, cv::Mat & output, int size)
释放函数
extern "C" __declspec(dllexport) int xyc_seg_lib_release() 

2、onnx推理图像分割函数说明：
初始化函数 model_path模型路径，模型为：seg_5060.onnx
目前使用paddleseg的deeplabv3+的模型，1748*1748图像使用fp16推理需要215ms左右,目前使用方案中的5060的deeplabv3+的模型，1748*1748图像使用fp16推理需要245ms左右
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号
sizeh sizew输入宽高,可以不固定。
pool_size 为推理池支持的线程数
extern "C" __declspec(dllexport) int xyc_seg_init(const char* model_path, int gpu_id, int sizeh,int sizew, int pool_size) 

推理函数 cv::Mat & input 输入图像, cv::Mat & output 输出图像, int size输入宽高，onnx没有限定，
norm_type确定使用哪一种预处理方式：0为 mean = cv::Scalar(0.486, 0.456, 0.406); std = cv::Scalar(0.229, 0.224, 0.225);1为 mean = cv::Scalar(0.5, 0.5, 0.5);std = cv::Scalar(0.5, 0.5, 0.5); 
split_num为分割系数，为1的话就是推理原图，为2输入分成4份推理后合图，为4是分为16份。
extern "C" __declspec(dllexport) int xyc_seg50train_run(cv::Mat & input, cv::Mat & output,int norm_type, int split_num)，5060上训练出的onnx此处选0，paddle训练出的此处选1。

推理函数 cv::Mat & input 输入图像, cv::Mat & output 输出图像, int size输入宽高，onnx没有限定，      
extern "C" __declspec(dllexport) int xyc_seg_run(cv::Mat & input, cv::Mat & output)

推理函数 cv::Mat & input 输入图像, cv::Mat & output 输出图像, int size输入宽高，onnx没有限定，
split_num为分割系数，为1的话就是推理原图，为2输入分成4份推理后合图，为4是分为16份（如Init设置hw为1748*1748，输入为3496*3496，split_num需要设置为2，分割成4张然后推理拼接）。
extern "C" __declspec(dllexport) int xyc_seg_split_run(cv::Mat & input, cv::Mat & output, int split_num)

释放函数
extern "C" __declspec(dllexport) int xyc_seg_release() 

3、OCR推理接口
初始化函数 model_path模型路径，模型为：
dict_path为查询字典的路径，为：
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号
height、width为输入需要规定的高宽，这边默认为48、320
size输入宽高（目前只能是1728，如需改变需要pytorch训练代码调整尺寸重新生成模型）,
extern "C" __declspec(dllexport) int xyc_ocr_init(const char* model_path, const char* dict_path, int gpu_id, int height, int width, int pool_size)
推理函数 cv::Mat & input 输入图像,  
char* result_buffer 输出结果如：char result_buffer[256];
int buffer_size ， result_buffer的尺寸，sizeof(result_buffer)
float* confidence 返回的置信度
extern "C" __declspec(dllexport) int xyc_ocr_run(cv::Mat & input, char* result_buffer, int buffer_size, float* confidence)
释放函数
extern "C" __declspec(dllexport) int xyc_ocr_release()

4、分类推理接口
（1）弃用方法：
初始化函数 model_path模型路径，模型为：xyc_class_libtorch.pt
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号
extern "C" __declspec(dllexport) int xyc_class_init(const char* model_path, int gpu_id)
cv::Mat& image， 输入的图像矩阵
int& max_index, 最大的分类索引
float& max_score ，置信度
extern "C" __declspec(dllexport) int xyc_class_run(cv::Mat& image, int& max_index, float& max_score)
释放函数
extern "C" __declspec(dllexport) int xyc_class_release()
（2）老方法（不支持多线程）
初始化函数 model_path模型路径，模型为：capacitor_classifier.onnx
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号
extern "C" __declspec(dllexport) int xyc_class_init(const char* model_path, int gpu_id)
（3）新方法（支持多线程）
初始化函数 model_path模型路径，模型为：capacitor_classifier.onnx
gpu_id使用的gpu编号，可以设置0、1、2等GPU编号， size_t pool_size，最多支持的线程数
extern "C" __declspec(dllexport) int xyc_class_init_mthread(const char* model_path, int gpu_id, size_t pool_size = 4)

5、单产品无监督
创建模型接口函数
int index 创建模型唯一索引号 从0开始递增
const char* path  模型路径目录，gpu_id使用的gpu号
extern "C" _declspec(dllexport) int creatModel(int index, const char* path,int gpu_id)
模型检测接口函数
int index 创建模型时对应的模型号
cv::Mat & testImage检测输入图片
float anomalyThresh缺陷检测阈值 范围0到1
cv::Mat & resultImage 缺陷结果检测图（灰度图）
函数返回值1 为有缺陷， 0 为没缺陷 ， 0时可以快速处理不用显示。
extern "C" _declspec(dllexport) int anomalyDetMatin(int index, cv::Mat& testImage, float anomalyThresh,cv::Mat& resultImage)
释放单独模型接口函数
extern "C" _declspec(dllexport) void deinitModel(int index)
释放所有模型接口函数
extern "C" _declspec(dllexport) void deinitAllModel()

6、单产品无监督多线程版本
创建模型接口函数
int modelIndex 创建模型唯一索引号 从0开始递增
const char* modelRootDir  模型路径目录，gpu_id使用的gpu号
int max_threads 当前模型最多支持的线程数。
extern "C" _declspec(dllexport) int creatModel_mthread(int modelIndex, const char* modelRootDir, int gpu_id, int max_threads)
模型检测接口函数
int modelIndex 创建模型时对应的模型号
cv::Mat & testImage检测输入图片
float anomalyThresh缺陷检测阈值 范围0到1
cv::Mat & resultImage 缺陷结果检测图（灰度图）
函数返回值1 为有缺陷， 0 为没缺陷 ， 0时可以快速处理不用显示。
extern "C" _declspec(dllexport) int anomalyDetMatin_mthread(int modelIndex, const cv::Mat & testImage, float anomalyThresh, cv::Mat & resultImage)
释放单独模型接口函数
int modelIndex 需要释放的模型的序号
releaseModel(model_index)

7、多产品无监督
const char* path  模型路径目录，gpu_id使用的gpu号，pool_size最多支持几个线程
extern "C" __declspec(dllexport) int xyc_mul_us_init(const char* model_path, int gpu_id, int input_size, int pool_size)
cv::Mat & input检测输入图片
float defect_threshold 缺陷显示面积范围检测阈值 范围0到1
float global_threshold 是否为真缺陷调整检测阈值 范围0到1，晶圆项目中默认填0.1
cv::Mat & result_mask 缺陷结果检测图（灰度图）
extern "C" __declspec(dllexport) int xyc_mul_us_run(cv::Mat & input, float defect_threshold, float global_threshold, cv::Mat & result_mask)
释放所有模型接口函数
extern "C" __declspec(dllexport) int xyc_mul_us_release()

8、有监督缺陷分割
extern "C" __declspec(dllexport) int yolo11_seg_init(const char* model_path, const char* yaml_path, int gpu_id, int pool_size, float conf_threshold, float iou_threshold)
extern "C" __declspec(dllexport) int yolo11_seg_run(cv::Mat & image, cv::Mat & results)
extern "C" __declspec(dllexport) void yolo11_seg_cleanup()
```

**核心发现**: 该文件是 API 参考文档，涵盖 8 大功能模块的 DLL 导出接口规范。其中第 5、6 节（单产品无监督 + 多线程版本）是我们的核心关注点。

---

## 2. 核心 .cpp 文件摘要

### 2.1 unsupervised_fp16.cpp

- **体量**: 31,923 bytes (31KB)
- **文件状态**: E-SafeNet 加密，无法直接读取；通过 EncryCode.md 解密内容分析
- **功能**: 单产品无监督异常检测 — 单线程核心类 + 多线程包装器
- **入口 (dllexport)**:
  - `creatModel(index, path, gpu_id)` — 创建单线程模型
  - `anomalyDetMatin(index, testImage, anomalyThresh, resultImage)` — 单线程推理
  - `deinitModel(index)` / `deinitAllModel()` — 释放
  - `creatModel_mthread(modelIndex, modelRootDir, gpu_id, max_threads)` — 创建线程池
  - `anomalyDetMatin_mthread(modelIndex, testImage, anomalyThresh, resultImage)` — 多线程推理
  - `releaseModel(modelIndex)` — 释放线程池
- **核心类**: `ADetctCls` — 封装模型加载、推理、KNN 搜索
- **关键算子**:
  - `torch::jit::load` — 加载 TorchScript 模型
  - `module.forward` — 模型推理 (FP16 输入)
  - `torch::cdist` — KNN 距离计算 (循环分块)
  - `distances.topk` — 最近邻选取
  - `F::interpolate` — 上/下采样
  - `cv::GaussianBlur` — 后处理平滑
- **依赖**: libtorch, opencv, yaml-cpp
- **线程池**: 内含 `ModelThreadPool` 类，通过 mutex + condition_variable 管理 `ADetctCls_m` 实例池
- **FP16 相关关键词扫描**:
  ```
  EncryCode.md 中 unsupervised_fp16.cpp 段落扫描结果:
  - torch::kHalf      : 2处 (tensor_image1.to(kHalf), tensor_image1.to(kHalf) in PatchCore)
  - torch::kFloat16   : 3处 (tensor_image.to(kFloat16), tmpMb.to(kFloat16), anomaly_map in PatchCoreCUT)
  - torch::kFloat32   : 2处 (top_10.to(kFloat32), tensor_image.to(kFloat32) in anomalyDetBigMatinPatchCore)
  - "fp16" / "FP16"   : 0处 (仅文件名含 fp16)
  - "Half" / "half"   : 同 kHalf
  
  结论: 客户已实现 FP16 推理路径 — 输入 kHalf/kFloat16 + MB kFloat16 + 结果转回 kFloat32
  ```

### 2.2 unsupervised_fp16_thread.cpp

- **体量**: 33,351 bytes (33KB)
- **文件状态**: E-SafeNet 加密；通过 EncryCode.md 解密内容分析
- **功能**: 单产品无监督异常检测 — 独立的多线程版本，变量名后缀 `_m`/`_mthread`
- **入口 (dllexport)**: 与 unsupervised_fp16.cpp 的多线程部分完全一致
  - `creatModel_mthread` / `anomalyDetMatin_mthread` / `releaseModel` / `getModelPoolStatus`
- **核心类**: `ADetctCls_m` — 与 `ADetctCls` 结构完全一致，仅变量名不同
- **关键算子**: 同 unsupervised_fp16.cpp
- **依赖**: libtorch, opencv, yaml-cpp
- **与 unsupervised_fp16.cpp 的关系**: 代码几乎完全复制，独立维护
- **FP16 相关关键词扫描**:
  ```
  结果与 unsupervised_fp16.cpp 完全一致:
  - torch::kHalf      : 2处
  - torch::kFloat16   : 3处
  - torch::kFloat32   : 2处
  
  结论: FP16 路径完全相同
  ```

### 2.3 test_ad_infer_new.cpp

- **体量**: 28,915 bytes (28KB)
- **文件状态**: E-SafeNet 加密，无法读取，**EncryCode.md 中未包含此文件解密内容**
- **功能**: 推测为主测试程序，调用各种 DLL 导出接口进行推理测试
- **入口**: 推测有 `main` 函数
- **关键算子**: 无法确认
- **依赖**: 推测为 libtorch + opencv (调用上述 DLL 接口)
- **FP16 扫描**: 无法执行 (文件加密 + 未解密)

### 2.4 test_class.cpp

- **体量**: 29,808 bytes (29KB)
- **文件状态**: E-SafeNet 加密，无法读取，**EncryCode.md 中未包含此文件解密内容**
- **功能**: 推测为分类推理测试 (对应 API 文档第 4 节)
- **入口**: 推测有 `main` 函数
- **关键算子**: 推测调用 `xyc_class_init` / `xyc_class_run` 等接口
- **依赖**: 推测为 onnxruntime + opencv
- **FP16 扫描**: 无法执行 (文件加密 + 未解密)

---

## 3. 其他 .cpp (P1 简述)

| 文件 | 体量 | 功能 | 解密状态 |
|------|------|------|----------|
| `test_seg_onnx.cpp` | 55KB | ONNX Runtime 图像分割推理 (deeplabv3+)，支持多线程池、split 分块推理、多种归一化方式 | EncryCode.md 中有 (行4120-5605，含注释旧版+活跃新版) |
| `test_ocr_onnx.cpp` | 24KB | ONNX Runtime OCR 文字识别推理 (对应 API 文档第 3 节) | 未解密 |
| `yoloseg_onnx.cpp` | 70KB | YOLO11 实例分割推理 (对应 API 文档第 8 节)，最大文件 | 未解密 |
| `multi_class_demo.cpp` | 41KB | 多产品无监督异常检测 ONNX 版 (对应 API 文档第 7 节)，含 `AnomalyInferenceEngine` + `AnomalyInferencePool` | EncryCode.md 中有 (行5608-6712，大部分注释) |
| `justtestseg.cpp` | 8KB | 简单分割测试脚本 | 未解密 |

---

## 4. 依赖关系图

```
                        xyc_all_AI.dll (统一输出 DLL)
                               |
        +----------+-----------+-----------+-----------+----------+
        |          |           |           |           |          |
  unsupervised   unsupervised  test_seg   test_ocr   yoloseg   multi_class
  _fp16.cpp      _fp16_thread  _onnx.cpp  _onnx.cpp  _onnx.cpp demo.cpp
  (31KB)         .cpp (33KB)   (55KB)     (24KB)     (70KB)    (41KB)
        |          |           |           |           |          |
        v          v           v           v           v          v
  +-----------+  +-----------+  +-------------------+  +-------------------+
  | libtorch  |  | libtorch  |  | ONNX Runtime GPU  |  | ONNX Runtime GPU  |
  | 2.8.0     |  | 2.8.0     |  | 1.22.0            |  | 1.22.0            |
  | +cu129    |  | +cu129    |  +-------------------+  +-------------------+
  +-----------+  +-----------+
        |          |                    |
        +----+-----+          +--------+--------+
             |                |                  |
        +---------+     +---------+        +---------+
        | OpenCV  |     | OpenCV  |        | OpenCV  |
        +---------+     +---------+        +---------+
             |
        +---------+
        | yaml-cpp|
        +---------+

  DLL 导出入口:
  ====================================================
  unsupervised_fp16.cpp:
    creatModel / anomalyDetMatin / deinitModel / deinitAllModel        (单线程, 第5节)
    creatModel_mthread / anomalyDetMatin_mthread / releaseModel        (多线程, 第6节)

  unsupervised_fp16_thread.cpp:
    creatModel_mthread / anomalyDetMatin_mthread / releaseModel        (多线程, 第6节)
    getModelPoolStatus                                                  (未实现)

  test_seg_onnx.cpp:
    xyc_seg_init / xyc_seg_run / xyc_seg_split_run / xyc_seg_release   (ONNX分割, 第2节)
    xyc_seg_lib_init / xyc_seg_lib_run / xyc_seg_lib_release           (libtorch分割, 第1节)
    xyc_seg50train_run                                                  (分割变体)

  test_ocr_onnx.cpp:
    xyc_ocr_init / xyc_ocr_run / xyc_ocr_release                      (OCR, 第3节)

  multi_class_demo.cpp:
    xyc_mul_us_init / xyc_mul_us_run / xyc_mul_us_release              (多产品无监督, 第7节)

  yoloseg_onnx.cpp:
    yolo11_seg_init / yolo11_seg_run / yolo11_seg_cleanup              (有监督分割, 第8节)
  ====================================================

  test_ad_infer_new.cpp / test_class.cpp:
    推测为测试 harness (有 main)，调用上述 DLL 导出函数

  内部调用链 (unsupervised_fp16*.cpp):
  creatModel_mthread
    -> ModelThreadPool::initializePool
      -> ADetctCls[_m]::initializeModel
        -> torch::jit::load("model.ckpt")
        -> MB2CPUORGPU("model.mb")

  anomalyDetMatin_mthread
    -> ModelThreadPool::acquireInstance
    -> ADetctCls[_m]::anomalyDetMatin_mthread
      -> anomalyDetBigMatinPatchCoreCUT
        -> cv::resize (预处理)
        -> tensor.to(kHalf) (FP16 转换)
        -> module.forward (backbone 推理)
        -> nearest_neighbors (循环 cdist KNN)
        -> F::interpolate (上采样)
        -> cv::GaussianBlur (后处理)
    -> ModelThreadPool::releaseInstance
```

---

## 5. 模型文件清单

### Model\ 下 (单产品无监督模型)

| 文件 | 大小 | 修改时间 | 说明 |
|------|------|----------|------|
| model.ckpt | 22,500,243 (21.5MB) | 2025/10 | TorchScript 模型 |
| model.mb | 57,226,013 (54.6MB) | 2025/10 | Memory Bank (embeddings + cut_index) |
| modelConfig.yaml | 759 bytes | 2025/10 | 模型配置 |
| yoloseg.onnx | 89,678,031 (85.5MB) | 2025/10 | YOLO 分割模型 (非无监督) |
| data.yaml | 68 bytes | 2025/10 | YOLO 类别配置 (6类: back,qx1-qx5) |
| resizeall.py | 6,501 bytes | 2025/09 | 图像尺寸调整脚本 |
| 测试图片 | 若干 .jpg/.png | — | 5.jpg, 6.jpg, 7.jpg, 8.jpg 等 |
| 子目录 | 500x500, 1000x1000, 1500_1500 | — | 不同分辨率的测试图 |

modelConfig.yaml 关键参数:
```yaml
exportType: 1           # PatchCore (带 memory bank)
forwardSize: [1000, 1000]  # 模型输入尺寸
maxTrainSize: [51200, 51200]
maxSearchNaxFeatrueSize1: 130960000  # cdist 分块阈值
overlap: 0
overlap1: 16
scoreRatio: 4
num_neighbors: 1
poolSize: 3
```

### mulunsupModel\ 下 (多产品无监督模型)

| 文件 | 大小 | 说明 |
|------|------|------|
| many_anomaly_detection.onnx | 535,525,363 (510.7MB) | ONNX 多类异常检测模型 |
| many_anomaly_detection1.onnx | 535,462,132 (510.6MB) | 另一版 ONNX 模型 |
| many_anomaly_detection.pt | 150,267,948 (143.3MB) | TorchScript 版本 |
| 测试图片 | 0-3(1).jpg, 000.jpg, 001.jpg, binary_mask.png | 测试用 |

### 根目录下

| 文件 | 大小 | 说明 |
|------|------|------|
| model.onnx | 107MB (估) | 独立 ONNX 模型 (用途待确认) |

### 对比旧模型 `D:\XYC_Dog_Agent\anomalyDet\anomalyDet\infer\model\`

| 文件 | 旧模型 | 新模型 (Model\) | 差异 |
|------|--------|-----------------|------|
| model.ckpt | 44,866,526 (42.8MB) | 22,500,243 (21.5MB) | **新模型小一半** |
| model.mb | 12,533,915 (11.9MB) | 57,226,013 (54.6MB) | **新 MB 大 4.6 倍** |
| modelConfig.yaml | 759 bytes | 759 bytes | 大小相同 (内容可能不同) |

**重要发现**: 新旧模型 **不是同一份**。新 model.ckpt 缩小了约 50%，但 model.mb 膨胀了约 4.6x。这意味着新模型可能:
- backbone 更小 (或 FP16 导出)
- memory bank 更大 (更多特征/更高采样率)

---

## 6. 关键发现

1. **客户已实现 FP16 推理路径**: 输入 `kHalf` → forward → KNN `kFloat16` → 结果转回 `kFloat32`。这与我们 Python RU-005 的 FP16 方案一致。

2. **客户 KNN 仍用循环 cdist**: `nearest_neighbors` 函数依然采用分块循环调用 `torch::cdist`，每次 topk(1) 再 cat，与我们在 Python 侧验证的 23.63x 优化方案 (一次性 cdist + topk) 有巨大差距。

3. **客户缺少的加速手段** (对照 Python 侧已验证有效的):

   | 加速手段 | Python 侧证实有效 | 客户 C++ 是否有 |
   |----------|-------------------|-----------------|
   | FP16 模型 + FP16 输入 | YES | **YES** (已实现) |
   | cudnn.benchmark | YES | **NO** (未见 `setBenchmarkCuDNN`) |
   | channels_last 内存布局 | YES | **NO** (未见 `memory_format`) |
   | Memory Bank FP16 预缓存到 GPU | YES | **NO** (每次推理 `tmpMb.to(GPU).to(FP16)`) |
   | chunked GPU matmul (替代 cdist 循环) | YES | **NO** (仍用循环 cdist) |
   | Feature max_pool_4x 降维 | YES | **NO** (无 pooling 降维) |
   | Tensor Core 启用 (setFloat32MatmulPrecision) | YES | **NO** (未见相关调用) |

4. **新旧模型差异显著**: model.ckpt 缩小 50%，model.mb 膨胀 4.6x。推理性能特征可能与我们之前在旧模型上的实验数据不完全对应。

5. **两个多线程 .cpp 文件高度重复**: `unsupervised_fp16.cpp` 和 `unsupervised_fp16_thread.cpp` 的线程池代码几乎完全复制，仅变量名后缀不同 (`ADetctCls` vs `ADetctCls_m`)。这表明代码在快速迭代中未做充分重构。

---

## 7. 待确认问题

1. **test_ad_infer_new.cpp 和 test_class.cpp 内容**: 这两个 P0 文件因 E-SafeNet 加密且 EncryCode.md 中未包含解密内容，无法分析。需要用户补充解密。

2. **两个 .cpp 文件的编译关系**: `unsupervised_fp16.cpp` 和 `unsupervised_fp16_thread.cpp` 是分别编译成不同 DLL，还是条件编译选其一？它们有重名的 DLL 导出符号 (`creatModel_mthread` 等)，如果同时链接会冲突。

3. **新模型实际推理耗时**: 新 model.ckpt 比旧模型小一半，forward 时间可能不同。新 model.mb 大 4.6 倍，KNN 耗时可能更长。需要实测基线。

4. **`unsupervised_fp16.cpp` 中 `ModelThreadPool` 引用了 `ADetctCls_m`**: 但该类定义在 `unsupervised_fp16_thread.cpp` 中。这意味着实际编译时可能只用其中一个文件。

5. **LibTorch 2.8.0+cu129 确认**: 已从 `libtorch/build-version` 确认版本为 `2.8.0+cu129`，支持 sm_120 (RTX 5060 Ti)。但客户是否已成功在 5060 上运行推理？有无基线数据？

6. **forwardSize 为 [1000, 1000]**: 新模型输入尺寸为 1000x1000，与旧模型可能不同。对我们之前的 Python 实验数据有何影响？

7. **getModelPoolStatus 未实现**: `unsupervised_fp16_thread.cpp` 中导出了此函数但返回 -1。客户是否有计划完善？

---

## 附录: 环境信息

| 项目 | 值 |
|------|-----|
| LibTorch 版本 | 2.8.0+cu129 |
| ONNX Runtime 版本 | 1.22.0 (GPU) |
| VS 工程文件 | test_ad_infer_new.sln / test_ad_infer_new.vcxproj |
| .cpp 文件总数 | 9 个 |
| 加密方式 | E-SafeNet (文件含 null bytes) |
| EncryCode.md 已解密文件 | unsupervised_fp16.cpp, unsupervised_fp16_thread.cpp, test_seg_onnx.cpp (部分), multi_class_demo.cpp |
| EncryCode.md 未覆盖文件 | test_ad_infer_new.cpp, test_class.cpp, test_ocr_onnx.cpp, yoloseg_onnx.cpp, justtestseg.cpp |
