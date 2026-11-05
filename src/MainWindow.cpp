#include "MainWindow.h"
#include "SettingsManager.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <cmath>

namespace {
QDoubleSpinBox *thresholdInput(QWidget *parent)
{
    auto *input = new QDoubleSpinBox(parent);
    input->setRange(1.0, 100.0);
    input->setDecimals(1);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setLocale(QLocale::c());
    input->setSuffix(QStringLiteral(" %"));
    input->setValue(80.0);
    input->setMinimumWidth(120);
    input->setMinimumHeight(32);
    return input;
}

QFrame *metricCard(const QString &title, QLabel *&value, QProgressBar *&bar,
                   QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("metricCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(12);
    auto *label = new QLabel(title, card);
    label->setObjectName(QStringLiteral("muted"));
    value = new QLabel(QStringLiteral("-- %"), card);
    value->setObjectName(QStringLiteral("metricValue"));
    bar = new QProgressBar(card);
    bar->setRange(0, 1000);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setFixedHeight(7);
    layout->addWidget(label);
    layout->addWidget(value);
    layout->addWidget(bar);
    return card;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("轻量性能监测"));
    resize(560, 650);
    setMinimumSize(480, 640);
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(28, 26, 28, 24);
    layout->setSpacing(18);
    auto *title = new QLabel(QStringLiteral("性能概览"), this);
    title->setObjectName(QStringLiteral("title"));
    layout->addWidget(title);
    auto *subtitle = new QLabel(QStringLiteral("每秒更新系统 CPU 与物理内存使用情况"), this);
    subtitle->setObjectName(QStringLiteral("muted"));
    layout->addWidget(subtitle);

    auto *cards = new QHBoxLayout;
    cards->setSpacing(14);
    cards->addWidget(metricCard(QStringLiteral("CPU 使用率"), m_cpuValue, m_cpuBar, this));
    cards->addWidget(metricCard(QStringLiteral("内存使用率"), m_memoryValue, m_memoryBar, this));
    layout->addLayout(cards);
    m_status = new QLabel(QStringLiteral("等待首次采样…"), this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto *settings = new QFrame(this);
    settings->setObjectName(QStringLiteral("settingsCard"));
    auto *settingsLayout = new QVBoxLayout(settings);
    settingsLayout->setContentsMargins(20, 16, 20, 16);
    settingsLayout->setSpacing(12);
    settingsLayout->addWidget(new QLabel(QStringLiteral("告警阈值"), this));
    auto *form = new QFormLayout;
    m_cpuThreshold = thresholdInput(this);
    m_memoryThreshold = thresholdInput(this);
    form->addRow(QStringLiteral("CPU 超过"), m_cpuThreshold);
    form->addRow(QStringLiteral("内存超过"), m_memoryThreshold);
    settingsLayout->addLayout(form);
    auto *hint = new QLabel(QStringLiteral("任一指标超过阈值，悬浮窗将平滑变红。"), this);
    hint->setObjectName(QStringLiteral("muted"));
    hint->setWordWrap(true);
    settingsLayout->addWidget(hint);
    auto *startWithWindows = new QCheckBox(QStringLiteral("开机自启动"), settings);
    startWithWindows->setChecked(SettingsManager::startWithWindows());
#ifndef Q_OS_WIN
    startWithWindows->setEnabled(false);
    startWithWindows->setToolTip(QStringLiteral("仅支持 Windows"));
#endif
    settingsLayout->addWidget(startWithWindows);
    auto *buttons = new QHBoxLayout;
    auto *reset = new QPushButton(QStringLiteral("恢复默认"), this);
    auto *apply = new QPushButton(QStringLiteral("保存设置"), this);
    reset->setMinimumHeight(36);
    apply->setMinimumHeight(36);
    apply->setObjectName(QStringLiteral("primary"));
    buttons->addStretch();
    buttons->addWidget(reset);
    buttons->addWidget(apply);
    settingsLayout->addLayout(buttons);
    layout->addWidget(settings);

    m_floatingVisible = new QCheckBox(QStringLiteral("显示桌面悬浮窗"), this);
    m_floatingVisible->setChecked(true);
    layout->addWidget(m_floatingVisible);
    auto *footer = new QLabel(QSystemTrayIcon::isSystemTrayAvailable()
        ? QStringLiteral("关闭窗口后继续在托盘运行 · 可从托盘菜单退出")
        : QStringLiteral("当前系统托盘不可用，关闭窗口将退出程序"), this);
    footer->setObjectName(QStringLiteral("muted"));
    footer->setWordWrap(true);
    layout->addWidget(footer);
    layout->addStretch();

    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f3f6fb; }"
        "QLabel, QCheckBox { color: #243249; font-size: 13px; }"
        "QLabel#title { font-size: 24px; font-weight: 600; }"
        "QLabel#muted { color: #66758c; font-size: 12px; }"
        "QLabel#metricValue { font-size: 30px; font-weight: 600; }"
        "QFrame#metricCard, QFrame#settingsCard { background: white; border: 1px solid #e1e7ef; border-radius: 12px; }"
        "QProgressBar { border: none; border-radius: 3px; background: #eaf0f6; }"
        "QProgressBar::chunk { background: #30ae83; border-radius: 3px; }"
        "QDoubleSpinBox { padding: 5px 10px; font-family: 'Segoe UI'; font-size: 14px; border: 1px solid #ced8e5; border-radius: 5px; background: white; color: #243249; }"
        "QDoubleSpinBox QLineEdit { font-family: 'Segoe UI'; font-size: 14px; color: #243249; background: transparent; selection-background-color: #356ddd; selection-color: white; }"
        "QPushButton { padding: 8px 15px; border: 1px solid #d4deeb; border-radius: 6px; background: #f8faff; color: #243249; }"
        "QPushButton:hover { background: #eaf0fa; }"
        "QPushButton#primary { background: #356ddd; border-color: #356ddd; color: white; }"
        "QPushButton#primary:hover { background: #295fc9; }"));

    connect(apply, &QPushButton::clicked, this, &MainWindow::applyThresholds);
    connect(startWithWindows, &QCheckBox::toggled, this, [this, startWithWindows](bool enabled) {
        if (!SettingsManager::setStartWithWindows(enabled)) {
            const QSignalBlocker blocker(startWithWindows);
            startWithWindows->setChecked(SettingsManager::startWithWindows());
            QMessageBox::warning(this, QStringLiteral("设置失败"),
                                 QStringLiteral("无法更新开机自启动设置，请检查当前用户的注册表访问权限。"));
        }
    });
    connect(reset, &QPushButton::clicked, this, [this] {
        setThresholds(80.0, 80.0);
        applyThresholds();
    });
    connect(m_floatingVisible, &QCheckBox::toggled,
            this, &MainWindow::floatingVisibilityRequested);
}

