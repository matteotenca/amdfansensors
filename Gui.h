#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QProgressBar>
#include "GpuManager.h"

/**
 * @brief Class responsible for managing the rendering context and game loop.
 * Now utilizes Qt 6.7.2 for cross-platform UI management.
 */
class GuiWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GuiWindow(GpuManager& manager, QWidget* parent = nullptr);
    ~GuiWindow() override;

    int OpenWin(GpuManager& manager, int refreshRateMs, int refreshRateFPS = 1000, bool fpsOnly = false);

private slots:
    void updateHardwareMetrics();
    void updateFpsMetrics();

private:
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;
    bool m_fpsOnlyMode = false;

    // UI Structure Elements
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;

    QGroupBox* m_hardwareGroup;
    QGroupBox* m_fpsGroup;

    // Hardware Labels & Widgets
    QLabel* m_statusLabel; // Per la VRAM
    QLabel* m_gpuTempLabel;
    QLabel* m_hotspotTempLabel;
    QLabel* m_memTempLabel;
    QLabel* m_fanLabel;
    QLabel* m_mempryLabel;
    QProgressBar* m_memoryBar;
    QProgressBar* m_fanBar;

    // FPS Labels
    QLabel* m_instantFpsLabel;
    QLabel* m_avgFpsLabel;
    QLabel* m_minMaxFpsLabel;

    void setupUi();
    void applyStylesheet();
};
#endif //AMDFANSENSORS_WINDOW_H