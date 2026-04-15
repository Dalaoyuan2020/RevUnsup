// L0 Baseline test harness - calls DLL exports to measure inference time
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <numeric>
#include <cmath>
#include <windows.h>
#include <opencv2/opencv.hpp>

// DLL export signatures (from test_ad_infer_new.cpp)
extern "C" __declspec(dllimport) int creatModel(int modelIndex, const char* modelRootDir, int gpu_id);
extern "C" __declspec(dllimport) int anomalyDetMatin(int modelIndex, cv::Mat& testImage, float anomalyThresh, cv::Mat& resultImage);
extern "C" __declspec(dllimport) void deinitModel(int modelIndex);
extern "C" __declspec(dllimport) void deinitAllModel();

struct BenchResult {
    std::string image_name;
    int width;
    int height;
    double runs[5];
    double avg;
    double std_dev;
    float anomaly_score;
    int ret_code;
};

int main() {
    std::cout << "=== L0 Baseline Test ===" << std::endl;

    // 1. Init model
    const char* modelDir = "D:\\XYC_Dog_Agent\\Reverse_unsupervised\\code\\code\\test_ad_infer_new\\Model";
    std::cout << "Loading model from: " << modelDir << std::endl;

    auto t_init_start = std::chrono::high_resolution_clock::now();
    int ret = creatModel(0, modelDir, 0);
    auto t_init_end = std::chrono::high_resolution_clock::now();
    double init_ms = std::chrono::duration<double, std::milli>(t_init_end - t_init_start).count();

    std::cout << "creatModel returned: " << ret << " (" << init_ms << " ms)" << std::endl;
    if (ret != 0) {
        std::cerr << "ERROR: creatModel failed with code " << ret << std::endl;
        return -1;
    }

    // 2. Test images
    std::string imgDir = "D:\\XYC_Dog_Agent\\Reverse_unsupervised\\code\\code\\test_ad_infer_new\\Model\\";
    std::vector<std::string> images = { "0.png", "5.jpg", "6.jpg", "7.jpg", "8.jpg" };

    const int NUM_RUNS = 5;
    const float THRESH = 0.1f;
    std::vector<BenchResult> results;

    // Warmup: 3 runs on first image
    {
        cv::Mat warmup_img = cv::imread(imgDir + images[0]);
        if (!warmup_img.empty()) {
            std::cout << "\n--- Warmup (3 runs) ---" << std::endl;
            for (int i = 0; i < 3; i++) {
                cv::Mat result_img;
                auto t0 = std::chrono::high_resolution_clock::now();
                int r = anomalyDetMatin(0, warmup_img, THRESH, result_img);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "Warmup " << i + 1 << ": " << ms << " ms (ret=" << r << ")" << std::endl;
            }
        }
    }

    // 3. Benchmark: 5 images x 5 runs
    std::cout << "\n=== Benchmark: 5 images x " << NUM_RUNS << " runs ===" << std::endl;

    for (const auto& img_name : images) {
        std::string img_path = imgDir + img_name;
        cv::Mat img = cv::imread(img_path);
        if (img.empty()) {
            std::cerr << "WARNING: Cannot read " << img_path << std::endl;
            continue;
        }

        BenchResult br;
        br.image_name = img_name;
        br.width = img.cols;
        br.height = img.rows;

        for (int r = 0; r < NUM_RUNS; r++) {
            cv::Mat result_img;
            auto t0 = std::chrono::high_resolution_clock::now();
            br.ret_code = anomalyDetMatin(0, img, THRESH, result_img);
            auto t1 = std::chrono::high_resolution_clock::now();
            br.runs[r] = std::chrono::duration<double, std::milli>(t1 - t0).count();

            // Record anomaly score from result (count non-zero pixels as rough metric)
            if (!result_img.empty()) {
                br.anomaly_score = static_cast<float>(cv::countNonZero(result_img)) / (result_img.rows * result_img.cols);
            } else {
                br.anomaly_score = 0.0f;
            }
        }

        // Compute avg and std
        double sum = 0;
        for (int r = 0; r < NUM_RUNS; r++) sum += br.runs[r];
        br.avg = sum / NUM_RUNS;

        double sq_sum = 0;
        for (int r = 0; r < NUM_RUNS; r++) sq_sum += (br.runs[r] - br.avg) * (br.runs[r] - br.avg);
        br.std_dev = std::sqrt(sq_sum / NUM_RUNS);

        results.push_back(br);

        std::cout << img_name << " (" << br.width << "x" << br.height << "): ";
        for (int r = 0; r < NUM_RUNS; r++) {
            std::cout << br.runs[r] << " ";
        }
        std::cout << "| avg=" << br.avg << " std=" << br.std_dev
                  << " ret=" << br.ret_code << " score=" << br.anomaly_score << std::endl;
    }

    // 4. Summary
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Image        | Size     | Run1    | Run2    | Run3    | Run4    | Run5    | Avg     | Std    | Score" << std::endl;
    std::cout << "-------------|----------|---------|---------|---------|---------|---------|---------|--------|------" << std::endl;
    double total_avg = 0;
    for (const auto& br : results) {
        char line[512];
        snprintf(line, sizeof(line), "%-12s | %4dx%-4d | %7.1f | %7.1f | %7.1f | %7.1f | %7.1f | %7.1f | %6.1f | %.4f",
                 br.image_name.c_str(), br.width, br.height,
                 br.runs[0], br.runs[1], br.runs[2], br.runs[3], br.runs[4],
                 br.avg, br.std_dev, br.anomaly_score);
        std::cout << line << std::endl;
        total_avg += br.avg;
    }
    if (!results.empty()) {
        std::cout << "\nOverall average: " << total_avg / results.size() << " ms" << std::endl;
    }

    // 5. Cleanup
    deinitAllModel();
    std::cout << "\nDone. Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
