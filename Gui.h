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

    // int OpenWin(GpuManager& manager, int refreshRateMs, int refreshRateFPS = 1000, bool fpsOnly = false);
    bool OpenWin();

private slots:
    void updateHardwareMetrics();
    void updateFpsMetrics();
    void onResetFpsClicked();

private:
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;
    bool m_fpsOnlyMode = false;

    int m_refreshRate;
    int m_refreshRateFPS;

    Ui::GuiWindow *ui;
};
#endif //AMDFANSENSORS_WINDOW_H