#include "Gui.h"
#include "ui_Gui.h"
#include <iostream>
#include <QString>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QMessageBox>
#include <QSettings>

GuiWindow::GuiWindow(GpuManager& manager, int refreshRateMs, int refreshRateFPS, bool fpsOnly, bool hwRateProvided, bool fpsRateProvided, QWidget* parent)
    : QMainWindow(parent), m_manager(manager), m_fpsOnlyMode(fpsOnly), m_refreshRate(refreshRateMs), m_refreshRateFPS(refreshRateFPS), m_refreshRateCliDefault(refreshRateMs), m_refreshRateFpsCliDefault(refreshRateFPS), m_hwRateProvided(hwRateProvided), m_fpsRateProvided(fpsRateProvided), ui(new Ui::GuiWindow)  {

    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/icon.ico"));

    m_metricsTimer = new QTimer(this);
    m_fpsTimer = new QTimer(this);

    connect(m_metricsTimer, &QTimer::timeout, this, &GuiWindow::updateHardwareMetrics);
    connect(m_fpsTimer, &QTimer::timeout, this, &GuiWindow::updateFpsMetrics);
    connect(ui->m_resetFpsButton, &QPushButton::clicked, this, &GuiWindow::onResetFpsClicked);

    ui->m_enableMonitoringCheckbox->setChecked(false);
    connect(ui->m_enableMonitoringCheckbox, &QCheckBox::toggled, this, &GuiWindow::onMonitoringToggled);

    ui->m_enableHardwareStatsCheckbox->setChecked(true);
    connect(ui->m_enableHardwareStatsCheckbox, &QCheckBox::toggled, this, &GuiWindow::onHardwareStatsToggled);

    // Initialize Curve Plot Widget
    m_curvePlot = new CurvePlotWidget(this);
    ui->controlLayout->addWidget(m_curvePlot, 3, 0, 1, 2);
    m_curvePlot->setMinimumHeight(150);
    m_curvePlot->setPoints(m_manager.GetFanCurvePoints());

    // Let the curve section grow with the window; keep other sections fixed.
    ui->controlLayout->setRowStretch(3, 1);
    ui->m_mainLayout->setStretch(0, 0);
    ui->m_mainLayout->setStretch(1, 1);
    ui->m_mainLayout->setStretch(2, 0);
    ui->m_mainLayout->setStretch(3, 0);
    ui->m_hardwareGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->m_fpsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->m_refreshRateGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // The custom-curve preview stash: what re-checking the box shows before Apply.
    // Seeds from the GPU when it holds a custom curve, otherwise from saved prefs.
    m_stashedCustomPoints = m_manager.IsDefaultFanCurve(m_curvePlot->points())
                                ? LoadSavedCurvePoints()
                                : m_curvePlot->points();

    int minTemp = 0, maxTemp = 100, minSpeed = 0, maxSpeed = 100;
    m_manager.GetFanRanges(minTemp, maxTemp, minSpeed, maxSpeed);
    const int dragMinSpeed = minSpeed;   // GPU's real minimum, kept for dragging
    minSpeed = 0;                        // display axis starts at 0%
    m_curvePlot->setRanges(minTemp, maxTemp, minSpeed, maxSpeed);
    m_curvePlot->setDragSpeedRange(dragMinSpeed, maxSpeed);
    m_curvePlot->setEditable(ui->m_fanCurveCheckbox->isChecked());
    connect(m_curvePlot, &CurvePlotWidget::pointsChanged, this, &GuiWindow::onCurveEdited);
    connect(ui->m_fanCurveCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        m_curvePlot->setEditable(checked);
        if (checked) {
            // Re-checking restores the custom-curve preview; nothing touches the GPU.
            if (!m_stashedCustomPoints.empty())
                m_curvePlot->setPoints(m_stashedCustomPoints);
        } else {
            // Unchecking previews the default curve; the custom one is stashed.
            m_stashedCustomPoints = m_curvePlot->points();
            m_curvePlot->setPoints(m_manager.GetDefaultFanCurvePoints());
        }
    });

    ui->controlLayout->activate();
    ui->m_controlGroup->update();

    onHardwareStatsToggled(ui->m_enableHardwareStatsCheckbox->isChecked());
    initializeTuningControls();
    initializeRefreshRateControls();

    if (m_fpsOnlyMode) {
        ui->m_hardwareGroup->hide();
        ui->m_controlGroup->hide();
        ui->m_hwRefreshLabelTitle->setEnabled(false);
        ui->m_hwRefreshSlider->setEnabled(false);
        ui->m_hwRefreshValueLabel->setEnabled(false);
    }

    setWindowTitle(QString("AMD GPU Monitor (Qt 6.7) - -a=%1 -A=%2").arg(m_refreshRate).arg(m_refreshRateFPS));
}

