#pragma once // 防止头文件被重复包含
#include "ModelCore.h" // 包含后端核心头文件
#include <QMainWindow> // Qt 主窗口类
#include <QDialog> // Qt 对话框类
#include <QWidget> // Qt 部件基类
#include <QFrame> // Qt 框架类
#include <QLabel> // Qt 标签类
#include <QPushButton> // Qt 按钮类
#include <QVBoxLayout> // Qt 垂直布局类
#include <QHBoxLayout> // Qt 水平布局类
#include <QGridLayout> // Qt 网格布局类
#include <QScrollArea> // Qt 滚动区域类
#include <QLineEdit> // Qt 单行输入框类
#include <QComboBox> // Qt 下拉列表框类
#include <QTextEdit> // Qt 多行文本编辑框类
#include <QListWidget> // Qt 列表控件类
#include <QProgressBar> // Qt 进度条类
#include <QSpinBox> // Qt 整数输入框类
#include <QtCharts> // Qt 图表模块
#include <QChart> // Qt 图表类
#include <QChartView> // Qt 图表视图类
#include <QLineSeries> // Qt 折线序列类
#include <QValueAxis> // Qt 数值坐标轴类

// 声明所有类，方便相互引用（前置声明）
class ModelCard;
class ModifyModelDialog;
class RecycleBinDialog;
class DraftBoxDialog;
class TrainingWindow;
class AddModelDialog;
class UseModelDialog;
class MainWindow;
class RegressionAnalysisDialog;

// ==================== ModelCard (模型卡片) ====================
// 继承自 QFrame，用于显示单个模型的简要信息和操作按钮
class ModelCard : public QFrame {
    Q_OBJECT // 启用 Qt 信号槽机制
public:
    // 构造函数：传入模型信息和父部件
    ModelCard(const ModelInfo& info, QWidget* parent = nullptr);
    // 获取内部存储的模型信息
    ModelInfo getInfo() const { return m_info; }

signals:
    // 自定义信号，用于通知主窗口执行相应操作
    void requestUse(QString id); // 请求使用模型
    void requestTest(QString id); // 请求测试模型
    void requestRetrain(QString id); // 请求重训模型
    void requestModify(QString id); // 请求修改模型信息
    void requestDelete(QString id); // 请求删除模型

private:
    ModelInfo m_info; // 存储模型信息的副本
    QLabel* nameLabel; // 显示名称的标签
    QLabel* typeLabel; // 显示类型的标签
    QLabel* introLabel; // 显示简介的标签
};

// ==================== Dialogs (各种子窗口) ====================

// 修改模型信息的对话框
class ModifyModelDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数，传入模型信息的引用（以便直接修改）
    ModifyModelDialog(ModelInfo& info, QWidget* parent = nullptr);
private:
    ModelInfo& m_info; // 引用
    QLineEdit* nameEdit; // 名称编辑框
    QTextEdit* introEdit; // 简介编辑框
};

// 回收站管理对话框
class RecycleBinDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数，传入已删除列表和活动列表的指针
    RecycleBinDialog(QList<ModelInfo>* deletedModels, QList<ModelInfo>* activeModels, QWidget* parent = nullptr);
private:
    QList<ModelInfo>* m_deleted; // 指向回收站列表
    QList<ModelInfo>* m_active; // 指向活动模型列表
    QListWidget* listWidget; // 显示删除模型的列表控件
    void refreshList(); // 刷新列表显示的私有函数
    void onRestore(); // 恢复模型的槽函数
    void onDeleteForever(); // 永久删除的槽函数
    void onEmptyBin(); // 清空回收站的槽函数
};

// 草稿箱对话框
class DraftBoxDialog : public QDialog {
    Q_OBJECT
public:
    ModelInfo selectedDraft; // 存储用户选中的草稿数据
    bool hasSelection = false; // 标记用户是否选中了草稿并点击了继续
    // 构造函数
    DraftBoxDialog(QList<ModelInfo>* drafts, QWidget* parent = nullptr);
private:
    QList<ModelInfo>* m_drafts; // 指向草稿列表
    QListWidget* listWidget; // 显示草稿的列表控件
};

// 训练过程监控窗口
class TrainingWindow : public QDialog {
    Q_OBJECT
public:
    // 构造函数
    TrainingWindow(ModelInfo info, QWidget* parent = nullptr);
private slots:
    // 槽函数：更新图表和数据
    void updateChart(int step, double trainLoss, double valLoss);
    // 槽函数：处理训练结束
    void onFinished();
private:
    ModelInfo m_info; // 模型信息
    QProgressBar* progressBar; // 进度条
    QLabel* statusLabel; // 状态文字
    QLineSeries* seriesVal;  // 记录 Validation Loss 的数据序列
    QLineSeries* seriesTrain;  // 记录 Train Loss 的数据序列
    QChart* chart; // 图表对象
    QValueAxis* axisX; // 保存X轴指针，用于动态调整
    QValueAxis* axisY; // 保存Y轴指针，用于动态调整
    QTableWidget* historyTable;  // 历史记录表格控件
    double m_minLoss;  // 记录最小Loss，用于 Y 轴缩放
    double m_maxLoss;  // 记录最大Loss，用于 Y 轴缩放

