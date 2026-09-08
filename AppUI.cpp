#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

// ==================== ModelCard Implementation ====================
// 模型卡片类的实现，用于在主页显示单个模型的概览

// 构造函数：初始化卡片 UI
ModelCard::ModelCard(const ModelInfo& info, QWidget* parent) : QFrame(parent), m_info(info) {
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised); // 设置边框样式为凸起面板
    // 设置样式表：背景色、圆角、按钮样式（蓝色正常，红色删除）
    this->setStyleSheet("ModelCard { background-color: #f9f9f9; border: 1px solid #ddd; border-radius: 8px; } "
        "QPushButton { background-color: #0078d7; color: white; border-radius: 4px; padding: 4px; border: none; }"
        "QPushButton:hover { background-color: #0063b1; }"
        "QPushButton#delBtn { background-color: #dc3545; }"
        "QPushButton#delBtn:hover { background-color: #c82333; }");
    this->setFixedSize(280, 220); // 设置卡片固定大小

    QVBoxLayout* mainLayout = new QVBoxLayout(this); // 创建垂直布局管理器

    nameLabel = new QLabel(info.name, this); // 创建显示模型名称的标签
    nameLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #333;"); // 设置字体样式

    nameLabel->setWordWrap(true); // 允许名称太长时换行
    nameLabel->setToolTip(info.name); // 鼠标悬停显示完整名称

    // 创建显示类型的标签，格式为 [任务类型] - 模型架构
    typeLabel = new QLabel(QString("[%1] - %2").arg(taskTypeToString(info.taskType)).arg(archTypeToString(info.archType)), this);
    typeLabel->setStyleSheet("color: #666; font-size: 12px;"); // 设置字体颜色和大小

    introLabel = new QLabel(info.intro, this); // 创建显示简介的标签
    introLabel->setWordWrap(true); // 允许简介换行
    introLabel->setStyleSheet("color: #555; font-style: italic; font-size: 11px; background: transparent;"); // 设置斜体和透明背景
    introLabel->setAlignment(Qt::AlignTop); // 设置顶部对齐

    QScrollArea* scrollArea = new QScrollArea(this); // 创建滚动区域，用于放置简介（防止简介过长）
    scrollArea->setWidget(introLabel); // 将简介标签放入滚动区域
    scrollArea->setWidgetResizable(true); // 允许内部组件调整大小
    scrollArea->setFrameShape(QFrame::NoFrame); // 去除滚动区域的边框
    scrollArea->setStyleSheet("QScrollArea { background: transparent; } QScrollBar:vertical { width: 4px; }"); // 设置滚动条样式

    mainLayout->addWidget(nameLabel); // 添加名称标签到布局
    mainLayout->addWidget(typeLabel); // 添加类型标签到布局
    mainLayout->addWidget(scrollArea, 1); // 添加滚动区域到布局，并设置拉伸因子为 1

    QHBoxLayout* btnLayout = new QHBoxLayout(); // 创建水平布局，用于放置按钮
    btnLayout->setSpacing(4); // 设置按钮间距
    btnLayout->setContentsMargins(0, 5, 0, 0); // 设置边距

    QPushButton* useBtn = new QPushButton("使用", this); // 创建“使用”按钮
    QPushButton* testBtn = new QPushButton("测试", this); // 创建“测试”按钮
    QPushButton* retrainBtn = new QPushButton("重训", this); // 创建“重训”按钮
    QPushButton* modifyBtn = new QPushButton("修改", this); // 创建“修改”按钮
    QPushButton* delBtn = new QPushButton("删除", this); // 创建“删除”按钮
    delBtn->setObjectName("delBtn"); // 设置对象名，以便应用特定的 CSS 样式

    btnLayout->addWidget(useBtn); // 添加按钮到布局
    btnLayout->addWidget(testBtn);
    btnLayout->addWidget(retrainBtn);
    btnLayout->addWidget(modifyBtn);
    btnLayout->addWidget(delBtn);

    mainLayout->addLayout(btnLayout); // 将按钮布局添加到主布局底部

    // 连接各个按钮的点击信号到 ModelCard 的自定义信号，传递模型 ID
    connect(useBtn, &QPushButton::clicked, [=]() { emit requestUse(m_info.id); });
    connect(testBtn, &QPushButton::clicked, [=]() { emit requestTest(m_info.id); });
    connect(retrainBtn, &QPushButton::clicked, [=]() { emit requestRetrain(m_info.id); });
    connect(modifyBtn, &QPushButton::clicked, [=]() { emit requestModify(m_info.id); });
    connect(delBtn, &QPushButton::clicked, [=]() { emit requestDelete(m_info.id); });
}

// ==================== ModifyModelDialog Implementation ====================
// 修改模型信息对话框的实现

// 构造函数
ModifyModelDialog::ModifyModelDialog(ModelInfo& info, QWidget* parent) : QDialog(parent), m_info(info) {
    setWindowTitle("修改模型信息"); // 设置窗口标题
    resize(400, 300); // 设置窗口大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 创建垂直布局

    layout->addWidget(new QLabel("模型名称（最多30字）:")); // 添加提示标签
    nameEdit = new QLineEdit(info.name, this); // 创建输入框并预填当前名称
    nameEdit->setMaxLength(30); // 限制名称长度
    layout->addWidget(nameEdit); // 添加输入框到布局

    layout->addWidget(new QLabel("模型简介（最多300字）:")); // 添加提示标签
    introEdit = new QTextEdit(this); // 创建文本编辑框
    introEdit->setText(info.intro); // 预填当前简介
    // 限制简介长度逻辑
    connect(introEdit, &QTextEdit::textChanged, [=]() {
        QString text = introEdit->toPlainText(); // 获取文本
        if (text.length() > 300) { // 如果超过 300 字
            introEdit->setText(text.left(300)); // 截取前 300 字
            QTextCursor cursor = introEdit->textCursor();
            cursor.movePosition(QTextCursor::End); // 将光标移到末尾
            introEdit->setTextCursor(cursor);
            QToolTip::showText(introEdit->mapToGlobal(QPoint(0, 0)), "字数已达上限 (300字)"); // 显示提示
        }
        });
    layout->addWidget(introEdit); // 添加文本框到布局

    QHBoxLayout* btnLayout = new QHBoxLayout(); // 创建按钮布局
    QPushButton* saveBtn = new QPushButton("保存修改", this); // 保存按钮
    QPushButton* cancelBtn = new QPushButton("取消", this); // 取消按钮
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout); // 添加按钮布局
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject); // 点击取消则关闭对话框
    connect(saveBtn, &QPushButton::clicked, [=]() {
        m_info.name = nameEdit->text(); // 更新名称到引用对象
        m_info.intro = introEdit->toPlainText(); // 更新简介到引用对象
        accept(); // 关闭对话框并返回 Accepted
        });
}

