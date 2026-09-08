#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <torch/torch.h>

namespace fs = std::filesystem;

// 枚举所有模型的状态
enum class State {
	Train,  // 训练状态
	Test,   // 测试状态
	Infer   // 推理（使用）状态
};

// 枚举所有的任务类型
enum class Task {
	Regression,  // 回归
	BinaryClassification,  // 二分类
	MultiClassification  // 多分类
};

// 神经网络基类
class NeuralNetwork {
public:
	// 定义回调函数类型（用于双线程）
	// LogCallback: 用于训练进度 (Current Epoch, Total Epochs, Train Loss, Val Loss, Log Message)
	using LogCallback = std::function<void(int, int, double, double, const std::string&)>;
	// ResultCallback: 用于推理/测试结果输出 (Message, isError)
	using ResultCallback = std::function<void(const std::string&, bool)>;
	// 进度回调类型
    using ProgressCallback = std::function<void(int)>;

	// 构造函数，只允许显式转换
	explicit NeuralNetwork(State s, Task t, const std::string& model_id)
		: state(s), taskType(t), id(model_id)
	{
		if (id.empty()) {
			throw std::invalid_argument("Model ID cannot be empty (Frontend must provide a timestamp ID).");
		}
		get_parameters_path();
		get_label_path();
	}
	
	// 设置回调函数 (供前端 Worker 调用)
	void set_callbacks(LogCallback log_cb, ResultCallback res_cb, ProgressCallback prog_cb) {
		this->logger = log_cb;
		this->result_sender = res_cb;
		this->progress_reporter = prog_cb;
	}

	// 析构函数
	virtual ~NeuralNetwork() = default;
	
	// 禁用 复制构造函数与赋值操作符
	NeuralNetwork(const NeuralNetwork&) = delete;
	NeuralNetwork& operator=(const NeuralNetwork&) = delete;
	
	// 允许 移动构造函数
	NeuralNetwork(NeuralNetwork&&) = default;
	NeuralNetwork& operator=(NeuralNetwork&&) = default;

	// 虚函数：反归一化输出（基类默认不处理，子类 ANN 会重写此函数）
    virtual torch::Tensor denormalize_output(const torch::Tensor& output) {
        return output; 
    }

	// 开始执行（将训练、测试、推理功能集合在一起）
	void conduct() {
		// 1. 获取数据/标签编码
		// 由于Test状态下获取数据的过程需要用到标签编码，所以获取数据应该放在后面
		if (state != State::Train && taskType != Task::Regression) {
			get_label();  // 获取标签编码
		}
		get_data();  // 获取数据

		// 2. 初始化模型 (构建 模型 与 优化器)
		if (taskType == Task::MultiClassification) {
			build_model(label_map.size());  // 多分类输出为标签类别数
		}
		else {
			build_model(1);  // 回归和二分类输出维度均为 1
		}
		build_optimizer();  // 构建优化器

		// 3. 获取参数（通常需要先构建模型结构，再加载参数）
		if (state != State::Train) get_parameters();

		// 4. 开始正式训练 / 测试 / 推理
		if (state == State::Train) train();
		else if (state == State::Test) test();
		else if (state == State::Infer) infer();
	}

	// 获取 id 的接口
	std::string get_id() { return id; }

	// 获取 Label Map 的接口
	const std::unordered_map<std::string, size_t>& get_label_map() const {
		return label_map;
	}

	// 用于存储回归测试的原始数据 (供 UI 绘图使用)
	std::vector<float> last_test_preds;
	std::vector<float> last_test_targets;

	// 用于存储分类测试的索引结果 (供 UI 绘图使用)
	std::vector<int64_t> last_test_preds_indices;
	std::vector<int64_t> last_test_targets_indices;

protected:
	// 辅助函数：统一输出日志
	void log_msg(const std::string& msg, bool is_error = false) {
		std::cout << msg << std::endl;  // 依然打印到控制台
		if (result_sender) result_sender(msg, is_error);  // 发送给 UI
	}

