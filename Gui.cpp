#include "Gui.h"
#include "ui_Gui.h"
#include "ui_SettingsWindow.h"
#include <iostream>
#include <QString>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QMessageBox>
#include <QSettings>
#include <QMenuBar>

static const QString lightStyleSheet() {
    return R"css(/* === Global Theme Variables === */
QMainWindow {
    background-color: #F3F4F9;
}

QWidget#GuiWindow, QWidget#m_centralWidget {
    background-color: #F3F4F9;
}

/* === Group Boxes === */
QGroupBox {
    font-weight: bold;
    color: #2A2C3A;
    border: 1px solid #C7C9D8;
    border-radius: 2px;
    margin-top: 15px;
    padding-top: 16px;
    background-color: #FFFFFF;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 6px;
}

/* === Section Headers (Hardware, Tuning, Performance) === */
QGroupBox#m_hardwareGroup {
    border-color: #9AA0B5;
}

QGroupBox#m_controlGroup {
    border-color: #0096B0;
}

QGroupBox#m_fpsGroup {
    border-color: #B8860B;
}

/* === Labels === */
QLabel {
    color: #33354A;
    font-family: 'Segoe UI', 'Cascadia Code', 'Consolas';
}

QLabel#m_gpuTempLabel, QLabel#m_hotspotTempLabel, QLabel#m_memTempLabel {
    color: #C16A00;
    font-weight: 600;
}

QLabel#m_systemMemoryLabel {
    color: #0096B0;
    font-weight: 500;
}

QLabel#m_fanLabel {
    color: #2E8B57;
    font-weight: 500;
}

QLabel#m_memoryLabel, QLabel#m_statusLabel {
    color: #B8860B;
    font-weight: 500;
}

QLabel#m_gpuUsageLabel {
    color: #0096B0;
    font-weight: 600;
}

/* === FPS Labels === */
QLabel#m_instantFpsLabel {
    color: #2A2C3A;
    font-size: 14px;
    font-weight: bold;
}

QLabel#m_avgFpsLabel, QLabel#m_minMaxFpsLabel {
    color: #2E8B57;
    font-size: 14px;
    font-weight: bold;
}

/* === Progress Bars === */
QProgressBar {
    border: none;
    border-radius: 6px;
    background-color: #E6E7F0;
    height: 10px;
    text-align: center;
}

QProgressBar#m_fanBar::chunk {
    border-radius: 5px;
    background: qlineargradient(spread:reflect, x1:0, y1:0, x2:1, y2:0, stop:0 #2E8B57, stop:1 #0096B0);
}

QProgressBar#m_memoryBar::chunk {
    border-radius: 5px;
    background: qlineargradient(spread:reflect, x1:0, y1:0, x2:1, y2:0, stop:0 #B8860B, stop:1 #C9528A);
}

QProgressBar#m_systemMemoryBar::chunk {
    border-radius: 5px;
    background: qlineargradient(spread:reflect, x1:0, y1:0, x2:1, y2:0, stop:0 #0096B0, stop:1 #0096B0);
}

QProgressBar#gpuUsageProgressBar::chunk {
    border-radius: 5px;
    background: qlineargradient(spread:reflect, x1:0, y1:0, x2:1, y2:0, stop:0 #C9528A, stop:1 #B8860B);
}

/* === Buttons === */
QPushButton {
    background-color: #E6E7F0;
    color: #2A2C3A;
    border: 1px solid #C7C9D8;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 600;
    font-size: 12px;
}

QPushButton:hover {
    background-color: #D3D5E2;
    border-color: #0096B0;
    color: #1A1C26;
}

QPushButton:pressed {
    background-color: #C7C9D8;
    color: #1A1C26;
}

QPushButton:disabled {
    background-color: #EBECF3;
    color: #9AA0B5;
    border-color: #E6E7F0;
}

/* === Apply button special style === */
QPushButton#m_applyTuningButton {
    background-color: #0096B0;
    color: #FFFFFF;
    border: none;
    font-weight: bold;
    padding: 10px 16px;
}

QPushButton#m_applyTuningButton:hover {
    background-color: #0096B0;
    color: #FFFFFF;
}

QPushButton#m_applyTuningButton:pressed {
    background-color: #00A38E;
}

QPushButton#m_applyTuningButton:disabled {
    background-color: #E6E7F0;
    color: #9AA0B5;
}

/* === Checkboxes === */
QCheckBox {
    color: #33354A;
    font-weight: 500;
    spacing: 8px;
}

