#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

#define NOMINMAX

#include <QMainWindow>
#include <QTimer>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QSettings>
#include <QLayout>
#include <QBoxLayout>
#include <cmath>
#include <algorithm>
#include <vector>
#include "GpuManager.h"

namespace Ui {
    class GuiWindow;
    class SettingsWindow;
}

class CurvePlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurvePlotWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    enum Theme { Dark, Light };
    void setTheme(Theme t) { m_theme = t; update(); }
    void setPoints(const std::vector<std::pair<int, int>>& points) {
        m_points = points;
        update();
    }
    const std::vector<std::pair<int, int>>& points() const { return m_points; }

    void setRanges(int minTemp, int maxTemp, int minSpeed, int maxSpeed) {
        m_minTemp = minTemp; m_maxTemp = maxTemp;
        m_minSpeed = minSpeed; m_maxSpeed = maxSpeed;
    }

    void setDragSpeedRange(int minDragSpeed, int maxDragSpeed) {
        m_minDragSpeed = minDragSpeed;
        m_maxDragSpeed = maxDragSpeed;
    }

    void setEditable(bool editable) {
        m_editable = editable;
        update();
    }

    bool isEditable() const { return m_editable; }

    void setCurrentPoint(double temp, int fan) {
        m_currentTemp = temp;
        m_currentFan = fan;
        m_showCurrentPoint = true;
        update();
    }

    void setCurrentPointVisible(bool visible) {
        m_showCurrentPoint = visible;
        update();
    }