// ==================== RecycleBinDialog Implementation ====================
// 回收站对话框的实现

// 构造函数
RecycleBinDialog::RecycleBinDialog(QList<ModelInfo>* deletedModels, QList<ModelInfo>* activeModels, QWidget* parent)
    : QDialog(parent), m_deleted(deletedModels), m_active(activeModels)
{
    setWindowTitle("回收站管理"); // 设置标题
    resize(600, 400); // 设置大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 垂直布局
    listWidget = new QListWidget(this); // 创建列表控件
    refreshList(); // 填充列表数据

    layout->addWidget(listWidget); // 添加列表控件
    QHBoxLayout* btnLayout = new QHBoxLayout(); // 按钮布局
    QPushButton* restoreBtn = new QPushButton("恢复选中模型", this); // 恢复按钮
    QPushButton* deleteBtn = new QPushButton("永久删除选中模型", this); // 修改文案，永久删除按钮
    QPushButton* emptyBtn = new QPushButton("清空回收站", this);   // 新增按钮：清空

    deleteBtn->setStyleSheet("color: red;"); // 永久删除按钮设为红色文字
    emptyBtn->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold;"); // 清空按钮设为红色背景

    btnLayout->addWidget(restoreBtn);
    btnLayout->addWidget(deleteBtn);
    btnLayout->addWidget(emptyBtn);

    layout->addLayout(btnLayout); // 添加按钮布局
    connect(restoreBtn, &QPushButton::clicked, this, &RecycleBinDialog::onRestore); // 绑定恢复槽
    connect(deleteBtn, &QPushButton::clicked, this, &RecycleBinDialog::onDeleteForever); // 绑定永久删除槽
    connect(emptyBtn, &QPushButton::clicked, this, &RecycleBinDialog::onEmptyBin); // 绑定清空槽
}

// 刷新列表显示
void RecycleBinDialog::refreshList() {
    listWidget->clear(); // 清空现有列表
    for (int i = 0; i < m_deleted->size(); ++i) { // 遍历已删除模型列表
        const auto& m = m_deleted->at(i);
        // 构造显示文本：名称 + 类型 + 删除时间
        QString label = QString("%1 (%2) - 删除于: %3")
            .arg(m.name)
            .arg(taskTypeToString(m.taskType))
            .arg(QDateTime::currentDateTime().toString("MM-dd hh:mm"));
        listWidget->addItem(label); // 添加到列表
    }
}

// 恢复模型逻辑
void RecycleBinDialog::onRestore() {
    // 检查回收站是否为空
    if (m_deleted->isEmpty()) {
        QMessageBox::information(this, "提示", "回收站目前没有模型。");
        return;
    }

    int row = listWidget->currentRow(); // 获取当前选中的行号
    // 检查是否选中
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个要恢复的模型。");
        return;
    }

    ModelInfo m = m_deleted->takeAt(row); // 从删除列表中移除该模型
    m.status = ModelStatus::Trained; // 状态改为已训练（激活）
    m_active->append(m); // 添加到活动模型列表
    refreshList(); // 刷新 UI
    QMessageBox::information(this, "成功", "模型已恢复到主页"); // 提示成功
}

// 永久删除逻辑
void RecycleBinDialog::onDeleteForever() {
    // 检查回收站是否为空
    if (m_deleted->isEmpty()) {
        QMessageBox::information(this, "提示", "回收站目前没有模型。");
        return;
    }

    int row = listWidget->currentRow(); // 获取选中行
    // 检查是否选中
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个要删除的模型。");
        return;
    }

    // 弹出确认警告框
    if (QMessageBox::warning(this, "警告", "确定要永久删除该模型吗？此操作不可逆！", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        ModelInfo m = m_deleted->at(row); // 获取模型信息
        DatabaseManager::instance().deleteModel(m.id); // 调用数据库管理器进行物理删除
        m_deleted->removeAt(row); // 从内存列表中移除
        refreshList(); // 刷新 UI
    }
}

