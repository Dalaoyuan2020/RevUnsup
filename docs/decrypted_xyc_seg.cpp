// 来源：PaddleSeg/deploy/cpp/src/xyc_seg.cpp（E-SafeNet 加密，由羊爸爸通过记事本手动粘贴解密）
// 日期：2026-04-09

#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <numeric>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "paddle/include/paddle_inference_api.h"
#include "yaml-cpp/yaml.h"

#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;


typedef struct YamlConfig {
  std::string model_file;
  std::string params_file;
  bool is_normalize;
  bool is_resize;
  int resize_width;
  int resize_height;
}YamlConfig;

std::shared_ptr<paddle_infer::Predictor> seg_model;
YamlConfig seg_yaml_config;

YamlConfig load_yaml(const std::string& yaml_path) {
  YAML::Node node = YAML::LoadFile(yaml_path);
  std::string model_file = node["Deploy"]["model"].as<std::string>();
  std::string params_file = node["Deploy"]["params"].as<std::string>();
  YamlConfig yaml_config = {model_file, params_file};
  if (node["Deploy"]["transforms"]) 
  {
    const YAML::Node& transforms = node["Deploy"]["transforms"];
    for (size_t i = 0; i < transforms.size(); i++) 
    {
      if (transforms[i]["type"].as<std::string>() == "Normalize") 
      {
        yaml_config.is_normalize = true;

      } 
      else if (transforms[i]["type"].as<std::string>() == "Resize") 
      {
        yaml_config.is_resize = true;
        const YAML::Node& target_size = transforms[i]["target_size"];
        yaml_config.resize_width = target_size[0].as<int>();
        yaml_config.resize_height = target_size[1].as<int>();

      }
    }
  }
  return yaml_config;
}

void hwc_img_2_chw_data(const cv::Mat& hwc_img, float* data) {
  int rows = hwc_img.rows;
  int cols = hwc_img.cols;
  int chs = hwc_img.channels();
  for (int i = 0; i < chs; ++i) {
    cv::extractChannel(hwc_img, cv::Mat(rows, cols, CV_32FC1, data + i * rows * cols), i);
  }
}

void boxlistSort(std::vector<cv::Rect>& output, int sortflag)
{

    if (sortflag == 0)
    {
        for (int i = 0; i < output.size(); ++i)
        {
            bool isSwap = false;
            for (int j = output.size() - 1; j > i; --j)
            {
                if (output[j].x < output[j-1].x) {
                    swap(output[j], output[j-1]);
                    isSwap = true;
                }
            }
            if (isSwap == false) {
                break;
            }
        }
    }
    else if (sortflag == 1)
    {
        for (int i = 0; i < output.size(); ++i)
        {
            bool isSwap = false;
            for (int j = output.size() - 1; j > i; --j)
            {
                if (output[j].y < output[j - 1].y) {
                    swap(output[j], output[j - 1]);
                    isSwap = true;
                }
            }
            if (isSwap == false) {
                break;
            }
        }
    }
    else if (sortflag == 2)
    {
        for (int i = 0; i < output.size(); ++i)
        {
            bool isSwap = false;
            for (int j = output.size() - 1; j > i; --j)
            {
                if (output[j].x*output[j - 1].y > output[j].x * output[j - 1].y) {
                    swap(output[j], output[j - 1]);
                    isSwap = true;
                }
            }
            if (isSwap == false) {
                break;
            }
        }

    }
}
void Calc_Distance_Offset(std::vector<cv::Rect> output, int sortflag, int &Distance, int &Offset)
{
    float avgDistance = 0;
    float avgOffset = 0;
    if (sortflag == 0)
    {
        avgOffset = output[0].y + output[0].height / 2;
        for (int i = 0; i < output.size()-1; ++i)
        {
            float tempDistance = fabs(output[i].x + output[i].width / 2 - output[i + 1].x - output[i + 1].width / 2);
            avgDistance += tempDistance;
            avgOffset += output[i + 1].y + output[i + 1].height / 2;
        }
        avgDistance = avgDistance / (output.size() - 1);
        avgOffset = avgOffset / output.size();
    }
    else if (sortflag == 1)
    {
        avgOffset = output[0].x + output[0].width / 2;
        for (int i = 0; i < output.size() - 1; ++i)
        {
            float tempDistance = fabs(output[i].y + output[i].height / 2 - output[i + 1].y - output[i + 1].height / 2);
            avgDistance += tempDistance;
            avgOffset += output[i + 1].x + output[i + 1].width / 2;
        }
        avgDistance = avgDistance / (output.size() - 1);
        avgOffset = avgOffset / output.size();
    }
    
    Distance = int(avgDistance+0.5);
    Offset = int(avgOffset + 0.5);
}

