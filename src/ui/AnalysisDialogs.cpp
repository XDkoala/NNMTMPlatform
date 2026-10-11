#include "AppUI.h" // 包含本模块的头文件
#include <QRegularExpression> // 包含正则表达式处理类
#include <QMessageBox> // 包含消息弹窗类
#include <QFileDialog> // 包含文件选择对话框类
#include <QDebug> // 包含调试输出类

// ==================== RegressionAnalysisDialog Implementation ====================
// 回归分析图表窗口实现

// 构造函数
RegressionAnalysisDialog::RegressionAnalysisDialog(const QVector<double>& preds, const QVector<double>& targets, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("回归模型性能分析"); // 设置标题
    resize(800, 600); // 设置大小
    QVBoxLayout* layout = new QVBoxLayout(this); // 主布局

    auto* chart = new PlotWidget("预测值 vs 真实值", "预测值 (Predicted)", "真实值 (Actual)", this);
    const int baseline = chart->addSeries("基准线 (y=x)", Qt::gray, PlotWidget::Style::Line, Qt::DashLine);
    const int fit = chart->addSeries("拟合线", Qt::red);
    const int samples = chart->addSeries("样本点", QColor(0, 120, 215, 150), PlotWidget::Style::Scatter);
    const qsizetype count = std::min(preds.size(), targets.size());
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    double minVal = std::numeric_limits<double>::infinity(), maxVal = -minVal;
    qsizetype validCount = 0;
    for (qsizetype i = 0; i < count; ++i) {
        const double p = preds[i], t = targets[i];
        if (!std::isfinite(p) || !std::isfinite(t)) continue;
        chart->append(samples, p, t);
        minVal = std::min({minVal, p, t});
        maxVal = std::max({maxVal, p, t});
        sumX += p; sumY += t; sumXY += p * t; sumXX += p * p;
        ++validCount;
    }
    if (validCount > 0) {
        if (minVal == maxVal) { minVal -= 0.5; maxVal += 0.5; }
        chart->append(baseline, minVal, minVal);
        chart->append(baseline, maxVal, maxVal);
        const double n = static_cast<double>(validCount);
        const double denominator = n * sumXX - sumX * sumX;
        if (validCount > 1 && std::abs(denominator) > 1e-12) {
            const double k = (n * sumXY - sumX * sumY) / denominator;
            const double offset = (sumY - k * sumX) / n;
            chart->append(fit, minVal, k * minVal + offset);
            chart->append(fit, maxVal, k * maxVal + offset);
        }
    }
    layout->addWidget(chart);

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

    QStringList categories;
    QVector<double> recalls;
    for (int i = 0; i < m_numClasses; ++i) {
        categories << m_indexToLabel[i];
        recalls << (totalPerClass[i] > 0
            ? static_cast<double>(correctPerClass[i]) / totalPerClass[i] * 100.0 : 0.0);
    }
    auto* chart = new PlotWidget("各类别召回率 (Recall Rate)", "类别", "Recall (%)", this);
    chart->setBars(categories, recalls);
    mainLayout->addWidget(chart, 1);
}
