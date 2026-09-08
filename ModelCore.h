#pragma once // 防止头文件被重复包含，确保编译效率

// Backend 头文件（必须先包含 torch，防止 slots 关键字冲突）
// 因为 Qt 定义了 'slots' 关键字，而 LibTorch 内部也使用了名为 'slots' 的变量，
// 所以必须先取消定义 'slots'，包含 torch 头文件后再重新定义它。
#undef slots 
#include <torch/torch.h> // 包含 LibTorch (PyTorch C++ 前端) 的核心头文件
#define slots Q_SLOTS // 重新定义 slots 为 Q_SLOTS，恢复 Qt 的信号槽功能

#include "ANN.h" // 包含人工神经网络 (ANN) 的定义头文件
#include "CNN.h" // 包含卷积神经网络 (CNN) 的定义头文件

// 包含必要的 Qt 核心头文件
#include <QString> // Qt 字符串类
#include <QDateTime> // Qt 日期时间类
#include <QUuid> // Qt 通用唯一识别码类
#include <QJsonObject> // Qt JSON 对象类，用于处理 JSON 数据
#include <QJsonDocument> // Qt JSON 文档类，用于读写 JSON
#include <QFile> // Qt 文件操作类
#include <QDir> // Qt 目录操作类
#include <QThread> // Qt 线程类，用于多线程处理
#include <QObject> // Qt 对象基类
#include <QTimer> // Qt 定时器类
#include <QRandomGenerator> // Qt 随机数生成器
#include <QCoreApplication> // Qt 核心应用程序类
#include <QDebug> // Qt 调试输出类
#include <cmath> // 标准 C++ 数学库

// 1. 数据定义

// 定义任务类型的枚举类，区分不同类型的机器学习任务
enum class TaskType {
    NumericRegression, // 数值回归任务（预测连续值）
    NumericBinaryClass, // 数值二分类任务
    NumericMultiClass, // 数值多分类任务
    ImageBinaryClass, // 图像二分类任务
    ImageMultiClass // 图像多分类任务
};

// 辅助内联函数：判断是否为数值型任务
inline bool isNumericTask(TaskType t) {
    // 如果是回归、数值二分类或数值多分类，则返回 true
    return t == TaskType::NumericRegression ||
        t == TaskType::NumericBinaryClass ||
        t == TaskType::NumericMultiClass;
}

// 定义模型架构类型的枚举类，列出支持的神经网络结构
enum class ArchType {
    // ANN Types (人工神经网络类型)
    SimpleMLP, // 简单多层感知机
    GeneralMLP, // 通用多层感知机
    DropoutMLP, // 带 Dropout 层的 MLP (防过拟合)
    L2RegularizedMLP, // 带 L2 正则化的 MLP
    DeepMLP, // 深度 MLP
    SuperDeepMLP, // 超深 MLP

    // CNN Types (卷积神经网络类型)
    LeNet, // LeNet 架构
    AlexNet, // AlexNet 架构
    VGGNet, // VGGNet 架构
    ResNet, // ResNet 架构

    Unknown // 未知类型
};

// 定义模型状态的枚举类
enum class ModelStatus {
    Draft, // 草稿状态
    Trained, // 已训练状态
    Deleted // 已删除状态（逻辑删除，在回收站中）
};

// 辅助转换函数：将前端的 ArchType 枚举转换为后端 ANN 具体的枚举类型

inline ANNType toANNType(ArchType t) {
    switch (t) {
    case ArchType::SimpleMLP: return ANNType::SimpleMLP; // 转换 SimpleMLP
    case ArchType::DropoutMLP: return ANNType::DropoutMLP; // 转换 DropoutMLP
    case ArchType::L2RegularizedMLP: return ANNType::L2RegularizedMLP; // 转换 L2正则化MLP
    case ArchType::DeepMLP: return ANNType::DeepMLP; // 转换 DeepMLP
    case ArchType::SuperDeepMLP: return ANNType::SuperDeepMLP; // 转换 SuperDeepMLP
    default: return ANNType::GeneralMLP; // 默认为通用 MLP
    }
}

