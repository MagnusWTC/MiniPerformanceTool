#pragma once

#include "MetricsProvider.h"
#include <QPoint>
#include <Qt>
#include <QWidget>

class ThemeAnimation;
class QVariantAnimation;
class QScreen;

class FloatingWindow final : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingWindow(QWidget *parent = nullptr);
    ~FloatingWindow() override;

public slots:
    void setMetrics(const MetricsSnapshot &snapshot);
    void setThresholds(double cpuThreshold, double memoryThreshold);

signals:
    void openMainWindowRequested();
    void visibilityChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void updateAlert();
    void keepOnScreen();
    void snapToScreenEdge(const QPoint &cursorPosition);
    QScreen *screenForPosition(const QPoint &position) const;
    ThemeAnimation *m_theme;
    QVariantAnimation *m_valuesAnimation;
    MetricsSnapshot m_metrics;
    double m_displayCpu = 0;
    double m_displayMemory = 0;
    double m_cpuThreshold = 80;
    double m_memoryThreshold = 80;
    bool m_alert = false;
    bool m_dragging = false;
    Qt::Edges m_dockedEdges;
    QPoint m_dragOffset;
};
