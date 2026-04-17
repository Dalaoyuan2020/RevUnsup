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
//torch::Device deviceGPU(torch::kCUDA, 0);
std::unique_ptr<torch::Device> deviceGPU;
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
			cout << "鏂囦欢鎵撳紑澶辫触" << endl;

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

		//cout <<"num_patches_h"<< num_patches_h << num_patches_w << endl;
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

		std::cout << "modelPath:" << modelPath << std::endl;
		std::cout << "mbmodelPath:" << memorybankPath << std::endl;
		struct stat buffer;
		if (stat(configPath.c_str(), &buffer) == 0)
		{
			YAML::Node config = YAML::LoadFile(configPath);

			std::cout << "configPath:" << configPath << std::endl;

			modelIndex = config["modelindex"].as<int>();
			maxTrainSize = config["maxTrainSize"].as<std::vector<int>>();
			forwardSize = config["forwardSize"].as<std::vector<int>>();

			exportType = config["exportType"].as<int>();
			std::cout << "export is" << exportType << std::endl;
			overlap1 = config["overlap"].as<int>();;
			scoreRatio = config["scoreRatio"].as<float>();

			maxSearchNaxFeatrueSize1 = config["maxSearchNaxFeatrueSize1"].as<float>();;

		}

		try
		{
			//std::cout << "........................" << std::endl;

			module = torch::jit::load(modelPath);
			//std::cout << "........................" << std::endl;

			module.eval();
			//std::cout << "........................" << std::endl;

			module.to(*deviceGPU);
			//std::cout<<"........................" << std::endl;
			for (int searchIndex = 0; searchIndex < maxCutModel; searchIndex++)//鎵惧埌涓€涓悎閫傜殑妯″瀷
			{

				if (!(models[searchIndex].inited))
				{
					models[searchIndex].module = torch::jit::load(modelPath);
					models[searchIndex].module.eval();

					if (useCuda && cudaUseableSymbol)
					{
						models[searchIndex].module.to(*deviceGPU);
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
			//std::cout << "........................" << std::endl;

			if (exportType == 1)
				MB2CPUORGPU(memorybankPath.c_str(), mbType);

		}

		catch (const c10::Error& e)
		{
			std::cerr << "error"<<std::endl;
			return -1;

		}
		return 0;
	}

	void deinitModel()
	{

		//module.~Module();
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

	//璇ユ柟娉曞皢鍥惧儚鍒嗗壊鎴愬皬鍧楋紝瀵规瘡鍧楃壒寰佹彁鍙栥€佸紓甯告娴嬶紝鏈€鍚庡皢寰楀埌缁撴灉涓婇噰鏍锋仮澶嶅埌鍘熷澶у皬
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
				tensor_image1 = tensor_image1.to(*deviceGPU);
			}
			tensor_image1 = tensor_image1.to(torch::kHalf);

			tensor_image1 = tensor_image1.permute({ 0,3,1,2 });
			if (modelInputW != input_w || modelInputH != input_h)
			{
				std::cout << modelInputW << "  " << modelInputH <<" "<< input_h<<" "<< input_w << std::endl;
				tensor_image1 = F::interpolate(tensor_image1, F::InterpolateFuncOptions().size(std::vector<int64_t>{input_h, input_w}));
			}
			if (pad_w > 0 || pad_h > 0)
				tensor_image1 = F::pad(tensor_image1, F::PadFuncOptions({ 0,pad_w, 0, pad_h }));

			torch::Tensor resultTensor = torch::zeros({ 1, 1, resized_h, resized_w });
			resultTensor = resultTensor.to(*deviceGPU);

			//瀵硅緭鍏ュ浘鍍忚繘琛屽垎鍧楀鐞嗭紝骞跺姣忓潡杩涜鐗瑰緛鎻愬彇銆佸紓甯告娴?
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
						tensor_image = tensor_image.to(*deviceGPU);
					}


					tensor_image = tensor_image.div_(255.0);

					tensor_image[0][0] = tensor_image[0][0].sub_(0.485).div_(0.229);
					tensor_image[0][1] = tensor_image[0][1].sub_(0.456).div_(0.224);
					tensor_image[0][2] = tensor_image[0][2].sub_(0.406).div_(0.225);

					tensor_image = tensor_image.to(torch::kFloat16);

					torch::Tensor anomaly_map = module.forward({ tensor_image }).toTensor();
					//anomaly_map = anomaly_map.to(torch::kFloat32);   // <-- 鍔犺繖涓€琛?

					torch::Tensor anomaly_map1 = anomaly_map.permute({ 0, 2, 3, 1 }).reshape({ -1,anomaly_map.size(1) });
					int mbIndexStartIndex = h_iterNum * num_patches_w + W_iterNum;

					torch::Tensor tmpMb = (*mbbeginiter).to(*deviceGPU);//鍙栧嚭褰撳墠鐨勭壒寰侀泦鍚坱mpMb

					tmpMb = tmpMb.to(torch::kFloat16);

					torch::Tensor top_10 = nearest_neighbors(anomaly_map1, tmpMb, 1);
					top_10 = top_10.to(torch::kFloat32);   // <-- 鍔犺繖涓€琛?

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
						continue;//灏忎簬闃堝€紅hresh锛屽垯璺宠繃鐜板湪鐨勫皬鍧楋紝娌℃湁寮傚父
					}


					anomaly_map = top_10.reshape({ anomaly_map.size(0),1, anomaly_map.size(2), anomaly_map.size(3) });//閲嶅鍥炲師濮嬬壒寰佸浘鐨勫舰鐘?
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

	//璇ュ嚱鏁版病鏈夎皟鐢?
	//瀹炵幇浜嗕竴涓紓甯告娴嬪嚱鏁?anomalyDetBigMatinPatchCore锛屽畠瀵硅緭鍏ュ浘鍍忚繘琛屽鐞嗭紝妫€娴嬪浘鍍忎腑鐨勫紓甯稿尯鍩燂紝骞跺皢缁撴灉杈撳嚭鍒?resultPicOut
	int  anomalyDetBigMatinPatchCore(cv::Mat& Matin, float halconRegionThreshMax, const int modelInputW, const int modelInputH, int cutSize, int overlap1, cv::Mat& resultPicOut, int  useCuda)
	{

		int ret = 0;// 鍒濆鍖栬繑鍥炲€硷紝0 琛ㄧず鏈娴嬪埌寮傚父锛? 琛ㄧず妫€娴嬪埌寮傚父
		try
		{
			double t = (double)getTickCount();
			int cutMergeWidthSize = 0;
			int overlapWidth = cutMergeWidthSize * 2;// 瀹氫箟灏忓潡涔嬮棿鐨勯噸鍙犲尯鍩熷搴?

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
				tensor_image = tensor_image.to(*deviceGPU);
			tensor_image = tensor_image.permute({ 0,3,1,2 });// 璋冩暣 Tensor 鐨勭淮搴﹂『搴忥紝浠ョ鍚堟ā鍨嬬殑杈撳叆瑕佹眰
			tensor_image = tensor_image.to(torch::kFloat32);
			tensor_image = tensor_image.div_(255.0);

			tensor_image[0][0] = tensor_image[0][0].sub_(0.485).div_(0.229);
			tensor_image[0][1] = tensor_image[0][1].sub_(0.456).div_(0.224);
			tensor_image[0][2] = tensor_image[0][2].sub_(0.406).div_(0.225);


			double t1 = (double)getTickCount();
			//cutSize锛氬鏋滄湭鎸囧畾灏忓潡澶у皬锛屽垯鏍规嵁妯″瀷杈撳叆澶у皬鍜岃缁冩暟鎹ぇ灏忚绠楀皬鍧楀ぇ灏忋€?
			//ceil锛氱‘淇濆皬鍧楀ぇ灏忔槸鏁存暟锛屽苟涓旇兘澶熻鐩栨暣涓浘鍍忓搴?
			if (cutSize == -1)
			{
				int maxTrainSize0 = maxTrainSize[0];
				float maxTrainSize1 = maxTrainSize[1];
				float total = maxTrainSize0 * maxTrainSize1;

				cutSize = ceil(total / modelInputH);

				if (cutSize < 32)
				{
					// 濡傛灉灏忓潡澶у皬灏忎簬 32锛岃繑鍥為敊璇?
					return -1;
				}
				if (modelInputW / cutSize > 0 && modelInputW % cutSize > 0)// 妯″瀷杈撳叆瀹藉害涓嶈兘琚皬鍧楀ぇ灏忔暣闄わ紝璋冩暣灏忓潡澶у皬
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

				if (cutStride == 0)// 濡傛灉鏄涓€涓皬鍧楋紝鍒濆鍖栧紓甯稿浘
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
		thresh = pow(thresh, 2.0);
		result = anomalyDetBigMatinPatchCoreCUT(Matin, thresh, modelInputW, modelInputH, cutSize, overlap, resultPicOut, useCuda);

		if (result == 0)
		{
			resultPicOut = cv::Mat::zeros(modelInputH, modelInputW, CV_8UC1);
		}
		else
		{
			//gammaCorrection(resultPicOut, resultPicOut,0.5);

			/*
			Mat tempresultPicOut;
			//Mat tempresultPicOut1;

			imwrite("resultPicOutorg.jpg", resultPicOut);


			// 浣跨敤cv::addWeighted鍙犲姞鍥惧儚
			cv::Mat result;
			cv::Mat resultPicOutcolor;
			Mat imgs[3];

			Mat zero = Mat::zeros(resultPicOut.rows, resultPicOut.cols, CV_8UC1);
			imgs[0] = zero;
			imgs[1] = resultPicOut;
			imgs[2] = resultPicOut;
			merge(imgs, 3, resultPicOutcolor);

			cv::addWeighted(Matin, 0.5, resultPicOutcolor, 1.0, 0.0, result);
			imshow("result", result);
			imwrite("result.jpg", result);
			cv::threshold(resultPicOut, resultPicOut, halconRegionThreshMax * 255, 255, cv::ThresholdTypes::THRESH_BINARY);
			*/
			/*
			//imwrite("resultPicOutorg.jpg", resultPicOut);
			//imwrite("tempresultPicOut.jpg", tempresultPicOut);
			vector<vector<Point>> contours;
			vector<Vec4i> hierarchy;
			findContours(tempresultPicOut, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point());

			Mat tempresultPicOut_expand = Mat::zeros(tempresultPicOut.size(), CV_8UC1);
			vector<vector<Point>> contours_expand;
			for (int i = 0; i < contours.size(); i++)
			{

				Rect boundRect = boundingRect(Mat(contours[i]));
				if (boundRect.width > 0.1*resultPicOut.cols || boundRect.height > 0.1*resultPicOut.rows || boundRect.height < 6 || boundRect.width < 6)
				{
					contours_expand.push_back(contours[i]);
				}
				else
				{
					vector<Point> out;
					expand_polygon(contours[i], out);
					contours_expand.push_back(out);
				}

				//temp_mapRect.copyTo(resultPicOut(boundRect));

			}
			for (int i = 0; i < contours_expand.size(); i++)
			{
				drawContours(tempresultPicOut_expand, contours_expand, i, Scalar(255), -1, 8);
			}
			cv::blur(tempresultPicOut_expand, tempresultPicOut_expand, cv::Size(5, 5));
			bitwise_and(tempresultPicOut_expand, tempresultPicOut, resultPicOut);
			*/
		}

		if (countNonZero(resultPicOut) < 1)
		{
			result = 0;
		}

		//cv::imwrite("Matin-result.jpg", resultPicOut);

		return result;
	}


};

ADetctCls* GlobleModels[maxDetectModel] = { nullptr };

extern "C" _declspec(dllexport) int anomalyDetMatin(int modelIndex, cv::Mat& testImage, float anomalyThresh, cv::Mat& resultImage)
{
	if (modelIndex < 0 || modelIndex >= maxDetectModel)
		return -1;
	//try
	{
		if (GlobleModels[modelIndex] != NULL)
		{
			const int modelInputW = testImage.cols;  const int modelInputH = testImage.rows;

			int  useCuda = 1; int overlap = 64; int cutSize = -1;
			int ret = GlobleModels[modelIndex]->anomalyDetMatin(testImage, anomalyThresh, modelInputW, modelInputH, resultImage, useCuda, overlap, cutSize);
			//imwrite("resultImage.jpg", resultImage);
			return ret;
		}
		else
		{
			return -1;
		}
	}
	/*
	catch (const c10::Error& e)
	{
		std::cerr << "Error";

	}*/


	return 0;
}

class ContinuousGPUUser {
private:
	std::atomic<bool> running_;
	std::thread worker_thread_;
	int model_type_;
	cv::Mat persistent_input_;
	cv::Mat persistent_output_;
	float threshold_;
	std::atomic<double> last_duration_;

public:
	ContinuousGPUUser(int model_type, const cv::Mat& input_mat, float threshold = 0.1f)
		: model_type_(model_type), persistent_input_(input_mat.clone()),
		threshold_(threshold), running_(false), last_duration_(0.0) {

		persistent_output_ = cv::Mat(input_mat.rows, input_mat.cols, CV_8UC1);

		// 寤虹珛绋冲畾鐨勫唴瀛樺崰鐢ㄦā寮?
		establishStablePattern();
	}

	void establishStablePattern() {
		std::cout << "=== 寤虹珛绋冲畾GPU鍗犵敤妯″紡 - 妯″瀷" << model_type_ << " ===" << std::endl;

		// 浣跨敤瀹屽叏鐩稿悓鐨勫弬鏁拌繛缁帹鐞嗭紝寤虹珛绋冲畾妯″紡
		for (int i = 0; i < 5; i++) {
			time_t start = clock();
			int ret = anomalyDetMatin(model_type_, persistent_input_, threshold_, persistent_output_);
			time_t end = clock();

			double duration = double(end - start) / CLOCKS_PER_SEC;
			last_duration_ = duration;

			std::cout << "[绋冲畾妯″紡寤虹珛] 妯″瀷" << model_type_ << " - " << i + 1
				<< " - 鑰楁椂: " << duration << "s" << std::endl;

			// 闈炲父鐭殑闂撮殧锛屼繚鎸佽繛缁崰鐢?
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		std::cout << "绋冲畾GPU鍗犵敤妯″紡寤虹珛瀹屾垚" << std::endl;
	}

	void start() {
		if (running_) return;

		running_ = true;
		worker_thread_ = std::thread([this]() {
			std::cout << "鎸佺画GPU鍗犵敤绾跨▼鍚姩 - 妯″瀷" << model_type_ << std::endl;

			int cycle_count = 0;
			auto last_cycle_time = std::chrono::steady_clock::now();

			while (running_) {
				auto cycle_start = std::chrono::steady_clock::now();

				// 楂橀浣庤礋杞藉崰鐢?
				for (int i = 0; i < 3 && running_; i++) {
					time_t start = clock();
					int ret = anomalyDetMatin(model_type_, persistent_input_, threshold_, persistent_output_);
					time_t end = clock();

					last_duration_ = double(end - start) / CLOCKS_PER_SEC;
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}

				cycle_count++;

				// 姣?0涓懆鏈熸姤鍛婁竴娆?
				if (cycle_count % 10 == 0) {
					auto now = std::chrono::steady_clock::now();
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_cycle_time).count();
					std::cout << "[鎸佺画鍗犵敤-妯″瀷" << model_type_
						<< "] 鍛ㄦ湡: " << cycle_count
						<< " - 鏈€杩戣€楁椂: " << last_duration_
						<< "s - 杩愯鏃堕棿: " << elapsed << "s" << std::endl;
				}

				// 淇濇寔娲昏穬浣嗛伩鍏嶈繃杞?
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

			std::cout << "鎸佺画GPU鍗犵敤绾跨▼鍋滄 - 妯″瀷" << model_type_ << std::endl;
			});
	}

	void stop() {
		running_ = false;
		if (worker_thread_.joinable()) {
			worker_thread_.join();
		}
	}

	double getLastDuration() const {
		return last_duration_;
	}
};

std::unique_ptr<ContinuousGPUUser> g_warmup_manager;

extern "C" _declspec(dllexport) int creatModel(int modelIndex, const char* modelRootDir, int gpu_id)
{
	if (torch::cuda::is_available()) {
		deviceGPU = std::make_unique<torch::Device>(torch::kCUDA, gpu_id);
		// === RU-020 B' 加速 ===
		at::globalContext().setBenchmarkCuDNN(true);
		at::globalContext().setAllowTF32CuBLAS(true);
		at::globalContext().setAllowFP16ReductionCuBLAS(true);
		// === end ===
	}
	else {
		deviceGPU = std::make_unique<torch::Device>(torch::kCPU);
	}

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
		int ret = GlobleModels[modelIndex]->initializeModel(modelRootDir, useCuda);
		if (ret != 0) 
		{
			return ret;
		}
	}
	// 鍚姩鍚庡彴淇濇寔娲昏穬绾跨▼
	//cv::Mat warmupData(1130, 1136, CV_8UC3, cv::Scalar(127, 127, 127)); // 涓€х伆鑹?
	//g_warmup_manager = std::make_unique<ContinuousGPUUser>(0, warmupData, 0.1f);
	//auto gpu_user1 = std::make_unique<ContinuousGPUUser>(1, matin, 0.1f);
	//g_warmup_manager->start();
	//gpu_user1->start();
	//cv::Mat warmupData(1300, 1300, CV_8UC3, cv::Scalar(127, 127, 127)); // 涓€х伆鑹?
	//cv::Mat warmupOutput(1300, 1300, CV_8UC1);
	//// 3. 鎵ц棰勭儹鎺ㄧ悊
	//std::cout << "妯″瀷棰勭儹涓?.." << std::endl;
	//for (int i = 0; i < 3; i++) {
	//	time_t start = clock();
	//	int ret = anomalyDetMatin(modelIndex, warmupData, 0.1f, warmupOutput);
	//	time_t end = clock();

	//	std::cout << "棰勭儹杞 " << i + 1 << ": " << double(end - start) / CLOCKS_PER_SEC << "s" << std::endl;

	//	if (ret != 1) {
	//		std::cout << "棰勭儹澶辫触锛岄敊璇爜: " << ret << std::endl;
	//		return -1;
	//	}
	//}
	//std::cout << "妯″瀷棰勭儹瀹屾垚" << std::endl;

	return 0;


}


extern "C" _declspec(dllexport) void deinitModel(int modelIndex)
{

	if (modelIndex < 0 || modelIndex >= maxDetectModel)
		return;

	if (GlobleModels[modelIndex] != NULL)
	{
		//Mat E = Mat::eye(66, 66, CV_64F);
		//cv::imwrite("D://" + to_string(index) + "deinitModel.jpg", E);
		GlobleModels[modelIndex]->deinitModel();
		delete GlobleModels[modelIndex];
		GlobleModels[modelIndex] = NULL;
	}

}

extern "C" _declspec(dllexport) void deinitAllModel()
{

	for (int searchIndex = 0; searchIndex < maxCutModel; searchIndex++)
	{
		//std::cout << searchIndex << std::endl;
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


//// 鍏ㄥ眬淇濇寔娲昏穬绠＄悊鍣?
//int main(int argc, char** argv)
//{
//	const char* cpuModel = "D:/wej_AI_5.0/test_ad_infer_new/Model";
//	const char* imgname = "D:/wej_AI_5.0/test_ad_infer_new/Model/5.jpg";
//
//
//	cv::Mat matin = cv::imread(imgname, 1);
//	std::cout << imgname << " " << matin.cols << " " << matin.rows << std::endl;
//
//	int ret = creatModel(0, cpuModel, 0);
//	std::cout << "妯″瀷鍒濆鍖栫粨鏋? " << ret << std::endl;
//	cv::Mat resultPicOut(matin.rows, matin.cols, CV_8UC1);
//
//	std::cout << "GPU淇濇寔娲昏穬宸插惎鍔紝灏嗘寔缁淮鎸丟PU鐘舵€? << std::endl;
//
//	// 姝ｅ紡鎺ㄧ悊寰幆
//	int frame_count = 0;
//	bool simulating_idle = false;
//
//	while (1)
//	{
//		float th = 0.1;
//		time_t start1 = clock();
//
//		ret = anomalyDetMatin(0, matin, th, resultPicOut);
//
//		time_t end1 = clock();
//
//		double elapsed = double(end1 - start1) / CLOCKS_PER_SEC;
//		std::cout << "甯?" << ++frame_count << " - 鑰楁椂: " << elapsed << "s" << std::endl;
//
//		// 妯℃嫙闀挎椂闂撮棽缃細姣?0甯т紤鎭?0绉?
//		/*if (frame_count % 20 == 0 && !simulating_idle) {
//			simulating_idle = true;
//			std::cout << "=== 妯℃嫙闀挎椂闂撮棽缃紙10绉掞級===" << std::endl;
//			for (int i = 20; i > 0; i--) {
//				std::cout << "闂茬疆鍊掕鏃? " << i << "绉? << std::endl;
//				std::this_thread::sleep_for(std::chrono::seconds(1));
//			}
//			std::cout << "闂茬疆缁撴潫锛屾仮澶嶆帹鐞嗭紙搴旇浠嶇劧寰堝揩锛?.." << std::endl;
//			simulating_idle = false;
//		}
//
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));*/
//	}
//
//	//g_gpu_keeper->stop();
//	deinitAllModel();
//	return 0;
//}