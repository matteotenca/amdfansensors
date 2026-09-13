#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

#define NOMINMAX

#include <QMainWindow>
#include <QTimer>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>
#include <vector>
#include "GpuManager.h"

namespace Ui {
    class GuiWindow;
}

class CurvePlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurvePlotWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    void setPoints(const std::vector<std::pair<int, int>>& points) {
        m_points = points;
        update();
    }
    const std::vector<std::pair<int, int>>& points() const { return m_points; }

    void setRanges(int minTemp, int maxTemp, int minSpeed, int maxSpeed) {
        m_minTemp = minTemp; m_maxTemp = maxTemp;
        m_minSpeed = minSpeed; m_maxSpeed = maxSpeed;
    }

    void setDragSpeedRange(int minDragSpeed, int maxDragSpeed) {
        m_minDragSpeed = minDragSpeed;
        m_maxDragSpeed = maxDragSpeed;
    }

    void setEditable(bool editable) {
        m_editable = editable;
        update();
    }

    void setCurrentPoint(double temp, int fan) {
        m_currentTemp = temp;
        m_currentFan = fan;
        m_showCurrentPoint = true;
        update();
    }

    void setCurrentPointVisible(bool visible) {
        m_showCurrentPoint = visible;
        update();
    }

signals:
    void pointsChanged();
    void editingFinished();

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Background
        painter.fillRect(rect(), QColor("#181825"));

        if (m_points.empty()) return;

        // Padding and effective area
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);

        // Grid & Axes
        QPen axisPen(QColor("#45475A"), 1, Qt::SolidLine);
        painter.setPen(axisPen);
        // X axis shifted to bottom padding
        painter.drawLine(padding, height() - padding, width() - padding, height() - padding);
        // Y axis shifted to left padding
        painter.drawLine(padding, padding, padding, height() - padding);

        // Gridlines & tick positions (adaptive)
        const auto tempTicks = computeTicks((float)m_minTemp, (float)m_maxTemp);
        const auto fanTicks = computeTicks((float)m_minSpeed, (float)m_maxSpeed);
        QPen gridPen(QColor(49, 50, 68, 140), 1, Qt::SolidLine); // #313244 @ 55%
        painter.setPen(gridPen);
        for (float t : tempTicks) {
            float gx, gy;
            toPixel(t, (float)m_minSpeed, gx, gy);
            painter.drawLine(QPointF(gx, padding), QPointF(gx, height() - padding));
        }
        for (float s : fanTicks) {
            float gx, gy;
            toPixel((float)m_minTemp, s, gx, gy);
            painter.drawLine(QPointF(padding, gy), QPointF(width() - padding, gy));
        }

        // Legend - Labels
        painter.setPen(QPen(QColor("#BAC2DE"), 1));
        QFont labelFont = painter.font();
        labelFont.setPointSize(8);
        painter.setFont(labelFont);

        painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "Temp (°C)");
        painter.save();
        painter.translate(11.0f, height() / 2);
        painter.rotate(-90);
        // Draw the vertical label centered on the rotated origin using font metrics
        const QString fanLabel = "Fan (%)";
        const QFontMetrics fm(labelFont);
        painter.drawText(-fm.horizontalAdvance(fanLabel) / 2.0,
                         (fm.ascent() - fm.descent()) / 2.0,
                         fanLabel);
        painter.restore();

        // X-axis tick labels (below plot area in bottom padding)
        painter.setPen(QPen(QColor("#BAC2DE"), 1));
        for (float t : tempTicks) {
            float gx, gy;
            toPixel(t, (float)m_minSpeed, gx, gy);
            painter.drawText(QPointF(gx - 10, height() - padding + 12), QString::number((int)std::lround(t)));
        }
        // Y-axis tick labels (left of plot area in left padding)
        for (float s : fanTicks) {
            float gx, gy;
            toPixel((float)m_minTemp, s, gx, gy);
            const QString stxt = QString::number((int)std::lround(s));
            const QFontMetrics fm2(labelFont);
            painter.drawText(QPointF(padding - 3 - fm2.horizontalAdvance(stxt), gy + 3), stxt);
        }

        // Draw Curve
        QPen curvePen(QColor("#89DCEB"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(curvePen);

        QPolygonF polyline;
        for (const auto& p : m_points) {
            float x, y;
            toPixel(p.first, p.second, x, y);
            polyline << QPointF(x, y);
        }
        painter.drawPolyline(polyline);

        // Draw Point Markers and Labels
        for (size_t i = 0; i < m_points.size(); ++i) {
            const auto& p = m_points[i];
            float x, y;
            toPixel(p.first, p.second, x, y);

            // Marker
            painter.setBrush(QBrush(m_editable && static_cast<int>(i) == m_dragIndex
                                        ? QColor("#F9E2AF")
                                        : QColor("#89DCEB")));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(x, y), 4, 4);

            // Coordinate Text
            painter.setPen(QPen(QColor("#CBA6F7"), 1));
            painter.setFont(labelFont);
            QString label = QString("%1°C / %2%").arg(p.first).arg(p.second);

            float textX = x + 25;
            float textY = y;
            painter.drawText(textX, textY, label);
        }

        // Live operating-point marker
        if (m_showCurrentPoint) {
            float mx, my;
            toPixel((float)m_currentTemp, (float)m_currentFan, mx, my);
            // Clamp marker to the plot area to avoid drawing outside the axis.
            mx = std::clamp(mx, padding, width() - padding);
            my = std::clamp(my, padding, height() - padding);

            QPen ringPen(QColor("#F38BA8"), 2, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(ringPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(mx, my), 6, 6);

            // Small label beside the marker
            painter.setPen(QPen(QColor("#F38BA8"), 1));
            painter.setFont(labelFont);
            painter.drawText(QPointF(mx + 10, my - 10),
                             QString("%1°C / %2%").arg((int)std::lround(m_currentTemp)).arg(m_currentFan));
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (!m_editable) { QWidget::mousePressEvent(event); return; }
        m_dragIndex = hitTest(event->pos());
        if (m_dragIndex >= 0) update();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!m_editable || m_dragIndex < 0) { QWidget::mouseMoveEvent(event); return; }
        auto& p = m_points[m_dragIndex];
        float tempF = 0.0f, speedF = 0.0f;
        toValue(event->pos(), tempF, speedF);
        int newTemp = static_cast<int>(std::lround(tempF));
        int newSpeed = static_cast<int>(std::lround(speedF));

        // Constrain temperature to remain monotonic non-decreasing across neighbors.
        int minT = m_minTemp;
        int maxT = m_maxTemp;
        if (m_dragIndex > 0) minT = std::max(minT, m_points[m_dragIndex - 1].first);
        if (m_dragIndex + 1 < static_cast<int>(m_points.size()))
            maxT = std::min(maxT, m_points[m_dragIndex + 1].first);

        newTemp = std::clamp(newTemp, minT, maxT);
        newSpeed = std::clamp(newSpeed, m_minDragSpeed, m_maxDragSpeed);

        if (p.first != newTemp || p.second != newSpeed) {
            p.first = newTemp;
            p.second = newSpeed;
            emit pointsChanged();
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_dragIndex >= 0) {
            emit editingFinished();
        }
        m_dragIndex = -1;
        update();
    }

    void toPixel(float temp, float speed, float& x, float& y) const {
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);
        x = padding + ((float)(temp - m_minTemp) / (float)(m_maxTemp - m_minTemp) * effectiveWidth);
        y = padding + ((float)(m_maxSpeed - speed) / (float)(m_maxSpeed - m_minSpeed) * effectiveHeight);
    }

    void toValue(const QPoint& pos, float& temp, float& speed) const {
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);
        float nx = (pos.x() - padding) / effectiveWidth;
        float ny = (pos.y() - padding) / effectiveHeight;
        temp = m_minTemp + nx * (float)(m_maxTemp - m_minTemp);
        speed = m_maxSpeed - ny * (float)(m_maxSpeed - m_minSpeed);
    }

    int hitTest(const QPoint& pos) const {
        int best = -1;
        float bestDist = 12.0f * 12.0f;
        for (size_t i = 0; i < m_points.size(); ++i) {
            float x, y;
            toPixel(m_points[i].first, m_points[i].second, x, y);
            float dx = pos.x() - x;
            float dy = pos.y() - y;
            float d = dx * dx + dy * dy;
            if (d < bestDist) { bestDist = d; best = static_cast<int>(i); }
        }
        return best;
    }

