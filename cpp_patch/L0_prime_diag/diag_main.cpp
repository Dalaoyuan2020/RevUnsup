// L0' FP16 Preprocessing Diagnosis
// Compares two preprocessing paths:
//   Path A: customer code (uint8 -> kHalf -> normalize)  [early FP16]
//   Path B: recommended  (uint8 -> kFloat -> normalize)  [late FP16]
// Then feeds both through backbone to check output divergence.

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

#include <torch/torch.h>
#include <torch/script.h>
#include <opencv2/opencv.hpp>

namespace F = torch::nn::functional;

struct DiagResult {
    std::string image_name;
    int width, height;
    // preprocessing diff
    float input_max_diff;
    float input_mean_diff;
    float input_val_A_sample;
    float input_val_B_sample;
    // backbone diff
    float bb_max_diff;
    float bb_mean_diff;
    float bb_val_A_sample;
    float bb_val_B_sample;
};

// Path A: Customer code order (uint8 -> GPU -> kHalf -> permute -> div -> normalize)
torch::Tensor preprocess_pathA(const cv::Mat& img, torch::Device& device) {
    auto opts = torch::TensorOptions().dtype(torch::kU8);
    torch::Tensor t = torch::from_blob(
        const_cast<uchar*>(img.data),
        {1, img.rows, img.cols, 3}, opts);

    t = t.to(device);
    t = t.to(torch::kHalf);          // ← EARLY FP16
    t = t.permute({0, 3, 1, 2});     // NCHW

    // normalize in FP16
    t = t.div_(255.0f);
    t[0][0] = t[0][0].sub_(0.485f).div_(0.229f);
    t[0][1] = t[0][1].sub_(0.456f).div_(0.224f);
    t[0][2] = t[0][2].sub_(0.406f).div_(0.225f);

    return t.to(torch::kFloat32);     // convert to FP32 for comparison
}

// Path B: Recommended order (uint8 -> GPU -> kFloat -> permute -> div -> normalize)
torch::Tensor preprocess_pathB(const cv::Mat& img, torch::Device& device) {
    auto opts = torch::TensorOptions().dtype(torch::kU8);
    torch::Tensor t = torch::from_blob(
        const_cast<uchar*>(img.data),
        {1, img.rows, img.cols, 3}, opts);

    t = t.to(device);
    t = t.to(torch::kFloat32);       // ← FP32 first
    t = t.permute({0, 3, 1, 2});     // NCHW

    // normalize in FP32
    t = t.div_(255.0f);
    t[0][0] = t[0][0].sub_(0.485f).div_(0.229f);
    t[0][1] = t[0][1].sub_(0.456f).div_(0.224f);
    t[0][2] = t[0][2].sub_(0.406f).div_(0.225f);

    return t;                          // already FP32
}