QCheckBox::indicator {
    width: 12px;
    height: 12px;
    border-radius: 4px;
    border: 2px solid #9AA0B5;
    background-color: #E6E7F0;
}

QCheckBox::indicator:checked {
    background-color: #0096B0;
    border-color: #0096B0;
}

QCheckBox::indicator:hover {
    border-color: #2A2C3A;
}

QCheckBox::indicator:disabled {
    border-color: #E6E7F0;
    background-color: #F3F4F9;
}

QCheckBox:disabled {
    color: #9AA0B5;
}

/* === Slider === */
QSlider::groove:horizontal {
    border: none;
    height: 6px;
    background: #E6E7F0;
    border-radius: 3px;
    margin: 0;
}

QSlider::add-page:horizontal {
    background: #E6E7F0;
    border-radius: 3px;
}

QSlider::sub-page:horizontal {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0096B0, stop:1 #0096B0);
    border-radius: 3px;
}

QSlider::handle:horizontal {
    width: 16px;
    margin: -5px 0;
    border-radius: 8px;
    background: #FFFFFF;
    border: none;
}

QSlider::handle:hover:horizontal {
    background: #FFFFFF;
}

QSlider::handle:disabled:horizontal {
    background: #9AA0B5;
}

/* === SpinBox === */
QSpinBox {
    background-color: #FFFFFF;
    color: #2A2C3A;
    border: 1px solid #C7C9D8;
    border-radius: 6px;
    padding: 6px 8px;
    font-family: 'Segoe UI', monospace;
    min-width: 80px;
}

QSpinBox:hover {
    border-color: #0096B0;
}

QSpinBox:focus {
    border-color: #0096B0;
    background-color: #F3F4F9;
}

/* === Menu Bar === */
QMenuBar {
    background-color: #F3F4F9;
    border-bottom: 1px solid #C7C9D8;
    border-radius: 0;
}
QMenuBar::item {
    background: transparent;
    color: #2A2C3A;
    padding: 4px 8px;
    border-radius: 2px;
}
QMenuBar::item:selected {
    background-color: #E5E7F0;
}
QMenu {
    background-color: #FFFFFF;
    border: 1px solid #C7C9D8;
    border-radius: 2px;
}
QMenu::item {
    background: transparent;
    color: #2A2C3A;
    padding: 4px 10px;
}
QMenu::item:selected {
    background-color: #E5E7F0;
    color: #2A2C3A;
}

QSpinBox:disabled {
    color: #9AA0B5;
    background-color: #F3F4F9;
    border-color: #E6E7F0;
}
)css";
}