// 清空回收站逻辑
void RecycleBinDialog::onEmptyBin() {
    // 检查回收站是否为空
    if (m_deleted->isEmpty()) {
        QMessageBox::information(this, "提示", "回收站已经是空的了。");
        return;
    }

    // 弹出严重警告框
    if (QMessageBox::critical(this, "危险", "确定要清空回收站吗？\n所有模型文件将被永久删除！", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        // 遍历列表，逐个进行物理删除
        for (const auto& m : *m_deleted) {
            DatabaseManager::instance().deleteModel(m.id);
        }
        m_deleted->clear(); // 清空内存列表
        refreshList(); // 刷新 UI
    }
}

// ==================== DraftBoxDialog Implementation ====================
// 草稿箱对话框实现

// 构造函数
DraftBoxDialog::DraftBoxDialog(QList<ModelInfo>* drafts, QWidget* parent)
    : QDialog(parent), m_drafts(drafts) {
    setWindowTitle("草稿箱"); // 设置标题
    resize(500, 400); // 设置大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 垂直布局
    listWidget = new QListWidget(this); // 列表控件
    // 填充列表：名称 - 任务类型
    for (const auto& m : *m_drafts) listWidget->addItem(m.name + " - " + taskTypeToString(m.taskType));
    layout->addWidget(listWidget);
    QHBoxLayout* btnLayout = new QHBoxLayout(); // 按钮布局
    QPushButton* continueBtn = new QPushButton("继续编辑/训练", this); // 继续按钮
    QPushButton* delBtn = new QPushButton("丢弃草稿", this); // 丢弃按钮
    btnLayout->addWidget(continueBtn);
    btnLayout->addWidget(delBtn);
    layout->addLayout(btnLayout);
    // 连接继续按钮逻辑
    connect(continueBtn, &QPushButton::clicked, [=]() {
        int row = listWidget->currentRow(); // 获取选中行
        if (row < 0) return;
        selectedDraft = m_drafts->takeAt(row); // 取出草稿数据
        hasSelection = true; // 标记已选择
        accept(); // 关闭对话框
        });
    // 连接丢弃按钮逻辑
    connect(delBtn, &QPushButton::clicked, [=]() {
        int row = listWidget->currentRow();
        if (row < 0) return;

        // 确认丢弃
        if (QMessageBox::question(this, "确认", "确定丢弃该草稿？\n此操作不可恢复。") == QMessageBox::Yes) {
            // 物理删除文件
            ModelInfo infoToDelete = m_drafts->at(row);
            DatabaseManager::instance().deleteModel(infoToDelete.id);

            // 移除内存和UI
            m_drafts->removeAt(row);
            delete listWidget->takeItem(row);
        }
        });
}

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

// ==================== AddModelDialog Implementation ====================
// 添加/编辑模型对话框实现

// 构造函数
AddModelDialog::AddModelDialog(int currentCount, const ModelInfo* initData, QWidget* parent) : QDialog(parent) {
    setWindowTitle(initData ? "编辑模型 / 继续训练" : "添加新模型"); // 根据是否有初始数据设置标题
    resize(500, 650); // 设置大小

    QVBoxLayout* layout = new QVBoxLayout(this); // 主布局

    layout->addWidget(new QLabel("模型名称（最多30字）:"));
    nameEdit = new QLineEdit(this); // 名称输入框
    nameEdit->setMaxLength(30); // 限制名称长度
    // 如果是编辑模式，填入原名，否则生成默认名
    nameEdit->setText(initData ? initData->name : QString("新模型%1").arg(currentCount + 1));
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel("模型简介（最多300字）:"));
    introEdit = new QTextEdit(this); // 简介输入框
    introEdit->setPlaceholderText("暂无介绍");
    introEdit->setText(initData ? initData->intro : ""); // 填入简介
    introEdit->setMaximumHeight(80); // 限制高度
    // 限制简介长度逻辑
    connect(introEdit, &QTextEdit::textChanged, [=]() {
        QString text = introEdit->toPlainText();
        if (text.length() > 300) {
            introEdit->setText(text.left(300));
            QTextCursor cursor = introEdit->textCursor();
            cursor.movePosition(QTextCursor::End);
            introEdit->setTextCursor(cursor);
            QToolTip::showText(introEdit->mapToGlobal(QPoint(0, 0)), "字数已达上限 (300字)");
        }
        });
    layout->addWidget(introEdit);

    layout->addWidget(new QLabel("任务类型 (必选):"));
    taskCombo = new QComboBox(this); // 任务类型下拉框
    taskCombo->addItem("请选择...", -1); // 默认项
    // 添加各个任务选项，并绑定枚举值
    taskCombo->addItem("数值回归", (int)TaskType::NumericRegression);
    taskCombo->addItem("数值二分类", (int)TaskType::NumericBinaryClass);
    taskCombo->addItem("数值多分类", (int)TaskType::NumericMultiClass);
    taskCombo->addItem("图像二分类", (int)TaskType::ImageBinaryClass);
    taskCombo->addItem("图像多分类", (int)TaskType::ImageMultiClass);
    if (initData) taskCombo->setCurrentIndex(taskCombo->findData((int)initData->taskType)); // 如果是编辑，选中原有类型
    layout->addWidget(taskCombo);

    // 特征数量输入控件（仅数值任务显示）
    featureLabel = new QLabel("输入特征数量 (Input Features):");
    featureSpin = new QSpinBox(this);
    featureSpin->setRange(1, 10000); // 范围 1-10000
    featureSpin->setValue(initData && initData->inputDim > 0 ? initData->inputDim : 1); // 设置初始值
    featureLabel->setVisible(false); // 默认隐藏
    featureSpin->setVisible(false); // 默认隐藏
    layout->addWidget(featureLabel);
    layout->addWidget(featureSpin);

    layout->addWidget(new QLabel("模型类型 (必选):"));
    archCombo = new QComboBox(this); // 架构类型下拉框
    archCombo->installEventFilter(this); // 安装事件过滤器（用于未选任务时拦截点击）
    layout->addWidget(archCombo);

    // 任务类型改变时的逻辑
    connect(taskCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        archCombo->clear(); // 清空架构选项
        if (index <= 0) { // 如果未选择
            featureLabel->setVisible(false);
            featureSpin->setVisible(false);
            return;
        }
        TaskType t = (TaskType)taskCombo->currentData().toInt(); // 获取当前选中的任务
        if (isNumericTask(t)) { // 如果是数值任务
            featureLabel->setVisible(true); // 显示特征输入框
            featureSpin->setVisible(true);
            // 添加 MLP 相关架构
            archCombo->addItem("SimpleMLP", (int)ArchType::SimpleMLP);
            archCombo->addItem("GeneralMLP", (int)ArchType::GeneralMLP);
            archCombo->addItem("DropoutMLP", (int)ArchType::DropoutMLP);
            archCombo->addItem("L2RegularizedMLP", (int)ArchType::L2RegularizedMLP);
            archCombo->addItem("DeepMLP", (int)ArchType::DeepMLP);
            archCombo->addItem("SuperDeepMLP", (int)ArchType::SuperDeepMLP);
        }
        else { // 如果是图像任务
            featureLabel->setVisible(false); // 隐藏特征输入框
            featureSpin->setVisible(false);
            // 添加 CNN 相关架构
            archCombo->addItem("LeNet", (int)ArchType::LeNet);
            archCombo->addItem("AlexNet", (int)ArchType::AlexNet);
            archCombo->addItem("VGGNet", (int)ArchType::VGGNet);
            archCombo->addItem("ResNet", (int)ArchType::ResNet);
        }
        });

    // 如果有初始数据，触发一次类型改变逻辑以初始化界面
    if (initData) {
        emit taskCombo->currentIndexChanged(taskCombo->currentIndex());
        int archIdx = archCombo->findData((int)initData->archType);
        if (archIdx != -1) archCombo->setCurrentIndex(archIdx);
    }

    // 数据导入区域
    layout->addWidget(new QLabel("训练数据导入:"));
    QHBoxLayout* fileLayout = new QHBoxLayout();
    pathEdit = new QLineEdit(this); // 路径显示框
    pathEdit->setReadOnly(true); // 只读
    if (initData) pathEdit->setText(initData->dataPath); // 填入原有路径
    QPushButton* browseBtn = new QPushButton("导入...", this); // 浏览按钮
    QPushButton* helpBtn = new QPushButton("?", this); // 帮助按钮
    helpBtn->setFixedSize(30, 30);
    fileLayout->addWidget(pathEdit); fileLayout->addWidget(browseBtn); fileLayout->addWidget(helpBtn);
    layout->addLayout(fileLayout);

    // 帮助按钮逻辑
    connect(helpBtn, &QPushButton::clicked, [=]() { QMessageBox::information(this, "说明", "数值任务: .txt文件 (m行, n+1列)\n图像任务: 文件夹"); });

    // 浏览按钮逻辑
    connect(browseBtn, &QPushButton::clicked, [=]() {
        if (taskCombo->currentIndex() <= 0) {
            QMessageBox::warning(this, "提示", "请先选择任务类型");
            return;
        }

        TaskType t = (TaskType)taskCombo->currentData().toInt();

        QString path;
        // 根据任务类型选择文件或文件夹
        if (isNumericTask(t))
            path = QFileDialog::getOpenFileName(this, "选择数据文件", "", "Text Files (*.txt)");
        else
            path = QFileDialog::getExistingDirectory(this, "选择图像文件夹");

        if (!path.isEmpty()) {
            // 校验逻辑
            if (isNumericTask(t)) {
                QFile f(path);
                if (f.open(QIODevice::ReadOnly)) {
                    if (f.size() == 0) {
                        QMessageBox::warning(this, "数据错误", "选择的文件为空！");
                        f.close();
                        return;
                    }

                    // 数据格式校验逻辑：检查列数是否匹配
                    QTextStream in(&f);
                    QString firstLine = in.readLine();
                    QStringList tokens = firstLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

                    int expectedCols = featureSpin->value() + 1; // 训练数据必须包含 label (特征数 + 1)
                    if (tokens.size() != expectedCols) {
                        QMessageBox::critical(this, "数据格式错误",
                            QString("您当前设置的特征数为 %1，因此数据文件每行应包含 %2 个数值（特征+标签）。\n但检测到的文件第一行有 %3 列数据。\n请检查文件或修改特征数量设置。")
                            .arg(featureSpin->value()).arg(expectedCols).arg(tokens.size()));
                        f.close();
                        return; // 不设置路径，强制用户重选
                    }
                    f.close();
                }
            }
            else { // 图像任务校验
                QDir d(path);
                if (d.isEmpty()) {
                    QMessageBox::warning(this, "数据错误", "选择的文件夹为空！");
                    return;
                }
            }
            pathEdit->setText(path); // 路径有效，显示路径
        }
        });

    layout->addStretch(); // 底部弹簧
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    QPushButton* cancelBtn = new QPushButton("取消", this); // 取消按钮
    QPushButton* draftBtn = new QPushButton("保存至草稿箱", this); // 草稿按钮
    QPushButton* trainBtn = new QPushButton("开始训练", this); // 训练按钮
    bottomLayout->addWidget(cancelBtn); bottomLayout->addWidget(draftBtn); bottomLayout->addWidget(trainBtn);
    layout->addLayout(bottomLayout);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject); // 取消
    // 保存草稿逻辑
    connect(draftBtn, &QPushButton::clicked, [=]() {
        if (fillResultInfo(ModelStatus::Draft)) { QMessageBox::information(this, "提示", "已保存到草稿箱"); this->done(2); } // done(2) 表示返回代码 2
        });
    // 开始训练逻辑
    connect(trainBtn, &QPushButton::clicked, [=]() {
        if (pathEdit->text().isEmpty()) { QMessageBox::warning(this, "错误", "请导入训练数据"); return; }
        if (fillResultInfo(ModelStatus::Trained)) {
            TrainingWindow tw(resultInfo, this); // 创建训练窗口
            if (tw.exec() == QDialog::Accepted) accept(); // 如果训练窗口返回 Accept，则本对话框也返回 Accept
        }
        });
}