	// 训练模式
	void train() {
		//std::cout << "Starting Training..." << std::endl;
		log_msg("Starting Training...");

		int64_t num_total = data_x.size(0); // 样本总数
		if (num_total == 0) throw std::runtime_error("No training data found.");

		// 划分训练集和验证集 (9:1)
		int64_t num_val = num_total * 0.1; // 10% 做验证
		if (num_val < 1) num_val = 1;      // 确保至少有1个验证样本
		int64_t num_train = num_total - num_val;

		// 生成随机索引并切分
		auto rand_indices = torch::randperm(num_total, torch::kLong);
		auto train_indices_all = rand_indices.slice(0, 0, num_train);  // 训练集索引池
		auto val_indices = rand_indices.slice(0, num_train, num_total); // 验证集索引 (固定)

		// 准备验证集数据 (放在循环外，避免重复切片)
		auto val_x = data_x.index_select(0, val_indices);
		auto val_y = data_y.index_select(0, val_indices);

		//std::cout << "Data Split -> Train: " << num_train << " | Validation: " << num_val << std::endl;
		std::ostringstream oss;
		oss << "Data Split -> Train: " << num_train << " | Validation: " << num_val;
		log_msg(oss.str());
		
		// 超参数设置
		int max_epochs = 100;    // 最大训练轮数 (Mini-batch收敛快，轮数可适当减少)
		int64_t batch_size = 32;   // 批次大小 (32, 64, 128)
		int patience = 10;        // 耐心值：多少轮 Loss 不下降则停止
		double min_delta = 1e-4;  // 最小改善阈值：Loss 减少小于此值视为未改善

		// 状态变量
		double best_val_loss = std::numeric_limits<double>::max();
		int trigger_times = 0;             // 计数器：记录 Loss 未改善的次数
		bool early_stopped = false;        // 标记是否触发了早停
		std::vector<double> loss_history;  // 记录最近的 Loss 用于最终分析
		
		model.ptr()->train();  // 确保模型处于训练模式

		// 开始训练循环
		for (int epoch = 0; epoch < max_epochs; ++epoch) {
			// 每个 Epoch 只打乱训练集的索引
			auto epoch_shuffled_indices = torch::randperm(num_train, torch::kLong);
			// 映射回原始数据索引
			auto train_indices = train_indices_all.index_select(0, epoch_shuffled_indices);

			// 累积当前 Epoch 的平均 Loss
			double train_loss_sum = 0.0;
			int batch_count = 0;

			model.ptr()->train(); // 确保是训练模式

			// Mini-batch 循环
			for (int64_t i = 0; i < num_train; i += batch_size) {
				// 当前 Batch 的结束索引
				int64_t end_idx = std::min(i + batch_size, num_train);

				// 获取打乱后的当前 Batch 索引，indices[i : end_idx]
				auto batch_idx = train_indices.slice(0, i, end_idx);

				// 根据索引提取数据 (使用 index_select)
				auto batch_x = data_x.index_select(0, batch_idx);
				auto batch_y = data_y.index_select(0, batch_idx);

				// 训练单步（Forward + Backward）
				optimizer->zero_grad(); // 梯度清零
				auto output = model.forward(batch_x); // 前向传播

				// 计算损失
				torch::Tensor loss;
				if (taskType == Task::BinaryClassification) {
					// 二分类任务：BCE With Logits (输入 logits，目标 float)
					loss = torch::binary_cross_entropy_with_logits(output, batch_y.to(torch::kFloat32));
				}
				else if (taskType == Task::MultiClassification) {
					// 多分类任务：Cross Entropy (输入 logits [N, C]，目标 Long [N])
					// data_y 形状是 (N, 1)，需要 squeeze 变成 (N)
					loss = torch::cross_entropy_loss(output, batch_y.squeeze(1).to(torch::kLong));
				}
				else {
					// 回归任务：MSE Loss
					loss = torch::mse_loss(output, batch_y.to(torch::kFloat32));
				}

				loss.backward();
				optimizer->step(); // 更新参数

				// 累加 Loss (用于计算 Epoch 平均 Loss)
				train_loss_sum += loss.item<double>();
				batch_count++;

				// 打印每个 Batch 的信息
				// std::cout << "\rEpoch " << epoch << " Batch " << batch_count << " Loss: " << loss.item<double>() << std::flush;
			}

			// 计算本轮平均 Loss
			double current_train_loss = train_loss_sum / batch_count;

			// Validation Step (计算验证集 Loss)
			model.ptr()->eval(); // 切换到评估模式 (关闭 Dropout, BN 使用移动平均)
			double current_val_loss = 0.0;

			{
				torch::NoGradGuard no_grad; // 不计算梯度，节省内存
				auto val_out = model.forward(val_x);
				torch::Tensor v_loss;

				// 复用 Loss 计算逻辑
				if (taskType == Task::BinaryClassification) {
					v_loss = torch::binary_cross_entropy_with_logits(val_out, val_y.to(torch::kFloat32));
				}
				else if (taskType == Task::MultiClassification) {
					v_loss = torch::cross_entropy_loss(val_out, val_y.squeeze(1).to(torch::kLong));
				}
				else {
					v_loss = torch::mse_loss(val_out, val_y.to(torch::kFloat32));
				}
				current_val_loss = v_loss.item<double>();
			}

			loss_history.push_back(current_val_loss); // 记录验证集 Loss 历史

			// 打印 Train Loss 和 Val Loss
			//std::cout << "Epoch [" << epoch + 1 << "/" << max_epochs
			//		  << "] Train Loss: " << std::fixed << std::setprecision(5) << current_train_loss
			//		  << " | Val Loss: " << std::fixed << std::setprecision(5) << current_val_loss << std::endl;
			
			// 向前端传输信息
			// 准备信息流（日志输出改为通过回调发送）
			std::ostringstream oss;
			oss << "Epoch [" << epoch + 1 << "/" << max_epochs
				<< "] Train Loss: " << std::fixed << std::setprecision(5) << current_train_loss
				<< " | Val Loss: " << std::fixed << std::setprecision(5) << current_val_loss;

			std::cout << oss.str() << std::endl;  // 控制台备份

			// 通过 callback 向 GUI 发送信息
			if (logger) {
				// 发送给 UI 更新图表 (当前轮数，总轮数，验证集Loss，文本日志)
				logger(epoch + 1, max_epochs, current_train_loss, current_val_loss, oss.str());
			}

			// Early Stopping（早停）（基于 Val Loss 判断）
			if (current_val_loss < best_val_loss - min_delta) {
				// 如果当前 Loss 比历史最佳 Loss 还要小且超过阈值，说明模型还在学
				best_val_loss = current_val_loss;
				trigger_times = 0; // 重置计数器
			}
			else {
				// Loss 没有显著下降
				trigger_times++;
				if (trigger_times >= patience) {
					//std::cout << "\n[Early Stopping] Triggered at epoch " << epoch + 1 << "." << std::endl;
					//std::cout << "Reason: Validation Loss did not improve for " << patience << " epochs." << std::endl;
					//std::cout << "Best Val Loss: " << best_val_loss << std::endl;
					std::ostringstream oss;
					oss << "\n[Early Stopping] Triggered at epoch " << epoch + 1 << "." << "\n"
						<< "Reason: Validation Loss did not improve for " << patience << " epochs." << "\n"
						<< "Best Val Loss: " << best_val_loss;
					log_msg(oss.str());
					
					early_stopped = true;
					break; // 提前退出循环
				}
			}
		}

		// 检测最终是否收敛（针对没有触发早停的情况）
		if (!early_stopped) {
			//std::cout << "\n[Training Completed] Reached maximum epochs (" << max_epochs << ")." << std::endl;
			std::ostringstream oss;
			oss << "\n[Training Completed] Reached maximum epochs (" << max_epochs << ").";
			log_msg(oss.str());

			// 分析最后一段 Loss 的趋势
			// 如果最后时刻计数器 trigger_times 依然很小 (例如 < 5)，说明直到最后几轮 Loss 还在显著下降
			// 这意味着模型可能还没有完全收敛 (Under-fitting or need more epochs)
			if (trigger_times < 5) {
				//std::cout << "Warning: The model might NOT have fully converged yet." << std::endl;
				log_msg("Warning: The model might NOT have fully converged yet.");
				std::cout << "Suggestion: The loss was still decreasing at the end. Consider increasing max_epochs." << std::endl;
			}
			else {
				// 如果 trigger_times 比较大 (但还没到 patience)，说明 Loss 已经震荡或趋于平稳
				//std::cout << "The model appears to have converged." << std::endl;
				log_msg("The model appears to have converged.");
				std::cout << "Reason: Loss stabilized towards the end of training." << std::endl;
			}
			//std::cout << "Final Val Loss: " << loss_history.back() << std::endl;
			log_msg("Final Val Loss: " + std::to_string(loss_history.back()));
		}

		save_model();  // 存储模型
		if (taskType != Task::Regression)
			save_label();  // 如果是分类问题，还需要存储标签编码

		log_msg("Training process finished.");
	}

