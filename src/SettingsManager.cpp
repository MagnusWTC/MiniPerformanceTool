#include "SettingsManager.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>

namespace {
QSettings settings()
{
    return QSettings(QStringLiteral("MiniPerformanceTool"), QStringLiteral("MiniPerformanceTool"));
}
}

double SettingsManager::cpuThreshold()
{
    return settings().value(QStringLiteral("thresholds/cpu"), 80.0).toDouble();
}

double SettingsManager::memoryThreshold()
{
    return settings().value(QStringLiteral("thresholds/memory"), 80.0).toDouble();
}

void SettingsManager::setCpuThreshold(double value)
{
    auto s = settings();
    s.setValue(QStringLiteral("thresholds/cpu"), value);
}

void SettingsManager::setMemoryThreshold(double value)
{
    auto s = settings();
    s.setValue(QStringLiteral("thresholds/memory"), value);
}

QPoint SettingsManager::floatingWindowPosition()
{
    return settings().value(QStringLiteral("floatingWindow/position"), QPoint(24, 24)).toPoint();
}

void SettingsManager::setFloatingWindowPosition(const QPoint &position)
{
    auto s = settings();
    s.setValue(QStringLiteral("floatingWindow/position"), position);
}

bool SettingsManager::startWithWindows()
{
#ifdef Q_OS_WIN
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    return run.contains(QStringLiteral("MiniPerformanceTool"));
#else
    return false;
#endif
}

bool SettingsManager::setStartWithWindows(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    const QString key = QStringLiteral("MiniPerformanceTool");
    if (enabled) {
        const QString executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        run.setValue(key, QStringLiteral("\"%1\"").arg(executable));
    } else {
        run.remove(key);
    }
    run.sync();
    return run.status() == QSettings::NoError;
#else
    Q_UNUSED(enabled);
    return false;
#endif
}
