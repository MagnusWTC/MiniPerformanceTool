#include "FloatingWindow.h"
#include "SettingsManager.h"
#include "ThemeAnimation.h"

#include <QDir>
#include <QImage>
#include <QSignalSpy>
#include <QtTest>

class WidgetBehaviorTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void themeTransitions();
    void themeReversesContinuously();
    void floatingThresholds_data();
    void floatingThresholds();
    void doubleClickOpensMainWindow();
    void renderFloatingStates();

private:
    QPoint m_originalPosition;
};

namespace {
const QColor normalColor(52, 211, 153);
const QColor alertColor(248, 87, 105);

MetricsSnapshot snapshot(double cpu, double memory, bool valid = true)
{
    MetricsSnapshot result;
    result.cpuUsagePercent = cpu;
    result.memoryUsagePercent = memory;
    result.valid = valid;
    return result;
}

bool saveRender(QWidget &widget, const QString &fileName)
{
    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);
    return image.save(QDir::current().filePath(fileName));
}
}

void WidgetBehaviorTest::initTestCase()
{
    m_originalPosition = SettingsManager::floatingWindowPosition();
}

void WidgetBehaviorTest::cleanupTestCase()
{
    SettingsManager::setFloatingWindowPosition(m_originalPosition);
}

void WidgetBehaviorTest::themeTransitions()
{
    ThemeAnimation theme;
    QSignalSpy changes(&theme, &ThemeAnimation::colorChanged);
    QCOMPARE(theme.color(), normalColor);
    theme.setAlert(true);
    QTRY_COMPARE_WITH_TIMEOUT(theme.color(), alertColor, 2000);
    QVERIFY(changes.count() > 1);
    theme.setAlert(false);
    QTRY_COMPARE_WITH_TIMEOUT(theme.color(), normalColor, 2000);
}

void WidgetBehaviorTest::themeReversesContinuously()
{
    ThemeAnimation theme;
    theme.setAlert(true);
    QTRY_VERIFY_WITH_TIMEOUT(theme.color() != normalColor, 1000);
    QTest::qWait(180);
    const QColor intermediate = theme.color();
    QVERIFY(intermediate != normalColor);
    QVERIFY(intermediate != alertColor);
    theme.setAlert(false);
    // Reversal must start from the visible color, without jumping to either endpoint.
    QCOMPARE(theme.color(), intermediate);
    QTRY_COMPARE_WITH_TIMEOUT(theme.color(), normalColor, 2000);
    theme.setAlert(true);
    QTRY_COMPARE_WITH_TIMEOUT(theme.color(), alertColor, 2000);
}

void WidgetBehaviorTest::floatingThresholds_data()
{
    QTest::addColumn<double>("cpu");
    QTest::addColumn<double>("memory");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("alert");
    QTest::newRow("both-below") << 30.0 << 40.0 << true << false;
    QTest::newRow("cpu-above") << 80.1 << 40.0 << true << true;
    QTest::newRow("memory-above") << 30.0 << 70.1 << true << true;
    QTest::newRow("both-above") << 91.0 << 85.0 << true << true;
    QTest::newRow("equal-thresholds") << 80.0 << 70.0 << true << false;
    QTest::newRow("invalid-sample") << 99.0 << 99.0 << false << false;
}

void WidgetBehaviorTest::floatingThresholds()
{
    QFETCH(double, cpu);
    QFETCH(double, memory);
    QFETCH(bool, valid);
    QFETCH(bool, alert);
    FloatingWindow window;
    window.setThresholds(80.0, 70.0);
    auto *theme = window.findChild<ThemeAnimation *>();
    QVERIFY(theme);
    // Begin red so normal/equality rows also verify a real return transition.
    window.setMetrics(snapshot(99.0, 99.0));
    QTRY_COMPARE_WITH_TIMEOUT(theme->color(), alertColor, 2000);
    window.setMetrics(snapshot(cpu, memory, valid));
    QTRY_COMPARE_WITH_TIMEOUT(theme->color(), alert ? alertColor : normalColor, 2000);
}

void WidgetBehaviorTest::doubleClickOpensMainWindow()
{
    FloatingWindow window;
    QSignalSpy opened(&window, &FloatingWindow::openMainWindowRequested);
    window.show();
    QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, window.rect().center());
    QCOMPARE(opened.count(), 1);
    QTest::mouseDClick(&window, Qt::RightButton, Qt::NoModifier, window.rect().center());
    QCOMPARE(opened.count(), 1);
}

void WidgetBehaviorTest::renderFloatingStates()
{
    FloatingWindow window;
    window.setThresholds(80.0, 80.0);
    auto *theme = window.findChild<ThemeAnimation *>();
    QVERIFY(theme);
    window.setMetrics(snapshot(24.5, 46.8));
    window.show();
    QTest::qWait(700);
    QCOMPARE(theme->color(), normalColor);
    QVERIFY(saveRender(window, QStringLiteral("floating-normal.png")));
    window.setMetrics(snapshot(92.4, 86.7));
    QTRY_COMPARE_WITH_TIMEOUT(theme->color(), alertColor, 2000);
    QVERIFY(saveRender(window, QStringLiteral("floating-alert.png")));
}

QTEST_MAIN(WidgetBehaviorTest)
#include "WidgetBehaviorTest.moc"