	// 测试模式
	void test() {
		//std::cout << "Starting Testing...\n" << std::endl;
		log_msg("Starting Testing...");
		// 先调用 predict() 获取预测值，再与目标值 data_y 进行比对
		torch::Tensor preds = predict(); // 根据任务不同，返回可能是数值、概率或索引

		// 确保目标值在 CPU 上以便后续计算
		torch::Tensor targets = data_y.to(torch::kCPU);
		preds = preds.to(torch::kCPU);

		// 准备 "真实刻度" 的数据用于展示
        // 我们希望计算 Loss 用归一化数据（和训练一致），但展示给用户看用真实数据
        torch::Tensor preds_real = denormalize_output(preds);
        torch::Tensor targets_real = denormalize_output(targets);

		// 获取总样本数，供进度计算使用
		int total_samples = preds_real.size(0);

		// 1. 回归任务
		// 如果是回归任务，需要给出可视化评估（预测值与目标值的散点图），并且给出 MSE 值。
		if (taskType == Task::Regression) {
			// 计算 MSE
			torch::Tensor mse = torch::mse_loss(preds, targets);
			//std::cout << "Test MSE: " << mse.item<float>() << std::endl;
			log_msg("Test MSE: " + std::to_string(mse.item<float>()));

			// 真实的误差平方均值，用 real 数据算：
            // torch::Tensor mse_real = torch::mse_loss(preds_real, targets_real);
            // log_msg("Test MSE (Real Scale): " + std::to_string(mse_real.item<float>()));

			// 可视化数据输出 
			// 保存数据供 UI 使用
			last_test_preds.clear();
			last_test_targets.clear();

			// 将 Tensor 转换为 std::vector
			auto preds_cont = preds_real.contiguous(); // 使用真实值
            auto targets_cont = targets_real.contiguous(); // 使用真实值

			// 简单的数据拷贝
			const float* p_ptr = preds_cont.data_ptr<float>();
            const float* t_ptr = targets_cont.data_ptr<float>();
            int count = preds_real.numel();

            last_test_preds.assign(p_ptr, p_ptr + count);
            last_test_targets.assign(t_ptr, t_ptr + count);

			// 详细数据输出
			//std::cout << "\n--- Sample Predictions (Prediction vs Target | Residual) ---" << std::endl;
			log_msg("\n--- Sample Predictions (Prediction vs Target | Residual) ---");
			//std::cout << std::left << std::setw(15) << "Prediction"
			//	<< std::setw(15) << "Target"
			//	<< std::setw(15) << "Residual" << std::endl;

			auto preds_a = preds_real.accessor<float, 2>(); // 使用真实值
            auto targets_a = targets_real.accessor<float, 2>(); // 使用真实值

			for (int i = 0; i < total_samples; ++i) {
				//float p = preds_a[i][0];
				//float t = targets_a[i][0];
				//float r = p - t;
				//std::cout << std::left << std::setw(15) << p
				//	<< std::setw(15) << t
				//	<< std::setw(15) << r << std::endl;

				// 发送至前端
				std::ostringstream oss;
				oss << "Sample " << i+1 << ": Pred=" << preds_a[i][0]
					<< " | Target=" << targets_a[i][0]
					<< " | Diff=" << (preds_a[i][0] - targets_a[i][0]);
				log_msg(oss.str());
				// 更新进度
				if (progress_reporter) 
					progress_reporter((int)((double)(i + 1) / total_samples * 100));
			}
		}
		// 2. 分类任务
		// 如果是分类任务，需要给出所有预测错误的数据在原文件中的行数 以及它的目标值和错误的预测值，
		// 并且最终还需要给出预测正确的样本占总样本的比例（准确率） 以及 Macro-F1 Score。
		else {
			torch::Tensor pred_labels;

			// 统一预测格式为整数标签
			if (taskType == Task::BinaryClassification) {
				// 二分类 predict 返回的是概率，需要 > 0.5 转换为 0/1
				pred_labels = (preds > 0.5).to(torch::kLong).view({ -1 });
			}
			else {
				// 多分类 predict 返回的已经是索引
				pred_labels = preds.to(torch::kLong).view({ -1 });
			}

			// 确保 targets 也是一维 Long
			torch::Tensor true_labels = targets.to(torch::kLong).view({ -1 });

			int64_t num_samples = true_labels.size(0);  // 样本数量

			// 计算 Metric 过程很快，先给一个中间进度
			if (progress_reporter) progress_reporter(50);

			// 统计错误样本
			int64_t correct_count = 0;
			std::vector<std::tuple<int, int, int>> errors; // <样本次序，预测值，真实值>

			auto p_data = pred_labels.accessor<int64_t, 1>();
			auto t_data = true_labels.accessor<int64_t, 1>();

			for (int i = 0; i < num_samples; ++i) {
				if (p_data[i] == t_data[i]) {
					correct_count++;
				}
				else {
					// 记录错误
					errors.emplace_back(i + 1, p_data[i], t_data[i]);
				}
			}

			// 保存数据供 UI 使用
			last_test_preds_indices.clear();
			last_test_targets_indices.clear();

			// 转为 CPU 并转为 vector
			auto preds_cpu = pred_labels.to(torch::kCPU);
			auto targets_cpu = targets.to(torch::kLong).view({ -1 }).to(torch::kCPU);

			auto preds_data = preds_cpu.accessor<int64_t, 1>();
			auto targets_data = targets_cpu.accessor<int64_t, 1>();

			for (int i = 0; i < preds_cpu.size(0); ++i) {
				last_test_preds_indices.push_back(preds_data[i]);
				last_test_targets_indices.push_back(targets_data[i]);
			}

			// 计算准确率
			double accuracy = static_cast<double>(correct_count) / num_samples;

			// 计算 Macro-F1 Score
			size_t num_classes = label_map.size();  // 类别数量

			std::vector<int> tp(num_classes, 0);
			std::vector<int> fp(num_classes, 0);
			std::vector<int> fn(num_classes, 0);

			for (int i = 0; i < num_samples; ++i) {
				int p = p_data[i]; // 预测类别
				int t = t_data[i]; // 真实类别

				if (p == t) {
					tp[p]++;
				}
				else {
					fp[p]++; // 预测为 p，但实际不是 p -> p 的假阳性
					fn[t]++; // 实际为 t，但预测不是 t -> t 的假阴性
				}
			}

			double macro_f1 = 0.0;
			for (size_t c = 0; c < num_classes; ++c) {
				double precision = (tp[c] + fp[c] == 0) ? 0.0 : (double)tp[c] / (tp[c] + fp[c]);
				double recall = (tp[c] + fn[c] == 0) ? 0.0 : (double)tp[c] / (tp[c] + fn[c]);
				double f1 = (precision + recall == 0) ? 0.0 : 2.0 * (precision * recall) / (precision + recall);
				macro_f1 += f1;
			}
			macro_f1 /= num_classes;

			// 输出结果
			//std::cout << "\n--- Classification Metrics ---" << std::endl;
			//std::cout << "Accuracy:    " << (accuracy * 100.0) << "% (" << correct_count << "/" << num_samples << ")" << std::endl;
			//std::cout << "Macro-F1:    " << macro_f1 << std::endl;
			log_msg("\n--- Classification Metrics ---");
			std::ostringstream oss;
			oss << "Accuracy:    " << (accuracy * 100.0) << "% (" << correct_count << "/" << num_samples << ")";
			log_msg(oss.str());
			log_msg("Macro-F1: " + std::to_string(macro_f1));

			if (!errors.empty()) {
				//std::cout << "\n--- Wrong Predictions ---" << std::endl;
				log_msg("\n--- Wrong Predictions ---");
				//std::cout << "Line\tPredicted\tActual" << std::endl;

				// 为了显示标签名，建立反向映射
				std::unordered_map<size_t, std::string> idx_to_label;
				reverse_map(idx_to_label);

				int i = 0;
				for (const auto& err : errors) {
					i++;
					//int line = std::get<0>(err);
					//int p_idx = std::get<1>(err);
					//int t_idx = std::get<2>(err);

					//std::string p_str = idx_to_label.count(p_idx) ? idx_to_label[p_idx] : std::to_string(p_idx);
					//std::string t_str = idx_to_label.count(t_idx) ? idx_to_label[t_idx] : std::to_string(t_idx);

					//std::cout << line << "\t" << p_str << "\t\t" << t_str << std::endl;

					std::ostringstream oss;
					oss << "Line " << std::get<0>(err)
						<< ": Pred=" << std::get<1>(err)
						<< " Expect=" << std::get<2>(err);
					log_msg(oss.str(), true);  // true 表示红色/错误信息
					// 更新进度
                	if (progress_reporter)
						progress_reporter((int)((double)i / total_samples * 100));
				}
			}
			else {
				//std::cout << "Perfect prediction! No errors found." << std::endl;
				log_msg("Perfect prediction! No errors found.");
			}
		}

	}

