#pragma once

/**
 * anomalyDet DLL 对外接口
 * 对齐旧版 AIAD_SDK.cs 调用约定
 *
 * C ABI，供 C# P/Invoke 调用
 */

#ifdef _WIN32
    #ifdef ANOMALYDET_EXPORTS
        #define ANOMALYDET_API __declspec(dllexport)
    #else
        #define ANOMALYDET_API __declspec(dllimport)
    #endif
#else
    #define ANOMALYDET_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化检测引擎
 * @param model_dir 模型目录（含 onnx + memory_bank + config）
 * @param gpu_id GPU 设备 ID
 * @return 0=成功
 */
ANOMALYDET_API int AIAD_Init(const char* model_dir, int gpu_id);

/**
 * 推理单张图片
 * @param image_path 图片路径
 * @param out_score [out] 异常分数
 * @param out_is_defect [out] 是否缺陷 (0/1)
 * @param out_heatmap [out] 热力图buffer (调用方分配, H*W*sizeof(float))
 * @param out_mask [out] 掩膜buffer (调用方分配, H*W*sizeof(uint8))
 * @return 0=成功
 */
ANOMALYDET_API int AIAD_Infer(
    const char* image_path,
    float* out_score,
    int* out_is_defect,
    float* out_heatmap,
    unsigned char* out_mask
);

/**
 * 释放引擎资源
 */
ANOMALYDET_API void AIAD_Release();

/**
 * 获取模型输入尺寸
 */
ANOMALYDET_API int AIAD_GetInputHeight();
ANOMALYDET_API int AIAD_GetInputWidth();

#ifdef __cplusplus
}
#endif
