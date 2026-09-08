#pragma once
#include <opencv2/opencv.hpp>
#include "NeuralNetwork.h"
#include "CNNModels.h"

// 枚举CNN（暂时）所有的模型类型
enum class CNNType {
	LeNet,
	AlexNet,
	VGGNet,
	ResNet
};

// 卷积神经网络（用于处理图像任务）
class CNN : public NeuralNetwork {
public:
	// 构造函数（模型状态，任务类型，模型类型，数据路径，模型名称）
	explicit CNN(State s, Task t, CNNType m, const std::string& path, const std::string& model_id)
		: NeuralNetwork(s, t, model_id), modelType(m) 
	{
		// 检查任务类型，如果是回归任务则抛出异常
		if (t == Task::Regression) {
			throw std::invalid_argument("Regression is not supported by CNN currently");
		}
		get_data_path(path);  // 获取用户提供的路径（顺便检查路径是否合法）
	}

	// 析构函数
	virtual ~CNN() = default;
	// 禁用 复制构造函数与赋值操作符
	CNN(const CNN&) = delete; CNN& operator=(const CNN&) = delete;
	// 允许 移动构造函数
	CNN(CNN&&) = default; CNN& operator=(CNN&&) = default;

private:
	// 获取模型数据路径（文件夹）
	void get_data_path(const std::string& path) override {
		// 检查用户提供的路径是否为空或不存在
		if (path.empty() || !fs::exists(path)) {
			throw std::invalid_argument("Invalid path.");
		}

		// 检查路径是否为文件夹
		if (fs::is_directory(path)) {
			fs::path p(path);
			p.make_preferred();
			data_path = p;
			std::cout << "Valid directory path: " << data_path << std::endl;
		}
		else {
			throw std::invalid_argument("Invalid path: The path must be a directory.");
		}
	}

	// 获取模型数据
	void get_data() override {
		std::cout << "Loading images from " << data_path << "..." << std::endl;

		std::vector<torch::Tensor> x_list; // 暂存图像 Tensor
		std::vector<int64_t> y_list;       // 暂存标签索引

		int total_samples = 0;

		// 定义支持的图片扩展名
		const std::vector<std::string> valid_extensions = { ".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff" };
		auto is_image = [&](const fs::path& p) {
			std::string ext = p.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			for (const auto& v : valid_extensions) {
				if (ext == v) return true;
			}
			return false;
			};

		// --- Train / Test 模式 ---
		if (state != State::Infer) {
			for (const auto& entry : fs::directory_iterator(data_path)) {
				throw_if_cancelled();
				if (!entry.is_directory()) continue;

				std::string label_name = entry.path().filename().string();
				int class_sample_count = 0; // 当前类别的图片计数

				// 打印当前正在处理的类别
				std::cout << "[Class]: " << std::left << std::setw(15) << label_name << " ... " << std::flush;

				// 处理标签映射
				if (state == State::Train) {
					if (label_map.find(label_name) == label_map.end()) {
						label_map.emplace(label_name, label_map.size());
					}
				}
				else if (state == State::Test) {
					if (label_map.find(label_name) == label_map.end()) {
						throw std::runtime_error("Unknown label in test set: " + label_name);
					}
				}

				int64_t label_idx = label_map[label_name];

				for (const auto& img_entry : fs::directory_iterator(entry.path())) {
					throw_if_cancelled();
					if (!img_entry.is_regular_file()) continue;
					if (!is_image(img_entry.path())) continue;

					torch::Tensor tensor_img = process_image(img_entry.path().string());

					if (tensor_img.defined()) {
						x_list.push_back(tensor_img);
						y_list.push_back(label_idx);
						total_samples++;
						class_sample_count++;
					}
				}
				// 该类别处理完毕，输出数量并换行
				std::cout << "Loaded: " << class_sample_count << " images." << std::endl;
			}
		}
		// --- Infer 模式 ---
		else {
			for (const auto& entry : fs::directory_iterator(data_path)) {
				throw_if_cancelled();
				if (!entry.is_regular_file()) continue;
				if (!is_image(entry.path())) continue;

				torch::Tensor tensor_img = process_image(entry.path().string());

				if (tensor_img.defined()) {
					x_list.push_back(tensor_img);
					total_samples++;

					// Infer 模式下没有子文件夹，图片可能很多
					// 使用 \r 动态更新计数，不换行，避免刷屏
					if (total_samples % 10 == 0) { // 每10张刷新一次显示，减少IO压力
						std::cout << "\rLoaded: " << total_samples << " images..." << std::flush;
					}
				}
			}
			// 循环结束后换行
			std::cout << "\rLoaded: " << total_samples << " images. Done.   " << std::endl;
		}

		if (x_list.empty()) {
			throw std::runtime_error("No valid images found in path.");
		}

		// 转换为 Batch Tensor
		data_x = torch::stack(x_list);

		if (state != State::Infer) {
			data_y = torch::tensor(y_list, torch::kLong).view({ (int64_t)total_samples, 1 });
		}

		std::cout << "--------------------------------------------------" << std::endl;
		std::cout << "All Data Loaded Successfully." << std::endl;
		std::cout << "Total Samples: " << total_samples
			<< " | Shape: " << data_x.sizes() << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;
	}

