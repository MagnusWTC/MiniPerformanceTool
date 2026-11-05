#include "ThemeAnimation.h"

#include <QEasingCurve>
#include <QVariantAnimation>

ThemeAnimation::ThemeAnimation(QObject *parent)
    : QObject(parent), m_transition(new QVariantAnimation(this))
{
    m_transition->setDuration(650);
    m_transition->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_transition, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_color = value.value<QColor>();
        emit colorChanged(m_color);
    });
}

QColor ThemeAnimation::color() const
{
    return m_color;
}

void ThemeAnimation::setAlert(bool alert)
{
    if (m_alert == alert)
        return;
    m_alert = alert;
    m_transition->stop();
    m_transition->setStartValue(m_color);
    m_transition->setEndValue(alert ? QColor(248, 87, 105) : QColor(52, 211, 153));
    m_transition->start();
}
