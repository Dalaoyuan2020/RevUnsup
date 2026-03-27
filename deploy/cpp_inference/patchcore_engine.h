#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * PatchCore C++ 推理引擎
 *
 * 部署架构（拆分式）：
 * - feature_extractor.onnx: ResNet18 特征提取
 * - memory_bank.npy: coreset embeddings
 * - deploy_config.json: 阈值 + 参数
 *
 * 推理流程：
 * 1. 图像预处理 (resize 256x256, normalize)
 * 2. ONNX Runtime 提取特征
 * 3. 与 memory bank 做最近邻搜索
 * 4. 计算异常分数
 * 5. 生成热力图 + 掩膜
 */

struct InferenceResult {
    float anomaly_score;
    bool is_defect;
    std::vector<float> heatmap;    // H*W flattened
    std::vector<uint8_t> mask;     // H*W binary mask
    int height;
    int width;
    float inference_time_ms;
};

struct DeployConfig {
    float image_threshold;
    float pixel_threshold;
    int input_height;
    int input_width;
    float coreset_sampling_ratio;
    int num_neighbors;
};

class PatchCoreEngine {
public:
    PatchCoreEngine();
    ~PatchCoreEngine();

    /**
     * 初始化引擎
     * @param onnx_path feature_extractor.onnx 路径
     * @param memory_bank_path memory_bank.npy 路径
     * @param config_path deploy_config.json 路径
     * @param gpu_id GPU 设备 ID
     * @return 0=成功, <0=失败
     */
    int init(
        const std::string& onnx_path,
        const std::string& memory_bank_path,
        const std::string& config_path,
        int gpu_id = 0
    );

    /**
     * 单张图片推理
     * @param image_path 输入图片路径
     * @return 推理结果
     */
    InferenceResult infer(const std::string& image_path);

    /**
     * 释放资源
     */
    void release();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