private:
    static constexpr float kPadding = 40.0f;

    static float niceStep(float range) {
        const float raw = range / 6.0f;
        const float magnitude = std::pow(10.0f, std::floor(std::log10(raw)));
        const float norm = raw / magnitude;
        float mult;
        if (norm < 1.5f) mult = 1.0f;
        else if (norm < 3.5f) mult = 2.0f;
        else if (norm < 7.5f) mult = 5.0f;
        else mult = 10.0f;
        return mult * magnitude;
    }

    static std::vector<float> computeTicks(float min, float max) {
        std::vector<float> ticks;
        if (max < min) return ticks;
        const float range = max - min;
        if (range <= 0.0f) { ticks.push_back(min); return ticks; }
        float step = niceStep(range);
        // First tick aligned to a multiple of step >= min
        float first = std::floor(min / step) * step;
        if (first < min) first += step;
        const int maxCount = 12;
        for (float v = first; v <= max + step && (int)ticks.size() < maxCount; v += step) {
            if (v >= min - step * 0.5f && v <= max + step * 0.5f)
                ticks.push_back(v);
        }
        return ticks;
    }

    std::vector<std::pair<int, int>> m_points;
    int m_minTemp = 25;
    int m_maxTemp = 100;
    int m_minSpeed = 0;
    int m_maxSpeed = 100;
    int m_minDragSpeed = 0;
    int m_maxDragSpeed = 100;
    bool m_editable = false;
    int m_dragIndex = -1;
    double m_currentTemp = 0.0;
    int m_currentFan = 0;
    bool m_showCurrentPoint = false;
};

class GuiWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GuiWindow(GpuManager& manager,  int refreshRateMs, int refreshRateFPS, bool fpsOnly, bool hwRateProvided, bool fpsRateProvided, QWidget* parent = nullptr);
    ~GuiWindow() override;
    bool OpenWin();

private slots:
    void updateHardwareMetrics() const;
    void updateFpsMetrics() const;
    void onResetFpsClicked() const;
    void onApplyTuningClicked();
    void onVoltageOffsetToggled(bool checked) const;
    void updateApplyButtonEnabled() const;
    void onMonitoringToggled(bool enabled) const;
    void onHardwareStatsToggled(bool enabled) const;
    void onHwRefreshSliderChanged(int value) const;
    void onFpsRefreshSliderChanged(int value) const;
    void onApplyRefreshRateClicked();
    void onCurveEdited();

private:
    void initializeTuningControls();
    void initializeRefreshRateControls();
    void updateRefreshApplyButtonEnabled() const;
    std::vector<std::pair<int, int>> LoadSavedCurvePoints() const;
    void SaveCurvePoints(const std::vector<std::pair<int, int>>& points) const;
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;
    bool m_fpsOnlyMode = false;

    int m_refreshRate;
    int m_refreshRateFPS;

    int m_refreshRateCliDefault;
    int m_refreshRateFpsCliDefault;
    bool m_hwRateProvided;
    bool m_fpsRateProvided;

    bool m_baseZeroRpm;
    bool m_baseFanCurve;
    adlx_int m_baseVoltage;
    bool m_curveEdited = false;
    std::vector<std::pair<int, int>> m_stashedCustomPoints;

    CurvePlotWidget* m_curvePlot;
    Ui::GuiWindow *ui;
};
#endif //AMDFANSENSORS_WINDOW_H