	// 推理模式
	void infer() {
		//std::cout << "Starting Inference..." << std::endl;
		log_msg("Starting Inference...");

		// 获取预测结果
		torch::Tensor preds = predict();
		preds = preds.to(torch::kCPU);

		// 反归一化为真实数值
        torch::Tensor preds_real = denormalize_output(preds);

		int total_samples = preds_real.size(0);  // 获取总数

		//std::cout << "\n--- Inference Results ---" << std::endl;
		log_msg("\n--- Inference Results ---");

		// 根据任务类型输出
		if (taskType == Task::Regression) {  // 回归任务：直接输出数值
			auto accessor = preds_real.accessor<float, 2>();
			for (int i = 0; i < preds_real.size(0); ++i) {
				//std::cout << "Sample " << (i + 1) << ": " << accessor[i][0] << std::endl;
				std::ostringstream oss;
				oss << "Sample " << (i + 1) << ": " << accessor[i][0];
				log_msg(oss.str());
				// 更新进度
				if (progress_reporter) 
					progress_reporter((int)((double)(i + 1) / total_samples * 100));
			}
		}
		else {  // 分类任务：输出对应的标签字符串
			// 准备反向映射 map (Index -> String)
			std::unordered_map<size_t, std::string> idx_to_label;
			reverse_map(idx_to_label);

			torch::Tensor result_indices;
			if (taskType == Task::BinaryClassification) {
				// 二分类：概率 > 0.5 为 1，否则为 0
				result_indices = (preds > 0.5).to(torch::kLong).view({ -1 });
			}
			else {
				// 多分类：已经是索引
				result_indices = preds.to(torch::kLong).view({ -1 });
			}

			auto accessor = result_indices.accessor<int64_t, 1>();
			for (int i = 0; i < result_indices.size(0); ++i) {
				int64_t idx = accessor[i];
				std::string label_name = "Unknown";
				if (idx_to_label.find(idx) != idx_to_label.end()) {
					label_name = idx_to_label[idx];
				}
				//std::cout << "Sample " << (i + 1) << ": " << label_name << std::endl;
				log_msg("Sample " + std::to_string(i + 1) + ": " + label_name);
				// 更新进度
				if (progress_reporter) 
					progress_reporter((int)((double)(i + 1) / total_samples * 100));
			}
		}
		//std::cout << "Inference completed." << std::endl;
		log_msg("Inference completed.");
	}

