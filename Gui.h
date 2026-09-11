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

        // Padding and effective area
        const float padding = 25.0f;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);

        // Grid & Axes
        QPen axisPen(QColor("#45475A"), 1, Qt::SolidLine);
        painter.setPen(axisPen);
        // X axis shifted to bottom padding
        painter.drawLine(padding, height() - padding, width() - padding, height() - padding); 
        // Y axis shifted to left padding
        painter.drawLine(padding, padding, padding, height() - padding);

        // Legend - Labels
        painter.setPen(QPen(QColor("#BAC2DE"), 1));
        QFont labelFont = painter.font();
        labelFont.setPointSize(8);
        painter.setFont(labelFont);
        
        painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "Temp (°C)");
        painter.save();
        painter.translate(padding / 2, height() / 2);
        painter.rotate(-90);
        // Draw the vertical label centered on the rotated origin using font metrics
        const QString fanLabel = "Fan (%)";
        const QFontMetrics fm(labelFont);
        painter.drawText(-fm.horizontalAdvance(fanLabel) / 2.0,
                         (fm.descent() - fm.ascent()) / 2.0,
                         fanLabel);
        painter.restore();

        // Draw Curve
        QPen curvePen(QColor("#89DCEB"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(curvePen);

        QPolygonF polyline;
        for (const auto& p : m_points) {
            float x = padding + ((float)p.first / 100.0f * effectiveWidth);
            float y = padding + ((float)(100 - p.second) / 100.0f * effectiveHeight);
            polyline << QPointF(x, y);
        }
        painter.drawPolyline(polyline);

        // Draw Point Markers and Labels
        for (size_t i = 0; i < m_points.size(); ++i) {
            const auto& p = m_points[i];
            float x = padding + ((float)p.first / 100.0f * effectiveWidth);
            float y = padding + ((float)(100 - p.second) / 100.0f * effectiveHeight);
            
            // Marker
            painter.setBrush(QBrush(QColor("#89DCEB")));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(x, y), 3, 3);

            // Coordinate Text
            painter.setPen(QPen(QColor("#CBA6F7"), 1));
            painter.setFont(labelFont);
            QString label = QString("%1°C / %2%").arg(p.first).arg(p.second);
            
            float textX = x + 25;
            float textY = y;
            painter.drawText(textX, textY, label);
        }
    }
private:
    std::vector<std::pair<int, int>> m_points;
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
    bool m_hwRateProvided;
    bool m_fpsRateProvided;

    bool m_baseZeroRpm;
    bool m_baseFanCurve;
    adlx_int m_baseVoltage;

    CurvePlotWidget* m_curvePlot;
    Ui::GuiWindow *ui;
};
#endif //AMDFANSENSORS_WINDOW_H