signals:
    void pointsChanged();
    void editingFinished();

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Background
        painter.fillRect(rect(), colorBg());

        if (m_points.empty()) return;

        // Padding and effective area
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);

        // Grid & Axes
        QPen axisPen(colorAxis(), 1, Qt::SolidLine);
        painter.setPen(axisPen);
        // X axis shifted to bottom padding
        painter.drawLine(padding, height() - padding, width() - padding, height() - padding);
        // Y axis shifted to left padding
        painter.drawLine(padding, padding, padding, height() - padding);

        // Gridlines & tick positions (adaptive)
        const auto tempTicks = computeTicks((float)m_minTemp, (float)m_maxTemp);
        const auto fanTicks = computeTicks((float)m_minSpeed, (float)m_maxSpeed);
        QPen gridPen(colorGrid(), 1, Qt::SolidLine);
        painter.setPen(gridPen);
        for (float t : tempTicks) {
            float gx, gy;
            toPixel(t, (float)m_minSpeed, gx, gy);
            painter.drawLine(QPointF(gx, padding), QPointF(gx, height() - padding));
        }
        for (float s : fanTicks) {
            float gx, gy;
            toPixel((float)m_minTemp, s, gx, gy);
            painter.drawLine(QPointF(padding, gy), QPointF(width() - padding, gy));
        }

        // Legend - Labels
        painter.setPen(QPen(colorLabel(), 1));
        QFont labelFont = painter.font();
        labelFont.setPointSize(8);
        painter.setFont(labelFont);

        painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "Temp (°C)");
        painter.save();
        painter.translate(11.0f, height() / 2);
        painter.rotate(-90);
        // Draw the vertical label centered on the rotated origin using font metrics
        const QString fanLabel = "Fan (%)";
        const QFontMetrics fm(labelFont);
        painter.drawText(-fm.horizontalAdvance(fanLabel) / 2.0,
                         (fm.ascent() - fm.descent()) / 2.0,
                         fanLabel);
        painter.restore();

        // X-axis tick labels (below plot area in bottom padding)
        painter.setPen(QPen(colorLabel(), 1));
        for (float t : tempTicks) {
            float gx, gy;
            toPixel(t, (float)m_minSpeed, gx, gy);
            painter.drawText(QPointF(gx - 10, height() - padding + 12), QString::number((int)std::lround(t)));
        }
        // Y-axis tick labels (left of plot area in left padding)
        for (float s : fanTicks) {
            float gx, gy;
            toPixel((float)m_minTemp, s, gx, gy);
            const QString stxt = QString::number((int)std::lround(s));
            const QFontMetrics fm2(labelFont);
            painter.drawText(QPointF(padding - 3 - fm2.horizontalAdvance(stxt), gy + 3), stxt);
        }

        // Draw Curve
        QPen curvePen(colorCurve(), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(curvePen);

        QPolygonF polyline;
        for (const auto& p : m_points) {
            float x, y;
            toPixel(p.first, p.second, x, y);
            polyline << QPointF(x, y);
        }
        painter.drawPolyline(polyline);

        // Draw Point Markers and Labels
        for (size_t i = 0; i < m_points.size(); ++i) {
            const auto& p = m_points[i];
            float x, y;
            toPixel(p.first, p.second, x, y);

            // Marker
            painter.setBrush(QBrush(m_editable && static_cast<int>(i) == m_dragIndex
                                        ? colorMarkerActive()
                                        : colorCurve()));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(x, y), 4, 4);

            // Coordinate Text
            painter.setPen(QPen(colorMarkerText(), 1));
            painter.setFont(labelFont);
            QString label = QString("%1°C / %2%").arg(p.first).arg(p.second);

            float textX = x + 25;
            float textY = y;
            painter.drawText(textX, textY, label);
        }

        // Live operating-point marker
        if (m_showCurrentPoint) {
            float mx, my;
            toPixel((float)m_currentTemp, (float)m_currentFan, mx, my);
            // Clamp marker to the plot area to avoid drawing outside the axis.
            mx = std::clamp(mx, padding, width() - padding);
            my = std::clamp(my, padding, height() - padding);

            QPen ringPen(colorRing(), 2, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(ringPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(mx, my), 6, 6);

            // Small label beside the marker
            painter.setPen(QPen(colorRing(), 1));
            painter.setFont(labelFont);
            painter.drawText(QPointF(mx + 10, my - 10),
                             QString("%1°C / %2%").arg((int)std::lround(m_currentTemp)).arg(m_currentFan));
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (!m_editable) { QWidget::mousePressEvent(event); return; }
        m_dragIndex = hitTest(event->pos());
        if (m_dragIndex >= 0) update();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!m_editable || m_dragIndex < 0) { QWidget::mouseMoveEvent(event); return; }
        auto& p = m_points[m_dragIndex];
        float tempF = 0.0f, speedF = 0.0f;
        toValue(event->pos(), tempF, speedF);
        int newTemp = static_cast<int>(std::lround(tempF));
        int newSpeed = static_cast<int>(std::lround(speedF));

        // Constrain temperature to remain monotonic non-decreasing across neighbors.
        int minT = m_minTemp;
        int maxT = m_maxTemp;
        if (m_dragIndex > 0) minT = std::max(minT, m_points[m_dragIndex - 1].first);
        if (m_dragIndex + 1 < static_cast<int>(m_points.size()))
            maxT = std::min(maxT, m_points[m_dragIndex + 1].first);

        newTemp = std::clamp(newTemp, minT, maxT);
        newSpeed = std::clamp(newSpeed, m_minDragSpeed, m_maxDragSpeed);

        if (p.first != newTemp || p.second != newSpeed) {
            p.first = newTemp;
            p.second = newSpeed;
            emit pointsChanged();
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_dragIndex >= 0) {
            emit editingFinished();
        }
        m_dragIndex = -1;
        update();
    }

    void toPixel(float temp, float speed, float& x, float& y) const {
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);
        x = padding + ((float)(temp - m_minTemp) / (float)(m_maxTemp - m_minTemp) * effectiveWidth);
        y = padding + ((float)(m_maxSpeed - speed) / (float)(m_maxSpeed - m_minSpeed) * effectiveHeight);
    }

    void toValue(const QPoint& pos, float& temp, float& speed) const {
        const float padding = kPadding;
        float effectiveWidth = width() - (padding * 2);
        float effectiveHeight = height() - (padding * 2);
        float nx = (pos.x() - padding) / effectiveWidth;
        float ny = (pos.y() - padding) / effectiveHeight;
        temp = m_minTemp + nx * (float)(m_maxTemp - m_minTemp);
        speed = m_maxSpeed - ny * (float)(m_maxSpeed - m_minSpeed);
    }

    int hitTest(const QPoint& pos) const {
        int best = -1;
        float bestDist = 12.0f * 12.0f;
        for (size_t i = 0; i < m_points.size(); ++i) {
            float x, y;
            toPixel(m_points[i].first, m_points[i].second, x, y);
            float dx = pos.x() - x;
            float dy = pos.y() - y;
            float d = dx * dx + dy * dy;
            if (d < bestDist) { bestDist = d; best = static_cast<int>(i); }
        }
        return best;
    }

