#pragma once

#include "MetricsProvider.h"

#include <QObject>
#include <memory>

class QTimer;

class MetricsController final : public QObject
{
    Q_OBJECT

public:
    explicit MetricsController(QObject *parent = nullptr);
    explicit MetricsController(std::unique_ptr<MetricsProvider> provider,
                               QObject *parent = nullptr);

    int intervalMs() const;
    bool isRunning() const;

public slots:
    void start();
    void stop();
    void setIntervalMs(int intervalMs);
    void sampleNow();

signals:
    void metricsUpdated(const MetricsSnapshot &snapshot);

private:
    std::unique_ptr<MetricsProvider> m_provider;
    QTimer *m_timer = nullptr;
    int m_intervalMs = 1000;
};