// 填充 resultInfo 结构体的辅助函数
bool AddModelDialog::fillResultInfo(ModelStatus status) {
    if (taskCombo->currentIndex() <= 0) return false; // 检查是否选了任务
    resultInfo.id = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"); // 生成新 ID (时间戳)
    resultInfo.name = nameEdit->text(); // 获取名称
    resultInfo.intro = introEdit->toPlainText(); // 获取简介
    resultInfo.taskType = (TaskType)taskCombo->currentData().toInt(); // 获取任务类型
    resultInfo.archType = (ArchType)archCombo->currentData().toInt(); // 获取架构类型
    resultInfo.dataPath = pathEdit->text(); // 获取数据路径
    resultInfo.createTime = QDateTime::currentDateTime(); // 获取当前时间
    resultInfo.status = status; // 设置状态
    resultInfo.inputDim = isNumericTask(resultInfo.taskType) ? featureSpin->value() : 0; // 设置输入维度
    return true;
}

// 事件过滤器：拦截架构下拉框的点击
bool AddModelDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == archCombo) {
        // 只有当任务类型未选择 (index <= 0) 且发生了鼠标点击事件时
        if (event->type() == QEvent::MouseButtonPress && taskCombo->currentIndex() <= 0) {
            QMessageBox::warning(this, "提示", "请先选择任务类型");
            return true; // 拦截事件，不让下拉框展开
        }
    }
    return QDialog::eventFilter(obj, event); // 其他情况走默认处理
}