GuiWindow::~GuiWindow() {
    delete ui;
}

void GuiWindow::onResetFpsClicked() const {
    m_manager.ResetFpsStats();
}

void GuiWindow::updateHardwareMetrics() const {
    if (m_fpsOnlyMode) return;

    m_manager.UpdateGpuTempStruct(false);
    const auto& info = m_manager.GetTemperatureInfo();

    ui->m_gpuTempLabel->setText(QString("GPU: %1°C").arg(info.gpuTemperatureC, 4, 'f', 0));
    ui->m_hotspotTempLabel->setText(QString("GPU Hotspot: %1°C").arg(info.gpuHotspotTemperatureC, 4, 'f', 0));
    ui->m_memTempLabel->setText(QString("GPU Memory: %1°C").arg(info.gpuMemoryTemperatureC, 4, 'f', 0));
    ui->m_statusLabel->setText(QString("VRAM Used: %1 MB/%2 MB").arg(info.vRam).arg(m_manager.GetStatusInfo().totalGPUMemoryMB));

    uint32_t totalSys = static_cast<uint32_t>(m_manager.GetStatusInfo().totalSystemRamMB);
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
    m_curvePlot->setCurrentPoint(info.gpuHotspotTemperatureC, info.fanPercent);

    ui->m_memoryLabel->setText(QString("GPU Memory Usage: %1 MB/%2 MB - Free %3 MB - %4% Used")
                       .arg(info.vRam)
                       .arg(m_manager.GetStatusInfo().totalGPUMemoryMB)
                       .arg(m_manager.GetStatusInfo().totalGPUMemoryMB - info.vRam)
                       .arg(m_manager.GetStatusInfo().totalGPUMemoryMB > 0 ? info.vRam * 100 / m_manager.GetStatusInfo().totalGPUMemoryMB : 0)
                       );
    ui->m_memoryBar->setMaximum(m_manager.GetStatusInfo().totalGPUMemoryMB);
    ui->m_memoryBar->setValue(info.vRam);

    ui->m_gpuUsageLabel->setText(QString("GPU Usage: %1% - Power %2W")
        .arg(info.gpuUsage, 0, 'f', 0)
        .arg(info.gpuPower, 0, 'f', 0));
    ui->gpuUsageProgressBar->setValue(static_cast<int>(info.gpuUsage));
}

void GuiWindow::updateFpsMetrics() const {
    m_manager.UpdateGpuTempStruct(true);
    const auto& info = m_manager.GetTemperatureInfo();

    ui->m_instantFpsLabel->setText(QString("Current FPS: %1 / Ring: %2 / Unattended: %3")
        .arg(info.instantFPS, 2)
        .arg(info.fullCounter, 2)
        .arg(info.totalUnattended, 2));
    ui->m_avgFpsLabel->setText(QString("Average: %1").arg(info.avgFPS, 2));
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
    } catch (const std::exception& e) {
        std::cerr << "[Gui] Failed to query ZeroRPM support: " << e.what() << std::endl;
        ui->m_zeroRpmCheckbox->setEnabled(false);
        ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
    } catch (...) {
        std::cerr << "[Gui] Failed to query ZeroRPM support: unknown error" << std::endl;
        ui->m_zeroRpmCheckbox->setEnabled(false);
        ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
    }

    adlx_int minVolt = -200;
    adlx_int maxVolt = 0;
    try {
        m_manager.GetVoltageOffsetRange(minVolt, maxVolt);
        if (maxVolt > 0) maxVolt = 0; 
        if (minVolt > 0) minVolt = -200;
    } catch (const std::exception& e) {
        std::cerr << "[Gui] Failed to query voltage offset range: " << e.what() << std::endl;
        minVolt = -200;
        maxVolt = 0;
    } catch (...) {
        std::cerr << "[Gui] Failed to query voltage offset range: unknown error" << std::endl;
        minVolt = -200;
        maxVolt = 0;
    }
    
    ui->m_voltageSlider->setRange(minVolt, maxVolt);
    ui->m_voltageSpinBox->setRange(minVolt, maxVolt);

    const bool isFanCurveDefaultOne = m_manager.IsFanCurveDefaultOne();
    ui->m_fanCurveCheckbox->setChecked(!isFanCurveDefaultOne);
    m_baseFanCurve = !isFanCurveDefaultOne;

    // Startup sync: when a custom curve is active on the GPU and differs from the
    // saved prefs, the GPU curve wins and prefs are refreshed to match it.
    if (!isFanCurveDefaultOne) {
        const auto gpuPoints = m_curvePlot->points();
        if (LoadSavedCurvePoints() != gpuPoints) {
            SaveCurvePoints(gpuPoints);
            std::cout << "Synced saved custom fan curve with the GPU curve." << std::endl;
        }
    }

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
    } catch (const std::exception& e) {
        std::cerr << "[Gui] Failed to query current voltage offset: " << e.what() << std::endl;
        ui->m_voltageOffsetCheckbox->setChecked(false);
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
        m_baseVoltage = 0;
    } catch (...) {
        std::cerr << "[Gui] Failed to query current voltage offset: unknown error" << std::endl;
        ui->m_voltageOffsetCheckbox->setChecked(false);
        ui->m_voltageSlider->setValue(0);
        ui->m_voltageSpinBox->setValue(0);
        m_baseVoltage = 0;
    }

    updateApplyButtonEnabled();
}

