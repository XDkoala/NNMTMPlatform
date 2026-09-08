#pragma once
#include "NeuralNetwork.h"
#include "ANNModels.h"

// 枚举ANN（暂时）所有的模型类型
enum class ANNType {
	SimpleMLP,
	GeneralMLP,
	DropoutMLP,
	L2RegularizedMLP,
	DeepMLP,
	SuperDeepMLP
};

// 人工神经网络基类（用于处理数值任务）
class ANN : public NeuralNetwork {
public:
	// 构造函数（模型状态，任务类型，模型类型，特征数量，数据路径，模型名称）
	explicit ANN(State s, Task t, ANNType m, size_t n, const std::string& path, const std::string& model_id)
		: NeuralNetwork(s, t, model_id), modelType(m), numX(n) 
	{
		get_data_path(path);  // 获取用户提供的路径（顺便检查路径是否合法）
	}
	
	// 析构函数
	virtual ~ANN() = default;
	// 禁用 复制构造函数与赋值操作符
	ANN(const ANN&) = delete; ANN& operator=(const ANN&) = delete;
	// 允许 移动构造函数
	ANN(ANN&&) = default; ANN& operator=(ANN&&) = default;

	// 实现基类的反归一化函数
    torch::Tensor denormalize_output(const torch::Tensor& output) override {
        // 只有回归任务且参数已初始化时才进行反归一化
        if (taskType == Task::Regression && target_std.defined() && target_mean.defined()) {
            // y_real = y_norm * std + mean
            return output * target_std + target_mean;
        }
        return output;
    }

private:
	// 获取模型数据路径（.txt文件）
	void get_data_path(const std::string& path) override {
		// 检查用户提供的路径是否为空或不存在
		if (path.empty() || !fs::exists(path)) {
			throw std::invalid_argument("Invalid path.");
		}

		// 检查路径是否为 .txt 文件
		fs::path p(path);
		if (fs::is_regular_file(path) && p.extension() == ".txt") {
			p.make_preferred();
			data_path = p;
			std::cout << "Valid .txt file path: " << data_path << std::endl;
		}
		else {
			throw std::invalid_argument("Invalid path: The path must be a .txt file.");
		}
	}

