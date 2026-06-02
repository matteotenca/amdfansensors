#include "Gui.h"
#include "ui_Gui.h"
#include <iostream>
#include <QString>

GuiWindow::GuiWindow(GpuManager& manager, int refreshRateMs, int refreshRateFPS, bool fpsOnly, QWidget* parent)
    : QMainWindow(parent), m_manager(manager), m_fpsOnlyMode(fpsOnly), m_refreshRate(refreshRateMs), m_refreshRateFPS(refreshRateFPS), ui(new Ui::GuiWindow)  {

    ui->setupUi(this);

    m_metricsTimer = new QTimer(this);
    m_fpsTimer = new QTimer(this);

    connect(m_metricsTimer, &QTimer::timeout, this, &GuiWindow::updateHardwareMetrics);
    connect(m_fpsTimer, &QTimer::timeout, this, &GuiWindow::updateFpsMetrics);
    connect(ui->m_resetFpsButton, &QPushButton::clicked, this, &GuiWindow::onResetFpsClicked);

    if (m_fpsOnlyMode) {
        ui->m_hardwareGroup->hide();
    }

    setWindowTitle(QString("AMD GPU Monitor (Qt 6.7) - -A=%1 -a=%2").arg(m_refreshRate).arg(m_refreshRateFPS));
}

GuiWindow::~GuiWindow() {
    delete ui;
}

void GuiWindow::onResetFpsClicked() {
    m_manager.m_temperatureInfo.maxFPS = 0;
    m_manager.m_temperatureInfo.minFPS = 0;
    m_manager.m_temperatureInfo.totalFPS = 0;
    m_manager.m_temperatureInfo.instantFPS = 0;
    m_manager.m_temperatureInfo.avgFPS = 0;
    m_manager.m_temperatureInfo.fullCounter = 0;
    m_manager.m_temperatureInfo.fpsHistory.reset();
}

void GuiWindow::updateHardwareMetrics() {
    if (m_fpsOnlyMode) return;

    m_manager.UpdateGpuTempStruct(false);
    auto info = m_manager.m_temperatureInfo;

    ui->m_gpuTempLabel->setText(QString("GPU: %1°C").arg(info.gpuTemperatureC, 4, 'f', 0));
    ui->m_hotspotTempLabel->setText(QString("Hotspot: %1°C").arg(info.gpuHotspotTemperatureC, 4, 'f', 0));
    ui->m_memTempLabel->setText(QString("Memory: %1°C").arg(info.gpuMemoryTemperatureC, 4, 'f', 0));
    ui->m_statusLabel->setText(QString("VRAM Used: %1 MB/%2 MB").arg(info.vRam).arg(m_manager.m_statusInfo.totalGPUMemoryMB));

    ui->m_fanLabel->setText(QString("Fan Speed: %1 RPM (%2%)").arg(info.fanSpeed).arg(info.fanPercent));
    ui->m_fanBar->setValue(info.fanPercent);
    ui->m_mempryLabel->setText(QString("Memory Usage: %1 MB/%2 MB - Free %3 MB").arg(info.vRam).arg(m_manager.m_statusInfo.totalGPUMemoryMB).arg(m_manager.m_statusInfo.totalGPUMemoryMB - info.vRam));
    ui->m_memoryBar->setMaximum(m_manager.m_statusInfo.totalGPUMemoryMB);
    ui->m_memoryBar->setValue(info.vRam);
    ui->m_gpuUsageLabel->setText(QString("GPU Usage: %1% - Power %2W")
        .arg(info.gpuUsage, 0, 'f', 0)
        .arg(info.gpuPower, 0, 'f', 0)); // Update the new label
    ui->gpuUsageProgressBar->setValue(static_cast<int>(info.gpuUsage));
}

void GuiWindow::updateFpsMetrics() {
    m_manager.UpdateGpuTempStruct(true);
    auto info = m_manager.m_temperatureInfo;

    ui->m_instantFpsLabel->setText(QString("Current FPS: %1 / Unattended: %2")
        .arg(info.instantFPS, 2, 'f', 0)
        .arg(info.fullCounter, 2, 'd', 0));
    ui->m_avgFpsLabel->setText(QString("Average: %1").arg(info.avgFPS, 2, 'f', 0));
    ui->m_minMaxFpsLabel->setText(QString("Min: %1 | Max: %2").arg(info.minFPS).arg(info.maxFPS));
}

bool GuiWindow::OpenWin() {
    try {
        m_metricsTimer->start(m_refreshRate);
        m_fpsTimer->start(m_refreshRateFPS);
        this->show();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}