private:
    static constexpr float kPadding = 40.0f;

    QColor colorBg() const {
        return m_theme == Light ? QColor("#FFFFFF") : QColor("#181825");
    }
    QColor colorAxis() const {
        return m_theme == Light ? QColor("#8A8CA0") : QColor("#45475A");
    }
    QColor colorGrid() const {
        return m_theme == Light ? QColor(181, 183, 200, 150) : QColor(49, 50, 68, 140);
    }
    QColor colorLabel() const {
        return m_theme == Light ? QColor("#5A5C6E") : QColor("#BAC2DE");
    }
    QColor colorCurve() const {
        return m_theme == Light ? QColor("#0096B0") : QColor("#89DCEB");
    }
    QColor colorMarkerActive() const {
        return m_theme == Light ? QColor("#D9A441") : QColor("#F9E2AF");
    }
    QColor colorMarkerText() const {
        return m_theme == Light ? QColor("#7A3FA0") : QColor("#CBA6F7");
    }
    QColor colorRing() const {
        return m_theme == Light ? QColor("#C94F8B") : QColor("#F38BA8");
    }

    static float niceStep(float range) {
        const float raw = range / 6.0f;
        const float magnitude = std::pow(10.0f, std::floor(std::log10(raw)));
        const float norm = raw / magnitude;
        float mult;
        if (norm < 1.5f) mult = 1.0f;
        else if (norm < 3.5f) mult = 2.0f;
        else if (norm < 7.5f) mult = 5.0f;
        else mult = 10.0f;
        return mult * magnitude;
    }

    static std::vector<float> computeTicks(float min, float max) {
        std::vector<float> ticks;
        if (max < min) return ticks;
        const float range = max - min;
        if (range <= 0.0f) { ticks.push_back(min); return ticks; }
        float step = niceStep(range);
        // First tick aligned to a multiple of step >= min
        float first = std::floor(min / step) * step;
        if (first < min) first += step;
        const int maxCount = 12;
        for (float v = first; v <= max + step && (int)ticks.size() < maxCount; v += step) {
            if (v >= min - step * 0.5f && v <= max + step * 0.5f)
                ticks.push_back(v);
        }
        return ticks;
    }

    std::vector<std::pair<int, int>> m_points;
    int m_minTemp = 25;
    int m_maxTemp = 100;
    int m_minSpeed = 0;
    int m_maxSpeed = 100;
    int m_minDragSpeed = 0;
    int m_maxDragSpeed = 100;
    bool m_editable = false;
    int m_dragIndex = -1;
    double m_currentTemp = 0.0;
    int m_currentFan = 0;
    bool m_showCurrentPoint = false;
    Theme m_theme = Dark;
};

class CollapsibleGroupBox : public QGroupBox {
    Q_OBJECT
public:
    explicit CollapsibleGroupBox(QWidget* parent = nullptr) : QGroupBox(parent) { setMouseTracking(true); }
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr) : QGroupBox(title, parent) { setMouseTracking(true); }

    void setCollapsed(bool collapsed) {
        if (m_collapsed == collapsed) return;
        m_collapsed = collapsed;
        if (collapsed) {
            setMaximumHeight(titleBandHeight());
            setChildrenVisible(false);
        } else {
            setChildrenVisible(true);
            setMaximumHeight(QWIDGETSIZE_MAX);
            updateGeometry();
        }
        update();
        resizeTopLevel();
    }