void MainWindow::setMetrics(const MetricsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_snapshot.valid = snapshot.valid && std::isfinite(snapshot.cpuUsagePercent)
        && std::isfinite(snapshot.memoryUsagePercent);
    if (m_snapshot.valid) {
        m_snapshot.cpuUsagePercent = qBound(0.0, snapshot.cpuUsagePercent, 100.0);
        m_snapshot.memoryUsagePercent = qBound(0.0, snapshot.memoryUsagePercent, 100.0);
        m_cpuValue->setText(QStringLiteral("%1 %").arg(m_snapshot.cpuUsagePercent, 0, 'f', 1));
        m_memoryValue->setText(QStringLiteral("%1 %").arg(m_snapshot.memoryUsagePercent, 0, 'f', 1));
        m_cpuBar->setValue(qRound(m_snapshot.cpuUsagePercent * 10));
        m_memoryBar->setValue(qRound(m_snapshot.memoryUsagePercent * 10));
    } else {
        m_cpuValue->setText(QStringLiteral("-- %"));
        m_memoryValue->setText(QStringLiteral("-- %"));
        m_cpuBar->setValue(0);
        m_memoryBar->setValue(0);
    }
    refreshStatus();
}

void MainWindow::setThresholds(double cpu, double memory)
{
    m_cpuThreshold->setValue(std::isfinite(cpu) ? cpu : 80.0);
    m_memoryThreshold->setValue(std::isfinite(memory) ? memory : 80.0);
    m_appliedCpuThreshold = m_cpuThreshold->value();
    m_appliedMemoryThreshold = m_memoryThreshold->value();
    refreshStatus();
}

void MainWindow::setFloatingVisible(bool visible)
{
    const QSignalBlocker blocker(m_floatingVisible);
    m_floatingVisible->setChecked(visible);
}

void MainWindow::applyThresholds()
{
    m_appliedCpuThreshold = m_cpuThreshold->value();
    m_appliedMemoryThreshold = m_memoryThreshold->value();
    emit thresholdsChanged(m_appliedCpuThreshold, m_appliedMemoryThreshold);
    refreshStatus();
}

void MainWindow::refreshStatus()
{
    if (!m_snapshot.valid) {
        m_status->setText(QStringLiteral("等待有效采样…"));
        m_status->setStyleSheet(QStringLiteral("color: #66758c;"));
        return;
    }
    const bool cpuAlert = m_snapshot.cpuUsagePercent > m_appliedCpuThreshold;
    const bool memoryAlert = m_snapshot.memoryUsagePercent > m_appliedMemoryThreshold;
    m_status->setText(cpuAlert && memoryAlert ? QStringLiteral("● CPU 与内存均超过阈值")
        : cpuAlert ? QStringLiteral("● CPU 使用率超过阈值")
        : memoryAlert ? QStringLiteral("● 内存使用率超过阈值") : QStringLiteral("● 运行正常"));
    m_status->setStyleSheet((cpuAlert || memoryAlert)
        ? QStringLiteral("color: #d24656;") : QStringLiteral("color: #228565;"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        event->ignore();
        hide();
    } else {
        event->accept();
        QApplication::quit();
    }
}
