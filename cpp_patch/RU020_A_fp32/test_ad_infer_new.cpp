// 来源：ad_code/infer/demo.cpp（E-SafeNet 加密，由羊爸爸通过记事本手动粘贴解密）
// 日期：2026-04-09

#include<opencv2/opencv.hpp>
#define no_init_all deprecated
#include <torch/torch.h>
#include <torch/script.h> 
#include<iostream>
#include<memory>
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include <yaml-cpp/node/parse.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//#include <cuda_runtime_api.h>
using namespace cv;
using namespace std;

using namespace torch::indexing;

#define EXETEST

float amax122 = 0;
const int maxCutModel = 400;
const int maxDetectModel = 400;
struct model {
	torch::jit::script::Module module;
	int forwardSize[2] = { -1,-1 };
	bool inited = false;

};
model models[maxCutModel];
namespace F = torch::nn::functional;
bool cudaUseableSymbol = torch::cuda::is_available();
torch::Device deviceGPU(torch::kCUDA, 0);
torch::Device deviceCPU(torch::kCPU);
torch::autograd::AutoGradMode guard(false);
void test_libtorch_version() {
	std::cout << "cuDNN : " << torch::cuda::cudnn_is_available() << std::endl;
	std::cout << "CUDA : " << torch::cuda::is_available() << std::endl;
	std::cout << "Device count : " << torch::cuda::device_count() << std::endl;
}

