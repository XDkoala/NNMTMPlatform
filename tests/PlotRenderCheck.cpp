#include "PlotWidget.h"
#include <QApplication>
#include <QImage>
#include <QDir>
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 9));
    const QString outputDir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QDir::currentPath();
    QDir().mkpath(outputDir);
    auto render = [&](PlotWidget& plot, const QString& name) {
        plot.resize(800, 500);
        QImage image(plot.size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        plot.render(&image);
        return image.save(outputDir + "/" + name + ".png");
    };
    PlotWidget losses("Training losses", "Epoch", "Loss");
    const int train = losses.addSeries("Train Loss", QColor("#16a34a"));
    const int val = losses.addSeries("Validation Loss", QColor("#2563eb"));
    for (int i = 1; i <= 10; ++i) {
        losses.append(train, i, 1.0 / i);
        losses.append(val, i, 1.2 / i + 0.1);
    }
    PlotWidget regression("Predicted vs actual", "Predicted", "Actual");
    const int baseline = regression.addSeries("y=x", Qt::gray, PlotWidget::Style::Line, Qt::DashLine);
    regression.append(baseline, 0, 0); regression.append(baseline, 3, 3);
    const int samples = regression.addSeries("Samples", Qt::blue, PlotWidget::Style::Scatter);
    regression.append(samples, 1, 1.1); regression.append(samples, 2, 1.9);
    PlotWidget bars("Recall", "Class", "Recall (%)");
    bars.setBars({"zero", "half", "full"}, {0, 50, 100});
    PlotWidget constant("Constant values", "X", "Y");
    const int single = constant.addSeries("Single point", Qt::blue);
    constant.append(single, 0, 0);
    PlotWidget empty("Empty", "X", "Y");
    if (!render(losses, "losses") || !render(regression, "regression") ||
        !render(bars, "recall") || !render(constant, "constant") || !render(empty, "empty")) {
        std::cerr << "Failed to save chart render checks.\n";
        return 1;
    }
    std::cout << "Rendered line, scatter, bar, constant and empty charts.\n";
    return 0;
}