GuiWindow::GuiWindow(GpuManager& manager, int refreshRateMs, int refreshRateFPS, bool fpsOnly, bool hwRateProvided, bool fpsRateProvided, QWidget* parent)
    : QMainWindow(parent), m_manager(manager), m_fpsOnlyMode(fpsOnly), m_refreshRate(refreshRateMs), m_refreshRateFPS(refreshRateFPS), m_refreshRateCliDefault(refreshRateMs), m_refreshRateFpsCliDefault(refreshRateFPS), m_hwRateProvided(hwRateProvided), m_fpsRateProvided(fpsRateProvided), ui(new Ui::GuiWindow)  {

    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/icon.ico"));
    m_darkStyleSheet = this->styleSheet();

    m_settingsWindow = new SettingsWindow(this);
    m_settingsWindow->ui = new Ui::SettingsWindow;
    m_settingsWindow->ui->setupUi(m_settingsWindow);

    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* fileMenu = menuBar->addMenu(QStringLiteral("File"));
    QAction* settingsAction = new QAction(QStringLiteral("Settings"), fileMenu);
    fileMenu->addAction(settingsAction);
    setMenuBar(menuBar);
    menuBar->setMinimumHeight(30);
    menuBar->show();
    connect(settingsAction, &QAction::triggered, this, &GuiWindow::onOpenSettings);

    m_metricsTimer = new QTimer(this);
    m_fpsTimer = new QTimer(this);

    connect(m_metricsTimer, &QTimer::timeout, this, &GuiWindow::updateHardwareMetrics);
    connect(m_fpsTimer, &QTimer::timeout, this, &GuiWindow::updateFpsMetrics);
    connect(ui->m_resetFpsButton, &QPushButton::clicked, this, &GuiWindow::onResetFpsClicked);

    ui->m_enableMonitoringCheckbox->setChecked(false);
    connect(ui->m_enableMonitoringCheckbox, &QCheckBox::toggled, this, &GuiWindow::onMonitoringToggled);

    ui->m_enableHardwareStatsCheckbox->setChecked(true);
    connect(ui->m_enableHardwareStatsCheckbox, &QCheckBox::toggled, this, &GuiWindow::onHardwareStatsToggled);

    QSettings settings("AmdFanSensors", "AmdFanSensors");
    const bool stayOnTop = settings.value("stayOnTop", false).toBool();
    m_settingsWindow->ui->m_stayOnTopCheckbox->setChecked(stayOnTop);
    setWindowFlag(Qt::WindowStaysOnTopHint, stayOnTop);
    connect(m_settingsWindow->ui->m_stayOnTopCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        show();
        QSettings s("AmdFanSensors", "AmdFanSensors");
        s.setValue("stayOnTop", checked);
    });

    // Restore saved collapsed states of the collapsible group boxes.
    for (const QString& name : { QStringLiteral("m_hardwareGroup"), QStringLiteral("m_controlGroup"),
                                  QStringLiteral("m_fpsGroup"), QStringLiteral("m_windowGroup"),
                                  QStringLiteral("m_updateGroup"), QStringLiteral("m_tuningGroup") }) {
        if (auto* cgb = qobject_cast<CollapsibleGroupBox*>(findChild<QGroupBox*>(name)))
            cgb->setCollapsed(settings.value("collapsedGroup/" + name, false).toBool());
    }

    // Initialize Curve Plot Widget
    m_curvePlot = new CurvePlotWidget(this);
    ui->controlLayout->addWidget(m_curvePlot, 1, 0, 1, 2);
    m_curvePlot->setMinimumHeight(160);
    m_appliedCurvePoints = m_manager.GetFanCurvePoints();
    m_curvePlot->setPoints(m_appliedCurvePoints);

    // Let the curve section grow with the window; keep other sections fixed.
    ui->controlLayout->setRowStretch(1, 1);
    ui->m_mainLayout->setStretch(0, 0);
    ui->m_mainLayout->setStretch(1, 1);
    ui->m_mainLayout->setStretch(2, 0);
    ui->m_hardwareGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->m_fpsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

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
    m_curvePlot->setEditable(m_settingsWindow->ui->m_fanCurveCheckbox->isChecked());
    connect(m_curvePlot, &CurvePlotWidget::pointsChanged, this, &GuiWindow::onCurveEdited);
    connect(m_settingsWindow->ui->m_fanCurveCheckbox, &QCheckBox::toggled, this, &GuiWindow::onFanCurveToggled);

    ui->controlLayout->activate();
    ui->m_controlGroup->update();

    // Light theme: opt-in, persisted; default stays dark.
    {
        QSettings themeSettings("AmdFanSensors", "AmdFanSensors");
        const bool lightTheme = themeSettings.value("lightTheme", false).toBool();
        m_settingsWindow->ui->m_lightThemeCheckbox->setChecked(lightTheme);
        connect(m_settingsWindow->ui->m_lightThemeCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            setTheme(checked);
            QSettings s("AmdFanSensors", "AmdFanSensors");
            s.setValue("lightTheme", checked);
        });
        setTheme(lightTheme);
    }

    onHardwareStatsToggled(ui->m_enableHardwareStatsCheckbox->isChecked());
    initializeTuningControls();
    initializeRefreshRateControls();

    if (m_fpsOnlyMode) {
        ui->m_hardwareGroup->hide();
        ui->m_controlGroup->hide();
        m_settingsWindow->ui->m_hwRefreshLabelTitle->setEnabled(false);
        m_settingsWindow->ui->m_hwRefreshSlider->setEnabled(false);
        m_settingsWindow->ui->m_hwRefreshValueLabel->setEnabled(false);
    }
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
        this->adjustSize();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void GuiWindow::setTheme(bool light) {
    const QString style = light ? lightStyleSheet() : m_darkStyleSheet;
    setStyleSheet(style);
    if (m_settingsWindow)
        m_settingsWindow->setStyleSheet(style);
    if (m_curvePlot)
        m_curvePlot->setTheme(light ? CurvePlotWidget::Light : CurvePlotWidget::Dark);
}