// 辅助转换函数：将前端的 ArchType 枚举转换为后端 CNN 具体的枚举类型
inline CNNType toCNNType(ArchType t) {
    switch (t) {
    case ArchType::AlexNet: return CNNType::AlexNet; // 转换 AlexNet
    case ArchType::VGGNet: return CNNType::VGGNet; // 转换 VGGNet
    case ArchType::ResNet: return CNNType::ResNet; // 转换 ResNet
    default: return CNNType::LeNet; // 默认为 LeNet
    }
}

// 辅助转换函数：将前端的 TaskType 转换为后端通用的 Task 枚举
inline Task toBackendTask(TaskType t) {
    switch (t) {
    case TaskType::NumericRegression:
        return Task::Regression; // 数值回归对应后端 Regression
    case TaskType::NumericBinaryClass:
    case TaskType::ImageBinaryClass:
        return Task::BinaryClassification; // 二分类对应后端 BinaryClassification
    default:
        return Task::MultiClassification; // 其他情况（多分类）对应 MultiClassification
    }
}

// 具体模块

// 定义模型信息的结构体，用于存储模型的元数据
struct ModelInfo {
    QString id; // 模型唯一标识符 ID
    QString name; // 模型名称
    QString intro; // 模型简介
    TaskType taskType; // 任务类型
    ArchType archType; // 架构类型
    ModelStatus status; // 当前状态
    QString dataPath; // 训练数据路径
    QDateTime createTime; // 创建时间
    double accuracy = 0.0; // 模型准确率（初始化为 0.0）
    int inputDim = 0; // 输入特征维度（初始化为 0）

    // 重载 == 运算符，用于比较两个 ModelInfo 对象是否相同（通过 ID 判断）
    bool operator==(const ModelInfo& other) const { return id == other.id; }

    // 将 ModelInfo 结构体序列化为 JSON 对象，方便存储到文件
    QJsonObject toJson() const {
        QJsonObject json; // 创建 JSON 对象
        json["id"] = id; // 存入 ID
        json["name"] = name; // 存入名称
        json["intro"] = intro; // 存入简介
        json["taskType"] = (int)taskType; // 存入任务类型（转为 int）
        json["archType"] = (int)archType; // 存入架构类型（转为 int）
        json["status"] = (int)status; // 存入状态（转为 int）
        json["dataPath"] = dataPath; // 存入数据路径
        json["createTime"] = createTime.toString(Qt::ISODate); // 存入创建时间（转为 ISO 字符串）
        json["accuracy"] = accuracy; // 存入准确率
        json["inputDim"] = inputDim; // 存入输入维度
        return json; // 返回 JSON 对象
    }

    // 从 JSON 对象反序列化为 ModelInfo 结构体，方便从文件读取
    static ModelInfo fromJson(const QJsonObject& json) {
        ModelInfo info; // 创建 ModelInfo 对象
        info.id = json["id"].toString(); // 读取 ID
        info.name = json["name"].toString(); // 读取名称
        info.intro = json["intro"].toString(); // 读取简介
        info.taskType = (TaskType)json["taskType"].toInt(); // 读取任务类型并转回枚举
        info.archType = (ArchType)json["archType"].toInt(); // 读取架构类型并转回枚举
        info.status = (ModelStatus)json["status"].toInt(); // 读取状态并转回枚举
        info.dataPath = json["dataPath"].toString(); // 读取数据路径
        info.createTime = QDateTime::fromString(json["createTime"].toString(), Qt::ISODate); // 读取并解析时间字符串
        info.accuracy = json["accuracy"].toDouble(); // 读取准确率
        info.inputDim = json["inputDim"].toInt(); // 读取输入维度
        return info; // 返回填充好的对象
    }
};

// 辅助函数：将 TaskType 枚举转换为可读的字符串
inline QString taskTypeToString(TaskType t) {
    switch (t) {
    case TaskType::NumericRegression: return "数值回归任务";
    case TaskType::NumericBinaryClass: return "数值二分类任务";
    case TaskType::NumericMultiClass: return "数值多分类任务";
    case TaskType::ImageBinaryClass: return "图像二分类任务";
    case TaskType::ImageMultiClass: return "图像多分类任务";
    default: return "未知任务";
    }
}