	// 获取模型数据
	void get_data() override {
		// 在 data_path 路径获取数据
		std::ifstream file(data_path);
		if (!file.is_open()) {
			throw std::invalid_argument("Failed to open the data file.");
		}

		std::vector<float> x_data; // 暂存特征
		std::vector<float> y_data_float; // 暂存回归目标
		std::vector<int64_t> y_data_long; // 暂存分类目标

		std::string line;  // 临时变量存储每行内容
		size_t line_count = 0;  // 记录行数（即m值）

		/* 任务描述：
			如果状态是 Train 或者 Test ，那么.txt文件内格式必须满足 m*(n+1)，
			即 m 行样本，每行有 n 个输入值（特征 x）和 1 个目标值（y），
			然而，如果 Train 状态下，且为分类任务，y值就是字符串，因此要求构建 label_map 。
			如果状态是 Test ，且为分类任务，还需要检查目标值是否存在于 label_map 中，
			如果不存在，需要抛出异常。
		
			如果状态是 Infer ，那么 .txt 文件内格式必须满足 m*n，
			即 m 行样本，每行有 n 个输入值（特征 x），没有目标值.
		*/

		// 读取每一行数据
		while (std::getline(file, line)) {
			throw_if_cancelled();
			if (line.empty()) continue; // 跳过空行
			std::istringstream stream(line);

			// 读取特征部分（train, test, infer）
			for (size_t i = 0; i < numX; ++i) {
				float feature;
				if (!(stream >> feature)) {  // 如果读取不到，则说明特征数不够，抛出异常
					throw std::invalid_argument("Insufficient features in line " + std::to_string(line_count + 1) + ". Expected " + std::to_string(numX) + " features.");
				}
				x_data.push_back(feature);
			}

			// 读取目标值部分（train, test）
			if (state != State::Infer) {
				if (taskType == Task::Regression) {  // 回归任务
					float label;
					if (!(stream >> label)) {  // 如果读取不到，则说明没有目标值，抛出异常
						throw std::invalid_argument("Missing label in regression task at line " + std::to_string(line_count + 1));
					}
					y_data_float.push_back(label);  // 回归任务存储为数值
				}
				else {  //分类任务
					std::string label;
					if (!(stream >> label)) {
						throw std::invalid_argument("Missing label in regression task at line " + std::to_string(line_count + 1));
					}

					// 对于分类任务，将字符串标签转换为整数标签
					if (state == State::Train) {  // 训练状态
						if (label_map.find(label) == label_map.end()) {  // 如果没有找到该标签
							label_map.emplace(label, label_map.size());  // 那么添加新标签
						}
					}
					else {  // 测试状态
						if (label_map.find(label) == label_map.end()) {  // 如果没有找到该标签
							throw std::invalid_argument("Invalid label: " + label);  // 说明用户数据有问题
						}
					}

					y_data_long.push_back(label_map[label]);  // 存储整数标签
				}

			}

			// Train、Test 和 Infer 都不允许期望字段之后还有额外内容。
			std::string extra_content;
			if (stream >> extra_content) {
				throw std::invalid_argument("Extra data found in line " + std::to_string(line_count + 1) +
					". Expected end of line, but found: '" + extra_content + "'");
			}
			++line_count;
		}
		file.close();
		if (line_count == 0) {
			throw std::invalid_argument("The data file contains no samples.");
		}
		if (state == State::Train && line_count < 2) {
			throw std::invalid_argument("Training requires at least two samples.");
		}

		// 将读取到的数据转换为 torch::Tensor
		// 特征数据，大小为 (m, n)
		data_x = torch::tensor(x_data).view({ static_cast<int64_t>(line_count), static_cast<int64_t>(numX) });
		
		// 目标数据转换
        if (state != State::Infer) {
            if (taskType == Task::Regression)
                data_y = torch::tensor(y_data_float).view({ (int64_t)line_count, 1 });
            else
                data_y = torch::tensor(y_data_long).view({ (int64_t)line_count, 1 });
        }

		// 数据归一化 (Standardization / Z-Score)
		// Train时计算并保存 Mean/Std；Test/Infer时读取 Mean/Std。
		// 定义归一化参数文件路径: data/parameters/model_ID/normalization.pt
		fs::path norm_path = parameters_path / "normalization.pt";

		if (state == State::Train) {
			// 计算均值和标准差（沿着 dim=0，即列方向计算）
			// 处理特征 X
			input_mean = data_x.mean(0);
			input_std = data_x.std(0, false);
				// 处理标准差为 0 的情况（防止除以 0），如果某列特征值完全相同，std 为 0，此时将其设为 1，避免 NaN
            input_std = torch::where(input_std == 0, torch::ones_like(input_std), input_std);
            data_x = (data_x - input_mean) / input_std;

			// 处理标签 Y（仅针对回归任务）
            if (taskType == Task::Regression) {
                target_mean = data_y.mean(0);
				target_std = data_y.std(0, false);
                // 防止 std 为 0 (虽然在回归标签中很少见，但为了健壮性)
                target_std = torch::where(target_std == 0, torch::ones_like(target_std), target_std);
                
                // 执行归一化：让 data_y 分布在 0 附近
                data_y = (data_y - target_mean) / target_std;
                
                std::cout << "[Label Normalization] Y normalized. Mean: " << target_mean.item<float>() 
                          << ", Std: " << target_std.item<float>() << std::endl;
            } else {
                // 分类任务，存个占位符，保持文件格式一致
                target_mean = torch::zeros({1});
                target_std = torch::ones({1});
            }

			// 保存参数到文件 (以便 Test/Infer 使用)
			create_parameters_dir(); // 确保目录存在
			std::vector<torch::Tensor> stats = { input_mean, input_std, target_mean, target_std };
            torch::save(stats, norm_path.string());

			std::cout << "[Data Normalization] Stats calculated and saved to: " << norm_path.string() << std::endl;
		}
		else {
			// Test 或 Infer 状态
			if (fs::exists(norm_path)) {
				// 1. 加载参数
				std::vector<torch::Tensor> stats;
				torch::load(stats, norm_path.string());

				// 兼容性检查：旧版文件可能只有2个tensor
                if (stats.size() >= 2) {
                    input_mean = stats[0];
                    input_std = stats[1];
                }
                if (stats.size() >= 4) {
                    target_mean = stats[2];
                    target_std = stats[3];
                } else {
                    // 如果读取旧模型文件，没有 Y 的参数，给予默认值
                    target_mean = torch::zeros({1});
                    target_std = torch::ones({1});
                }

				// 2. 执行归一化 (使用训练集的参数)
				data_x = (data_x - input_mean) / input_std;
				
                if (state == State::Test && taskType == Task::Regression) {
                    data_y = (data_y - target_mean) / target_std;
                }

				std::cout << "[Data Normalization] Stats loaded. Data normalized using training statistics." << std::endl;
			}
			else {
				throw std::runtime_error("Normalization stats file not found at: " + norm_path.string() +
					"\nMake sure to train the model first to generate normalization statistics.");
			}
		}

		std::cout << "Data loaded. Samples: " << line_count << ", Features: " << numX << std::endl;
	}

