#include "WindowsMetricsProvider.h"

#include <QDateTime>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace
{
#ifdef Q_OS_WIN
quint64 fileTimeToUInt64(const FILETIME &fileTime)
{
    ULARGE_INTEGER value;
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return value.QuadPart;
}
#endif
}

WindowsMetricsProvider::WindowsMetricsProvider() = default;

MetricsSnapshot WindowsMetricsProvider::sample()
{
    MetricsSnapshot snapshot;
    snapshot.timestampMs = QDateTime::currentMSecsSinceEpoch();

#ifdef Q_OS_WIN
    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);

    const bool cpuRead = GetSystemTimes(&idleTime, &kernelTime, &userTime) != 0;
    const bool memoryRead = GlobalMemoryStatusEx(&memoryStatus) != 0;

    bool cpuIntervalRead = false;
    if (cpuRead) {
        const quint64 idle = fileTimeToUInt64(idleTime);
        const quint64 kernel = fileTimeToUInt64(kernelTime);
        const quint64 user = fileTimeToUInt64(userTime);

        if (m_hasCpuBaseline) {
            const quint64 idleDelta = idle - m_previousIdle;
            const quint64 totalDelta = (kernel - m_previousKernel) + (user - m_previousUser);
            if (totalDelta > 0) {
                const double busy = 1.0 - (static_cast<double>(idleDelta) /
                                           static_cast<double>(totalDelta));
                snapshot.cpuUsagePercent = qBound(0.0, busy * 100.0, 100.0);
                cpuIntervalRead = true;
            }
        }

        m_previousIdle = idle;
        m_previousKernel = kernel;
        m_previousUser = user;
        m_hasCpuBaseline = true;
    }

    if (memoryRead && memoryStatus.ullTotalPhys > 0) {
        const auto available = static_cast<double>(memoryStatus.ullAvailPhys);
        const auto total = static_cast<double>(memoryStatus.ullTotalPhys);
        snapshot.memoryUsagePercent = qBound(0.0, (1.0 - available / total) * 100.0, 100.0);
    }

    snapshot.valid = cpuIntervalRead && memoryRead;
#else
    // The provider remains buildable on non-Windows platforms. A platform-specific
    // implementation can be added later without changing the controller API.
    snapshot.valid = false;
#endif

    return snapshot;
}