int main() {
    torch::NoGradGuard no_grad;
    std::cout << "=== L0' FP16 Preprocessing Diagnosis ===" << std::endl;
    std::cout << "CUDA available: " << torch::cuda::is_available() << std::endl;

    if (!torch::cuda::is_available()) {
        std::cerr << "ERROR: CUDA not available" << std::endl;
        return -1;
    }

    torch::Device device(torch::kCUDA, 0);

    // Load model for backbone comparison
    std::string modelPath = "D:\\XYC_Dog_Agent\\Reverse_unsupervised\\code\\code\\test_ad_infer_new\\Model\\model.ckpt";
    std::cout << "Loading model: " << modelPath << std::endl;
    torch::jit::script::Module model;
    try {
        model = torch::jit::load(modelPath);
        model.eval();
        model.to(device);
        std::cout << "Model loaded OK" << std::endl;
    } catch (const c10::Error& e) {
        std::cerr << "ERROR loading model: " << e.what() << std::endl;
        return -1;
    }

    // Test images
    std::string imgDir = "D:\\XYC_Dog_Agent\\Reverse_unsupervised\\code\\code\\test_ad_infer_new\\Model\\";
    std::vector<std::string> images = {"0.png", "5.jpg", "6.jpg", "7.jpg", "8.jpg"};

    std::vector<DiagResult> results;

    // Warmup
    {
        cv::Mat warmup(256, 256, CV_8UC3, cv::Scalar(128, 128, 128));
        auto tA = preprocess_pathA(warmup, device);
        auto tB = preprocess_pathB(warmup, device);
        try {
            auto outA = model.forward({tA.to(torch::kHalf)}).toTensor();
            auto outB = model.forward({tB.to(torch::kHalf)}).toTensor();
            std::cout << "Warmup backbone forward OK" << std::endl;
        } catch (const c10::Error& e) {
            std::cout << "Warmup backbone forward failed (expected if model needs specific size): " << e.what() << std::endl;
            std::cout << "Will skip backbone comparison" << std::endl;
        }
    }

    std::cout << "\n=== Running diagnosis on " << images.size() << " images ===" << std::endl;

    for (const auto& img_name : images) {
        std::string img_path = imgDir + img_name;
        cv::Mat img = cv::imread(img_path);
        if (img.empty()) {
            std::cerr << "WARNING: Cannot read " << img_path << std::endl;
            continue;
        }

        DiagResult dr;
        dr.image_name = img_name;
        dr.width = img.cols;
        dr.height = img.rows;

        std::cout << "\n--- " << img_name << " (" << img.cols << "x" << img.rows << ") ---" << std::endl;

        // Run both preprocessing paths
        auto tensorA = preprocess_pathA(img, device);  // customer path (early FP16)
        auto tensorB = preprocess_pathB(img, device);  // correct path (FP32)

        // Compare preprocessing output
        auto input_diff = (tensorA - tensorB).abs();
        dr.input_max_diff = input_diff.max().item<float>();
        dr.input_mean_diff = input_diff.mean().item<float>();

        // Sample values at [0,0,0,0] and [0,1,0,0] and [0,2,0,0]
        dr.input_val_A_sample = tensorA[0][0][0][0].item<float>();
        dr.input_val_B_sample = tensorB[0][0][0][0].item<float>();

        std::cout << "  [Input] max_abs_diff: " << std::fixed << std::setprecision(6) << dr.input_max_diff << std::endl;
        std::cout << "  [Input] mean_abs_diff: " << dr.input_mean_diff << std::endl;
        std::cout << "  [Input] sample [0,0,0,0]: A=" << dr.input_val_A_sample
                  << " B=" << dr.input_val_B_sample
                  << " diff=" << std::abs(dr.input_val_A_sample - dr.input_val_B_sample) << std::endl;

        // Per-channel stats
        for (int c = 0; c < 3; c++) {
            auto ch_diff = (tensorA[0][c] - tensorB[0][c]).abs();
            std::cout << "  [Input] ch" << c << " max_diff=" << ch_diff.max().item<float>()
                      << " mean_diff=" << ch_diff.mean().item<float>() << std::endl;
        }

        // Value distribution comparison
        std::cout << "  [Input] A range: [" << tensorA.min().item<float>() << ", " << tensorA.max().item<float>() << "]" << std::endl;
        std::cout << "  [Input] B range: [" << tensorB.min().item<float>() << ", " << tensorB.max().item<float>() << "]" << std::endl;

        // Backbone comparison (Step 4)
        dr.bb_max_diff = -1;
        dr.bb_mean_diff = -1;
        dr.bb_val_A_sample = 0;
        dr.bb_val_B_sample = 0;

        try {
            // Feed both through backbone (model expects FP16 input)
            auto outA = model.forward({tensorA.to(torch::kHalf)}).toTensor().to(torch::kFloat32);
            auto outB = model.forward({tensorB.to(torch::kHalf)}).toTensor().to(torch::kFloat32);

            auto bb_diff = (outA - outB).abs();
            dr.bb_max_diff = bb_diff.max().item<float>();
            dr.bb_mean_diff = bb_diff.mean().item<float>();

            // Sample backbone output
            dr.bb_val_A_sample = outA.flatten()[0].item<float>();
            dr.bb_val_B_sample = outB.flatten()[0].item<float>();

            std::cout << "  [Backbone] output shape: " << outA.sizes() << std::endl;
            std::cout << "  [Backbone] max_abs_diff: " << dr.bb_max_diff << std::endl;
            std::cout << "  [Backbone] mean_abs_diff: " << dr.bb_mean_diff << std::endl;
            std::cout << "  [Backbone] sample[0]: A=" << dr.bb_val_A_sample
                      << " B=" << dr.bb_val_B_sample << std::endl;

            // Relative error
            auto outB_abs = outB.abs();
            auto mask = outB_abs > 1e-6;
            if (mask.any().item<bool>()) {
                auto rel_err = bb_diff.index({mask}) / outB_abs.index({mask});
                std::cout << "  [Backbone] relative_error: max=" << rel_err.max().item<float>()
                          << " mean=" << rel_err.mean().item<float>() << std::endl;
            }

        } catch (const c10::Error& e) {
            std::cout << "  [Backbone] forward failed: " << e.what() << std::endl;
        }

        results.push_back(dr);
    }

    // Summary table
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << std::left << std::setw(10) << "Image"
              << std::right << std::setw(8) << "Size"
              << std::setw(14) << "In_MaxDiff"
              << std::setw(14) << "In_MeanDiff"
              << std::setw(14) << "BB_MaxDiff"
              << std::setw(14) << "BB_MeanDiff"
              << std::setw(12) << "Verdict"
              << std::endl;
    std::cout << std::string(86, '-') << std::endl;

    for (const auto& dr : results) {
        std::string size_str = std::to_string(dr.width) + "x" + std::to_string(dr.height);
        std::string verdict;
        if (dr.input_max_diff > 0.1f) verdict = "SEVERE";
        else if (dr.input_max_diff > 0.01f) verdict = "BUG";
        else if (dr.input_max_diff > 0.001f) verdict = "minor";
        else verdict = "clean";

        std::cout << std::left << std::setw(10) << dr.image_name
                  << std::right << std::setw(8) << size_str
                  << std::setw(14) << std::fixed << std::setprecision(6) << dr.input_max_diff
                  << std::setw(14) << dr.input_mean_diff;
        if (dr.bb_max_diff >= 0) {
            std::cout << std::setw(14) << dr.bb_max_diff
                      << std::setw(14) << dr.bb_mean_diff;
        } else {
            std::cout << std::setw(14) << "N/A"
                      << std::setw(14) << "N/A";
        }
        std::cout << std::setw(12) << verdict << std::endl;
    }

    // Verdict
    float worst_input = 0, worst_bb = 0;
    for (const auto& dr : results) {
        worst_input = std::max(worst_input, dr.input_max_diff);
        if (dr.bb_max_diff > 0) worst_bb = std::max(worst_bb, dr.bb_max_diff);
    }
    std::cout << "\n=== VERDICT ===" << std::endl;
    std::cout << "Worst input max_diff: " << worst_input << std::endl;
    std::cout << "Worst backbone max_diff: " << worst_bb << std::endl;
    if (worst_input > 0.01f) {
        std::cout << ">>> FP16 PREPROCESSING BUG CONFIRMED <<<" << std::endl;
        std::cout << "Recommendation: Fix preprocessing order (normalize in FP32, then convert to FP16)" << std::endl;
    } else {
        std::cout << ">>> No significant preprocessing difference detected <<<" << std::endl;
    }

    std::cout << "\nDone." << std::endl;
    return 0;
}
