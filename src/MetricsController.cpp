#include "MetricsController.h"

#include "WindowsMetricsProvider.h"

#include <QMetaType>
#include <QTimer>

MetricsController::MetricsController(QObject *parent)
    : MetricsController(std::make_unique<WindowsMetricsProvider>(), parent)
{
}

MetricsController::MetricsController(std::unique_ptr<MetricsProvider> provider, QObject *parent)
    : QObject(parent)
    , m_provider(std::move(provider))
    , m_timer(new QTimer(this))
{
    qRegisterMetaType<MetricsSnapshot>("MetricsSnapshot");
    m_timer->setInterval(m_intervalMs);
    connect(m_timer, &QTimer::timeout, this, &MetricsController::sampleNow);
}

int MetricsController::intervalMs() const
{
    return m_intervalMs;
}

bool MetricsController::isRunning() const
{
    return m_timer->isActive();
}

void MetricsController::start()
{
    if (!m_timer->isActive())
        m_timer->start();

    sampleNow();
}

void MetricsController::stop()
{
    m_timer->stop();
}

void MetricsController::setIntervalMs(int intervalMs)
{
    m_intervalMs = qBound(100, intervalMs, 60000);
    m_timer->setInterval(m_intervalMs);
}

void MetricsController::sampleNow()
{
    if (m_provider)
        emit metricsUpdated(m_provider->sample());
}