void GuiWindow::initializeTuningControls() {
    connect(m_settingsWindow->ui->m_voltageSlider, &QSlider::valueChanged, m_settingsWindow->ui->m_voltageSpinBox, &QSpinBox::setValue);
    connect(m_settingsWindow->ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_settingsWindow->ui->m_voltageSlider, &QSlider::setValue);
    connect(m_settingsWindow->ui->m_voltageOffsetCheckbox, &QCheckBox::toggled, this, &GuiWindow::onVoltageOffsetToggled);
    connect(m_settingsWindow->ui->m_voltageSlider, &QSlider::valueChanged, this, &GuiWindow::onVoltageValueChanged);
    connect(m_settingsWindow->ui->m_voltageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GuiWindow::onVoltageValueChanged);
    connect(m_settingsWindow->ui->m_zeroRpmCheckbox, &QCheckBox::toggled, this, &GuiWindow::onZeroRpmToggled);
    connect(m_settingsWindow->ui->m_fanCurveCheckbox, &QCheckBox::toggled, this, &GuiWindow::onFanCurveToggled);
    connect(ui->m_applyTuningButton, &QPushButton::clicked, this, &GuiWindow::onApplyCurveClicked);

    try {
        bool zeroRpmSupported = m_manager.IsZeroRPMSupported();
        m_settingsWindow->ui->m_zeroRpmCheckbox->setEnabled(zeroRpmSupported);
        if (zeroRpmSupported) {
            m_settingsWindow->ui->m_zeroRpmCheckbox->setChecked(m_manager.GetZeroRPMState());
        } else {
            m_settingsWindow->ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
        }
    } catch (const std::exception& e) {
        std::cerr << "[Gui] Failed to query ZeroRPM support: " << e.what() << std::endl;
        m_settingsWindow->ui->m_zeroRpmCheckbox->setEnabled(false);
        m_settingsWindow->ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
    } catch (...) {
        std::cerr << "[Gui] Failed to query ZeroRPM support: unknown error" << std::endl;
        m_settingsWindow->ui->m_zeroRpmCheckbox->setEnabled(false);
        m_settingsWindow->ui->m_zeroRpmCheckbox->setText("Zero RPM (Unsupported)");
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
    
    m_settingsWindow->ui->m_voltageSlider->setRange(minVolt, maxVolt);
    m_settingsWindow->ui->m_voltageSpinBox->setRange(minVolt, maxVolt);

    const bool isFanCurveDefaultOne = m_manager.IsFanCurveDefaultOne();
    m_settingsWindow->ui->m_fanCurveCheckbox->setChecked(!isFanCurveDefaultOne);

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
            m_settingsWindow->ui->m_voltageOffsetCheckbox->setChecked(true);
            m_settingsWindow->ui->m_voltageSlider->setValue(currentOffset);
            m_settingsWindow->ui->m_voltageSpinBox->setValue(currentOffset);
            m_settingsWindow->ui->m_voltageSlider->setEnabled(true);
            m_settingsWindow->ui->m_voltageSpinBox->setEnabled(true);
        } else {
            m_settingsWindow->ui->m_voltageOffsetCheckbox->setChecked(false);
            m_settingsWindow->ui->m_voltageSlider->setValue(0);
            m_settingsWindow->ui->m_voltageSpinBox->setValue(0);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Gui] Failed to query current voltage offset: " << e.what() << std::endl;
        m_settingsWindow->ui->m_voltageOffsetCheckbox->setChecked(false);
        m_settingsWindow->ui->m_voltageSlider->setValue(0);
        m_settingsWindow->ui->m_voltageSpinBox->setValue(0);
    } catch (...) {
        std::cerr << "[Gui] Failed to query current voltage offset: unknown error" << std::endl;
        m_settingsWindow->ui->m_voltageOffsetCheckbox->setChecked(false);
        m_settingsWindow->ui->m_voltageSlider->setValue(0);
        m_settingsWindow->ui->m_voltageSpinBox->setValue(0);
    }

    updateCurveButtonEnabled();
}

void GuiWindow::initializeRefreshRateControls() {
    m_settingsWindow->ui->m_hwRefreshSlider->setRange(100, 5000);
    m_settingsWindow->ui->m_fpsRefreshSlider->setRange(100, 5000);

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

    connect(m_settingsWindow->ui->m_hwRefreshSlider, &QSlider::valueChanged, this, &GuiWindow::onHwRefreshSliderChanged);
    connect(m_settingsWindow->ui->m_fpsRefreshSlider, &QSlider::valueChanged, this, &GuiWindow::onFpsRefreshSliderChanged);

    m_settingsWindow->ui->m_hwRefreshSlider->setValue(hwRate);
    m_settingsWindow->ui->m_hwRefreshValueLabel->setText(QString("%1 ms").arg(hwRate));

    m_settingsWindow->ui->m_fpsRefreshSlider->setValue(fpsRate);
    m_settingsWindow->ui->m_fpsRefreshValueLabel->setText(QString("%1 ms").arg(fpsRate));
}

