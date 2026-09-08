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