	// 辅助函数：单张图片处理
	torch::Tensor process_image(const std::string& img_path) {
		// 1. OpenCV 读取 (BGR)
		cv::Mat img = cv::imread(img_path);
		if (img.empty()) {
			std::cerr << "Warning: Failed to read image: " << img_path << std::endl;
			return torch::Tensor();
		}

		// 2. Resize (224, 224)
		cv::resize(img, img, cv::Size(224, 224));

		// 3. 转换颜色空间 BGR -> RGB (LibTorch 模型通常预期 RGB)
		cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

		// 4. 归一化并转浮点 (0-255 -> 0.0-1.0)
		img.convertTo(img, CV_32F, 1.0f / 255.0f);

		// 5. 转换为 Tensor
		// OpenCV 内存布局是 HWC (Height, Width, Channels)
		// LibTorch 构造时需要根据此布局读取
		auto tensor = torch::from_blob(img.data, { 224, 224, 3 }, torch::kFloat32);

		// 6. 维度重排: HWC -> CHW (Channels, Height, Width)
		// clone() 是必须的，因为 from_blob 不拥有内存，permute 也不拷贝内存
		// 如果不 clone，当 cv::Mat 析构后，Tensor 数据会损坏
		tensor = tensor.permute({ 2, 0, 1 }).clone();

		return tensor;
	}

	// 辅助函数：构建模型
	void build_model(int output_dim) override {
		switch (modelType) {
		case CNNType::LeNet:
			model = std::make_shared<LeNet>(output_dim);
			break;
		case CNNType::AlexNet:
			model = std::make_shared<AlexNet>(output_dim);
			break;
		case CNNType::VGGNet:
			model = std::make_shared<VGGNet>(output_dim);
			break;
		case CNNType::ResNet:
			model = std::make_shared<ResNet>(output_dim);
			break;
		default:
			throw std::invalid_argument("Unknown CNNType");
		}

		// 注册模型参数以便 LibTorch 追踪
		// 注意：nn::AnyModule 自动处理，但如果是手动管理需 register_module
		model.ptr()->to(torch::kFloat32); // 确保是 Float 类型
		std::cout << "CNN Model built. Type: " << (int)modelType
			   	  << ", Output Classes: " << output_dim << std::endl;
	}

	// 辅助函数：构建优化器
	void build_optimizer() override {
		double learning_rate = 0.001;

		// CNN 通常使用 Adam 或 SGD+Momentum
		// 这里使用 Adam
		optimizer = std::make_shared<torch::optim::Adam>(
			model.ptr()->parameters(),
			torch::optim::AdamOptions(learning_rate)
		);
		std::cout << "Optimizer built (Adam, lr=" << learning_rate << ")" << std::endl;
	}

	const CNNType modelType;  // 模型类型（LeNet, AlexNet, VGGNet, ResNet）
};