void GuiWindow::initializeRefreshRateControls() {
    ui->m_hwRefreshSlider->setRange(100, 5000);
    ui->m_fpsRefreshSlider->setRange(100, 5000);

    QSettings settings("AmdFanSensors", "AmdFanSensors");
    int savedHwRate = settings.value("refreshRate", 1000).toInt();
    int savedFpsRate = settings.value("refreshRateFPS", 1000).toInt();

    // Use CLI-provided value when given; otherwise fall back to the saved value.
    int hwRate = m_hwRateProvided ? m_refreshRateCliDefault : savedHwRate;
    int fpsRate = m_fpsRateProvided ? m_refreshRateFpsCliDefault : savedFpsRate;

    m_refreshRateCliDefault = hwRate;
    m_refreshRateFpsCliDefault = fpsRate;
    m_refreshRate = hwRate;
    m_refreshRateFPS = fpsRate;

    ui->m_hwRefreshSlider->setValue(hwRate);
    ui->m_hwRefreshValueLabel->setText(QString("%1 ms").arg(hwRate));

    ui->m_fpsRefreshSlider->setValue(fpsRate);
    ui->m_fpsRefreshValueLabel->setText(QString("%1 ms").arg(fpsRate));

    connect(ui->m_hwRefreshSlider, &QSlider::valueChanged, this, &GuiWindow::onHwRefreshSliderChanged);
    connect(ui->m_fpsRefreshSlider, &QSlider::valueChanged, this, &GuiWindow::onFpsRefreshSliderChanged);
    connect(ui->m_applyRefreshRateButton, &QPushButton::clicked, this, &GuiWindow::onApplyRefreshRateClicked);

    updateRefreshApplyButtonEnabled();
}

void GuiWindow::updateRefreshApplyButtonEnabled() const {
    bool hwRateChanged = ui->m_hwRefreshSlider->value() != m_refreshRateCliDefault;
    bool fpsRateChanged = ui->m_fpsRefreshSlider->value() != m_refreshRateFpsCliDefault;

    ui->m_applyRefreshRateButton->setEnabled(hwRateChanged || fpsRateChanged);
}

void GuiWindow::onHwRefreshSliderChanged(int value) const {
    ui->m_hwRefreshValueLabel->setText(QString("%1 ms").arg(value));
    updateRefreshApplyButtonEnabled();
}

void GuiWindow::onFpsRefreshSliderChanged(int value) const {
    ui->m_fpsRefreshValueLabel->setText(QString("%1 ms").arg(value));
    updateRefreshApplyButtonEnabled();
}

void GuiWindow::onApplyRefreshRateClicked() {
    m_refreshRate = ui->m_hwRefreshSlider->value();
    m_refreshRateFPS = ui->m_fpsRefreshSlider->value();

    if (m_metricsTimer->isActive())
        m_metricsTimer->start(m_refreshRate);
    if (m_fpsTimer->isActive())
        m_fpsTimer->start(m_refreshRateFPS);

    m_refreshRateCliDefault = m_refreshRate;
    m_refreshRateFpsCliDefault = m_refreshRateFPS;

    QSettings settings("AmdFanSensors", "AmdFanSensors");
    settings.setValue("refreshRate", m_refreshRate);
    settings.setValue("refreshRateFPS", m_refreshRateFPS);

    updateRefreshApplyButtonEnabled();
    setWindowTitle(QString("AMD GPU Monitor (Qt 6.7) - -a=%1 -A=%2").arg(m_refreshRate).arg(m_refreshRateFPS));
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

    ui->m_applyTuningButton->setEnabled(zeroRpmChanged || fanCurveChanged || voltageChanged || m_curveEdited);
}