protected:
    void resizeEvent(QResizeEvent* event) override {
        QGroupBox::resizeEvent(event);
        if (m_collapsed) setMaximumHeight(titleBandHeight());
    }

    // Visual hint that the title bar is clickable: a chevron at its right end
    // (down = expanded, up = collapsed) and a pointing-hand cursor over the band.
    void paintEvent(QPaintEvent* event) override {
        QGroupBox::paintEvent(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor c = palette().color(QPalette::WindowText);
        c.setAlpha(170);
        p.setPen(QPen(c, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const qreal cx = width() - 16;
        const qreal cy = 12 + (fontMetrics().height() + 4) / 2.0;
        const qreal d = m_collapsed ? -3.0 : 3.0;
        p.drawPolyline(QPolygonF{ QPointF(cx - 5, cy - d / 2), QPointF(cx, cy + d / 2), QPointF(cx + 5, cy - d / 2) });
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->pos().y() < titleBandHeight()) setCursor(Qt::PointingHandCursor);
        else unsetCursor();
        QGroupBox::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        unsetCursor();
        QGroupBox::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && event->pos().y() < titleBandHeight()) {
            toggleCollapsed();
            event->accept();
            return;
        }
        QGroupBox::mousePressEvent(event);
    }

private:
    void toggleCollapsed() {
        setCollapsed(!m_collapsed);
        QSettings s("AmdFanSensors", "AmdFanSensors");
        s.setValue(QString("collapsedGroup/%1").arg(objectName()), m_collapsed);
    }

    void setChildrenVisible(bool visible) {
        const auto children = findChildren<QWidget*>();
        for (QWidget* w : children)
            w->setVisible(visible);
    }

    void resizeTopLevel() {
        if (!isVisible()) return;
        if (QMainWindow* mw = qobject_cast<QMainWindow*>(window())) {
            QWidget* cw = mw->centralWidget();
            if (auto* bl = cw ? qobject_cast<QBoxLayout*>(cw->layout()) : nullptr) {
                const auto settle = [this, mw, bl]() {
                    QMargins mg = bl->contentsMargins();
                    int h = mg.top() + mg.bottom();
                    int visible = 0;
                    for (int i = 0; i < bl->count(); ++i) {
                        QLayoutItem* item = bl->itemAt(i);
                        if (!item || item->isEmpty()) continue;
                        ++visible;
                        QWidget* w = item->widget();
                        const int hint = w ? w->sizeHint().height() : item->sizeHint().height();
                        const int hi = w ? w->maximumHeight() : QWIDGETSIZE_MAX;
                        h += qBound(w ? w->minimumHeight() : 0, hint, hi);
                    }
                    if (visible > 1 && bl->spacing() >= 0)
                        h += bl->spacing() * (visible - 1);
                    if (h > 0)
                        mw->resize(mw->width(), h);
                };
                QTimer::singleShot(0, this, settle);
                QTimer::singleShot(90, this, settle);
            }
        }
    }

    int titleBandHeight() const { return fontMetrics().height() + 18; }

    bool m_collapsed = false;
};

// Themed title bar drawn inside the (frameless) window so it matches the content.
// Dragging, snapping and edge resizing are done natively through WM_NCHITTEST (see frameHitTest).
class TitleBar : public QWidget {
    Q_OBJECT
public:
    TitleBar(QMainWindow* window, bool canMinMax) : QWidget(window), m_window(window) {
        setObjectName("titleBar");
        setAttribute(Qt::WA_StyledBackground, true);
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(10, 0, 0, 0);
        lay->setSpacing(8);
        auto* icon = new QLabel(this);
        icon->setPixmap(window->windowIcon().pixmap(16, 16));
        m_title = new QLabel(window->windowTitle(), this);
        m_title->setObjectName("titleBarText");
        connect(window, &QWidget::windowTitleChanged, m_title, &QLabel::setText);
        lay->addWidget(icon);
        lay->addWidget(m_title);
        lay->addStretch(1);
        if (canMinMax) {
            lay->addWidget(makeButton(QString(QChar(0xE921)), [window]() { window->showMinimized(); }, "titleBtn"));
            m_max = makeButton(QString(QChar(0xE922)), [window]() {
                if (window->isMaximized()) window->showNormal(); else window->showMaximized();
            }, "titleBtn");
            lay->addWidget(m_max);
        }
        lay->addWidget(makeButton(QString(QChar(0xE8BB)), [window]() { window->close(); }, "titleBtnClose"));
        setFixedHeight(32);
        window->installEventFilter(this);
    }

    void setTheme(bool light) {
        const QString fg = light ? "#1E1E2E" : "#E6E6F0";
        const QString hover = light ? "rgba(0,0,0,25)" : "rgba(255,255,255,30)";
        setStyleSheet(QString(
            "#titleBar { background: transparent; }"
            "#titleBarText { color: %1; background: transparent; }"
            "QPushButton#titleBtn, QPushButton#titleBtnClose { border: none; border-radius: 0; background: transparent;"
            " color: %1; font-family: 'Segoe MDL2 Assets'; font-size: 10px; padding-bottom: 12px; }"
            "QPushButton#titleBtn:hover { background: %2; }"
            "QPushButton#titleBtnClose:hover { background: #E81123; color: white; }").arg(fg, hover));
    }

protected:
    bool eventFilter(QObject* o, QEvent* e) override {
        if (o == m_window && e->type() == QEvent::WindowStateChange && m_max)
            m_max->setText(QString(QChar(m_window->isMaximized() ? 0xE923 : 0xE922)));
        return QWidget::eventFilter(o, e);
    }

private:
    template <typename F>
    QPushButton* makeButton(const QString& glyph, F onClick, const char* name) {
        auto* b = new QPushButton(glyph, this);
        b->setObjectName(name);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(46, 32);
        connect(b, &QPushButton::clicked, this, onClick);
        return b;
    }

