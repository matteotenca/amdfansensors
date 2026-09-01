#include "Gui.h"
#include "ui_Gui.h"
#include <iostream>
#include <QString>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>

GuiWindow::GuiWindow(GpuManager& manager, int refreshRateMs, int refreshRateFPS, bool fpsOnly, QWidget* parent)
    : QMainWindow(parent), m_manager(manager), m_fpsOnlyMode(fpsOnly), m_refreshRate(refreshRateMs), m_refreshRateFPS(refreshRateFPS), ui(new Ui::GuiWindow)  {

    ui->setupUi(this);

    m_metricsTimer = new QTimer(this);
    m_fpsTimer = new QTimer(this);

    connect(m_metricsTimer, &QTimer::timeout, this, &GuiWindow::updateHardwareMetrics);
    connect(m_fpsTimer, &QTimer::timeout, this, &GuiWindow::updateFpsMetrics);
    connect(ui->m_resetFpsButton, &QPushButton::clicked, this, &GuiWindow::onResetFpsClicked);

    ui->m_enableMonitoringCheckbox->setChecked(false);
    connect(ui->m_enableMonitoringCheckbox, &QCheckBox::toggled, this, &GuiWindow::onMonitoringToggled);

    ui->m_enableHardwareStatsCheckbox->setChecked(true);
    connect(ui->m_enableHardwareStatsCheckbox, &QCheckBox::toggled, this, &GuiWindow::onHardwareStatsToggled);

    onHardwareStatsToggled(ui->m_enableHardwareStatsCheckbox->isChecked());
    initializeTuningControls();

    if (m_fpsOnlyMode) {
        ui->m_hardwareGroup->hide();
        ui->m_controlGroup->hide();
    }

    setWindowTitle(QString("AMD GPU Monitor (Qt 6.7) - -a=%1 -A=%2").arg(m_refreshRate).arg(m_refreshRateFPS));
}

GuiWindow::~GuiWindow() {
    delete ui;
}

void GuiWindow::onResetFpsClicked() const {
    m_manager.m_temperatureInfo.fpsHistory.reset();
    m_manager.m_temperatureInfo.maxFPS = 0;
    m_manager.m_temperatureInfo.minFPS = 0;
    m_manager.m_temperatureInfo.totalFPS = 0;
    m_manager.m_temperatureInfo.instantFPS = 0;
    m_manager.m_temperatureInfo.avgFPS = 0;
    m_manager.m_temperatureInfo.fullCounter = 0;
}

void GuiWindow::updateHardwareMetrics() const {
    if (m_fpsOnlyMode) return;

    m_manager.UpdateGpuTempStruct(false);
    auto info = m_manager.m_temperatureInfo;

    ui->m_gpuTempLabel->setText(QString("GPU: %1°C").arg(info.gpuTemperatureC, 4, 'f', 0));
    ui->m_hotspotTempLabel->setText(QString("GPU Hotspot: %1°C").arg(info.gpuHotspotTemperatureC, 4, 'f', 0));
    ui->m_memTempLabel->setText(QString("GPU Memory: %1°C").arg(info.gpuMemoryTemperatureC, 4, 'f', 0));
    ui->m_statusLabel->setText(QString("VRAM Used: %1 MB/%2 MB").arg(info.vRam).arg(m_manager.m_statusInfo.totalGPUMemoryMB));

    uint32_t totalSys = static_cast<uint32_t>(m_manager.m_statusInfo.totalSystemRamMB);
    int usedSys = info.usedSystemRam;
    int freeSys = totalSys - usedSys;

    ui->m_systemMemoryLabel->setText(QString("System Memory: %1 MB/%2 MB - Free %3 MB - %4% Used")
                                    .arg(usedSys)
                                    .arg(totalSys)
                                    .arg(freeSys)
                                    .arg(totalSys > 0 ? usedSys * 100 / totalSys : 0));

    ui->m_systemMemoryBar->setMaximum(static_cast<int>(totalSys));
    ui->m_systemMemoryBar->setValue(usedSys);

    ui->m_fanLabel->setText(QString("Fan Speed: %1 RPM (%2%)").arg(info.fanSpeed).arg(info.fanPercent));
    ui->m_fanBar->setValue(info.fanPercent);

    ui->m_memoryLabel->setText(QString("GPU Memory Usage: %1 MB/%2 MB - Free %3 MB - %4% Used")
                       .arg(info.vRam)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB - info.vRam)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB > 0 ? info.vRam * 100 / m_manager.m_statusInfo.totalGPUMemoryMB : 0)
                       );
    ui->m_memoryBar->setMaximum(m_manager.m_statusInfo.totalGPUMemoryMB);
    ui->m_memoryBar->setValue(info.vRam);

    ui->m_gpuUsageLabel->setText(QString("GPU Usage: %1% - Power %2W")
        .arg(info.gpuUsage, 0, 'f', 0)
        .arg(info.gpuPower, 0, 'f', 0));
    ui->gpuUsageProgressBar->setValue(static_cast<int>(info.gpuUsage));
}

