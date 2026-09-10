#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QWidget>
#include <QPainter>
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
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Background
        painter.fillRect(rect(), QColor("#181825"));

        if (m_points.empty()) return;

        // Grid & Axes
        QPen axisPen(QColor("#45475A"), 1, Qt::SolidLine);
        painter.setPen(axisPen);
        painter.drawLine(0, height() - 1, width(), height() - 1); // X axis
        painter.drawLine(0, 0, 0, height() - 1); // Y axis

        // Legend - Labels
        painter.setPen(QPen(QColor("#BAC2DE"), 1));
        QFont labelFont = painter.font();
        labelFont.setPointSize(8);
        painter.setFont(labelFont);
        
        painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "Temp (°C)");
        painter.save();
        painter.translate(10, height() / 2);
        painter.rotate(-90);
        painter.drawText(rect(), Qt::AlignHCenter, "Fan (%)");
        painter.restore();

        // Draw Curve
        QPen curvePen(QColor("#89DCEB"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(curvePen);

        QPolygonF polyline;
        for (const auto& p : m_points) {
            float x = (float)p.first / 100.0f * width();
            float y = (float)(100 - p.second) / 100.0f * height();
            polyline << QPointF(x, y);
        }
        painter.drawPolyline(polyline);
    }
private:
    std::vector<std::pair<int, int>> m_points;
};

class GuiWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GuiWindow(GpuManager& manager,  int refreshRateMs, int refreshRateFPS, bool fpsOnly, QWidget* parent = nullptr);
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

private:
    void initializeTuningControls();
    void initializeRefreshRateControls();
    void updateRefreshApplyButtonEnabled() const;
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;
    bool m_fpsOnlyMode = false;

    int m_refreshRate;
    int m_refreshRateFPS;

    int m_refreshRateCliDefault;
    int m_refreshRateFpsCliDefault;

    bool m_baseZeroRpm;
    bool m_baseFanCurve;
    adlx_int m_baseVoltage;

    CurvePlotWidget* m_curvePlot;
    Ui::GuiWindow *ui;
};
#endif //AMDFANSENSORS_WINDOW_H