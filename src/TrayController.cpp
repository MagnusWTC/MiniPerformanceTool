#include "TrayController.h"
#include "FloatingWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QWidget>

namespace {
QIcon performanceIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(24, 34, 44));
    painter.setPen(QPen(QColor(52, 211, 153), 3));
    painter.drawRoundedRect(QRectF(4, 4, 56, 56), 14, 14);
    painter.setPen(QPen(QColor(52, 211, 153), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const QPointF points[] = {{13, 36}, {23, 36}, {29, 20}, {36, 46}, {42, 29}, {51, 29}};
    painter.drawPolyline(points, 6);
    return QIcon(pixmap);
}
}

TrayController::TrayController(QWidget *mainWindow, FloatingWindow *floatingWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow), m_floatingWindow(floatingWindow),
      m_tray(new QSystemTrayIcon(this)), m_menu(new QMenu), m_toggleFloating(nullptr)
{
    const QIcon icon = performanceIcon();
    QApplication::setWindowIcon(icon);
    m_tray->setIcon(icon);
    m_tray->setToolTip(QStringLiteral("MiniPerformanceTool · 性能监测"));
    m_menu->addAction(QStringLiteral("打开主界面"), this, &TrayController::showMainWindow);
    m_toggleFloating = m_menu->addAction(QStringLiteral("显示悬浮窗"));
    m_toggleFloating->setCheckable(true);
    m_toggleFloating->setChecked(m_floatingWindow->isVisible());
    connect(m_toggleFloating, &QAction::triggered, m_floatingWindow, &QWidget::setVisible);
    connect(m_floatingWindow, &FloatingWindow::visibilityChanged, m_toggleFloating, &QAction::setChecked);
    connect(m_floatingWindow, &FloatingWindow::openMainWindowRequested, this, &TrayController::showMainWindow);
    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("退出程序"), this, [this] {
        m_mainWindow->removeEventFilter(this);
        QApplication::quit();
    });
    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showMainWindow();
    });
    m_mainWindow->installEventFilter(this);
    m_tray->show();
}

TrayController::~TrayController()
{
    m_mainWindow->removeEventFilter(this);
    m_tray->hide();
    m_tray->setContextMenu(nullptr);
    delete m_menu;
}

void TrayController::showMainWindow()
{
    m_mainWindow->showNormal();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}

bool TrayController::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_mainWindow && event->type() == QEvent::Close
        && QSystemTrayIcon::isSystemTrayAvailable()) {
        static_cast<QCloseEvent *>(event)->ignore();
        m_mainWindow->hide();
        return true;
    }
    return QObject::eventFilter(watched, event);
}
