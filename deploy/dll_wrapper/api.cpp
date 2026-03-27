#include "api.h"
#include "../cpp_inference/patchcore_engine.h"

// TODO: Agent 3 实现
// 全局 engine 实例，封装为 C ABI 供 C# 调用

static PatchCoreEngine* g_engine = nullptr;

ANOMALYDET_API int AIAD_Init(const char* model_dir, int gpu_id) {
    // TODO: Agent 3 实现
    return -1;
}

ANOMALYDET_API int AIAD_Infer(
    const char* image_path,
    float* out_score,
    int* out_is_defect,
    float* out_heatmap,
    unsigned char* out_mask
) {
    // TODO: Agent 3 实现
    return -1;
}

ANOMALYDET_API void AIAD_Release() {
    // TODO: Agent 3 实现
}

ANOMALYDET_API int AIAD_GetInputHeight() { return 256; }
ANOMALYDET_API int AIAD_GetInputWidth() { return 256; }