	// 获取模型数据路径（从前端用户的输入获取）
	// 由于在ANN和CNN中数据路径要求不同，所有在此设为虚函数
	// 具体来说，ANN要求路径对应的是一个.txt文件，CNN要求路径对应的是文件夹
	virtual void get_data_path(const std::string& path) {
		// 检查用户提供的路径是否为空或不存在
		if (path.empty() || !fs::exists(path)) {
			throw std::invalid_argument("Invalid path.");
		}
		data_path = path;
	}

	// 获取模型参数路径（自生成）
	void get_parameters_path() {
		parameters_path = base_data_path / ("parameters/model_" + id);
		parameters_path.make_preferred();  // 统一分隔符
	}

	// 获取模型标签编码路径（自生成，仅用于分类问题）
	void get_label_path() {
		label_path = base_data_path / ("label/model_" + id);
		label_path.make_preferred();  // 统一分隔符
	}

	// 创建模型参数目录（自生成，仅用于Train）
	void create_parameters_dir() {
		try {
			if (!fs::exists(parameters_path)) {
				fs::create_directories(parameters_path);
				std::cout << "Created directory: " << parameters_path << std::endl;
			} else {
				std::cout << "Directory already exists: " << parameters_path << std::endl;
			}
		}
		catch (const fs::filesystem_error& ex) {
			std::cerr << "Error creating directory: " << ex.what() << std::endl;
			throw std::runtime_error("Failed to create directories: " + std::string(ex.what()));
		}
	}

