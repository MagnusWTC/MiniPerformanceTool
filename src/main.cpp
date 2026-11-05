#include <QApplication>
#include <QFont>
#include <QStyleFactory>

#include "FloatingWindow.h"
#include "MainWindow.h"
#include "MetricsController.h"
#include "SettingsManager.h"
#include "TrayController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Fusion avoids native Windows spin-box painting artifacts with stylesheets.
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
    QApplication::setApplicationName(QStringLiteral("MiniPerformanceTool"));
    QApplication::setOrganizationName(QStringLiteral("MiniPerformanceTool"));
    QApplication::setQuitOnLastWindowClosed(false);

    MetricsController metrics;
    FloatingWindow floatingWindow;
    MainWindow mainWindow;
    TrayController tray(&mainWindow, &floatingWindow);

    QObject::connect(&metrics, &MetricsController::metricsUpdated,
                     &floatingWindow, &FloatingWindow::setMetrics);
    QObject::connect(&metrics, &MetricsController::metricsUpdated,
                     &mainWindow, &MainWindow::setMetrics);
    QObject::connect(&mainWindow, &MainWindow::thresholdsChanged,
                     &floatingWindow, &FloatingWindow::setThresholds);
    QObject::connect(&mainWindow, &MainWindow::thresholdsChanged,
                     [](double cpu, double memory) {
        SettingsManager::setCpuThreshold(cpu);
        SettingsManager::setMemoryThreshold(memory);
    });
    QObject::connect(&mainWindow, &MainWindow::floatingVisibilityRequested,
                     &floatingWindow, &QWidget::setVisible);
    QObject::connect(&floatingWindow, &FloatingWindow::visibilityChanged,
                     &mainWindow, &MainWindow::setFloatingVisible);
    mainWindow.setThresholds(SettingsManager::cpuThreshold(), SettingsManager::memoryThreshold());

    floatingWindow.show();
    mainWindow.show();
    metrics.start();
    return app.exec();
}