    QPushButton* saveBtn; // 保存按钮
    TrainingWorker* worker; // 训练后台线程指针
};

// 添加/编辑模型对话框
class AddModelDialog : public QDialog {
    Q_OBJECT
public:
    ModelInfo resultInfo; // 存储最终生成的模型信息
    // 构造函数，initData 不为空时为编辑模式
    AddModelDialog(int currentCount, const ModelInfo* initData = nullptr, QWidget* parent = nullptr);
    // 事件过滤器重写，用于拦截特定控件事件
    bool eventFilter(QObject* obj, QEvent* event) override;  // 声明事件过滤器
    // 拒绝（取消/关闭）事件重写
    void reject() override;
private:
    QLineEdit* nameEdit, * pathEdit; // 名称和路径输入框
    QTextEdit* introEdit; // 简介输入框
    QComboBox* taskCombo, * archCombo; // 任务和架构下拉框
    QSpinBox* featureSpin; // 特征数量输入框
    QLabel* featureLabel; // 特征数量标签
    bool fillResultInfo(ModelStatus status); // 填充 resultInfo 的辅助函数
};

// 使用/测试模型对话框
class UseModelDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数，isTestMode 决定是测试模式还是使用模式
    UseModelDialog(ModelInfo info, bool isTestMode, QWidget* parent = nullptr);
private:
    ModelInfo m_info; // 模型信息
    QLineEdit* pathEdit; // 数据路径输入框
    QProgressBar* progressBar; // 进度条
};

// 回归分析交互框 (显示预测 vs 真实散点图)
class RegressionAnalysisDialog : public QDialog {
    Q_OBJECT
public:
    // 构造函数，传入预测值和真实值向量
    RegressionAnalysisDialog(const QVector<double>& preds, const QVector<double>& targets, QWidget* parent = nullptr);
private:
    QChart* chart; // 图表对象
    QChartView* chartView; // 图表视图
};

// 绘制混淆矩阵（测试模式下分类任务可视化）
class ClassificationAnalysisDialog : public QDialog {
    Q_OBJECT
public:
    ClassificationAnalysisDialog(
        const QVector<int>& preds,
        const QVector<int>& targets,
        const QMap<QString, int>& labelMap,
        QWidget* parent = nullptr
    );
private:
    void setupConfusionMatrix();
    void setupBarChart();

    QVector<int> m_preds;
    QVector<int> m_targets;
    QMap<QString, int> m_labelMap;  // 正向映射
    QMap<int, QString> m_indexToLabel; // 反向映射
    int m_numClasses;
};

// ==================== MainWindow (主窗口) ====================

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // 构造函数
    MainWindow(QWidget* parent = nullptr);
    // 析构函数
    ~MainWindow();

private:
    // UI组件
    QLineEdit* searchBar; // 搜索框
    QComboBox* filterCombo; // 筛选下拉框
    QWidget* modelContainer; // 模型卡片容器 Widget
    QGridLayout* modelLayout; // 模型卡片布局管理器

    // 数据
    QList<ModelInfo> allModels; // 所有正常模型列表
    QList<ModelInfo> recycleBin; // 回收站列表
    QList<ModelInfo> draftBox; // 草稿箱列表

    void setupUI(); // 初始化 UI 函数
    void refreshModelList(); // 刷新模型列表显示函数
    void addModelCard(const ModelInfo& info, int index); // (已废弃/保留接口) 添加单个卡片

    // 槽函数
    void onSearch(); // 搜索触发
    void onFilterChanged(); // 筛选触发
    void onAddNewModel(); // 添加新模型触发
    void onOpenRecycleBin(); // 打开回收站触发
    void onOpenDraftBox(); // 打开草稿箱触发

    // 业务逻辑槽函数
    void useModel(QString id); // 使用模型
    void testModel(QString id); // 测试模型
    void retrainModel(QString id); // 重训模型
    void modifyModel(QString id); // 修改模型
    void deleteModel(QString id); // 删除模型

    // 窗口大小改变事件重写，用于响应式布局
    void resizeEvent(QResizeEvent* event) override;
    // 重排卡片布局函数
    void reflowModelCards();
};