void GuiWindow::onHwRefreshSliderChanged(int value) {
    m_settingsWindow->ui->m_hwRefreshValueLabel->setText(QString("%1 ms").arg(value));
    m_refreshRate = value;
    if (m_metricsTimer->isActive())
        m_metricsTimer->start(m_refreshRate);
    QSettings settings("AmdFanSensors", "AmdFanSensors");
    settings.setValue("refreshRate", m_refreshRate);
}

void GuiWindow::onFpsRefreshSliderChanged(int value) {
    m_settingsWindow->ui->m_fpsRefreshValueLabel->setText(QString("%1 ms").arg(value));
    m_refreshRateFPS = value;
    if (m_fpsTimer->isActive())
        m_fpsTimer->start(m_refreshRateFPS);
    QSettings settings("AmdFanSensors", "AmdFanSensors");
    settings.setValue("refreshRateFPS", m_refreshRateFPS);
}

void GuiWindow::onOpenSettings() {
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void GuiWindow::onFanCurveToggled(bool checked) {
    if (checked) {
        if (m_stashedCustomPoints.empty())
            m_stashedCustomPoints = m_appliedCurvePoints;
        m_curvePlot->setPoints(m_stashedCustomPoints);
        m_curvePlot->setEditable(true);
        m_curveEdited = (m_stashedCustomPoints != m_appliedCurvePoints);
    } else {
        // Default curve applied to the GPU immediately; the current (custom)
        // curve is stashed so re-checking restores it.
        m_stashedCustomPoints = m_curvePlot->points();
        m_appliedCurvePoints = m_manager.GetDefaultFanCurvePoints();
        m_manager.SetFanCurvePoints(m_appliedCurvePoints);
        m_curvePlot->setPoints(m_appliedCurvePoints);
        m_curvePlot->setEditable(false);
        m_curveEdited = false;
    }
    updateCurveButtonEnabled();
}

void GuiWindow::onZeroRpmToggled(bool checked) {
    if (m_settingsWindow->ui->m_zeroRpmCheckbox->isEnabled())
        m_manager.SetZeroRPM(checked);
    updateCurveButtonEnabled();
}

void GuiWindow::onVoltageOffsetToggled(bool checked) {
    m_settingsWindow->ui->m_voltageSlider->setEnabled(checked);
    m_settingsWindow->ui->m_voltageSpinBox->setEnabled(checked);
    int value = checked ? m_settingsWindow->ui->m_voltageSpinBox->value() : 0;
    m_settingsWindow->ui->m_voltageSlider->setValue(value);
    m_settingsWindow->ui->m_voltageSpinBox->setValue(value);
    m_manager.SetVoltageOffset(value);
    updateCurveButtonEnabled();
}

void GuiWindow::onVoltageValueChanged(int value) {
    if (m_settingsWindow->ui->m_voltageOffsetCheckbox->isChecked())
        m_manager.SetVoltageOffset(value);
    updateCurveButtonEnabled();
}

void GuiWindow::updateCurveButtonEnabled() const {
    ui->m_applyTuningButton->setEnabled(m_curvePlot->isEditable() &&
        (m_curveEdited || m_curvePlot->points() != m_appliedCurvePoints));
}

void GuiWindow::onCurveEdited() {
    m_curveEdited = true;
    updateCurveButtonEnabled();
}

void GuiWindow::onApplyCurveClicked() {
    try {
        m_appliedCurvePoints = m_curvePlot->points();
        m_manager.SetFanCurvePoints(m_appliedCurvePoints);
        SaveCurvePoints(m_appliedCurvePoints);

        m_curveEdited = false;
        updateCurveButtonEnabled();

        std::cout << "Applied fan curve: " << m_appliedCurvePoints.size() << " points." << std::endl;

        ui->m_applyTuningButton->setText("Applied!");
        QTimer::singleShot(2500, ui->m_applyTuningButton, [this]() { ui->m_applyTuningButton->setText("Apply Fan Curve"); });
    } catch (const std::exception& e) {
        std::cerr << "Error applying fan curve: " << e.what() << std::endl;
        QMessageBox::critical(this, "Curve Error",
            QString("Failed to apply fan curve:\n%1").arg(e.what()));
    }
    catch (...) {
        std::cerr << "Error applying fan curve: unknown error" << std::endl;
        QMessageBox::critical(this, "Curve Error", "Failed to apply fan curve: unknown error.");
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