void GuiWindow::onCurveEdited() {
    m_curveEdited = true;
    updateApplyButtonEnabled();
}

void GuiWindow::onApplyTuningClicked() {
    try {
        if (ui->m_zeroRpmCheckbox->isEnabled()) {
            bool checked = ui->m_zeroRpmCheckbox->isChecked();
            m_manager.SetZeroRPM(checked);
        }

        bool customFan = ui->m_fanCurveCheckbox->isChecked();
        bool curveApplied = false;
        if (customFan) {
            curveApplied = m_manager.SetFanCurvePoints(m_curvePlot->points());
        } else {
            // Unchecked: apply the default curve to the GPU. The saved custom
            // curve in prefs is retained for future re-use.
            curveApplied = m_manager.SetFanCurvePoints(m_manager.GetDefaultFanCurvePoints());
        }

        adlx_int voltOffset = 0;
        if (ui->m_voltageOffsetCheckbox->isChecked()) {
            voltOffset = ui->m_voltageSpinBox->value();
        }
        m_manager.SetVoltageOffset(voltOffset);

        m_baseZeroRpm = ui->m_zeroRpmCheckbox->isChecked();
        m_baseFanCurve = ui->m_fanCurveCheckbox->isChecked();
        m_baseVoltage = voltOffset;
        m_curveEdited = false;

        std::cout << "Applied settings: ZeroRPM=" << ui->m_zeroRpmCheckbox->isChecked()
                  << ", CustomFan=" << customFan
                  << ", VoltageOffset=" << voltOffset << " mV" << std::endl;
        const auto gpuPoints = m_manager.GetFanCurvePoints();
        m_curvePlot->setPoints(gpuPoints);
        if (!m_manager.IsDefaultFanCurve(gpuPoints)) {
            m_stashedCustomPoints = gpuPoints;
        }
        if (customFan && curveApplied) {
            SaveCurvePoints(gpuPoints);
        }
        updateApplyButtonEnabled();

        ui->m_applyTuningButton->setText("Applied!");
        QTimer::singleShot(2500, ui->m_applyTuningButton, [this]() { ui->m_applyTuningButton->setText("Apply Tuning Settings"); });

    } catch (const std::exception& e) {
        std::cerr << "Error applying tuning settings: " << e.what() << std::endl;
        QMessageBox::critical(this, "Tuning Error",
            QString("Failed to apply tuning settings:\n%1").arg(e.what()));
    }
    catch (...) {
        std::cerr << "Error applying tuning settings: unknown error" << std::endl;
        QMessageBox::critical(this, "Tuning Error", "Failed to apply tuning settings: unknown error.");
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
        m_curvePlot->setCurrentPointVisible(true);
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
        m_curvePlot->setCurrentPointVisible(false);
    }
}

void GuiWindow::SaveCurvePoints(const std::vector<std::pair<int, int>>& points) const {
    if (points.empty()) return;
    // Preferences must only ever hold a custom curve; never persist the default one.
    if (m_manager.IsDefaultFanCurve(points)) return;

    QVariantList temps;
    QVariantList speeds;
    for (const auto& p : points) {
        temps << p.first;
        speeds << p.second;
    }

    QSettings settings("AmdFanSensors", "AmdFanSensors");
    settings.setValue("fanCurveTemps", temps);
    settings.setValue("fanCurveSpeeds", speeds);
}

std::vector<std::pair<int, int>> GuiWindow::LoadSavedCurvePoints() const {
    std::vector<std::pair<int, int>> points;

    QSettings settings("AmdFanSensors", "AmdFanSensors");
    if (!settings.contains("fanCurveTemps") || !settings.contains("fanCurveSpeeds"))
        return points;

    const QVariantList temps = settings.value("fanCurveTemps").toList();
    const QVariantList speeds = settings.value("fanCurveSpeeds").toList();
    if (temps.isEmpty() || temps.size() != speeds.size())
        return points;

    points.reserve(temps.size());
    for (int i = 0; i < temps.size(); ++i) {
        bool okT = false, okS = false;
        const int t = temps[i].toInt(&okT);
        const int s = speeds[i].toInt(&okS);
        if (!okT || !okS) return {};
        points.emplace_back(t, s);
    }
    // A default-looking curve in prefs is stale data, not a custom curve: ignore it.
    if (m_manager.IsDefaultFanCurve(points)) return {};
    return points;
}