// 辅助函数：将 ArchType 枚举转换为可读的字符串
inline QString archTypeToString(ArchType t) {
    switch (t) {
    case ArchType::SimpleMLP: return "SimpleMLP";
    case ArchType::GeneralMLP: return "GeneralMLP";
    case ArchType::DropoutMLP: return "DropoutMLP";
    case ArchType::L2RegularizedMLP: return "L2RegularizedMLP";
    case ArchType::DeepMLP: return "DeepMLP";
    case ArchType::SuperDeepMLP: return "SuperDeepMLP";

    case ArchType::LeNet: return "LeNet";
    case ArchType::AlexNet: return "AlexNet";
    case ArchType::VGGNet: return "VGGNet";
    case ArchType::ResNet: return "ResNet";
    default: return "未知模型";
    }
}

// 2. DatabaseManager (模拟数据库)
// 单例类，负责管理模型信息的持久化存储（保存到 JSON 文件）

class DatabaseManager {
public:
    // 获取单例实例的静态方法
    static DatabaseManager& instance() {
        static DatabaseManager instance; // 静态局部变量，确保只初始化一次
        return instance; // 返回单例引用
    }

    // 初始化数据库目录
    void init() {
        QString root = "data"; // 定义根目录名为 data
        infoDir = QDir(root + "/info"); // 定义信息存储子目录 data/info
        if (!infoDir.exists()) infoDir.mkpath("."); // 如果目录不存在，则创建它
    }

    // 保存单个模型信息到文件
    void saveModel(const ModelInfo& model) {
        QString modelInfoPath = infoDir.filePath(model.id); // 根据 ID 生成文件路径
        QDir().mkpath(modelInfoPath); // 创建该模型对应的文件夹
        QFile file(modelInfoPath + "/info.json"); // 定义具体的 info.json 文件
        if (file.open(QIODevice::WriteOnly)) { // 以只写模式打开文件
            QJsonDocument doc(model.toJson()); // 将模型信息转为 JSON 文档
            file.write(doc.toJson()); // 写入 JSON 数据到文件
            file.close(); // 关闭文件
        }
    }

    // 永久删除模型及其所有相关文件
    void deleteModel(const QString& id) {
        // 删除 info (元数据)
        QDir dir(infoDir.filePath(id)); // 定位到元数据目录
        if (dir.exists()) {
            dir.removeRecursively(); // 递归删除目录及内容
        }

        // 删除 parameters (模型参数/权重文件)
        QString paramPath = "data/parameters/model_" + id; // 定位参数目录
        QDir paramDir(paramPath);
        if (paramDir.exists()) {
            paramDir.removeRecursively(); // 递归删除
        }

        // 3. 删除 label (分类标签映射文件)
        QString labelPath = "data/label/model_" + id; // 定位标签目录
        QDir labelDir(labelPath);
        if (labelDir.exists()) {
            labelDir.removeRecursively(); // 递归删除
        }

        qDebug() << "Model" << id << "permanently deleted."; // 打印调试信息，确认删除
    }

    // 加载所有已保存的模型
    QList<ModelInfo> loadAllModels() {
        QList<ModelInfo> list; // 创建模型列表
        // 获取所有子目录名（即模型 ID），排除 . 和 ..
        QStringList subDirs = infoDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& id : subDirs) { // 遍历每个 ID
            QString jsonPath = infoDir.filePath(id) + "/info.json"; // 构建 JSON 文件路径
            QFile file(jsonPath);
            if (file.open(QIODevice::ReadOnly)) { // 以只读模式打开
                QByteArray data = file.readAll(); // 读取所有内容
                QJsonDocument doc = QJsonDocument::fromJson(data); // 解析 JSON
                ModelInfo info = ModelInfo::fromJson(doc.object()); // 转换为 ModelInfo 对象
                list.append(info); // 添加到列表
            }
        }
        return list; // 返回模型列表
    }