	// 创建模型标签编码路径（自生成，仅用于Train，仅用于分类问题）
	void create_label_dir() {
		try {
			if (!fs::exists(label_path)) {
				fs::create_directories(label_path);
				std::cout << "Created directory: " << label_path << std::endl;
			}
			else {
				std::cout << "Directory already exists: " << label_path << std::endl;
			}
		}
		catch (const fs::filesystem_error& ex) {
			std::cerr << "Error creating directory: " << ex.what() << std::endl;
			throw std::runtime_error("Failed to create directories: " + std::string(ex.what()));
		}
	}
	
	// 获取模型参数
	void get_parameters() {
		// 在 parameters_path 存储的路径下获取模型参数
		// 1. 构造文件路径
		fs::path model_file = parameters_path / "model.pt";

		// 2. 检查文件是否存在
		if (!fs::exists(model_file)) {
			throw std::runtime_error("Model parameters file not found: " + model_file.string());
		}

		// 3. 加载参数
		try {
			std::cout << "Loading model parameters from: " << model_file << std::endl;
			
			// torch::load(model, model_file.string());  // 报错！
			
			// [修复方案] 使用 InputArchive 显式加载，绕过 operator>>
			torch::serialize::InputArchive archive;
			// 从文件读取数据到归档
			archive.load_from(model_file.string());
			// 将归档中的参数加载进现有的模型结构中
			model.ptr()->load(archive);
			
			model.ptr()->eval();
			std::cout << "Model parameters loaded successfully." << std::endl;
		}
		catch (const std::exception& e) {
			throw std::runtime_error("Failed to load model parameters: " + std::string(e.what()));
		}
	}

