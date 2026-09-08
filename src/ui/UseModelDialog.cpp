#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

// ==================== UseModelDialog Implementation ====================
// 使用/测试模型对话框实现

// 构造函数
UseModelDialog::UseModelDialog(ModelInfo info, bool isTestMode, QWidget* parent) : QDialog(parent), m_info(info) {
    setWindowTitle(isTestMode ? "模型测试 - " + info.name : "模型使用 - " + info.name); // 根据模式设置标题
    resize(700, 500); // 设置大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 主布局
    QHBoxLayout* impLayout = new QHBoxLayout(); // 导入栏布局
    pathEdit = new QLineEdit(this); // 路径输入框
    QPushButton* browseBtn = new QPushButton("导入数据...", this); // 导入按钮
    impLayout->addWidget(pathEdit); impLayout->addWidget(browseBtn);
    layout->addLayout(impLayout);
    progressBar = new QProgressBar(this); // 进度条
    QTextEdit* resultArea = new QTextEdit(this); // 结果显示区域
    resultArea->setReadOnly(true); // 只读
    layout->addWidget(progressBar); layout->addWidget(resultArea);
    QPushButton* actionBtn = new QPushButton(isTestMode ? "开始测试" : "开始推理", this); // 动作按钮
    actionBtn->setEnabled(false); // 初始禁用
    layout->addWidget(actionBtn);

    // 浏览按钮逻辑
    connect(browseBtn, &QPushButton::clicked, [=]() {
        QString path;
        // 根据任务类型选择文件
        if (isNumericTask(m_info.taskType)) path = QFileDialog::getOpenFileName(this, "选择数据文件", "", "Text Files (*.txt)");
        else path = QFileDialog::getExistingDirectory(this, "选择数据文件夹");
        if (!path.isEmpty()) { pathEdit->setText(path); actionBtn->setEnabled(true); } // 路径有效则启用按钮
        });

    // 动作按钮逻辑
    connect(actionBtn, &QPushButton::clicked, [=]() {
        if (pathEdit->text().isEmpty()) return;
        resultArea->clear();
        resultArea->append("正在初始化引擎...");

        // 创建推理线程
        worker = new InferenceWorker(m_info, isTestMode, pathEdit->text(), this);

        // 绑定信号
        connect(worker, &InferenceWorker::progressUpdated, progressBar, &QProgressBar::setValue);
        connect(worker, &InferenceWorker::statusChanged, this, [=](QString s) { resultArea->append(s); });

        // 绑定结果输出信号
        connect(worker, &InferenceWorker::resultReady, this, [=](QString text, bool isCorrect) {
            resultArea->setTextColor(isCorrect ? Qt::black : Qt::red); // 错误用红色显示
            resultArea->append(text + (isCorrect ? "" : " [错误]"));
            });

        // 绑定完成信号
        connect(worker, &InferenceWorker::inferenceFinished, this, [=](bool success, bool cancelled) {
            if (success) {
                progressBar->setValue(100);
                QMessageBox::information(this, "完成", "处理完成！");
            } else if (!cancelled) {
                QMessageBox::critical(this, "处理失败", "处理未完成，请检查上方错误信息。");
            }

            actionBtn->setText("返回");  // 改变按钮行为为“返回”
            actionBtn->setEnabled(true);

            // 将改变后的按钮断开原有连接，连接到关闭窗口
            actionBtn->disconnect();
            connect(actionBtn, &QPushButton::clicked, this, &QDialog::accept);
            });

        // 绑定回归数据信号（仅回归测试模式有效）
        connect(worker, &InferenceWorker::regressionDataReady, this,
            [=](QVector<double> preds, QVector<double> targets) {
                RegressionAnalysisDialog dlg(preds, targets, this); // 创建分析对话框
                dlg.exec(); // 模态弹窗
            });

        // 绑定分类数据信号（仅分类测试模式有效）
        connect(worker, &InferenceWorker::classificationDataReady, this,
            [=](QVector<int> preds, QVector<int> targets, QMap<QString, int> labelMap) {
                ClassificationAnalysisDialog dlg(preds, targets, labelMap, this); // 创建分类分析对话框
                dlg.exec(); // 模态弹窗
            });

        actionBtn->setEnabled(false); // 禁用按钮防止重复点击
        worker->start(); // 启动线程
        });
}

UseModelDialog::~UseModelDialog() {
    if (worker && worker->isRunning()) {
        worker->requestInterruption();
        worker->wait();
    }
}