    QMainWindow* m_window;
    QLabel* m_title = nullptr;
    QPushButton* m_max = nullptr;
};

// Native hit-testing for a frameless window: edge/corner resize and title-bar drag.
bool frameHitTest(QWidget* window, QWidget* titleBar, void* message, qintptr* result);

class SettingsWindow : public QMainWindow {
public:
    explicit SettingsWindow(QWidget* parent = nullptr) : QMainWindow(parent, Qt::Window | Qt::FramelessWindowHint) {}
    Ui::SettingsWindow *ui = nullptr;
    TitleBar* titleBar = nullptr;

protected:
    bool nativeEvent(const QByteArray& type, void* message, qintptr* result) override {
        if (titleBar && frameHitTest(this, titleBar, message, result)) return true;
        return QMainWindow::nativeEvent(type, message, result);
    }

    // Remember where the user left the window. Saved only once it has been shown, so
    // closing the app without ever opening Settings can't overwrite the stored position.
    void hideEvent(QHideEvent* e) override {
        QMainWindow::hideEvent(e);
        saveGeometryToSettings();
    }

public:
    void saveGeometryToSettings() {
        if (!m_shown) return;
        QSettings("AmdFanSensors", "AmdFanSensors").setValue("settingsWindowGeometry", saveGeometry());
    }

protected:
    void showEvent(QShowEvent* e) override {
        QMainWindow::showEvent(e);
        m_shown = true;
    }

private:
    bool m_shown = false;
};

class GuiWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GuiWindow(GpuManager& manager, QWidget* parent = nullptr);
    ~GuiWindow() override;
    bool OpenWin();

protected:
    bool nativeEvent(const QByteArray& type, void* message, qintptr* result) override {
        if (m_titleBar && frameHitTest(this, m_titleBar, message, result)) return true;
        return QMainWindow::nativeEvent(type, message, result);
    }
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateHardwareMetrics() const;
    void updateFpsMetrics() const;
    void onResetFpsClicked() const;
    void onOpenSettings();
    void onFanCurveToggled(bool checked);
    void onZeroRpmToggled(bool checked);
    void onVoltageOffsetToggled(bool checked);
    void onVoltageValueChanged(int value);
    void saveVoltageOffset(int value);
        void onApplyCurveClicked();
        void onExportCurveClicked();
        void onImportCurveClicked();
        void onMonitoringToggled(bool enabled) const;
    void onHardwareStatsToggled(bool enabled) const;
    void onHwRefreshSliderChanged(int value);
    void onFpsRefreshSliderChanged(int value);
    void onCurveEdited();

private:
    void setTheme(bool light);
    void initializeTuningControls();
    void initializeRefreshRateControls();
    void updateCurveButtonEnabled() const;
        std::vector<std::pair<int, int>> LoadSavedCurvePoints() const;
        void SaveCurvePoints(const std::vector<std::pair<int, int>>& points) const;
        bool importCurve(const QString& path, std::vector<std::pair<int, int>>& points);
    GpuManager& m_manager;
    QTimer* m_metricsTimer;
    QTimer* m_fpsTimer;

    int m_refreshRate;
    int m_refreshRateFPS;


        bool m_curveEdited = false;
        std::vector<std::pair<int, int>> m_appliedCurvePoints;
        std::vector<std::pair<int, int>> m_stashedCustomPoints;
        int m_curveMinTemp = 25, m_curveMaxTemp = 100;
        int m_curveMinDragSpeed = 0, m_curveMaxDragSpeed = 100;

        CurvePlotWidget* m_curvePlot;
    Ui::GuiWindow *ui;
    SettingsWindow* m_settingsWindow;
    QString m_darkStyleSheet;
    TitleBar* m_titleBar = nullptr;
    bool m_settingsGeometryRestored = false;
};
#endif //AMDFANSENSORS_WINDOW_H