#include "Gui.h"
#include <QApplication>
#include <QString>
#include <QFont>

GuiWindow::GuiWindow(GpuManager& manager, QWidget* parent)
    : QMainWindow(parent), m_manager(manager) {
    m_metricsTimer = new QTimer(this);
    m_fpsTimer = new QTimer(this);

    connect(m_metricsTimer, &QTimer::timeout, this, &GuiWindow::updateHardwareMetrics);
    connect(m_fpsTimer, &QTimer::timeout, this, &GuiWindow::updateFpsMetrics);

    setupUi();
    applyStylesheet();
}

GuiWindow::~GuiWindow() = default;

void GuiWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);

    // Font monospace per i numeri (evita lo sfarfallio del layout)
    QFont monoFont("Consolas", 10);
    if (monoFont.styleHint() == QFont::SansSerif) {
        monoFont.setFamily("Courier New"); // Fallback per Linux/Mac
    }

    // ==================== GRUPPO HARDWARE ====================
    m_hardwareGroup = new QGroupBox("GPU Metrics", this);
    QGridLayout* hwLayout = new QGridLayout(m_hardwareGroup);
    hwLayout->setSpacing(10);

    m_gpuTempLabel = new QLabel("GPU: --°C", this);
    m_hotspotTempLabel = new QLabel("Hotspot: --°C", this);
    m_memTempLabel = new QLabel("Memory: --°C", this);
    m_statusLabel = new QLabel("VRAM Used: -- MB", this);
    m_fanLabel = new QLabel("Fan: -- RPM (0%)", this);
    m_mempryLabel = new QLabel("Memory Usage: -- MB", this);


    m_fanBar = new QProgressBar(this);
    m_fanBar->setRange(0, 100);
    m_fanBar->setValue(0);
    m_fanBar->setTextVisible(false);
    m_fanBar->setFixedHeight(8);

    m_memoryBar = new QProgressBar(this);
    m_memoryBar->setRange(0, m_manager.m_statusInfo.totalGPUMemoryMB);
    m_memoryBar->setValue(0);
    m_memoryBar->setTextVisible(false);
    m_memoryBar->setFixedHeight(8);

    // Applichiamo il font monospace dove serve stabilità numerica
    m_gpuTempLabel->setFont(monoFont);
    m_hotspotTempLabel->setFont(monoFont);
    m_memTempLabel->setFont(monoFont);
    m_statusLabel->setFont(monoFont);
    m_fanLabel->setFont(monoFont);
    m_mempryLabel->setFont(monoFont);

    // Posizionamento nella griglia (Riga, Colonna)
    hwLayout->addWidget(m_gpuTempLabel, 0, 0);
    hwLayout->addWidget(m_hotspotTempLabel, 0, 1);
    hwLayout->addWidget(m_memTempLabel, 1, 0);
    hwLayout->addWidget(m_statusLabel, 1, 1);
    hwLayout->addWidget(m_fanLabel, 2, 0, 1, 2); // Occupa 2 colonne
    hwLayout->addWidget(m_fanBar, 3, 0, 1, 2);
    hwLayout->addWidget(m_mempryLabel, 4, 0, 1, 2); // Occupa 2 colonne
    hwLayout->addWidget(m_memoryBar, 5, 0, 1, 2);

    m_mainLayout->addWidget(m_hardwareGroup);

    // ==================== GRUPPO FPS ====================
    m_fpsGroup = new QGroupBox("Performance", this);
    QGridLayout* fpsLayout = new QGridLayout(m_fpsGroup);

    m_instantFpsLabel = new QLabel("Current FPS: --", this);
    m_avgFpsLabel = new QLabel("Average: --", this);
    m_minMaxFpsLabel = new QLabel("Min: -- | Max: --", this);

    // Font più grande per gli FPS correnti
    QFont fpsFont = monoFont;
    fpsFont.setPointSize(12);
    fpsFont.setBold(true);
    m_instantFpsLabel->setFont(fpsFont);
    m_avgFpsLabel->setFont(monoFont);
    m_minMaxFpsLabel->setFont(monoFont);

    fpsLayout->addWidget(m_instantFpsLabel, 0, 0, 1, 2);
    fpsLayout->addWidget(m_avgFpsLabel, 1, 0);
    fpsLayout->addWidget(m_minMaxFpsLabel, 1, 1);

    m_mainLayout->addWidget(m_fpsGroup);

    // Finalizza Finestra
    setCentralWidget(m_centralWidget);
    setWindowTitle("AMD GPU Monitor (Qt 6.7)");
    resize(550, 650);
}

void GuiWindow::updateHardwareMetrics() {
    if (m_fpsOnlyMode) return;

    m_manager.UpdateGpuTempStruct(false);
    auto info = m_manager.m_temperatureInfo;

    m_gpuTempLabel->setText(QString("GPU: %1°C").arg(info.gpuTemperatureC, 5, 'f', 1));
    m_hotspotTempLabel->setText(QString("Hotspot: %1°C").arg(info.gpuHotspotTemperatureC, 5, 'f', 1));
    m_memTempLabel->setText(QString("Memory: %1°C").arg(info.gpuMemoryTemperatureC, 5, 'f', 1));
    m_statusLabel->setText(QString("VRAM Used: %1 MB/%2 MB").arg(info.vRam).arg(m_manager.m_statusInfo.totalGPUMemoryMB));

    m_fanLabel->setText(QString("Fan Speed: %1 RPM (%2%)").arg(info.fanSpeed).arg(info.fanPercent));
    m_fanBar->setValue(static_cast<int>(info.fanPercent));
    m_mempryLabel->setText(QString("Memory Usage: %1 MB/%2 MB - Free %3 MB").arg(info.vRam).arg(m_manager.m_statusInfo.totalGPUMemoryMB).arg(m_manager.m_statusInfo.totalGPUMemoryMB - info.vRam));
    m_memoryBar->setValue(info.vRam);
}

void GuiWindow::updateFpsMetrics() {
    m_manager.UpdateGpuTempStruct(true);
    auto info = m_manager.m_temperatureInfo;

    m_instantFpsLabel->setText(QString("Current FPS: %1").arg(info.instantFPS, 4, 'f', 1));
    m_avgFpsLabel->setText(QString("Average: %1").arg(info.avgFPS, 4, 'f', 1));
    m_minMaxFpsLabel->setText(QString("Min: %1 | Max: %2").arg(info.minFPS).arg(info.maxFPS));
}

int GuiWindow::OpenWin(GpuManager& manager, int refreshRateMs, int refreshRateFPS, bool fpsOnly) {
    m_fpsOnlyMode = fpsOnly;

    if (m_fpsOnlyMode) {
        m_hardwareGroup->hide();
    }

    m_metricsTimer->start(refreshRateMs);
    m_fpsTimer->start(refreshRateFPS);

    this->show();
    return 0;
}

void GuiWindow::applyStylesheet() {
    // Look & Feel scuro (stile gaming/monitoring)
    QString darkStyle = R"(
        QMainWindow {
            background-color: #121212;
        }
        QGroupBox {
            font-weight: bold;
            color: #E0E0E0;
            border: 1px solid #2C2C2C;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 3px;
        }
        QLabel {
            color: #B0B0B0;
        }
        QProgressBar {
            border: 1px solid #2C2C2C;
            border-radius: 3px;
            background-color: #1A1A1A;
        }
        QProgressBar::chunk {
            background-color: #00E676; /* Verde per ventole/performance, stile AMD Radeon Eco */
            width: 1px;
        }
    )";

    this->setStyleSheet(darkStyle);
}