void GuiWindow::updateFpsMetrics() const {
    m_manager.UpdateGpuTempStruct(true);
    auto info = m_manager.m_temperatureInfo;

    ui->m_instantFpsLabel->setText(QString("Current FPS: %1 / Ring: %2 / Unattended: %3")
        .arg(info.instantFPS, 2, 'f', 0)
        .arg(info.fullCounter, 2, 'd', 0)
        .arg(info.totalUnattended, 2, 'd', 0));
    ui->m_avgFpsLabel->setText(QString("Average: %1").arg(info.avgFPS, 2, 'f', 0));
    ui->m_minMaxFpsLabel->setText(QString("Min: %1 | Max: %2").arg(info.minFPS).arg(info.maxFPS));
}

bool GuiWindow::OpenWin() {
    try {
        onMonitoringToggled(ui->m_enableMonitoringCheckbox->isChecked());
        onHardwareStatsToggled(ui->m_enableHardwareStatsCheckbox->isChecked());
        this->show();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void GuiWindow::initializeTuningControls() {
    connect(ui->m_voltageSlider, &QSlider::valueChanged, ui->m_voltageSpinBox, &QSpinBox::setValue);
    connect(ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->m_voltageSlider, &QSlider::setValue);

    connect(ui->m_voltageOffsetCheckbox, &QCheckBox::toggled, this, &GuiWindow::onVoltageOffsetToggled);

    connect(ui->m_applyTuningButton, &QPushButton::clicked, this, &GuiWindow::onApplyTuningClicked);

    connect(ui->m_zeroRpmCheckbox, &QCheckBox::toggled, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_fanCurveCheckbox, &QCheckBox::toggled, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_voltageSlider, &QSlider::valueChanged, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GuiWindow::updateApplyButtonEnabled);

    try {
        bool zeroRpmSupported = m_manager.IsZeroRPMSupported();
        ui->m_zeroRpmCheckbox->setEnabled(zeroRpmSupported);
        if (zeroRpmSupported) {
            ui->m_zeroRpmCheckbox->setChecked(m_manager.GetZeroRPMState());
        } else {
            ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
        }
        m_baseZeroRpm = m_manager.GetZeroRPMState();
    } catch (...) {
        ui->m_zeroRpmCheckbox->setEnabled(false);
        ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
    }

    adlx_int minVolt = -200;
    adlx_int maxVolt = 0;
    try {
        m_manager.GetVoltageOffsetRange(minVolt, maxVolt);
        if (maxVolt > 0) maxVolt = 0; 
        if (minVolt > 0) minVolt = -200;
    } catch (...) {
        minVolt = -200;
        maxVolt = 0;
    }
    
    ui->m_voltageSlider->setRange(minVolt, maxVolt);
    ui->m_voltageSpinBox->setRange(minVolt, maxVolt);

    const bool isFanCurveDefaultOne = m_manager.IsFanCurveDefaultOne();
    ui->m_fanCurveCheckbox->setChecked(!isFanCurveDefaultOne);
    m_baseFanCurve = !isFanCurveDefaultOne;

    try {
        adlx_int currentOffset = m_manager.GetVoltageOffset();
        if (currentOffset != 0) {
            ui->m_voltageOffsetCheckbox->setChecked(true);
            ui->m_voltageSlider->setValue(currentOffset);
            ui->m_voltageSpinBox->setValue(currentOffset);
            ui->m_voltageSlider->setEnabled(true);
            ui->m_voltageSpinBox->setEnabled(true);
        } else {
            ui->m_voltageOffsetCheckbox->setChecked(false);
            ui->m_voltageSlider->setValue(0);
            ui->m_voltageSpinBox->setValue(0);
        }
        m_baseVoltage = currentOffset;
    } catch (...) {
        ui->m_voltageOffsetCheckbox->setChecked(false);
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
        m_baseVoltage = 0;
    }

    updateApplyButtonEnabled();
}

void GuiWindow::onVoltageOffsetToggled(bool checked) const {
    ui->m_voltageSlider->setEnabled(checked);
    ui->m_voltageSpinBox->setEnabled(checked);
    if (!checked) {
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
    }
    updateApplyButtonEnabled();
}

void GuiWindow::updateApplyButtonEnabled() const {
    bool zeroRpmChanged = ui->m_zeroRpmCheckbox->isChecked() != m_baseZeroRpm;
    bool fanCurveChanged = ui->m_fanCurveCheckbox->isChecked() != m_baseFanCurve;
    bool voltageChanged = ui->m_voltageSpinBox->value() != m_baseVoltage;

    ui->m_applyTuningButton->setEnabled(zeroRpmChanged || fanCurveChanged || voltageChanged);
}

void GuiWindow::onApplyTuningClicked() {
    try {
        if (ui->m_zeroRpmCheckbox->isEnabled()) {
            bool checked = ui->m_zeroRpmCheckbox->isChecked();
            m_manager.SetZeroRPM(checked);
        }

        bool customFan = ui->m_fanCurveCheckbox->isChecked();
        m_manager.ShowGetAndSetFan(true, !customFan);

        adlx_int voltOffset = 0;
        if (ui->m_voltageOffsetCheckbox->isChecked()) {
            voltOffset = ui->m_voltageSpinBox->value();
        }
        m_manager.SetVoltageOffset(voltOffset);

        m_baseZeroRpm = ui->m_zeroRpmCheckbox->isChecked();
        m_baseFanCurve = ui->m_fanCurveCheckbox->isChecked();
        m_baseVoltage = voltOffset;

        std::cout << "Applied settings: ZeroRPM=" << ui->m_zeroRpmCheckbox->isChecked()
                  << ", CustomFan=" << customFan
                  << ", VoltageOffset=" << voltOffset << " mV" << std::endl;

        updateApplyButtonEnabled();

    } catch (const std::exception& e) {
        std::cerr << "Error applying tuning settings: " << e.what() << std::endl;
    }
}

void GuiWindow::onMonitoringToggled(bool enabled) const {
    if (enabled) {
        m_fpsTimer->start(m_refreshRateFPS);
    } else {
        m_fpsTimer->stop();
    }
}

void GuiWindow::onHardwareStatsToggled(bool enabled) const {
    if (enabled && !m_fpsOnlyMode) {
        m_metricsTimer->start(m_refreshRate);
    } else {
        m_metricsTimer->stop();
        ui->m_gpuTempLabel->setText("GPU: --°C");
        ui->m_hotspotTempLabel->setText("Hotspot: --°C");
        ui->m_memTempLabel->setText("Memory: --°C");
        ui->m_statusLabel->setText("VRAM Used: -- MB");
        ui->m_systemMemoryLabel->setText("System: -- MB");
        ui->m_systemMemoryBar->setValue(0);
        ui->m_fanLabel->setText("Fan Speed: -- RPM (0%)");
        ui->m_fanBar->setValue(0);
        ui->m_memoryLabel->setText("GPU Mem: -- MB");
        ui->m_memoryBar->setValue(0);
        ui->m_gpuUsageLabel->setText("GPU Usage: --%");
        ui->gpuUsageProgressBar->setValue(0);
    }
}