	// 获取标签编码（Test, Infer状态，分类问题特有）
	void get_label() {
		fs::path label_file = label_path / "labels.txt";  // 文件名

		if (!fs::exists(label_file)) {  // 检查文件是否存在
			throw std::runtime_error("Label file not found: " + label_file.string());
		}

		std::ifstream file(label_file);
		if (!file.is_open()) {  // 检查文件是否能打开
			throw std::runtime_error("Failed to open label file for reading.");
		}

		std::cout << "Loading labels from: " << label_file << std::endl;

		// 读取文件并重建 map
		label_map.clear();
		std::string label_name;
		size_t label_idx;

		while (std::getline(file, label_name)) {
			if (file >> label_idx) {
				label_map[label_name] = label_idx;
				// 一定要消除行末的换行符，否则下一次 getline 读取的就是换行符
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
			}
		}

		file.close();
		std::cout << "Labels loaded. Total classes: " << label_map.size() << std::endl;
	}

	// 虚函数：获取模型数据
	virtual void get_data() {  // 本虚函数无实用
		switch (state) {
		case State::Train: case State::Test:  // train和test的数据获取方式是一样的，可以写在一起
			std::cout << "Get train/test data..." << std::endl;
			break;
		case State::Infer:  // infer数据获取方式不同
			std::cout << "Get infer data..." << std::endl;
			break;
		default:
			throw std::invalid_argument("Invalid model state.");
		}
	}

	// 虚函数：构建模型（ANN）
	virtual void build_model(int output_dim) = 0;

	// 虚函数：构建优化器
	virtual void build_optimizer() = 0;