// 重写 reject (关闭/取消) 逻辑
void AddModelDialog::reject() {
    // 检查用户是否输入了内容 (简单的非空判断，防止误触)
    bool hasContent = !nameEdit->text().isEmpty() || !introEdit->toPlainText().isEmpty() || !pathEdit->text().isEmpty();

    // 如果是编辑模式(initData不为空)或者有输入内容，则提示
    if (hasContent) {
        QMessageBox::StandardButton res = QMessageBox::question(
            this, "确认退出",
            "当前编辑的内容未保存，确定要退出吗？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No
        );

        if (res == QMessageBox::No) { return; } // 用户选择不退出
    }

    // 调用基类的 reject 关闭窗口
    QDialog::reject();
}

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
        InferenceWorker* worker = new InferenceWorker(m_info, isTestMode, pathEdit->text());

        // 绑定信号
        connect(worker, &InferenceWorker::progressUpdated, progressBar, &QProgressBar::setValue);
        connect(worker, &InferenceWorker::statusChanged, this, [=](QString s) { resultArea->append(s); });

        // 绑定结果输出信号
        connect(worker, &InferenceWorker::resultReady, this, [=](QString text, bool isCorrect) {
            resultArea->setTextColor(isCorrect ? Qt::black : Qt::red); // 错误用红色显示
            resultArea->append(text + (isCorrect ? "" : " [错误]"));
            });

        // 绑定完成信号
        connect(worker, &InferenceWorker::inferenceFinished, this, [=]() {
            progressBar->setValue(100);
            QMessageBox::information(this, "完成", "处理完成！");
            worker->deleteLater(); // 清理线程对象

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

// ==================== RegressionAnalysisDialog Implementation ====================
// 回归分析图表窗口实现

// 构造函数
RegressionAnalysisDialog::RegressionAnalysisDialog(const QVector<double>& preds, const QVector<double>& targets, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("回归模型性能分析"); // 设置标题
    resize(800, 600); // 设置大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 主布局

    chart = new QChart(); // 创建图表
    chart->setTitle("预测值 vs 真实值 (Prediction vs Ground Truth)"); // 设置图表标题
    chart->setTheme(QChart::ChartThemeLight); // 设置主题

    // 1. 散点图 (Scatter Plot)
    QScatterSeries* scatterSeries = new QScatterSeries();
    scatterSeries->setName("样本点");
    scatterSeries->setMarkerSize(8.0); // 点大小
    scatterSeries->setColor(QColor(0, 120, 215, 150)); // 半透明蓝
    scatterSeries->setBorderColor(Qt::transparent); // 无边框

    double minVal = 1e9, maxVal = -1e9; // 初始化极大极小值

    // 最小二乘法累计变量 (用于计算拟合线)
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    int n = preds.size(); // 样本数

    // 遍历数据点
    for (int i = 0; i < n; ++i) {
        double p = preds[i]; // X: 预测值
        double t = targets[i]; // Y: 真实值
        scatterSeries->append(p, t); // 添加点

        // 更新极值
        if (p < minVal) minVal = p; if (p > maxVal) maxVal = p;
        if (t < minVal) minVal = t; if (t > maxVal) maxVal = t;

        // 累计求和
        sumX += p;
        sumY += t;
        sumXY += (p * t);
        sumXX += (p * p);
    }

    // 坐标轴范围稍微扩大一点，留出边距
    double range = maxVal - minVal;
    minVal -= range * 0.1;
    maxVal += range * 0.1;

    // 2. 基准线 (y = x) - 理想预测线
    QLineSeries* baselineSeries = new QLineSeries();
    baselineSeries->setName("基准线 (y=x)");
    baselineSeries->append(minVal, minVal);
    baselineSeries->append(maxVal, maxVal);
    QPen basePen(Qt::gray); // 灰色
    basePen.setStyle(Qt::DashLine); // 虚线
    basePen.setWidth(2);
    baselineSeries->setPen(basePen);

    // 3. 拟合线 (Linear Fit) - 实际预测趋势
    // 公式: y = kx + b
    double k = 0, b = 0;
    if (n > 1 && (n * sumXX - sumX * sumX) != 0) {
        k = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX); // 计算斜率 k
        b = (sumY - k * sumX) / n; // 计算截距 b
    }

    QLineSeries* fitSeries = new QLineSeries();
    fitSeries->setName(QString("拟合线 (k=%.3f)").arg(k)); // 显示斜率
    fitSeries->append(minVal, k * minVal + b); // 起点
    fitSeries->append(maxVal, k * maxVal + b); // 终点
    QPen fitPen(Qt::red); // 红色
    fitPen.setWidth(3);
    fitSeries->setPen(fitPen);

    // 添加所有序列到图表
    chart->addSeries(baselineSeries);
    chart->addSeries(fitSeries);
    chart->addSeries(scatterSeries); // 散点放在最上面

    chart->createDefaultAxes(); // 创建默认坐标轴
    chart->axes(Qt::Horizontal).first()->setRange(minVal, maxVal); // 设置 X 轴范围
    chart->axes(Qt::Vertical).first()->setRange(minVal, maxVal); // 设置 Y 轴范围
    chart->axes(Qt::Horizontal).first()->setTitleText("预测值 (Predicted)"); // X 轴标签
    chart->axes(Qt::Vertical).first()->setTitleText("真实值 (Actual)"); // Y 轴标签

    chartView = new QChartView(chart); // 创建视图
    chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿
    layout->addWidget(chartView);

    // 底部添加说明标签
    QLabel* note = new QLabel("说明：点越靠近灰色虚线(y=x)，表示预测越准确。\n红色实线为模型预测趋势的线性拟合，k值越接近1越好。", this);
    note->setAlignment(Qt::AlignCenter);
    note->setStyleSheet("color: #666; font-size: 12px;");
    layout->addWidget(note);
}

// ==================== ClassificationAnalysisDialog Implementation ====================
ClassificationAnalysisDialog::ClassificationAnalysisDialog(const QVector<int>& preds,
    const QVector<int>& targets,
    const QMap<QString, int>& labelMap,
    QWidget* parent)
    : QDialog(parent), m_preds(preds), m_targets(targets), m_labelMap(labelMap)
{
    setWindowTitle("分类模型性能分析 (Confusion Matrix & Recall)");
    // 调整窗口尺寸：改为竖向长方形，适应上下布局
    resize(700, 900);

    // 建立反向映射
    for (auto it = m_labelMap.begin(); it != m_labelMap.end(); ++it) {
        m_indexToLabel.insert(it.value(), it.key());
    }
    m_numClasses = m_labelMap.size();

    // 使用 QVBoxLayout 实现垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20); // 增加上下两部分的间距
    mainLayout->setContentsMargins(30, 30, 30, 30); // 增加窗口边距

    // 第一部分：混淆矩阵

    QLabel* matrixTitle = new QLabel("混淆矩阵 (Confusion Matrix)", this);
    matrixTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    matrixTitle->setAlignment(Qt::AlignCenter);

    QLabel* matrixSubtitle = new QLabel("纵轴: 真实类别 (Actual)  |  横轴: 预测类别 (Predicted)", this);
    matrixSubtitle->setStyleSheet("font-size: 12px; color: #666; margin-bottom: 5px;");
    matrixSubtitle->setAlignment(Qt::AlignCenter);

    QTableWidget* table = new QTableWidget(this);
    table->setRowCount(m_numClasses);
    table->setColumnCount(m_numClasses);

    // 设置表头自动拉伸，使其看起来更整齐
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    table->setSelectionMode(QAbstractItemView::NoSelection);   // 禁止选中

    // 设置表头标签
    QStringList headers;
    for (int i = 0; i < m_numClasses; ++i) headers << m_indexToLabel[i];
    table->setHorizontalHeaderLabels(headers);
    table->setVerticalHeaderLabels(headers);

    // 计算混淆矩阵数据
    QVector<QVector<int>> matrix(m_numClasses, QVector<int>(m_numClasses, 0));
    int maxVal = 0;
    for (int i = 0; i < m_preds.size(); ++i) {
        int p = m_preds[i];
        int t = m_targets[i];
        if (p >= 0 && p < m_numClasses && t >= 0 && t < m_numClasses) {
            matrix[t][p]++;
            if (matrix[t][p] > maxVal) maxVal = matrix[t][p];
        }
    }

    // 填充表格并上色
    for (int r = 0; r < m_numClasses; ++r) {
        for (int c = 0; c < m_numClasses; ++c) {
            int count = matrix[r][c];
            QTableWidgetItem* item = new QTableWidgetItem(QString::number(count));
            item->setTextAlignment(Qt::AlignCenter);

            // 颜色算法
            int alpha = 0;
            if (maxVal > 0) alpha = (int)(255.0 * count / maxVal);

            if (r == c) { // 正确 (绿色系)
                item->setBackground(QColor(230 - alpha * 0.5, 255, 230 - alpha * 0.5));
                // 如果颜色太深，文字变白
                if (alpha > 150) item->setForeground(Qt::black);
                // 字体加粗
                QFont f = item->font(); f.setBold(true); item->setFont(f);
            }
            else { // 错误 (红色系)
                if (count > 0) {
                    item->setBackground(QColor(255, 235 - alpha * 0.5, 235 - alpha * 0.5));
                }
                else {
                    item->setBackground(Qt::white);
                    item->setForeground(QColor("#ddd")); // 0值显示淡一点
                }
            }
            table->setItem(r, c, item);
        }
    }

    // 添加到主布局 (伸缩因子 1)
    mainLayout->addWidget(matrixTitle);
    mainLayout->addWidget(matrixSubtitle);
    mainLayout->addWidget(table, 1);

    // 添加一条分割线
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // 第二部分：柱状图 (Bottom)

    // 计算 Recall
    QVector<int> totalPerClass(m_numClasses, 0);
    QVector<int> correctPerClass(m_numClasses, 0);

    for (int i = 0; i < m_preds.size(); ++i) {
        int t = m_targets[i];
        int p = m_preds[i];
        if (t >= 0 && t < m_numClasses && p >= 0 && p < m_numClasses) {
            totalPerClass[t]++;
            if (t == p) correctPerClass[t]++;
        }
    }

    QBarSet* set0 = new QBarSet("Recall (%)");
    set0->setColor(QColor(64, 158, 255)); // 科技蓝

    QStringList categories;
    for (int i = 0; i < m_numClasses; ++i) {
        categories << m_indexToLabel[i];
        double recall = 0.0;
        if (totalPerClass[i] > 0) {
            recall = (double)correctPerClass[i] / totalPerClass[i] * 100.0;
        }
        *set0 << recall;
    }

    QBarSeries* series = new QBarSeries();
    series->append(set0);
    series->setLabelsVisible(true);

    // 将标签放在柱状图内部顶端，并将颜色设为白色，这样 Y 轴就可以设置为完美的 100% 而不会切断文字
    series->setLabelsPosition(QAbstractBarSeries::LabelsInsideEnd);
    series->setLabelsFormat("@value%");

    // 1. 文字放内部 (InsideEnd)
    // 2. 轴设为 100
    // 3. 将 Bar 颜色调浅一点，或者相信 Qt 默认的文字颜色处理

    series->setLabelsPosition(QAbstractBarSeries::LabelsInsideEnd);

    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("各类别召回率 (Recall Rate)");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(false);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    // 强制设置为 0 - 100
    axisY->setRange(0, 100);
    axisY->setTitleText("Recall (%)");
    axisY->setLabelFormat("%d"); // 整数显示
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 添加到布局 (伸缩因子 1)
    mainLayout->addWidget(chartView, 1);
}

// ==================== MainWindow Implementation ====================
// 主窗口实现

// 构造函数
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI(); // 初始化 UI
    DatabaseManager::instance().init(); // 初始化数据库
    allModels = DatabaseManager::instance().loadAllModels(); // 加载所有模型

    QList<ModelInfo> loaded = allModels; // 临时备份
    allModels.clear(); recycleBin.clear(); draftBox.clear(); // 清空各个列表
    // 根据状态分发模型到不同列表
    for (const auto& m : loaded) {
        if (m.status == ModelStatus::Deleted) recycleBin.append(m); // 已删除 -> 回收站
        else if (m.status == ModelStatus::Draft) draftBox.append(m); // 草稿 -> 草稿箱
        else allModels.append(m); // 其他 -> 正常显示
    }
    refreshModelList(); // 刷新显示
    this->setMinimumWidth(320); // 设置最小宽度
}

