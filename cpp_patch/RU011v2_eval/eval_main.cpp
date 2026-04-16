// RU-011-v2: ChipROI02 evaluation harness using customer-pipeline trained model
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <numeric>
#include <cmath>
#include <filesystem>
#include <windows.h>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// DLL export signatures
extern "C" __declspec(dllimport) int creatModel(int modelIndex, const char* modelRootDir, int gpu_id);
extern "C" __declspec(dllimport) int anomalyDetMatin(int modelIndex, cv::Mat& testImage, float anomalyThresh, cv::Mat& resultImage);
extern "C" __declspec(dllimport) void deinitAllModel();

struct EvalResult {
    std::string image_name;
    std::string category;  // good / real_bad / synth_bad
    int width;
    int height;
    double runs[5];
    double avg;
    double std_dev;
    float anomaly_score;
    int ret_code;
};

std::vector<std::string> listImages(const std::string& dir) {
    std::vector<std::string> result;
    if (!fs::exists(dir)) return result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string ext = entry.path().extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
            result.push_back(entry.path().string());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

int main() {
    std::cout << "=== RU-011-v2 ChipROI02 Evaluation (customer pipeline) ===" << std::endl;

    // 1. Init model (ChipROI02 trained model)
    const char* modelDir = "D:\\RevUnsup\\cpp_patch\\RU011v2_eval\\model";
    std::cout << "Loading model from: " << modelDir << std::endl;

    auto t0 = std::chrono::high_resolution_clock::now();
    int ret = creatModel(0, modelDir, 0);
    auto t1 = std::chrono::high_resolution_clock::now();
    double init_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "creatModel returned: " << ret << " (" << init_ms << " ms)" << std::endl;
    if (ret != 0) {
        std::cerr << "ERROR: creatModel failed with code " << ret << std::endl;
        return -1;
    }

    // 2. Collect images by category
    std::string baseDir = "D:\\RevUnsup\\cpp_patch\\RU011v2_eval\\test_images\\";
    struct Category { std::string name; std::string path; };
    std::vector<Category> categories = {
        {"good",      baseDir + "good"},
        {"real_bad",  baseDir + "real_bad"},
        {"synth_bad", baseDir + "synth_bad"},
    };

    const int NUM_RUNS = 5;
    const float THRESH = 0.1f;
    std::vector<EvalResult> results;

    // 3. Warmup
    {
        auto imgs = listImages(categories[0].path);
        if (!imgs.empty()) {
            cv::Mat warmup_img = cv::imread(imgs[0]);
            if (!warmup_img.empty()) {
                std::cout << "\n--- Warmup (3 runs) ---" << std::endl;
                for (int i = 0; i < 3; i++) {
                    cv::Mat result_img;
                    auto wt0 = std::chrono::high_resolution_clock::now();
                    int r = anomalyDetMatin(0, warmup_img, THRESH, result_img);
                    auto wt1 = std::chrono::high_resolution_clock::now();
                    double ms = std::chrono::duration<double, std::milli>(wt1 - wt0).count();
                    std::cout << "Warmup " << i+1 << ": " << ms << " ms (ret=" << r << ")" << std::endl;
                }
            }
        }
    }

    // 4. Evaluate all 18 images
    for (const auto& cat : categories) {
        auto imgs = listImages(cat.path);
        std::cout << "\n=== " << cat.name << " (" << imgs.size() << " images) ===" << std::endl;

        for (const auto& img_path : imgs) {
            cv::Mat img = cv::imread(img_path);
            if (img.empty()) {
                std::cerr << "WARNING: Cannot read " << img_path << std::endl;
                continue;
            }

            EvalResult er;
            // Extract filename
            size_t sep = img_path.find_last_of("\\/");
            er.image_name = (sep != std::string::npos) ? img_path.substr(sep + 1) : img_path;
            er.category = cat.name;
            er.width = img.cols;
            er.height = img.rows;

            for (int r = 0; r < NUM_RUNS; r++) {
                cv::Mat result_img;
                auto rt0 = std::chrono::high_resolution_clock::now();
                er.ret_code = anomalyDetMatin(0, img, THRESH, result_img);
                auto rt1 = std::chrono::high_resolution_clock::now();
                er.runs[r] = std::chrono::duration<double, std::milli>(rt1 - rt0).count();

                if (!result_img.empty()) {
                    er.anomaly_score = static_cast<float>(cv::countNonZero(result_img))
                                       / (result_img.rows * result_img.cols);
                } else {
                    er.anomaly_score = 0.0f;
                }
            }

            // avg + std
            double sum = 0;
            for (int r = 0; r < NUM_RUNS; r++) sum += er.runs[r];
            er.avg = sum / NUM_RUNS;
            double sq = 0;
            for (int r = 0; r < NUM_RUNS; r++) sq += (er.runs[r] - er.avg) * (er.runs[r] - er.avg);
            er.std_dev = std::sqrt(sq / NUM_RUNS);

            results.push_back(er);

            char buf[256];
            snprintf(buf, sizeof(buf), "  %-45s %4dx%-4d avg=%.1fms std=%.1f ret=%d score=%.4f",
                     er.image_name.c_str(), er.width, er.height,
                     er.avg, er.std_dev, er.ret_code, er.anomaly_score);
            std::cout << buf << std::endl;
        }
    }

    // 5. Summary table
    std::cout << "\n=== FULL SUMMARY ===" << std::endl;
    std::cout << "Category  | Image                                          | Size      | Avg ms | Std  | ret | score  | Correct?" << std::endl;
    std::cout << "----------|------------------------------------------------|-----------|--------|------|-----|--------|--------" << std::endl;

    int good_correct = 0, real_correct = 0, synth_correct = 0;
    int good_total = 0, real_total = 0, synth_total = 0;

    for (const auto& er : results) {
        bool expected_ret = (er.category != "good");  // good->ret=0, bad->ret=1
        bool correct = (er.ret_code == (expected_ret ? 1 : 0));

        if (er.category == "good")      { good_total++;  if (correct) good_correct++;  }
        else if (er.category == "real_bad")  { real_total++;  if (correct) real_correct++;  }
        else if (er.category == "synth_bad") { synth_total++; if (correct) synth_correct++; }

        char line[512];
        snprintf(line, sizeof(line), "%-9s | %-46s | %4dx%-4d | %6.1f | %4.1f | %3d | %.4f | %s",
                 er.category.c_str(), er.image_name.c_str(),
                 er.width, er.height, er.avg, er.std_dev,
                 er.ret_code, er.anomaly_score,
                 correct ? "YES" : "NO");
        std::cout << line << std::endl;
    }

    std::cout << "\n=== ACCURACY ===" << std::endl;
    std::cout << "Good correct:      " << good_correct  << "/" << good_total  << std::endl;
    std::cout << "Real bad correct:  " << real_correct  << "/" << real_total  << std::endl;
    std::cout << "Synth bad correct: " << synth_correct << "/" << synth_total << std::endl;
    std::cout << "Overall: " << (good_correct + real_correct + synth_correct)
              << "/" << (good_total + real_total + synth_total) << std::endl;

    // 6. Cleanup
    deinitAllModel();
    std::cout << "\nDone. Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
