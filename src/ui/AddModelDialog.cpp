#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

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

    layout->addWidget(new QLabel("训练参数:"));
    QHBoxLayout* trainingOptionsLayout = new QHBoxLayout();
    epochSpin = new QSpinBox(this);
    epochSpin->setRange(1, 10000);
    epochSpin->setValue(initData ? initData->maxEpochs : 100);
    epochSpin->setPrefix("轮数 ");
    batchSpin = new QSpinBox(this);
    batchSpin->setRange(1, 1024);
    batchSpin->setValue(initData ? initData->batchSize : 32);
    batchSpin->setPrefix("批次 ");
    learningRateSpin = new QDoubleSpinBox(this);
    learningRateSpin->setDecimals(6);
    learningRateSpin->setRange(0.000001, 1.0);
    learningRateSpin->setSingleStep(0.0001);
    learningRateSpin->setValue(initData ? initData->learningRate : 0.001);
    learningRateSpin->setPrefix("学习率 ");
    trainingOptionsLayout->addWidget(epochSpin);
    trainingOptionsLayout->addWidget(batchSpin);
    trainingOptionsLayout->addWidget(learningRateSpin);
    layout->addLayout(trainingOptionsLayout);

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
    resultInfo.maxEpochs = epochSpin->value();
    resultInfo.batchSize = batchSpin->value();
    resultInfo.learningRate = learningRateSpin->value();
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
