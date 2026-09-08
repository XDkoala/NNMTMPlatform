#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

namespace {
bool saveModelWithFeedback(QWidget* parent, const ModelInfo& model) {
    try {
        DatabaseManager::instance().saveModel(model);
        return true;
    } catch (const std::exception& error) {
        QMessageBox::critical(parent, "保存失败",
            "模型信息未能写入磁盘：\n" + QString::fromUtf8(error.what()));
        return false;
    }
}
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
        if (!saveModelWithFeedback(this, dialog.resultInfo)) return;
        allModels.append(dialog.resultInfo); // 添加到列表
        refreshModelList(); // 刷新
    }
    else if (res == 2) { // 保存为草稿
        if (!saveModelWithFeedback(this, dialog.resultInfo)) return;
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
            if (!saveModelWithFeedback(this, *it)) return;
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
            if (!saveModelWithFeedback(this, *it)) {
                it->status = ModelStatus::Trained;
                return;
            }
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
                if (!saveModelWithFeedback(this, dialog.resultInfo)) return;

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
                if (!saveModelWithFeedback(this, dialog.resultInfo)) return;
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
    for (const auto& m : allModels) {
        if (!saveModelWithFeedback(this, m)) break;
    }
    for (const auto& m : recycleBin) {
        if (!saveModelWithFeedback(this, m)) break;
    }
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
            if (!saveModelWithFeedback(this, addDlg.resultInfo)) return;
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
            if (!saveModelWithFeedback(this, addDlg.resultInfo)) return;
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
