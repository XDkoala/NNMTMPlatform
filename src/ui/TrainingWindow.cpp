#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

// ==================== TrainingWindow Implementation ====================
// 训练监控窗口实现

// 构造函数
TrainingWindow::TrainingWindow(ModelInfo info, QWidget* parent)
    : QDialog(parent), m_info(info), m_minLoss(std::numeric_limits<double>::max()), m_maxLoss(0.0)
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
    chart = new QChart(); // 创建 QtChart 对象
    chart->setTitle("实时 Loss 曲线"); // 设置标题
    chart->setTheme(QChart::ChartThemeLight); // 设置明亮主题
    chart->setAnimationOptions(QChart::NoAnimation); // 禁用动画以提高性能

    seriesVal = new QLineSeries(); // 验证集 Loss 曲线
    seriesVal->setName("Validation Loss");
    seriesVal->setColor(Qt::blue); // 蓝色
    // [修复] 移除 setUseOpenGL(true)，避免小数据量下的渲染错误
    // seriesVal->setUseOpenGL(true);

    seriesTrain = new QLineSeries(); // 训练集 Loss 曲线
    seriesTrain->setName("Train Loss");
    seriesTrain->setColor(Qt::green); // 绿色
    // [修复] 移除 setUseOpenGL(true)
    // seriesTrain->setUseOpenGL(true);

    chart->addSeries(seriesVal); // 添加曲线到图表
    chart->addSeries(seriesTrain);

    // 初始化坐标轴
    axisX = new QValueAxis; // X 轴 (Epoch)
    axisX->setTitleText("Epoch (轮数)");
    axisX->setLabelFormat("%d");
    axisX->setRange(0, 10); // 初始范围 0-10
    axisX->setTickCount(11); // 保证整数刻度

    axisY = new QValueAxis; // Y 轴 (Loss)
    axisY->setTitleText("Loss (损失值)");
    axisY->setLabelFormat("%.4f"); // 保留 4 位小数
    axisY->setRange(0, 1); // 初始范围 0-1

    chart->addAxis(axisX, Qt::AlignBottom); // 添加 X 轴到底部
    chart->addAxis(axisY, Qt::AlignLeft); // 添加 Y 轴到左侧
    seriesVal->attachAxis(axisX); // 绑定轴
    seriesVal->attachAxis(axisY);
    seriesTrain->attachAxis(axisX);
    seriesTrain->attachAxis(axisY);

    QChartView* chartView = new QChartView(chart); // 创建图表视图控件
    chartView->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿

    // 将图表加入内容布局 (比例 2)
    contentLayout->addWidget(chartView, 2);

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
    worker = new TrainingWorker(m_info); // 创建训练线程

    // 连接信号槽：更新进度条、状态、图表
    connect(worker, &TrainingWorker::progressUpdated, progressBar, &QProgressBar::setValue);
    connect(worker, &TrainingWorker::statusChanged, statusLabel, &QLabel::setText);
    connect(worker, &TrainingWorker::lossUpdated, this, &TrainingWindow::updateChart);

    // 连接训练完成信号
    connect(worker, &TrainingWorker::trainingFinished, this,
        [=]() {
            this->onFinished(); // 处理完成逻辑
            // 如果状态包含错误信息，弹出警告
            if (statusLabel->text().contains("出错") || statusLabel->text().contains("Error")) {
                QMessageBox::critical(this, "训练失败", statusLabel->text() + "\n请检查数据格式是否符合要求。");
                saveBtn->setEnabled(false); // 保持禁用
            }
        }
    );

    // 取消按钮逻辑
    connect(cancelBtn, &QPushButton::clicked, [=]() {
        if (QMessageBox::question(this, "确认", "确定要取消训练并删除模型吗？") == QMessageBox::Yes) {
            worker->terminate(); // 终止线程
            reject(); // 关闭窗口返回 Reject
        }
        });

    // 保存按钮逻辑
    connect(saveBtn, &QPushButton::clicked, [=]() {
        QMessageBox::information(this, "成功", "模型保存成功！");
        accept(); // 关闭窗口返回 Accept
        });

    worker->start(); // 启动线程
}

// 更新图表和表格的槽函数
void TrainingWindow::updateChart(int step, double trainLoss, double valLoss) {
    seriesVal->append(step, valLoss); // 添加验证 Loss 点
    seriesTrain->append(step, trainLoss); // 添加训练 Loss 点

    // 更新表格
    int row = historyTable->rowCount();
    historyTable->insertRow(row); // 插入新行
    historyTable->setItem(row, 0, new QTableWidgetItem(QString::number(step))); // 设置 Epoch
    historyTable->setItem(row, 1, new QTableWidgetItem(QString::number(trainLoss, 'f', 5))); // 设置 Train Loss
    historyTable->setItem(row, 2, new QTableWidgetItem(QString::number(valLoss, 'f', 5))); // 设置 Val Loss
    historyTable->scrollToBottom(); // 滚动到底部

    // === 动态调整坐标轴 ===

    // 1. 调整 X 轴
    // 如果当前步数超过了轴的最大值，扩展轴
    if (step > axisX->max()) {
        axisX->setMax(step < 10 ? 10 : step + 1);
    }

    // 2. 调整 Y 轴
    // 更新全局最大最小值
    double localMax = std::max(trainLoss, valLoss);
    double localMin = std::min(trainLoss, valLoss);

    bool rangeChanged = false;
    // 首次运行时初始化
    if (step == 1) {
        m_minLoss = localMin;
        m_maxLoss = localMax;
        rangeChanged = true;
    }
    else {
        // 更新极值
        if (localMin < m_minLoss) { m_minLoss = localMin; rangeChanged = true; }
        if (localMax > m_maxLoss) { m_maxLoss = localMax; rangeChanged = true; }
    }

    // 只有当范围确实发生变化，或者刚开始时，才刷新 Y 轴
    if (rangeChanged || step == 1) {
        double currentMin = m_minLoss;
        double currentMax = m_maxLoss;

        // 防止最大等于最小导致 crash 或显示异常
        if (std::abs(currentMax - currentMin) < 1e-6) {
            currentMax += 0.1;
            currentMin = std::max(0.0, currentMin - 0.1);
        }

        // 添加 10% 的上下缓冲空间，让曲线不贴边
        double margin = (currentMax - currentMin) * 0.1;
        if (margin == 0) margin = 0.1; // 再次保底

        axisY->setRange(std::max(0.0, currentMin - margin), currentMax + margin); // 设置新范围
    }
}
// 训练结束处理
void TrainingWindow::onFinished() {
    statusLabel->setText("训练结束");
    progressBar->setValue(100); // 进度条填满
    saveBtn->setEnabled(true); // 启用保存按钮
}
