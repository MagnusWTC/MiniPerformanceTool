#include "MetricsController.h"

#include <QSignalSpy>
#include <QTest>

#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace
{
class FakeProvider final : public MetricsProvider
{
public:
    explicit FakeProvider(std::vector<MetricsSnapshot> snapshots)
        : m_snapshots(std::move(snapshots))
    {
    }

    MetricsSnapshot sample() override
    {
        ++calls;
        if (m_snapshots.empty())
            return {};

        const auto index = qMin(static_cast<std::size_t>(calls - 1), m_snapshots.size() - 1);
        return m_snapshots[index];
    }

    int calls = 0;

private:
    std::vector<MetricsSnapshot> m_snapshots;
};

void compareSnapshot(const MetricsSnapshot &actual, const MetricsSnapshot &expected)
{
    QCOMPARE(actual.cpuUsagePercent, expected.cpuUsagePercent);
    QCOMPARE(actual.memoryUsagePercent, expected.memoryUsagePercent);
    QCOMPARE(actual.timestampMs, expected.timestampMs);
    QCOMPARE(actual.valid, expected.valid);
}
}

class MetricsControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void forwardsInitialInvalidAndSubsequentValidSamples();
    void startSchedulesUpdatesAndStopCancelsThem();
    void intervalBounds_data();
    void intervalBounds();
};

void MetricsControllerTest::forwardsInitialInvalidAndSubsequentValidSamples()
{
    const MetricsSnapshot initial{0.0, 37.5, 1000, false};
    const MetricsSnapshot subsequent{92.25, 46.75, 2000, true};
    auto provider = std::make_unique<FakeProvider>(
        std::vector<MetricsSnapshot>{initial, subsequent});
    MetricsController controller(std::move(provider));
    QSignalSpy spy(&controller, &MetricsController::metricsUpdated);
    QVERIFY(spy.isValid());

    controller.sampleNow();
    controller.sampleNow();

    QCOMPARE(spy.count(), 2);
    compareSnapshot(qvariant_cast<MetricsSnapshot>(spy.at(0).at(0)), initial);
    compareSnapshot(qvariant_cast<MetricsSnapshot>(spy.at(1).at(0)), subsequent);
}

void MetricsControllerTest::startSchedulesUpdatesAndStopCancelsThem()
{
    auto provider = std::make_unique<FakeProvider>(
        std::vector<MetricsSnapshot>{{12.5, 35.0, 1000, true}});
    auto *fakeProvider = provider.get();
    MetricsController controller(std::move(provider));
    controller.setIntervalMs(100);
    QSignalSpy spy(&controller, &MetricsController::metricsUpdated);
    QVERIFY(spy.isValid());
    QVERIFY(!controller.isRunning());

    controller.start();
    QVERIFY(controller.isRunning());
    // Starting provides a baseline immediately; subsequent samples must come
    // from the event loop rather than requiring another explicit sampleNow().
    QCOMPARE(spy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 2, 2000);

    controller.stop();
    QVERIFY(!controller.isRunning());
    const int countAtStop = spy.count();
    const int callsAtStop = fakeProvider->calls;
    QTest::qWait(300);
    QCOMPARE(spy.count(), countAtStop);
    QCOMPARE(fakeProvider->calls, callsAtStop);

    controller.start();
    QVERIFY(controller.isRunning());
    QCOMPARE(spy.count(), countAtStop + 1);
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= countAtStop + 2, 2000);
    controller.stop();
}

void MetricsControllerTest::intervalBounds_data()
{
    QTest::addColumn<int>("requested");
    QTest::addColumn<int>("expected");
    QTest::newRow("negative") << -100 << 100;
    QTest::newRow("zero") << 0 << 100;
    QTest::newRow("minimum") << 100 << 100;
    QTest::newRow("normal") << 1250 << 1250;
    QTest::newRow("maximum") << 60000 << 60000;
    QTest::newRow("too-large") << std::numeric_limits<int>::max() << 60000;
}

void MetricsControllerTest::intervalBounds()
{
    QFETCH(int, requested);
    QFETCH(int, expected);
    MetricsController controller(std::make_unique<FakeProvider>(
        std::vector<MetricsSnapshot>{}));

    controller.setIntervalMs(requested);

    QCOMPARE(controller.intervalMs(), expected);
    QVERIFY(!controller.isRunning());
}

QTEST_GUILESS_MAIN(MetricsControllerTest)

#include "MetricsControllerTest.moc"
