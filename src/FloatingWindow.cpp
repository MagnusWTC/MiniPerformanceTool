#include "FloatingWindow.h"
#include "SettingsManager.h"
#include "ThemeAnimation.h"

#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QVariantAnimation>
#include <algorithm>

namespace {
constexpr int snapDistance = 24;
constexpr int visibleDockedPixels = 18;

QPoint globalPosition(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}
}

FloatingWindow::FloatingWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_theme(new ThemeAnimation(this)), m_valuesAnimation(new QVariantAnimation(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
    setWindowTitle(QStringLiteral("性能悬浮窗"));
    setFixedSize(104, 104);
    setCursor(Qt::OpenHandCursor);
    setToolTip(QStringLiteral("拖动移动 · 双击打开主界面 · 右键显示菜单"));
    m_cpuThreshold = SettingsManager::cpuThreshold();
    m_memoryThreshold = SettingsManager::memoryThreshold();
    move(SettingsManager::floatingWindowPosition());
    keepOnScreen();
    connect(m_theme, &ThemeAnimation::colorChanged, this, [this] { update(); });
    m_valuesAnimation->setDuration(550);
    m_valuesAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

FloatingWindow::~FloatingWindow()
{
    SettingsManager::setFloatingWindowPosition(pos());
}

void FloatingWindow::setMetrics(const MetricsSnapshot &snapshot)
{
    m_metrics = snapshot;
    updateAlert();
    m_valuesAnimation->stop();
    disconnect(m_valuesAnimation, nullptr, this, nullptr);
    const double startCpu = m_displayCpu;
    const double startMemory = m_displayMemory;
    const double cpu = std::clamp(snapshot.cpuUsagePercent, 0.0, 100.0);
    const double memory = std::clamp(snapshot.memoryUsagePercent, 0.0, 100.0);
    connect(m_valuesAnimation, &QVariantAnimation::valueChanged, this,
            [this, startCpu, startMemory, cpu, memory](const QVariant &value) {
        const double progress = value.toDouble();
        m_displayCpu = startCpu + (cpu - startCpu) * progress;
        m_displayMemory = startMemory + (memory - startMemory) * progress;
        update();
    });
    m_valuesAnimation->setStartValue(0.0);
    m_valuesAnimation->setEndValue(1.0);
    m_valuesAnimation->start();
    update();
}

void FloatingWindow::setThresholds(double cpuThreshold, double memoryThreshold)
{
    m_cpuThreshold = std::clamp(cpuThreshold, 0.0, 100.0);
    m_memoryThreshold = std::clamp(memoryThreshold, 0.0, 100.0);
    updateAlert();
}

void FloatingWindow::updateAlert()
{
    m_alert = m_metrics.valid && (m_metrics.cpuUsagePercent > m_cpuThreshold
                                || m_metrics.memoryUsagePercent > m_memoryThreshold);
    m_theme->setAlert(m_alert);
    update();
}

void FloatingWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor accent = m_theme->color();
    const QRectF panel(6, 6, width() - 12, height() - 12);
    for (int spread = 4; spread > 0; --spread) {
        QColor glow = accent;
        glow.setAlpha(4 + (6 - spread) * 2);
        painter.setPen(QPen(glow, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(panel.adjusted(-spread, -spread, spread, spread));
    }
    QRadialGradient background(QPointF(34, 27), 87);
    background.setColorAt(0, QColor(44, 62, 68, 250));
    QColor tinted = accent.darker(680);
    tinted.setAlpha(248);
    background.setColorAt(1, tinted);
    QColor border = accent;
    border.setAlpha(125);
    painter.setPen(QPen(border, 1));
    painter.setBrush(background);
    painter.drawEllipse(panel);
    QFont font = painter.font();
    const auto drawMetric = [&](int y, const QString &name, double value) {
        font.setPixelSize(10);
        font.setBold(false);
        painter.setFont(font);
        painter.setPen(QColor(207, 218, 228));
        painter.drawText(QRectF(16, y, 72, 14), Qt::AlignCenter, name);
        font.setFamily(QStringLiteral("Segoe UI"));
        font.setPixelSize(15);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(accent);
        painter.drawText(QRectF(16, y + 13, 72, 21), Qt::AlignCenter,
                         m_metrics.valid ? QString::number(value, 'f', 0) + QStringLiteral("%")
                                         : QStringLiteral("--"));
        font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    };
    drawMetric(17, QStringLiteral("CPU"), m_displayCpu);
    painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
    painter.drawLine(QPointF(28, 53), QPointF(76, 53));
    drawMetric(55, QStringLiteral("内存"), m_displayMemory);
}

void FloatingWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Clearing the dock state on press lets the clipped window become a
        // normal draggable circle as soon as it is moved away from the edge.
        m_dockedEdges = {};
        m_dragging = true;
        m_dragOffset = globalPosition(event) - frameGeometry().topLeft();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void FloatingWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && event->buttons().testFlag(Qt::LeftButton)) {
        move(globalPosition(event) - m_dragOffset);
        event->accept();
    }
}

void FloatingWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        snapToScreenEdge(globalPosition(event));
        SettingsManager::setFloatingWindowPosition(pos());
        event->accept();
    }
}

void FloatingWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        emit openMainWindowRequested();
    }
}

void FloatingWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(QStringLiteral("打开主界面"), this, &FloatingWindow::openMainWindowRequested);
    menu.addAction(QStringLiteral("隐藏悬浮窗"), this, &QWidget::hide);
    menu.exec(event->globalPos());
}

void FloatingWindow::keepOnScreen()
{
    QScreen *screen = screenForPosition(frameGeometry().center());
    if (!screen)
        return;
    const QRect area = screen->availableGeometry();
    const int right = area.right() + 1;
    const int bottom = area.bottom() + 1;
    const int windowRight = x() + width();
    const int windowBottom = y() + height();

    // Keep a previously saved dock position when the window still has a
    // usable grab area inside this screen's available geometry.
    m_dockedEdges = {};
    if (x() < area.left() && windowRight >= area.left() + visibleDockedPixels)
        m_dockedEdges |= Qt::LeftEdge;
    else if (windowRight > right && x() <= right - visibleDockedPixels)
        m_dockedEdges |= Qt::RightEdge;
    if (y() < area.top() && windowBottom >= area.top() + visibleDockedPixels)
        m_dockedEdges |= Qt::TopEdge;
    else if (windowBottom > bottom && y() <= bottom - visibleDockedPixels)
        m_dockedEdges |= Qt::BottomEdge;

    if (m_dockedEdges != Qt::Edges())
        return;

    move(qBound(area.left(), x(), qMax(area.left(), area.right() - width() + 1)),
         qBound(area.top(), y(), qMax(area.top(), area.bottom() - height() + 1)));
}

QScreen *FloatingWindow::screenForPosition(const QPoint &position) const
{
    QScreen *screen = QGuiApplication::screenAt(position);
    return screen ? screen : QGuiApplication::primaryScreen();
}

void FloatingWindow::snapToScreenEdge(const QPoint &cursorPosition)
{
    QScreen *screen = screenForPosition(cursorPosition);
    if (!screen) {
        keepOnScreen();
        return;
    }

    const QRect area = screen->availableGeometry();
    const int right = area.right() + 1;
    const int bottom = area.bottom() + 1;
    const int windowRight = x() + width();
    const int windowBottom = y() + height();

    m_dockedEdges = {};
    if (x() <= area.left() + snapDistance) {
        m_dockedEdges |= Qt::LeftEdge;
        move(area.left() - width() + visibleDockedPixels, y());
    } else if (windowRight >= right - snapDistance) {
        m_dockedEdges |= Qt::RightEdge;
        move(right - visibleDockedPixels, y());
    }

    if (y() <= area.top() + snapDistance) {
        m_dockedEdges |= Qt::TopEdge;
        move(x(), area.top() - height() + visibleDockedPixels);
    } else if (windowBottom >= bottom - snapDistance) {
        m_dockedEdges |= Qt::BottomEdge;
        move(x(), bottom - visibleDockedPixels);
    }

    if (m_dockedEdges == Qt::Edges())
        keepOnScreen();
}

void FloatingWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    keepOnScreen();
    emit visibilityChanged(true);
}

void FloatingWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}
