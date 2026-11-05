#pragma once

#include <QMetaType>
#include <QtGlobal>

struct MetricsSnapshot
{
    double cpuUsagePercent = 0.0;
    double memoryUsagePercent = 0.0;
    qint64 timestampMs = 0;
    bool valid = false;
};

class MetricsProvider
{
public:
    virtual ~MetricsProvider() = default;
    virtual MetricsSnapshot sample() = 0;
};

Q_DECLARE_METATYPE(MetricsSnapshot)
