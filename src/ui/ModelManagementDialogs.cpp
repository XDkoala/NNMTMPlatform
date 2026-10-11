#include "AppUI.h" // 包含本模块的头文件
#include <QToolTip>
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

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
