#include "patchcore_engine.h"
// TODO: Agent 3 实现
// 核心步骤:
// 1. init(): 加载 ONNX 模型 + memory bank + config
// 2. infer(): 预处理 -> 特征提取 -> 最近邻搜索 -> 异常分数 -> 热力图
// 3. release(): 释放 ONNX Runtime session 和 GPU 资源

struct PatchCoreEngine::Impl {
    // ONNX Runtime session
    // Memory bank data
    // Deploy config
    // TODO: Agent 3 填充
};

PatchCoreEngine::PatchCoreEngine() = default;
PatchCoreEngine::~PatchCoreEngine() = default;

int PatchCoreEngine::init(
    const std::string& onnx_path,
    const std::string& memory_bank_path,
    const std::string& config_path,
    int gpu_id
) {
    // TODO: Agent 3 实现
    return -1;
}

InferenceResult PatchCoreEngine::infer(const std::string& image_path) {
    // TODO: Agent 3 实现
    return {};
}

void PatchCoreEngine::release() {
    // TODO: Agent 3 实现
}