	// 推理步骤
	torch::Tensor predict() {
		// 检查模型是否存在
		if (model.is_empty())
			throw std::runtime_error("Model not initialized.");

		torch::NoGradGuard no_grad; // 禁用梯度计算
		model.ptr()->eval(); // 设置为评估模式

		auto x = data_x.to(torch::kFloat32);
		auto output = model.forward(x);

		// 根据任务类型处理输出
		if (taskType == Task::BinaryClassification) {
			// 二分类：Sigmoid 激活 -> 概率 -> 阈值判决 (0.5)
			// 这里返回概率值，如果需要 0/1 标签可以使用 (output.sigmoid() > 0.5).to(torch::kLong)
			return output.sigmoid();
		}
		else if (taskType == Task::MultiClassification) {
			// 多分类：Softmax -> Argmax 获取类别索引
			return output.argmax(1); // 返回形状 (N)
		}
		else {
			// 回归：直接返回预测值
			return output;
		}
	}

	// 保存模型（Train状态特有）
	void save_model() {
		// 确保存储目录存在
		create_parameters_dir();

		// 获取路径
		fs::path file_p = parameters_path / "model.pt";
		file_p.make_preferred();
		std::string save_path = file_p.string();

		try {
			//torch::save(model, save_path);  // 报错！
			
			// [修复方案] 使用 OutputArchive 显式保存，绕过 operator<<
			torch::serialize::OutputArchive archive;
			// model.ptr() 返回 shared_ptr<Module>，调用其基类的 save 方法
			model.ptr()->save(archive);
			// 将归档内容写入文件
			archive.save_to(save_path);

			std::cout << "Model saved successfully to: " << save_path << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving model: " << e.what() << std::endl;
		}
	}
	
	// 保存标签编码（Train状态，分类问题特有）
	void save_label() {
		// 确保目录存在
		create_label_dir();

		fs::path label_file = label_path / "labels.txt";  // 文件名
		label_file.make_preferred();

		std::ofstream file(label_file);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open label file for writing: " + label_file.string());
		}

		// 遍历 map 并写入文件 (格式: 标签名 索引)
		for (const auto& pair : label_map) {
			file << pair.first << "\n" << pair.second << "\n";
		}

		file.close();
		std::cout << "Label map saved to: " << label_file << " (Total labels: " << label_map.size() << ")" << std::endl;
	}

	// 反向建立标签编码映射
	void reverse_map(std::unordered_map<size_t, std::string>& idx_to_label) {
		for (const auto& pair : label_map) {
			idx_to_label[pair.second] = pair.first;
		}
	}

	// 基础数据路径，统一为"data"，便于管理。
	// 模型基础信息以及模型参数都会存储到"data"目录下。
	static const fs::path base_data_path;

	const State state;  // 模型状态（train, test, infer）
	const Task taskType;  // 任务类型（回归，二分类、多分类）
	std::string id;  // 模型编号（时间戳）
	LogCallback logger = nullptr;       		   // 日志回调
	ResultCallback result_sender = nullptr; 	   // 结果回调
	ProgressCallback progress_reporter = nullptr;  // 进度回调

	fs::path data_path;  // 模型数据数据路径（不同状态下，数据的意义不同）
	fs::path parameters_path; // 模型参数路径
	fs::path label_path; // 模型标签编码路径（仅用于分类问题）

	torch::Tensor data_x;  // 输入数据 x
	torch::Tensor data_y;  // 目标数据 y （infer状态下没有）
	torch::Tensor weight;  // 模型权重
	torch::Tensor bias;    // 模型偏置

	// 标签编码：字符串到整数的映射（仅用于分类任务）
	// ps：由于存在高频查找环节，所以查找平均时间复杂度为O(1)的unordered_map更加适合
	std::unordered_map<std::string, size_t> label_map;

	torch::nn::AnyModule model;  // 使用 AnyModule 存储动态类型的 MLP 模型
	std::shared_ptr<torch::optim::Optimizer> optimizer;  // 使用智能指针管理优化器
};

// 初始化静态成员（加上 inline 的原因：确保在所有文件中只算一份）
inline const fs::path NeuralNetwork::base_data_path = "data";