	// 辅助函数：构建模型
	void build_model(int output_dim) override {
		const int input_dim = static_cast<int>(numX);
		switch (modelType) {
		case ANNType::SimpleMLP: // 结构与 General 相同
			model = std::make_shared<GeneralMLP>(input_dim, output_dim);
			break;
			case ANNType::GeneralMLP:
			case ANNType::L2RegularizedMLP: // 结构与 General 相同
				model = std::make_shared<GeneralMLP>(input_dim, output_dim);
				break;
			case ANNType::DropoutMLP:
				model = std::make_shared<DropoutMLP>(input_dim, output_dim);
				break;
			case ANNType::DeepMLP:
				model = std::make_shared<DeepMLP>(input_dim, output_dim);
				break;
			case ANNType::SuperDeepMLP:
				model = std::make_shared<SuperDeepMLP>(input_dim, output_dim);
				break;
			default:
				throw std::invalid_argument("Unknown ANNType");
		}

		// 注册模型参数以便 LibTorch 追踪
		// 注意：nn::AnyModule 自动处理，但如果是手动管理需 register_module
		model.ptr()->to(torch::kFloat32); // 确保是 Float 类型
		std::cout << "Model built. Structure: " << (int)modelType << ", Output Dim: " << output_dim << std::endl;
	}

	// 辅助函数：构建优化器
	void build_optimizer() override {
		double weight_decay = 0.0;

		// 如果是 L2 正则化模型，设置 weight_decay
		if (modelType == ANNType::L2RegularizedMLP) {
			weight_decay = 1e-3; // 设置 L2 惩罚系数
		}

		// 使用 Adam 优化器
		optimizer = std::make_shared<torch::optim::Adam>(
			model.ptr()->parameters(),
			torch::optim::AdamOptions(learning_rate).weight_decay(weight_decay)
		);
		std::cout << "Optimizer built (Adam, lr=" << learning_rate << ", wd=" << weight_decay << ")" << std::endl;
	}

	const ANNType modelType;  // 模型类型（普通MLP，带dropout的MLP，带L2正则化的MLP，深层MLP）
	const size_t numX;  // 特征数量（在对象创建前必须确定）

	// 归一化参数 (Mean, Std)
	torch::Tensor input_mean;  // x 的均值
	torch::Tensor input_std;   // y 的标准差
	torch::Tensor target_mean; // y 的均值
    torch::Tensor target_std;  // y 的标准差
};