//初始化 modelRootDir模型文件夹地址 gpu gpuID 默认为0
extern "C" __declspec(dllexport) int xyc_seg_init(const char* modelRootDir)
{
    try
    {
        const char* yaml_path = "/deploy";
        char* yaml_modelRootDir = new char[strlen(modelRootDir) + strlen(yaml_path) + 1];
        strcpy(yaml_modelRootDir, modelRootDir);
        strcat(yaml_modelRootDir, yaml_path);

        seg_yaml_config = load_yaml(yaml_modelRootDir);

        const char* temp_path = "/";
        char* modelpath = new char[strlen(modelRootDir) + strlen(temp_path) + 1];
        strcpy(modelpath, modelRootDir);
        strcat(modelpath, temp_path);

        paddle_infer::Config infer_config;
        infer_config.SetModel(modelpath + seg_yaml_config.model_file, modelpath + seg_yaml_config.params_file);
        infer_config.EnableMemoryOptim();
        infer_config.EnableUseGpu(100, 0);
        seg_model = paddle_infer::CreatePredictor(infer_config);
        delete yaml_modelRootDir;
        delete modelpath;
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
//推理 input输入图 output结果图
extern "C" __declspec(dllexport) int xyc_seg_run(cv::Mat & input,cv::Mat & output, float anomalyThresh )
{
    try
    {
	    cv::Mat img;
	    cv::cvtColor(input, img, cv::COLOR_BGR2RGB);
	    std::cout << seg_yaml_config.resize_width << "  " << seg_yaml_config.resize_height << std::endl;

	    if (seg_yaml_config.is_resize)
	    {
	      cv::resize(img, img, cv::Size(seg_yaml_config.resize_width, seg_yaml_config.resize_height));
	    }
	    if (seg_yaml_config.is_normalize) 
	    {
		    img.convertTo(img, CV_32F, 1.0 / 255, 0);
		    img = (img - 0.5) / 0.5;
	    }

	    int rows = img.rows;
	    int cols = img.cols;
	    int chs = img.channels();
	    std::cout << cols << "  " << rows << std::endl;
	    std::vector<float> input_data(1 * chs * rows * cols, 0.0f);
	    hwc_img_2_chw_data(img, input_data.data());

	    // Set input
	    auto input_names = seg_model->GetInputNames();
	    auto input_t = seg_model->GetInputHandle(input_names[0]);
	    std::vector<int> input_shape = { 1, chs, rows, cols };
	    input_t->Reshape(input_shape);
	    input_t->CopyFromCpu(input_data.data());

	    // Run
	    seg_model->Run();

	    // Get output
	    auto output_names = seg_model->GetOutputNames();
	    auto output_t = seg_model->GetOutputHandle(output_names[0]);
	    std::vector<int> output_shape = output_t->shape();

	    int out_num = std::accumulate(output_shape.begin(), output_shape.end(), 1,
		    std::multiplies<int>());
	    std::vector<int32_t> out_data(out_num);
	    output_t->CopyToCpu(out_data.data());

	    // Get pseudo image
	    std::vector<uint8_t> out_data_u8(out_num);
	    for (int i = 0; i < out_num; i++) {
		    out_data_u8[i] = static_cast<uint8_t>(out_data[i]);
	    }
	    cv::Mat out_gray_img(output_shape[1], output_shape[2], CV_8UC1, out_data_u8.data());
        cv::resize(out_gray_img, output, cv::Size(input.cols, input.rows));
	    return 0;
    }
    
    catch (...)
    {
        return -1;
    }
}

//反初始化 
extern "C" __declspec(dllexport) int xyc_seg_uninit()
{
    if (seg_model != nullptr)
    {
        seg_model.reset();
        seg_model = nullptr;
    }

	return 0;
}


int main(int argc, char* argv[])
{
    cv::Mat img = cv::imread("M250110002_P00001-A1-001_IntensityMap.jpg", cv::IMREAD_COLOR);
    xyc_seg_init("inference_model");

    cv::Mat output;
    {
        time_t start = clock();
        xyc_seg_run(img, output, 0.1);
        time_t end = clock();
        std::cout << "the running time is : " << double(end - start) / CLOCKS_PER_SEC << std::endl;
    }
    output = output * 100;
    cv::imwrite("result.jpg", output);

    xyc_seg_uninit();
    return 0;
}
