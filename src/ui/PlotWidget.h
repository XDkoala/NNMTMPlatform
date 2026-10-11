#pragma once

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <limits>

// Small, project-owned charts painted with LGPL Qt Widgets/Gui.
class PlotWidget : public QWidget {
public:
    enum class Style { Line, Scatter };
    struct Series {
        QString name;
        QColor color;
        QVector<QPointF> points;
        Style style = Style::Line;
        Qt::PenStyle penStyle = Qt::SolidLine;
    };

    explicit PlotWidget(const QString& title, const QString& xLabel,
                        const QString& yLabel, QWidget* parent = nullptr)
        : QWidget(parent), title_(title), xLabel_(xLabel), yLabel_(yLabel) {
        setMinimumSize(320, 240);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    int addSeries(const QString& name, QColor color, Style style = Style::Line,
                  Qt::PenStyle penStyle = Qt::SolidLine) {
        series_.push_back({name, color, {}, style, penStyle});
        update();
        return static_cast<int>(series_.size() - 1);
    }

    void append(int seriesIndex, double x, double y) {
        if (std::isfinite(x) && std::isfinite(y)) {
            series_[seriesIndex].points.append(QPointF(x, y));
            update();
        }
    }

    void setBars(const QStringList& categories, const QVector<double>& values) {
        categories_ = categories;
        bars_ = values;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);
        painter.setPen(QColor("#333333"));
        painter.drawText(QRectF(10, 8, width() - 20, 28), Qt::AlignCenter, title_);
        const QRectF plot(78, 66, std::max(1, width() - 100), std::max(1, height() - 132));

        double xMin = std::numeric_limits<double>::infinity();
        double xMax = -xMin, yMin = xMin, yMax = xMax;
        const bool barMode = !categories_.isEmpty();
        if (barMode) {
            xMin = 0; xMax = static_cast<double>(categories_.size());
            yMin = 0; yMax = 100;
        } else {
            for (const auto& series : series_) {
                for (const auto& point : series.points) {
                    xMin = std::min(xMin, point.x()); xMax = std::max(xMax, point.x());
                    yMin = std::min(yMin, point.y()); yMax = std::max(yMax, point.y());
                }
            }
            if (!std::isfinite(xMin)) { xMin = 0; xMax = 10; yMin = 0; yMax = 1; }
            const double xPad = xMax > xMin ? (xMax - xMin) * 0.05 : std::max(0.5, std::abs(xMin) * 0.05);
            const double yPad = yMax > yMin ? (yMax - yMin) * 0.1 : std::max(0.1, std::abs(yMin) * 0.05);
            xMin -= xPad; xMax += xPad; yMin -= yPad; yMax += yPad;
        }

        auto mapPoint = [&](const QPointF& point) {
            return QPointF(plot.left() + (point.x() - xMin) / (xMax - xMin) * plot.width(),
                           plot.bottom() - (point.y() - yMin) / (yMax - yMin) * plot.height());
        };

        for (int tick = 0; tick <= 4; ++tick) {
            const double fraction = tick / 4.0;
            const double y = plot.bottom() - fraction * plot.height();
            painter.setPen(QColor("#e5e7eb"));
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            painter.setPen(QColor("#555555"));
            painter.drawText(QRectF(2, y - 10, 68, 20), Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(yMin + fraction * (yMax - yMin), 'g', 4));
            if (!barMode) {
                const double x = plot.left() + fraction * plot.width();
                painter.drawText(QRectF(x - 38, plot.bottom() + 6, 76, 20), Qt::AlignCenter,
                                 QString::number(xMin + fraction * (xMax - xMin), 'g', 4));
            }
        }
        painter.setPen(QColor("#9ca3af"));
        painter.drawRect(plot);
        painter.setPen(QColor("#333333"));
        painter.drawText(QRectF(plot.left(), height() - 28, plot.width(), 22), Qt::AlignCenter, xLabel_);
        painter.save();
        painter.translate(17, plot.center().y());
        painter.rotate(-90);
        painter.drawText(QRectF(-plot.height() / 2, -12, plot.height(), 24), Qt::AlignCenter, yLabel_);
        painter.restore();

        if (barMode) {
            const double slot = plot.width() / static_cast<double>(categories_.size());
            for (qsizetype i = 0; i < categories_.size(); ++i) {
                const double value = i < bars_.size() && std::isfinite(bars_[i])
                    ? std::clamp(bars_[i], 0.0, 100.0) : 0.0;
                const double x = plot.left() + (static_cast<double>(i) + 0.15) * slot;
                const double h = plot.height() * value / 100.0;
                painter.fillRect(QRectF(x, plot.bottom() - h, slot * 0.7, h), QColor("#409eff"));
                painter.setPen(QColor("#333333"));
                painter.drawText(QRectF(x - slot * 0.15, plot.bottom() - h - 22, slot, 20),
                                 Qt::AlignCenter, QString::number(value, 'f', 1) + "%");
                const QString label = painter.fontMetrics().elidedText(categories_[i], Qt::ElideRight,
                                                                       std::max(1, static_cast<int>(slot - 4)));
                painter.drawText(QRectF(x - slot * 0.15, plot.bottom() + 6, slot, 20), Qt::AlignCenter, label);
            }
        } else {
            painter.save();
            painter.setClipRect(plot.adjusted(-1, -1, 1, 1));
            for (const auto& series : series_) {
                painter.setPen(QPen(series.color, 2, series.penStyle));
                if (series.style == Style::Scatter) {
                    painter.setBrush(series.color);
                    for (const auto& point : series.points) painter.drawEllipse(mapPoint(point), 3.5, 3.5);
                } else if (!series.points.isEmpty()) {
                    QPainterPath path;
                    path.moveTo(mapPoint(series.points.first()));
                    for (qsizetype i = 1; i < series.points.size(); ++i) path.lineTo(mapPoint(series.points[i]));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawPath(path);
                    if (series.points.size() == 1) painter.drawEllipse(mapPoint(series.points.first()), 2, 2);
                }
            }
            painter.restore();
            double legendX = plot.left();
            for (const auto& series : series_) {
                painter.setPen(QPen(series.color, 2, series.penStyle));
                painter.drawLine(QPointF(legendX, 48), QPointF(legendX + 14, 48));
                painter.setPen(QColor("#333333"));
                painter.drawText(QPointF(legendX + 19, 52), series.name);
                legendX += painter.fontMetrics().horizontalAdvance(series.name) + 35;
            }
        }
    }

private:
    QString title_, xLabel_, yLabel_;
    QVector<Series> series_;
    QStringList categories_;
    QVector<double> bars_;
};
