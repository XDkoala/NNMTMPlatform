#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

// ==================== TrainingWindow Implementation ====================
// 训练监控窗口实现

// 构造函数
TrainingWindow::TrainingWindow(ModelInfo info, QWidget* parent)
    : QDialog(parent), m_info(info)
{
    setWindowTitle("模型训练监控平台 - " + info.name); // 设置标题
    resize(1000, 700); // 设置大小

    // 1. 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 2. 顶部状态和进度条
    statusLabel = new QLabel("准备就绪", this); // 状态标签
    statusLabel->setStyleSheet("font-weight: bold; color: #333; margin-bottom: 5px;");

    progressBar = new QProgressBar(this); // 进度条
    progressBar->setRange(0, 100); // 范围 0-100
    progressBar->setTextVisible(true); // 显示百分比文字

    // 添加到主布局
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(progressBar);

    // 3. 中间内容区域 (左侧图表 + 右侧表格)
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // === 左侧：图表 ===
    chart = new PlotWidget("实时 Loss 曲线", "Epoch (轮数)", "Loss (损失值)", this);
    seriesVal = chart->addSeries("Validation Loss", QColor("#2563eb"));
    seriesTrain = chart->addSeries("Train Loss", QColor("#16a34a"));
    contentLayout->addWidget(chart, 2);

    // === 右侧：历史数据表格 ===
    QFrame* rightFrame = new QFrame(); // 右侧容器
    QVBoxLayout* rightLayout = new QVBoxLayout(rightFrame); // 右侧垂直布局
    rightLayout->setContentsMargins(0, 0, 0, 0); // 减少边距
    rightLayout->addWidget(new QLabel("训练历史记录:"));

    historyTable = new QTableWidget(this); // 创建表格控件
    historyTable->setColumnCount(3); // 3 列
    historyTable->setHorizontalHeaderLabels({ "Epoch", "Train Loss", "Val Loss" }); // 表头
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽自适应
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    rightLayout->addWidget(historyTable);

    // 将右侧Frame加入内容布局 (比例 1)
    contentLayout->addWidget(rightFrame, 1);

    // 将内容布局加入主布局
    mainLayout->addLayout(contentLayout, 1);

    // 4. 底部按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* cancelBtn = new QPushButton("取消训练并删除模型", this); // 取消按钮
    saveBtn = new QPushButton("保存模型", this); // 保存按钮
    saveBtn->setEnabled(false); // 初始禁用，训练完成后启用
    saveBtn->setDefault(true); // 设为默认按钮
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);

    // 加入主布局
    mainLayout->addLayout(btnLayout);

    // 5. Worker 连接
    worker = new TrainingWorker(m_info, this); // 创建训练线程

    // 连接信号槽：更新进度条、状态、图表
    connect(worker, &TrainingWorker::progressUpdated, progressBar, &QProgressBar::setValue);
    connect(worker, &TrainingWorker::statusChanged, statusLabel, &QLabel::setText);
    connect(worker, &TrainingWorker::lossUpdated, this, &TrainingWindow::updateChart);

    // 连接训练完成信号
    connect(worker, &TrainingWorker::trainingFinished, this,
        [=](bool success, bool cancelled) {
            if (success) {
                this->onFinished();
            } else if (cancelled) {
                try {
                    DatabaseManager::instance().deleteModel(m_info.id);
                } catch (const std::exception& error) {
                    QMessageBox::warning(this, "清理失败", QString::fromUtf8(error.what()));
                }
                reject();
            } else {
                QMessageBox::critical(this, "训练失败", statusLabel->text() + "\n请检查数据格式是否符合要求。");
                saveBtn->setEnabled(false);
            }
        }
    );

    // 取消按钮逻辑
    connect(cancelBtn, &QPushButton::clicked, [=]() {
        if (QMessageBox::question(this, "确认", "确定要取消训练并删除模型吗？") == QMessageBox::Yes) {
            worker->requestInterruption();
            cancelBtn->setEnabled(false);
            saveBtn->setEnabled(false);
            statusLabel->setText("正在安全取消训练...");
        }
        });

    // 保存按钮逻辑
    connect(saveBtn, &QPushButton::clicked, [=]() {
        QMessageBox::information(this, "成功", "模型保存成功！");
        accept(); // 关闭窗口返回 Accept
        });

    worker->start(); // 启动线程
}

TrainingWindow::~TrainingWindow() {
    if (worker && worker->isRunning()) {
        worker->requestInterruption();
        worker->wait();
    }
}

// 更新图表和表格的槽函数
void TrainingWindow::updateChart(int step, double trainLoss, double valLoss) {
    chart->append(seriesVal, step, valLoss); // 添加验证 Loss 点
    chart->append(seriesTrain, step, trainLoss); // 添加训练 Loss 点

    // 更新表格
    int row = historyTable->rowCount();
    historyTable->insertRow(row); // 插入新行
    historyTable->setItem(row, 0, new QTableWidgetItem(QString::number(step))); // 设置 Epoch
    historyTable->setItem(row, 1, new QTableWidgetItem(QString::number(trainLoss, 'f', 5))); // 设置 Train Loss
    historyTable->setItem(row, 2, new QTableWidgetItem(QString::number(valLoss, 'f', 5))); // 设置 Val Loss
    historyTable->scrollToBottom(); // 滚动到底部

}
// 训练结束处理
void TrainingWindow::onFinished() {
    progressBar->setValue(100); // 进度条填满
    saveBtn->setEnabled(true); // 启用保存按钮
}