int gammaCorrection(cv::Mat srcMat, cv::Mat& dstMat, float gamma) {

	unsigned char lut[256];

	for (int i = 0; i < 256; i++)
	{
		lut[i] = cv::saturate_cast<uchar>(pow((float)(i / 255.0f), gamma) * 255.0f);
	}

	srcMat.copyTo(dstMat);

	cv::MatIterator_<uchar> it, end;
	for (it = dstMat.begin<uchar>(), end = dstMat.end<uchar>(); it != end; it++) {
		*it = lut[(*it)];
	}

	return 0;

}
extern "C" class _declspec(dllexport) ADetctCls
{
public:


	ADetctCls() {


	}
	torch::jit::script::Module module;

	int fowardModelIndex;

	bool ratioMapLInited = false;
	float scoreRatio = 1;
	std::vector<int> maxTrainSize;
	std::vector<int> forwardSize;
	std::string modelPath;
	int modelIndex = 1;
	int exportType;

	int mbType = 1;
	string memorybankPath;

	int num_patches_h;
	int num_patches_w;

	int input_w;
	int input_h;
	int resized_h;
	int resized_w;
	int pad_w;
	int pad_h;
	int overlap1 = 16;
	int cut_index_array[400][400][6][4];
	float maxSearchNaxFeatrueSize1 = 1396000000;

	list<torch::Tensor> tmpMbLIST;
	vector<char> get_the_bytes(string filename) 
	{

		ifstream input(filename, ios::binary);
		if (!input.is_open())
		{
			cout << "文件打开失败" << endl;

		}

		vector<char> bytes(
			(std::istreambuf_iterator<char>(input)),
			(std::istreambuf_iterator<char>()));

		input.close();
		return bytes;
	}


	void MB2CPUORGPU(const char* memorybankPathin, int  type)
	{
		torch::Tensor cut_index;
		torch::Tensor cut_index1;

		std::vector<char> f = get_the_bytes(memorybankPathin);
		torch::IValue x = torch::pickle_load(f);
		c10::Dict<torch::IValue, torch::IValue>  tpl = x.toGenericDict();

		auto embeddings = tpl.at("embeddings").toList();
		int embeddingsSize = embeddings.size();
		for (int i = 0; i < embeddingsSize; i++)
		{
			torch::IValue h = embeddings[i];
			torch::Tensor tmpMb = h.toTensor();
			tmpMbLIST.push_back(tmpMb);
		}
		embeddings.clear();

		cut_index = tpl.at("cut_index").toTensor();
		cut_index1 = tpl.at("cut_index1").toTensor();

		num_patches_h = cut_index.size(0);
		num_patches_w = cut_index.size(1);

		input_w = cut_index1[0][2].item<int>();
		input_h = cut_index1[0][3].item<int>();
		resized_h = cut_index1[1][2].item<int>();
		resized_w = cut_index1[1][3].item<int>();
		pad_w = cut_index1[2][2].item<int>();
		pad_h = cut_index1[2][3].item<int>();

		for (int h_iterNum = 0; h_iterNum < num_patches_h; h_iterNum++)
		{
			for (int W_iterNum = 0; W_iterNum < num_patches_w; W_iterNum++)
			{
				double t9999 = (double)getTickCount();
				cut_index_array[h_iterNum][W_iterNum][0][0] = cut_index.index({ h_iterNum, W_iterNum, 0, 0 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][0][1] = cut_index.index({ h_iterNum, W_iterNum, 0, 1 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][0][2] = cut_index.index({ h_iterNum, W_iterNum, 0, 2 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][0][3] = cut_index.index({ h_iterNum, W_iterNum, 0, 3 }).item<int>();

				cut_index_array[h_iterNum][W_iterNum][2][0] = cut_index.index({ h_iterNum, W_iterNum, 2, 0 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][2][1] = cut_index.index({ h_iterNum, W_iterNum, 2, 1 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][2][2] = cut_index.index({ h_iterNum, W_iterNum, 2, 2 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][2][3] = cut_index.index({ h_iterNum, W_iterNum, 2, 3 }).item<int>();

				cut_index_array[h_iterNum][W_iterNum][1][0] = cut_index.index({ h_iterNum, W_iterNum, 1, 0 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][1][1] = cut_index.index({ h_iterNum, W_iterNum,1, 1 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][1][2] = cut_index.index({ h_iterNum, W_iterNum, 1, 2 }).item<int>();
				cut_index_array[h_iterNum][W_iterNum][1][3] = cut_index.index({ h_iterNum, W_iterNum,1, 3 }).item<int>();

				int mbIndexStartIndex = h_iterNum * num_patches_w + W_iterNum;

			}
		}
	}



	int initializeModel(const char* folderPath, int  useCuda)
	{

		std::string tempfolderPath = folderPath;
		std::string modelPath = tempfolderPath + "\\model.ckpt";
		memorybankPath = tempfolderPath + "\\model.mb";
		
		string configPath = tempfolderPath + "\\modelConfig.yaml";
		struct stat buffer;
		if (stat(configPath.c_str(), &buffer) == 0)
		{
			YAML::Node config = YAML::LoadFile(configPath);

			modelIndex = config["modelindex"].as<int>();
			maxTrainSize = config["maxTrainSize"].as<std::vector<int>>();
			forwardSize = config["forwardSize"].as<std::vector<int>>();

			exportType = config["exportType"].as<int>();;
			overlap1 = config["overlap"].as<int>();;
			scoreRatio = config["scoreRatio"].as<float>();;

			maxSearchNaxFeatrueSize1 = config["maxSearchNaxFeatrueSize1"].as<float>();;

		}

		try
		{
			module = torch::jit::load(modelPath);
			module.eval();
			module.to(deviceGPU);

			for (int searchIndex = 0; searchIndex < maxCutModel; searchIndex++)
			{

				if (!(models[searchIndex].inited))
				{
					models[searchIndex].module = torch::jit::load(modelPath);
					models[searchIndex].module.eval();

					if (useCuda && cudaUseableSymbol)
					{
						models[searchIndex].module.to(deviceGPU);
					}
					models[searchIndex].forwardSize[0] = forwardSize[0];
					models[searchIndex].forwardSize[1] = forwardSize[1];
					if (exportType == 2)
					{
						models[searchIndex].forwardSize[0] = -1;
						models[searchIndex].forwardSize[1] = -1;
					}
					fowardModelIndex = searchIndex;
					models[searchIndex].inited = true;

					break;
				}
				if ((models[searchIndex].forwardSize[0] == forwardSize[0]) && (models[searchIndex].forwardSize[1] == forwardSize[1]))

				{
					fowardModelIndex = searchIndex;
					break;
				}
			}

			if (exportType == 1)
				MB2CPUORGPU(memorybankPath.c_str(), mbType);

		}
		
		catch (const c10::Error& e)
		{
			std::cerr << "error";
			return -1;

		}
		return 0;
	}

	void deinitModel()
	{

		if (exportType == 1)
			tmpMbLIST.clear();

	}

	torch::Tensor  nearest_neighbors(torch::Tensor embedding, torch::Tensor self_memory_bank, int n_neighbors)
	{


		int embedding__size0 = embedding.size(0);
		float spanlen1 = ceil(float(maxSearchNaxFeatrueSize1) / float(embedding__size0));
		int selfmemorybanksize0 = self_memory_bank.size(0);
		int iterTimes = ceil(float(selfmemorybanksize0) / spanlen1);
		int  spanlen = ceil(selfmemorybanksize0 / iterTimes);


		int start = 0;
		int end = min(spanlen, selfmemorybanksize0);
		int l1 = self_memory_bank.size(0);
		int l2 = self_memory_bank.size(1);
		int l3 = embedding.size(0);
		int l4 = embedding.size(1);
	
		torch::Tensor distances = torch::cdist(embedding, self_memory_bank.index({ Slice(start, end), Slice(None) }), 2);
		int n_neighbors1 = min(end - start, n_neighbors);
		std::tuple<torch::Tensor, torch::Tensor> topk_class = distances.topk(1, -1, false);

		torch::Tensor patch_scores = std::get<0>(topk_class);


		for (int i = 1; i < iterTimes; i++) 
		{

			start = i * spanlen;
			int end = min((i + 1) * spanlen, selfmemorybanksize0);

			n_neighbors1 = min(end - start, n_neighbors);
			torch::Tensor distances = torch::cdist(embedding, self_memory_bank.index({ Slice(start, end), Slice(None) }), 2);
			distances = torch::cat({ patch_scores.index({ Slice(None), 0 }).unsqueeze(1), distances }, 1);

			int n_neighbors1 = min(end - start, n_neighbors);
			std::tuple<torch::Tensor, torch::Tensor> topk_class = distances.topk(1, -1, false);
			patch_scores = std::get<0>(topk_class);

		}
		return patch_scores;
	}


	int  anomalyDetBigMatinPatchCoreCUT(cv::Mat& Matin, float thresh, const int modelInputW, const int modelInputH, int cutSize, int overlap1, cv::Mat& resultPicOut, int  useCuda)
	{

		int ret = 0;
		try
		{
			double t4444 = (double)getTickCount();

			int cutMergeWidthSize = 0;
			int overlapWidth = cutMergeWidthSize * 2;

			auto device = torch::Device(torch::kCUDA, 0);

			cv::Mat  input_image;

			if (Matin.channels() == 1)
			{
				Mat temp = Matin;

				if (modelInputW != Matin.cols || modelInputH != Matin.rows)
				{
					cv::Size scale(modelInputW, modelInputH);
					cv::resize(temp, temp, scale, 0, 0, cv::INTER_LINEAR);
				}
				temp = temp * 255;
				temp.convertTo(temp, CV_8UC1);
				Mat imgs[3];
				imgs[0] = temp;
				imgs[1] = temp;
				imgs[2] = temp;
				merge(imgs, 3, input_image);
			}
			else 
			{
				if (modelInputW != Matin.cols || modelInputH != Matin.rows)
				{
					cv::Size scale(modelInputW, modelInputH);
					cv::resize(Matin, input_image, scale, 0, 0, cv::INTER_LINEAR);
				}
				else
				{
					input_image = Matin;
				}
			}

			auto opts = torch::TensorOptions().dtype(torch::kU8);
			torch::Tensor tensor_image1 = torch::from_blob(input_image.data, { 1,input_image.rows, input_image.cols,3 }, opts);
			BOOL inputGPUSymbol = true;
			if (useCuda && cudaUseableSymbol && (inputGPUSymbol))
			{
				tensor_image1 = tensor_image1.to(deviceGPU);
			}


			tensor_image1 = tensor_image1.permute({ 0,3,1,2 });
			if (modelInputW != input_w || modelInputH != input_h)
				tensor_image1 = F::interpolate(tensor_image1, F::InterpolateFuncOptions().size(std::vector<int64_t>{input_h, input_w}));
			if (pad_w > 0 || pad_h > 0)
				tensor_image1 = F::pad(tensor_image1, F::PadFuncOptions({ 0,pad_w, 0, pad_h }));

			torch::Tensor resultTensor = torch::zeros({ 1, 1, resized_h, resized_w });
			resultTensor = resultTensor.to(deviceGPU);


			list<torch::Tensor>::iterator mbbeginiter = tmpMbLIST.begin();
			for (int h_iterNum = 0; h_iterNum < num_patches_h; h_iterNum++)
			{
				for (int W_iterNum = 0; W_iterNum < num_patches_w; W_iterNum++)
				{

					int loc_W_ovelapL = cut_index_array[h_iterNum][W_iterNum][0][0];
					int loc_h_ovelapL = cut_index_array[h_iterNum][W_iterNum][0][1];
					int loc_W_ovelapR = cut_index_array[h_iterNum][W_iterNum][0][2];
					int loc_h_ovelapR = cut_index_array[h_iterNum][W_iterNum][0][3];


					torch::Tensor tensor_image = tensor_image1.index({ Slice(None), Slice(None), Slice(loc_h_ovelapL, loc_h_ovelapR), Slice(loc_W_ovelapL, loc_W_ovelapR) });
					if (useCuda && cudaUseableSymbol && (!inputGPUSymbol))
					{
						tensor_image = tensor_image.to(deviceGPU);
					}


					tensor_image = tensor_image.to(torch::kFloat32);
					tensor_image = tensor_image.div_(255.0);

					tensor_image[0][0] = tensor_image[0][0].sub_(0.485).div_(0.229);
					tensor_image[0][1] = tensor_image[0][1].sub_(0.456).div_(0.224);
					tensor_image[0][2] = tensor_image[0][2].sub_(0.406).div_(0.225);

					torch::Tensor anomaly_map = module.forward({ tensor_image }).toTensor();

					torch::Tensor anomaly_map1 = anomaly_map.permute({ 0, 2, 3, 1 }).reshape({ -1,anomaly_map.size(1) });
					int mbIndexStartIndex = h_iterNum * num_patches_w + W_iterNum;

					torch::Tensor tmpMb = (*mbbeginiter).to(deviceGPU);
					torch::Tensor top_10 = nearest_neighbors(anomaly_map1, tmpMb, 1);

					++mbbeginiter;
					torch::Tensor patch_scores;

					float amax1 = top_10.amax().item<float>();

#ifdef EXETEST


					if (amax122 < amax1)
					{
						amax122 = amax1;

					}
#endif 

					if (amax1 < thresh)
					{
						continue;
					}
								

					anomaly_map = top_10.reshape({ anomaly_map.size(0),1, anomaly_map.size(2), anomaly_map.size(3) });
					int h = loc_h_ovelapR - loc_h_ovelapL;
					int w = loc_W_ovelapR - loc_W_ovelapL;
					anomaly_map = F::interpolate(anomaly_map, F::InterpolateFuncOptions().size(std::vector<int64_t>{h, w}));


					int loc_w = cut_index_array[h_iterNum][W_iterNum][2][0];
					int loc_h = cut_index_array[h_iterNum][W_iterNum][2][1];

					int loc_wAddtile_size_w = cut_index_array[h_iterNum][W_iterNum][2][2];
					int loc_hAddtile_size_h = cut_index_array[h_iterNum][W_iterNum][2][3];

					int w_L = cut_index_array[h_iterNum][W_iterNum][1][0];
					int h_L = cut_index_array[h_iterNum][W_iterNum][1][1];

					int w_LAddtile_size_w = cut_index_array[h_iterNum][W_iterNum][1][2];
					int h_LAddtile_size_h = cut_index_array[h_iterNum][W_iterNum][1][3];

					anomaly_map = anomaly_map.index({ Slice(None), Slice(None), Slice(h_L, h_LAddtile_size_h), Slice(w_L, w_LAddtile_size_w) });
					if (anomaly_map.amax().item<float>() < thresh)
					{
						continue;
					}

					ret = 1;
					resultTensor.index({ Slice(None), Slice(None), Slice(loc_h, loc_hAddtile_size_h), Slice(loc_w, loc_wAddtile_size_w) }) = anomaly_map;

				}
			}
			if (ret == 0)
				return ret;
			resultTensor = resultTensor.index({ Slice(None), Slice(None), Slice(0, input_h), Slice(0, input_w) });
			if (pad_w > 0)
				resultTensor.index({ Slice(None), Slice(None), Slice(None), Slice(input_w - overlap1, input_w) }) = 0;
			if (pad_h > 0)
				resultTensor.index({ Slice(None), Slice(None), Slice(input_h - overlap1, input_h), Slice(None) }) = 0;
			if (modelInputW != input_w || modelInputH != input_h)
			{
				resultTensor = F::interpolate(resultTensor, F::InterpolateFuncOptions().size(std::vector<int64_t>{modelInputH, modelInputW}));
			}

			if (useCuda && cudaUseableSymbol)
				resultTensor = resultTensor.clamp(0, 3.0).to(deviceCPU);
			cv::Mat anomaly(cv::Size(modelInputW, modelInputH), CV_32F, resultTensor.data_ptr());
			anomaly = (anomaly) / 3.0;
			
			int sigma = 2;
			int kernel_size = 2 * int(4.0 * sigma + 0.5) + 1;
			
			cv::GaussianBlur(anomaly, anomaly, cv::Size(kernel_size, kernel_size), sigma, sigma);

			anomaly = anomaly *255;
			anomaly.convertTo(anomaly, CV_8U);
			gammaCorrection(anomaly, anomaly, 1.2);

			resultPicOut = anomaly;

		}

		catch (const c10::Error& e)
		{

			return -1;
		}
		return ret;
	}



	int  anomalyDetBigMatinPatchCore(cv::Mat& Matin, float halconRegionThreshMax, const int modelInputW, const int modelInputH, int cutSize, int overlap1, cv::Mat& resultPicOut, int  useCuda)
	{

		int ret = 0;
		try
		{
			double t = (double)getTickCount();
			int cutMergeWidthSize = 0;
			int overlapWidth = cutMergeWidthSize * 2;

			auto device = torch::Device(torch::kCUDA, 0);

			cv::Mat  input_image;
			if (modelInputW != Matin.cols || modelInputH != Matin.rows)
			{
				cv::resize(Matin, Matin, cv::Size(modelInputW, modelInputH));
			}
			if (Matin.channels() == 1)
			{
				cvtColor(Matin, input_image, COLOR_GRAY2BGR);
			}
			else
			{
				input_image = Matin;
			}


			auto opts = torch::TensorOptions().dtype(torch::kU8);
			torch::Tensor tensor_image = torch::from_blob(input_image.data, { 1,input_image.rows, input_image.cols,3 }, opts);


			if (useCuda && cudaUseableSymbol)
				tensor_image = tensor_image.to(deviceGPU);
			tensor_image = tensor_image.permute({ 0,3,1,2 });
			tensor_image = tensor_image.to(torch::kFloat32);
			tensor_image = tensor_image.div_(255.0);

			tensor_image[0][0] = tensor_image[0][0].sub_(0.485).div_(0.229);
			tensor_image[0][1] = tensor_image[0][1].sub_(0.456).div_(0.224);
			tensor_image[0][2] = tensor_image[0][2].sub_(0.406).div_(0.225);

	
			double t1 = (double)getTickCount();
			if (cutSize == -1)
			{
				int maxTrainSize0 = maxTrainSize[0];
				float maxTrainSize1 = maxTrainSize[1];
				float total = maxTrainSize0 * maxTrainSize1;

				cutSize = ceil(total / modelInputH);

				if (cutSize < 32)
				{
					return -1;
				}
				if (modelInputW / cutSize > 0 && modelInputW % cutSize > 0)
				{
					int cNum = (float(modelInputW) / float(cutSize));

					cutSize = ceil(float(modelInputW) / float(cNum));
				}
			}

			int stridex = modelInputW / cutSize;
			int ys = modelInputW % cutSize;

			if (ys > 0)
			{
				stridex += 1;
			}
			at::Tensor anomaly_map;
			for (int cutStride = 0; cutStride < stridex; cutStride++)
			{
				int startXIndex = cutStride * cutSize;
				int endXIndex = startXIndex + cutSize + cutMergeWidthSize;

				endXIndex = MIN(endXIndex, modelInputW);
				startXIndex = MAX(startXIndex - cutMergeWidthSize, 0);
				int tempW = endXIndex - startXIndex;
				torch::Tensor tensor_imageSlice = tensor_image.slice(3, startXIndex, endXIndex);

				auto out = module.forward({ tensor_imageSlice }); ;

				torch::Tensor anomaly_maptemp = out.toTuple()->elements()[0].toTensor();

				if (cutStride == 0)
				{
					anomaly_map = anomaly_maptemp;
				}
				else
				{
					anomaly_map = torch::cat({ anomaly_map, anomaly_maptemp }, 3);
				}
			}

			at::Tensor  amax = anomaly_map.amax();
			at::Tensor  amin = anomaly_map.amin();


			if (amax.item().toFloat() >= halconRegionThreshMax)
			{
				ret = 1;

			}
#ifdef EXETEST


			if (amax122 < amax.item().toFloat())
			{
				amax122 = amax.item().toFloat();

			}
#endif 

	
			if (useCuda && cudaUseableSymbol)
				anomaly_map = anomaly_map.clamp(0, 3.0).to(deviceCPU); 
			cv::Mat anomaly(cv::Size(modelInputW, modelInputH), CV_32F, anomaly_map.data_ptr());
			anomaly = (anomaly) / 3.0;

			int sigma = 2;
			int kernel_size = 2 * int(4.0 * sigma + 0.5) + 1;

			cv::GaussianBlur(anomaly, anomaly, cv::Size(kernel_size, kernel_size), sigma, sigma);
			anomaly = anomaly * 255;
			anomaly.convertTo(anomaly, CV_8U);
			gammaCorrection(anomaly, anomaly, 1.2);

			resultPicOut = anomaly;


		}
		catch (const c10::Error& e)
		{
			return -1;
		}
		return ret;
	}




	int anomalyDetMatin(cv::Mat& Matin, float thresh, const int modelInputW, const int modelInputH, cv::Mat& resultPicOut, int  useCuda, int overlap = 64, int cutSize = -1)
	{


		int result = -1;
		thresh = pow(thresh,2.0);
		result = anomalyDetBigMatinPatchCoreCUT(Matin, thresh, modelInputW, modelInputH, cutSize, overlap, resultPicOut, useCuda);

		if (result == 0)
		{
			resultPicOut = cv::Mat::zeros(modelInputH, modelInputW, CV_8UC1);
		}
		else
		{
		}

		if (countNonZero(resultPicOut) < 1)
		{
			result = 0;
		}

		return result;
	}


};

ADetctCls* GlobleModels[maxDetectModel] = { nullptr };

extern "C" _declspec(dllexport) int creatModel(int modelIndex, const char* modelRootDir, int gpu_id)
{
	if (modelIndex < 0 || modelIndex >= maxDetectModel)
		return -1;
	int useCuda = 1;
	if (GlobleModels[modelIndex] != NULL)
	{
		GlobleModels[modelIndex]->deinitModel();
		delete GlobleModels[modelIndex];
		GlobleModels[modelIndex] = NULL;
	}

	if (GlobleModels[modelIndex] == NULL)
	{
		GlobleModels[modelIndex] = new ADetctCls();
		int ret =GlobleModels[modelIndex]->initializeModel(modelRootDir, useCuda);
		return ret;
	}
	
}
extern "C" _declspec(dllexport) int anomalyDetMatin(int modelIndex, cv::Mat& testImage, float anomalyThresh,cv::Mat& resultImage)
{
	if (modelIndex < 0 || modelIndex >= maxDetectModel)
		return -1;
	{
		if (GlobleModels[modelIndex] != NULL)
		{
			const int modelInputW = testImage.cols;  const int modelInputH = testImage.rows;

			int  useCuda=1; int overlap = 64; int cutSize = -1;
			int ret = GlobleModels[modelIndex]->anomalyDetMatin(testImage, anomalyThresh, modelInputW, modelInputH, resultImage, useCuda, overlap, cutSize);
			return ret;
		}
		else
		{
			return -1;
		}
	}


	return 0;
}


extern "C" _declspec(dllexport) void deinitModel(int modelIndex)
{
	if (modelIndex < 0 || modelIndex >= maxDetectModel)
		return;

	if (GlobleModels[modelIndex] != NULL)
	{
		GlobleModels[modelIndex]->deinitModel();
		delete GlobleModels[modelIndex];
		GlobleModels[modelIndex] = NULL;
	}
}

extern "C" _declspec(dllexport) void deinitAllModel()
{


	for (int searchIndex = 0; searchIndex < maxCutModel; searchIndex++)
	{
		if ((models[searchIndex].inited))
		{
			models[searchIndex].module.~Module();
			models[searchIndex].inited = false;
			models[searchIndex].forwardSize[0] = -1;
			models[searchIndex].forwardSize[1] = -1;
		}
	}

	for (int igg = 0; igg < maxDetectModel; igg++)
	{
		if (GlobleModels[igg] != NULL)
		{
			GlobleModels[igg]->deinitModel();
			delete GlobleModels[igg];
			GlobleModels[igg] = NULL;
		}
	}
}



int main(int argc, char **argv)
{



	int useCudasymbol = 1;
	const char* cpuModel = "E:\\SVN\\XAlogrithm_Develop\\bin\\Debug\\Model\\202503211511237808";
	int ret = creatModel(0, cpuModel, 0);
	cout << "ret: " << ret << endl;
	cpuModel = "E:\\SVN\\XAlogrithm_Develop\\bin\\Debug\\Model\\202503211511237808\\model";
	ret = creatModel(0, cpuModel, 0);
	cout << "ret: " << ret << endl;

	const char* imgname = "E:\\SVN\\XAlogrithm_Develop\\bin\\Debug\\mat.jpg";
	cv::Mat matin = cv::imread(imgname, 1);
	cout << imgname << " " << matin.cols << " " << matin.rows << endl;
	cv::Mat resultPicOut(matin.rows, matin.cols, CV_8UC1);


	{
		float th = 0.1;
		time_t start1 = clock();
		ret = anomalyDetMatin(0, matin, th, resultPicOut);

		time_t end1 = clock();

		cout << ret << "the running time is : " << double(end1 - start1) / CLOCKS_PER_SEC << "s" << endl;
	}

	cv::Mat resultPicOutth;
	cv::threshold(resultPicOut, resultPicOutth, 150, 255, cv::ThresholdTypes::THRESH_BINARY);
	imshow("resultPicOutth", resultPicOutth);
	imshow("resultPicOut", resultPicOut);
	imwrite("resultPicOut.jpg", resultPicOut);
	imwrite("resultPicOutth.jpg", resultPicOutth);
	cv::waitKey(0);
		


	resultPicOut = resultPicOut;
	return 0;
}