// 析构函数
MainWindow::~MainWindow() {}

// 初始化 UI 布局
void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this); // 创建中心部件
    this->setCentralWidget(centralWidget); // 设置为中心部件
    this->resize(1200, 800); // 初始大小
    this->setWindowTitle("神经网络模型综合使用和制作平台"); // 窗口标题
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget); // 主垂直布局
    QFrame* topFrame = new QFrame(this); // 顶部控制栏
    topFrame->setFrameShape(QFrame::StyledPanel);
    topFrame->setMaximumHeight(100); // 限制高度
    QHBoxLayout* topLayout = new QHBoxLayout(topFrame); // 顶部水平布局
    searchBar = new QLineEdit(this); // 搜索框
    searchBar->setPlaceholderText("输入关键词搜索模型...");
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearch); // 搜索逻辑
    filterCombo = new QComboBox(this); // 筛选下拉框
    filterCombo->addItem("所有类型", -1);
    filterCombo->addItem("数值回归", (int)TaskType::NumericRegression);
    filterCombo->addItem("数值二分类", (int)TaskType::NumericBinaryClass);
    filterCombo->addItem("数值多分类", (int)TaskType::NumericMultiClass);
    filterCombo->addItem("图像二分类", (int)TaskType::ImageBinaryClass);
    filterCombo->addItem("图像多分类", (int)TaskType::ImageMultiClass);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged); // 筛选逻辑
    QPushButton* recycleBtn = new QPushButton("回收站", this); // 回收站按钮
    QPushButton* draftBtn = new QPushButton("草稿箱", this); // 草稿箱按钮
    QPushButton* addBtn = new QPushButton("+ 添加新模型", this); // 添加按钮
    addBtn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 8px; border-radius: 5px;"); // 绿色样式
    connect(recycleBtn, &QPushButton::clicked, this, &MainWindow::onOpenRecycleBin);
    connect(draftBtn, &QPushButton::clicked, this, &MainWindow::onOpenDraftBox);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddNewModel);
    topLayout->addWidget(new QLabel("搜索:")); topLayout->addWidget(searchBar);
    topLayout->addWidget(new QLabel("筛选:")); topLayout->addWidget(filterCombo);
    topLayout->addStretch(); // 弹簧
    topLayout->addWidget(recycleBtn); topLayout->addWidget(draftBtn); topLayout->addWidget(addBtn);
    mainLayout->addWidget(topFrame); // 添加顶部栏
    QScrollArea* scrollArea = new QScrollArea(this); // 滚动区域（用于显示模型卡片）
    scrollArea->setWidgetResizable(true);
    modelContainer = new QWidget(); // 模型卡片容器
    modelLayout = new QGridLayout(modelContainer); // 网格布局
    modelLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft); // 对齐方式
    scrollArea->setWidget(modelContainer);
    mainLayout->addWidget(scrollArea); // 添加滚动区域到主布局
}

