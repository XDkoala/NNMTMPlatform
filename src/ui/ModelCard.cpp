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
