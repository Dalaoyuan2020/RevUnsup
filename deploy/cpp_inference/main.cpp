#include "patchcore_engine.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: patchcore_infer --model <dir> --image <path>" << std::endl;
        std::cerr << "  --model: directory containing onnx + memory_bank + config" << std::endl;
        std::cerr << "  --image: input image path" << std::endl;
        return 1;
    }

    std::string model_dir;
    std::string image_path;

    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--model") model_dir = argv[i + 1];
        else if (arg == "--image") image_path = argv[i + 1];
    }

    PatchCoreEngine engine;
    int ret = engine.init(
        model_dir + "/feature_extractor.onnx",
        model_dir + "/memory_bank.npy",
        model_dir + "/deploy_config.json"
    );
    if (ret != 0) {
        std::cerr << "Engine init failed: " << ret << std::endl;
        return ret;
    }

    auto result = engine.infer(image_path);
    std::cout << "Anomaly Score: " << result.anomaly_score << std::endl;
    std::cout << "Is Defect: " << (result.is_defect ? "YES" : "NO") << std::endl;
    std::cout << "Inference Time: " << result.inference_time_ms << " ms" << std::endl;

    engine.release();
    return 0;
}