// 刷新模型列表显示
void MainWindow::refreshModelList() {
    // 1. 清理现有布局项
    QLayoutItem* item;
    while ((item = modelLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget(); // 删除 Widget
        delete item; // 删除 LayoutItem
    }

    QString keyword = searchBar->text().toLower(); // 获取搜索词（转小写）
    int filterType = filterCombo->currentData().toInt(); // 获取筛选类型

    QList<QWidget*> visibleCards; // 暂存可见卡片

    // 2. 遍历并创建卡片
    for (const auto& model : allModels) {
        if (model.status != ModelStatus::Trained) continue; // 只显示已训练模型
        // 匹配逻辑：名称/简介包含关键词
        bool matchSearch = model.name.toLower().contains(keyword) || model.intro.toLower().contains(keyword);
        // 匹配逻辑：类型匹配或全选
        bool matchFilter = (filterType == -1) || ((int)model.taskType == filterType);

        if (matchSearch && matchFilter) {
            // 创建卡片
            ModelCard* card = new ModelCard(model, modelContainer);

            // [关键] 必须在这里连接信号，因为我们不再调用 addModelCard 函数了
            // 连接卡片的各个信号到主窗口的槽函数
            connect(card, &ModelCard::requestUse, this, &MainWindow::useModel);
            connect(card, &ModelCard::requestTest, this, &MainWindow::testModel);
            connect(card, &ModelCard::requestRetrain, this, &MainWindow::retrainModel);
            connect(card, &ModelCard::requestModify, this, &MainWindow::modifyModel);
            connect(card, &ModelCard::requestDelete, this, &MainWindow::deleteModel);

            visibleCards.append(card); // 加入暂存列表
        }
    }

    // 3. 将卡片加入布局 (此时还未排版，稍后由 reflowModelCards 处理)
    for (auto* w : visibleCards) modelLayout->addWidget(w);

    // 4. 调用排版函数计算位置 (响应式布局)
    reflowModelCards();

    // 5. 如果没有模型显示提示
    if (visibleCards.isEmpty()) {
        QLabel* emptyLabel = new QLabel(this);
        if (allModels.isEmpty()) {
            emptyLabel->setText("暂无模型，请点击右上角添加");
        }
        else {
            emptyLabel->setText("没有找到符合条件的模型");
        }
        emptyLabel->setStyleSheet("color: #888; font-size: 18px; margin-top: 50px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        modelLayout->addWidget(emptyLabel, 0, 0, 1, 4); // 跨列居中
    }
}

// 添加新模型逻辑
void MainWindow::onAddNewModel() {
    AddModelDialog dialog(allModels.size(), nullptr, this); // 打开添加对话框
    int res = dialog.exec();
    if (res == QDialog::Accepted) { // 训练完成
        DatabaseManager::instance().saveModel(dialog.resultInfo); // 保存
        allModels.append(dialog.resultInfo); // 添加到列表
        refreshModelList(); // 刷新
    }
    else if (res == 2) { // 保存为草稿
        DatabaseManager::instance().saveModel(dialog.resultInfo);
        draftBox.append(dialog.resultInfo);
    }
}

// 修改模型信息逻辑
void MainWindow::modifyModel(QString id) {
    // 查找模型
    auto it = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
    if (it != allModels.end()) {
        ModifyModelDialog dlg(*it, this); // 打开修改对话框
        if (dlg.exec() == QDialog::Accepted) {
            DatabaseManager::instance().saveModel(*it); // 保存更新
            refreshModelList(); // 刷新
            QMessageBox::information(this, "成功", "模型信息已更新");
        }
    }
}

// 使用模型逻辑
void MainWindow::useModel(QString id) {
    // 查找模型
    auto it = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
    if (it != allModels.end()) { UseModelDialog dlg(*it, false, this); dlg.exec(); } // 打开使用对话框 (testMode=false)
}

// 测试模型逻辑
void MainWindow::testModel(QString id) {
    // 查找模型
    auto it = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
    if (it != allModels.end()) { UseModelDialog dlg(*it, true, this); dlg.exec(); } // 打开测试对话框 (testMode=true)
}

// 删除模型逻辑 (移入回收站)
void MainWindow::deleteModel(QString id) {
    // 查找模型
    auto it = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
    if (it != allModels.end()) {
        if (QMessageBox::question(this, "删除", "确认将模型放入回收站？") == QMessageBox::Yes) {
            it->status = ModelStatus::Deleted; // 改变状态
            DatabaseManager::instance().saveModel(*it); // 保存状态改变
            recycleBin.append(*it); // 移入回收站列表
            allModels.erase(it); // 从主列表移除
            refreshModelList(); // 刷新
        }
    }
}

// 重训模型逻辑
void MainWindow::retrainModel(QString id) {
    // 查找模型
    auto it = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
    if (it != allModels.end()) {
        if (QMessageBox::question(this, "重训", "确认重新训练该模型？") == QMessageBox::Yes) {
            ModelInfo oldInfo = *it; // 备份旧信息

            /*allModels.erase(it);
            refreshModelList();*/

            AddModelDialog dialog(allModels.size(), &oldInfo, this); // 打开添加对话框（带入旧信息）
            int res = dialog.exec();

            if (res == QDialog::Accepted) {
                // 1. 保存新模型 (ID 是新的)
                DatabaseManager::instance().saveModel(dialog.resultInfo);

                // 2. 物理删除旧模型
                DatabaseManager::instance().deleteModel(oldInfo.id);

                // 3. 更新内存列表（先移除旧的，再加新的，或者重新加载）
                // 重新查找迭代器，因为上面的操作可能导致迭代器失效（虽然 list 不会，但为了安全）
                auto it_retry = std::find_if(allModels.begin(), allModels.end(), [id](const ModelInfo& m) { return m.id == id; });
                if (it_retry != allModels.end()) {
                    allModels.erase(it_retry); // 移除旧条目
                }

                allModels.append(dialog.resultInfo); // 添加新条目
                refreshModelList(); // 刷新
            }
            else if (res == 2) { // 存为草稿
                DatabaseManager::instance().saveModel(dialog.resultInfo);
                draftBox.append(dialog.resultInfo);
            }
        }
    }
}

// 搜索槽函数
void MainWindow::onSearch() { refreshModelList(); }
// 筛选槽函数
void MainWindow::onFilterChanged() { refreshModelList(); }

// 打开回收站逻辑
void MainWindow::onOpenRecycleBin() {
    RecycleBinDialog dlg(&recycleBin, &allModels, this); // 传入列表指针
    dlg.exec();
    // 退出回收站后，保存所有更改到文件系统
    for (const auto& m : allModels) DatabaseManager::instance().saveModel(m);
    for (const auto& m : recycleBin) DatabaseManager::instance().saveModel(m);
    refreshModelList(); // 刷新主界面
}

// 打开草稿箱逻辑
void MainWindow::onOpenDraftBox() {
    DraftBoxDialog dlg(&draftBox, this); // 传入列表指针

    // 如果用户选择了一个草稿并点击了“继续”
    if (dlg.exec() == QDialog::Accepted && dlg.hasSelection) {
        // 打开添加/训练窗口
        AddModelDialog addDlg(allModels.size(), &dlg.selectedDraft, this);
        int res = addDlg.exec();

        if (res == QDialog::Accepted) {
            // 训练成功，保存新模型（ID是新的）
            DatabaseManager::instance().saveModel(addDlg.resultInfo);
            allModels.append(addDlg.resultInfo);

            // 删除旧的草稿文件
            DatabaseManager::instance().deleteModel(dlg.selectedDraft.id);

            // 从草稿箱列表中移除
            auto it = std::find(draftBox.begin(), draftBox.end(), dlg.selectedDraft);
            if (it != draftBox.end()) draftBox.erase(it);

            refreshModelList();
        }
        else if (res == 2) {
            // 用户再次存为草稿，addDlg 会生成一个新的 ID
            DatabaseManager::instance().saveModel(addDlg.resultInfo);
            draftBox.append(addDlg.resultInfo);

            // 删除旧草稿，避免草稿箱里出现两个相似的版本
            DatabaseManager::instance().deleteModel(dlg.selectedDraft.id);
            auto it = std::find(draftBox.begin(), draftBox.end(), dlg.selectedDraft);
            if (it != draftBox.end()) draftBox.erase(it);
        }
    }
}

// 响应式布局：重排模型卡片
void MainWindow::reflowModelCards() {
    // 获取视口宽度 (减去滚动条和边距的大致宽度)
    int viewportWidth = modelContainer->width();
    int cardWidth = 280; // ModelCard 的固定宽度
    int spacing = 8;    // 卡片间距

    // 计算一行能放下多少个
    int cols = (viewportWidth - spacing) / (cardWidth + spacing);
    if (cols < 1) cols = 1; // 至少显示一列

    QList<QWidget*> widgets; // 临时存储所有 Widget
    while (modelLayout->count()) {
        QLayoutItem* item = modelLayout->takeAt(0);
        if (item->widget()) {
            widgets.append(item->widget()); // 保存 widget 指针
        }
        delete item;
    }

    // 重新添加到 Layout，按计算出的列数排列
    for (int i = 0; i < widgets.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        modelLayout->addWidget(widgets[i], row, col);
    }

    modelLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop); // 设置整体居中
}

// 窗口大小改变事件
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event); // 调用基类处理
    // 使用单次定时器延时调用重排，避免频繁计算导致的卡顿，并确保尺寸已更新
    QTimer::singleShot(10, this, [this]() {
        reflowModelCards();
        });
}