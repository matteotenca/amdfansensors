#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

#include <QMainWindow>
#include <QTimer>
#include "GpuManager.h"

namespace Ui {
    class GuiWindow;
}

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

    Ui::GuiWindow *ui;
};
#endif //AMDFANSENSORS_WINDOW_H