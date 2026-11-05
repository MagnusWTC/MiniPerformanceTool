#pragma once

#include "MetricsProvider.h"

class WindowsMetricsProvider final : public MetricsProvider
{
public:
    WindowsMetricsProvider();
    MetricsSnapshot sample() override;

private:
    bool m_hasCpuBaseline = false;
    quint64 m_previousIdle = 0;
    quint64 m_previousKernel = 0;
    quint64 m_previousUser = 0;
};