private:
    DatabaseManager() {} // 私有构造函数，防止外部实例化（单例模式）
    QDir infoDir; // 存储信息目录的 QDir 对象
};

// 3. 后台线程

// 训练工作线程类，继承自 QThread，用于在后台执行耗时的训练任务
class TrainingWorker : public QThread {
    Q_OBJECT // Qt 宏，启用信号槽机制
public:
    ModelInfo m_info;  // 存储模型信息（以便对应建立模型）

    // 构造函数，接收 ModelInfo
    explicit TrainingWorker(ModelInfo info) : m_info(info) {}

    // 线程主执行函数
    void run() override {
        emit statusChanged("正在初始化计算图..."); // 发送信号：状态更新

        try {
            // 确定模型状态和任务类型
            State state = State::Train; // 设置为训练状态
            Task task = toBackendTask(m_info.taskType); // 获取后端任务类型

            // 使用智能指针管理神经网络对象，初始化神经网络 (ANN / CNN)
            std::unique_ptr<NeuralNetwork> net;

            // 根据任务类型实例化 ANN 或 CNN
            if (isNumericTask(m_info.taskType)) {
                // 如果是数值任务，实例化 ANN
                net = std::make_unique<ANN>(
                    state,
                    task,
                    toANNType(m_info.archType), // 转换架构类型
                    m_info.inputDim, // 输入维度
                    m_info.dataPath.toStdString(), // 数据路径（转 std::string）
                    m_info.id.toStdString() // 模型 ID
                );
            }
            else {
                // 如果是图像任务，实例化 CNN
                net = std::make_unique<CNN>(
                    state,
                    task,
                    toCNNType(m_info.archType), // 转换架构类型
                    m_info.dataPath.toStdString(), // 数据路径
                    m_info.id.toStdString() // 模型 ID
                );
            }

            // 绑定回调函数 (后端 -> 前端)
            // 当后端训练出一轮 Loss 时，lambda 被调用，发出 Qt 信号
            net->set_callbacks(
                // LossCallback: 每一轮 Epoch 结束时调用
                [this](int epoch, int total, double t_loss, double v_loss, const std::string& msg)
                {
                    emit lossUpdated(epoch, t_loss, v_loss); // 发送 Loss 更新信号，更新图表
                    emit statusChanged(QString::fromStdString(msg)); // 发送状态更新信号
                    // 进度条按 Epoch 计算
                    int progress = (int)((double)epoch / total * 100); // 计算百分比
                    emit progressUpdated(progress); // 发送进度更新信号
                },
                nullptr,  // 训练模式下的 ResultCallback 用不上，设为 nullptr
                nullptr   // 训练模式下的 ProgressCallback 用不上，设为 nullptr
            );

            emit statusChanged("开始读取数据并训练..."); // 状态：开始训练
            net->conduct(); // 执行神经网络的主流程（数据读取 -> 训练循环）

            emit statusChanged("训练完成！模型已保存。"); // 状态：完成
            emit trainingFinished(); // 发送训练完成信号

        }
        catch (const std::exception& e) {
            emit statusChanged(QString("训练出错: ") + e.what()); // 捕获标准异常并发送错误信息
            emit trainingFinished(); // 即使出错也发送完成信号以重置 UI
        }
        catch (...) {
            emit statusChanged("发生未知错误，训练终止。"); // 捕获未知异常
            emit trainingFinished();
        }
    }

signals:
    void progressUpdated(int value); // 信号：进度条更新
    void lossUpdated(int step, double trainLoss, double valLoss); // 信号：Loss 图表更新
    void statusChanged(QString status); // 信号：文本状态更新
    void trainingFinished(); // 信号：训练过程结束
};

// 推理工作线程类，用于在后台执行预测/测试任务
class InferenceWorker : public QThread {
    Q_OBJECT // Qt 宏
public:
    ModelInfo m_info; // 模型信息
    bool isTestMode;   // true=测试模式(有标签，计算准确率), false=使用模式(无标签，仅预测)
    QString inputPath; // 用户临时导入的数据路径

