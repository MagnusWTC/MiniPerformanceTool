#pragma once

#include <QMainWindow>
#include "MetricsProvider.h"

class QCheckBox;
class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
class QProgressBar;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void setMetrics(const MetricsSnapshot &snapshot);
    void setThresholds(double cpu, double memory);
    void setFloatingVisible(bool visible);

signals:
    void thresholdsChanged(double cpu, double memory);
    void floatingVisibilityRequested(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void applyThresholds();
    void refreshStatus();

    QLabel *m_cpuValue;
    QLabel *m_memoryValue;
    QLabel *m_status;
    QProgressBar *m_cpuBar;
    QProgressBar *m_memoryBar;
    QDoubleSpinBox *m_cpuThreshold;
    QDoubleSpinBox *m_memoryThreshold;
    QCheckBox *m_floatingVisible;
    MetricsSnapshot m_snapshot;
    double m_appliedCpuThreshold = 80.0;
    double m_appliedMemoryThreshold = 80.0;
};
