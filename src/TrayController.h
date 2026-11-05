#pragma once

#include <QObject>

class QAction;
class FloatingWindow;
class QMenu;
class QSystemTrayIcon;
class QWidget;

class TrayController final : public QObject
{
    Q_OBJECT
public:
    explicit TrayController(QWidget *mainWindow, FloatingWindow *floatingWindow,
                            QObject *parent = nullptr);
    ~TrayController() override;

public slots:
    void showMainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_mainWindow;
    FloatingWindow *m_floatingWindow;
    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    QAction *m_toggleFloating;
};