    // 构造函数
    InferenceWorker(ModelInfo info, bool testMode, QString path)
        : m_info(info), isTestMode(testMode), inputPath(path) {
    }

    // 线程主执行函数
    void run() override {
        emit statusChanged("正在加载模型参数..."); // 状态：加载中

        try {
            State state = isTestMode ? State::Test : State::Infer; // 确定状态是测试还是推理
            Task task = toBackendTask(m_info.taskType); // 获取任务类型
            std::unique_ptr<NeuralNetwork> net; // 定义神经网络指针

            // 1. 实例化 (注意这里 dataPath 使用的是用户在 UseDialog 选的新路径)
            if (isNumericTask(m_info.taskType)) {
                // 实例化 ANN
                net = std::make_unique<ANN>(
                    state, task,
                    toANNType(m_info.archType),
                    m_info.inputDim,
                    inputPath.toStdString(), // 使用用户输入的新路径
                    m_info.id.toStdString() // 读取训练好的权重（基于 ID）
                );
            }
            else {
                // 实例化 CNN
                net = std::make_unique<CNN>(
                    state, task,
                    toCNNType(m_info.archType),
                    inputPath.toStdString(), // 使用用户输入的新路径
                    m_info.id.toStdString()
                );
            }

            // 2. 绑定结果回调
            // 当后端 infer/test 输出每一行结果时，触发此回调
            net->set_callbacks(
                [this](int, int, double, double, const std::string& msg) {
                    // Log Callback (用于加载数据进度的显示，或者通用日志)
                    emit statusChanged(QString::fromStdString(msg));
                },
                [this](const std::string& res, bool isError) {
                    // Result Callback (用于显示每一条预测结果)
                    emit resultReady(QString::fromStdString(res), !isError); // 发送单条结果
                },
                // 进度回调 (例如处理了多少张图片)
                [this](int progress) {
                    emit progressUpdated(progress); // 发送进度
                }
            );

            // 3. 执行
            emit statusChanged("正在执行推理计算..."); // 状态：计算中
            net->conduct(); // 执行推理流程

            // 如果是测试模式且是回归任务，提取数据发送给前端进行绘图分析
            if (isTestMode && m_info.taskType == TaskType::NumericRegression) {
                QVector<double> q_preds, q_targets; // 定义 Qt 向量
                for (float v : net->last_test_preds) q_preds.append(v); // 拷贝预测值
                for (float v : net->last_test_targets) q_targets.append(v); // 拷贝真实值
                emit regressionDataReady(q_preds, q_targets); // 发送数据给回归分析弹窗
            }
            // 如果是测试模式且是分类任务
            else if (isTestMode && m_info.taskType != TaskType::NumericRegression) {
                QVector<int> q_preds, q_targets;

                // 从 net 中获取数据
                for (auto v : net->last_test_preds_indices) q_preds.append(static_cast<int>(v));
                for (auto v : net->last_test_targets_indices) q_targets.append(static_cast<int>(v));

                // 获取 label_map
                QMap<QString, int> q_labelMap;
                for (const auto& kv : net->get_label_map()) {
                    q_labelMap.insert(QString::fromStdString(kv.first), static_cast<int>(kv.second));
                }

                emit classificationDataReady(q_preds, q_targets, q_labelMap); // 发送数据给分类分析弹窗
            }
        }
        catch (const std::exception& e) {
            emit resultReady(QString("Critical Error: ") + e.what(), false); // 捕获异常并作为错误结果发送
        }

        emit inferenceFinished(); // 发送结束信号
    }

signals:
    void progressUpdated(int value); // 信号：进度更新
    void statusChanged(QString status); // 信号：状态文字更新
    void resultReady(QString result, bool isCorrect); // 信号：单条结果就绪
    void inferenceFinished(); // 信号：推理结束
    void regressionDataReady(QVector<double> preds, QVector<double> targets); // 信号：回归分析数据就绪
    void classificationDataReady(QVector<int> preds, QVector<int> targets, QMap<QString, int> labelMap);
};