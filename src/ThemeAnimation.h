#pragma once

#include <QColor>
#include <QObject>

class QVariantAnimation;

class ThemeAnimation final : public QObject
{
    Q_OBJECT
public:
    explicit ThemeAnimation(QObject *parent = nullptr);
    QColor color() const;
    void setAlert(bool alert);

signals:
    void colorChanged(const QColor &color);

private:
    QColor m_color{52, 211, 153};
    bool m_alert = false;
    QVariantAnimation *m_transition;
};
