# RU-020 编译手册

## 环境
- VS2022 Community 17.12+ (MSVC 14.44)
- LibTorch 2.8.0+cu129 (解包到 `D:\XYC_Dog_Agent\Reverse_unsupervised\code\code\test_ad_infer_new\libtorch`)
- CUDA 12.9 (与 LibTorch cu129 版本匹配)
- OpenCV 4.5.1 (同目录 `opencv\`)

## vcxproj 关键配置
- Platform: x64
- Configuration: Release
- IncludePath: libtorch\include; opencv\include
- LibraryPath: libtorch\lib; opencv\lib
- Link: torch.lib; c10.lib; torch_cpu.lib; torch_cuda.lib; opencv_world451.lib

## 命令行编译

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "D:\RevUnsup\cpp_patch\RU020_C_maxpool\test_ad_infer_new.vcxproj" `
  /p:Configuration=Release /p:Platform=x64
```

产物: `x64\Release\xyc_all_AI.dll` + `test_main.exe`

## 导出接口 (dllexport)

```cpp
extern "C" _declspec(dllexport) int creatModel(int modelIndex, const char* modelRootDir, int gpu_id);
extern "C" _declspec(dllexport) int anomalyDetMatin(int modelIndex, cv::Mat& testImage, float anomalyThresh, cv::Mat& resultImage);
extern "C" _declspec(dllexport) void deinitModel(int modelIndex);
extern "C" _declspec(dllexport) void deinitAllModel();
```

## C 版本核心改动

在 `module.forward` 之后、`permute` 之前插入:
```cpp
anomaly_map = torch::nn::functional::max_pool2d(
    anomaly_map,
    torch::nn::functional::MaxPool2dFuncOptions(4)
);
```

在 `creatModel` 内 `deviceGPU = ...` 之后插入 (B' + C 都有):
```cpp
at::globalContext().setBenchmarkCuDNN(true);
at::globalContext().setAllowTF32CuBLAS(true);
at::globalContext().setAllowFP16ReductionCuBLAS(true);
```
