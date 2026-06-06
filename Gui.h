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
    void updateApplyButtonEnabled() const; // Added method to refresh button state based on changes

private:
    void initializeTuningControls();
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;
    bool m_fpsOnlyMode = false;

    int m_refreshRate;
    int m_refreshRateFPS;

    // Baseline values to check for changes
    bool m_baseZeroRpm;
    bool m_baseFanCurve;
    adlx_int m_baseVoltage;

    Ui::GuiWindow *ui;
};
#endif //AMDFENSORS_WINDOW_H