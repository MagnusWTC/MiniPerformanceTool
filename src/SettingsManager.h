#pragma once

#include <QPoint>

class SettingsManager
{
public:
    static double cpuThreshold();
    static double memoryThreshold();
    static void setCpuThreshold(double value);
    static void setMemoryThreshold(double value);
    static QPoint floatingWindowPosition();
    static void setFloatingWindowPosition(const QPoint &position);
    static bool startWithWindows();
    static bool setStartWithWindows(bool enabled);
};
