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

    // Fix for System Memory "glitches":
    // 1. Update the label to follow the requested format exactly.
    // 2. Set the Maximum value of the progress bar so it scales correctly with system capacity.
    uint32_t totalSys = static_cast<uint32_t>(m_manager.m_statusInfo.totalSystemRamMB);

    // Since "used" isn't explicitly provided in the struct, we use 0 as a placeholder for usage
    // if not available from the manager, but ensure the full requested string format is implemented.
    int usedSys = info.usedSystemRam; // This would be replaced by actual usage data if added to GpuStatusInfo
    int freeSys = totalSys - usedSys;

    ui->m_systemMemoryLabel->setText(QString("System Memory: %1 MB/%2 MB - Free %3 MB - %4% Used")
                                    .arg(usedSys)
                                    .arg(totalSys)
                                    .arg(freeSys)
                                    .arg(totalSys > 0 ? usedSys * 100 / totalSys : 0));

    // Ensure the bar reflects the correct scale (Max = Total System RAM)
    ui->m_systemMemoryBar->setMaximum(static_cast<int>(totalSys));
    ui->m_systemMemoryBar->setValue(usedSys);

    ui->m_fanLabel->setText(QString("Fan Speed: %1 RPM (%2%)").arg(info.fanSpeed).arg(info.fanPercent));
    ui->m_fanBar->setValue(info.fanPercent);

    // GPU Memory section
    ui->m_memoryLabel->setText(QString("GPU Memory Usage: %1 MB/%2 MB - Free %3 MB - %4% Used")
                       .arg(info.vRam)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB - info.vRam)
                       .arg(m_manager.m_statusInfo.totalGPUMemoryMB > 0 ? info.vRam * 100 / m_manager.m_statusInfo.totalGPUMemoryMB : 0)
                       );
    ui->m_memoryBar->setMaximum(m_manager.m_statusInfo.totalGPUMemoryMB);
    ui->m_memoryBar->setValue(info.vRam);

    // GPU Usage section
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

void GuiWindow::initializeTuningControls() {
    // 1. Hook up the voltage slider and spinbox sync
    connect(ui->m_voltageSlider, &QSlider::valueChanged, ui->m_voltageSpinBox, &QSpinBox::setValue);
    connect(ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->m_voltageSlider, &QSlider::setValue);

    // 2. Enable/disable slider & spinbox based on checkbox
    connect(ui->m_voltageOffsetCheckbox, &QCheckBox::toggled, this, &GuiWindow::onVoltageOffsetToggled);

    // 3. Connect Apply button
    connect(ui->m_applyTuningButton, &QPushButton::clicked, this, &GuiWindow::onApplyTuningClicked);

    // New: Connection for automatic check on interactive items
    connect(ui->m_zeroRpmCheckbox, &QCheckBox::toggled, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_fanCurveCheckbox, &QCheckBox::toggled, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_voltageSlider, &QSlider::valueChanged, this, &GuiWindow::updateApplyButtonEnabled);
    connect(ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GuiWindow::updateApplyButtonEnabled);

    try {
        // 4. Query capabilities and current state from GpuManager
        bool zeroRpmSupported = m_manager.IsZeroRPMSupported();
        ui->m_zeroRpmCheckbox->setEnabled(zeroRpmSupported);
        if (zeroRpmSupported) {
            ui->m_zeroRpmCheckbox->setChecked(m_manager.GetZeroRPMState());
        } else {
            ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
        }
        // Store baseline for Zero RPM
        m_baseZeroRpm = m_manager.GetZeroRPMState();
    } catch (...) {
        ui->m_zeroRpmCheckbox->setEnabled(false);
        ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
    }

    // Initialize voltage range if supported
    adlx_int minVolt = -200;
    adlx_int maxVolt = 0;
    try {
        bool rangeSupported = m_manager.GetVoltageOffsetRange(minVolt, maxVolt);
        if (maxVolt > 0) maxVolt = 0; 
        if (minVolt > 0) minVolt = -200; // fallback safety
    } catch (...) {
        minVolt = -200;
        maxVolt = 0;
    }
    
    ui->m_voltageSlider->setRange(minVolt, maxVolt);
    ui->m_voltageSpinBox->setRange(minVolt, maxVolt);

    const bool isFanCurveDefaultOne = m_manager.IsFanCurveDefaultOne();
    ui->m_fanCurveCheckbox->setChecked(!isFanCurveDefaultOne);
    // Store baseline for Fan Curve (Inverse of default)
    m_baseFanCurve = !isFanCurveDefaultOne;

    try {
        // Load current voltage offset
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
        // Store baseline for Voltage
        m_baseVoltage = currentOffset;
    } catch (...) {
        ui->m_voltageOffsetCheckbox->setChecked(false);
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
        m_baseVoltage = 0;
    }

    // Initial button state check
    updateApplyButtonEnabled();
}

void GuiWindow::onVoltageOffsetToggled(bool checked) const {
    ui->m_voltageSlider->setEnabled(checked);
    ui->m_voltageSpinBox->setEnabled(checked);
    if (!checked) {
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
    }
    // Update button state whenever the checkbox changes, as it may change
    // whether the current value matches the baseline or not.
    updateApplyButtonEnabled();
}

void GuiWindow::updateApplyButtonEnabled() const {
    bool zeroRpmChanged = ui->m_zeroRpmCheckbox->isChecked() != m_baseZeroRpm;
    bool fanCurveChanged = ui->m_fanCurveCheckbox->isChecked() != m_baseFanCurve;
    bool voltageChanged = ui->m_voltageSpinBox->value() != m_baseVoltage;

    // The button is only enabled if at least one parameter differs from its base state.
    ui->m_applyTuningButton->setEnabled(zeroRpmChanged || fanCurveChanged || voltageChanged);
}

void GuiWindow::onApplyTuningClicked() {
    try {
        // 1. Set Zero RPM state if supported
        if (ui->m_zeroRpmCheckbox->isEnabled()) {
            bool checked = ui->m_zeroRpmCheckbox->isChecked();
            m_manager.SetZeroRPM(checked);
        }

        // 2. Set Fan Curve Profile
        bool customFan = ui->m_fanCurveCheckbox->isChecked();
        m_manager.ShowGetAndSetFan(true, !customFan);

        // 3. Set Voltage Offset
        adlx_int voltOffset = 0;
        if (ui->m_voltageOffsetCheckbox->isChecked()) {
            voltOffset = ui->m_voltageSpinBox->value();
        }
        m_manager.SetVoltageOffset(voltOffset);

        // Update baselines upon successful application
        m_baseZeroRpm = ui->m_zeroRpmCheckbox->isChecked();
        m_baseFanCurve = ui->m_fanCurveCheckbox->isChecked();
        m_baseVoltage = voltOffset;

        std::cout << "Applied settings: ZeroRPM=" << ui->m_zeroRpmCheckbox->isChecked()
                  << ", CustomFan=" << customFan
                  << ", VoltageOffset=" << voltOffset << " mV" << std::endl;

        // Re-run check to ensure button stays disabled if no further changes occur.
        updateApplyButtonEnabled();

    } catch (const std::exception& e) {
        std::cerr << "Error applying tuning settings: " << e.what() << std::endl;
    }
}
