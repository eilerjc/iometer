// tst_demo.cpp — Tests for QtDemoEngine (simulated I/O engine)
// Verifies signals, state transitions, and configuration round-trips.
#include <QObject>
#include <QTest>
#include <QThread>
#include <QSignalSpy>
#include "QtDemoEngine.h"

class DemoEngineTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Initial state ────────────────────────────────────────────────────────
    void initialState_notRunning() {
        QtDemoEngine eng;
        QVERIFY(!eng.isRunning());
    }
    void initialState_hasManager() {
        QtDemoEngine eng;
        QVERIFY(!eng.managers().isEmpty());
    }
    void initialState_hasWorkers() {
        QtDemoEngine eng;
        QVERIFY(!eng.managers().first().workers.empty());
    }
    void initialState_hasSpecsFromLibrary() {
        QtDemoEngine eng;
        QVERIFY(eng.accessSpecs().size() >= 32);
    }

    // ── startTest / stopTest signals ─────────────────────────────────────────
    void startTest_emitsTestStarted() {
        QtDemoEngine eng;
        QSignalSpy spy(&eng, &QtIometerEngine::testStarted);
        eng.startTest();
        QCOMPARE(spy.count(), 1);
        eng.stopTest();
    }
    void startTest_setsRunning() {
        QtDemoEngine eng;
        eng.startTest();
        QVERIFY(eng.isRunning());
        eng.stopTest();
    }
    void stopTest_emitsTestStopped() {
        QtDemoEngine eng;
        eng.startTest();
        QSignalSpy spy(&eng, &QtIometerEngine::testStopped);
        eng.stopTest();
        QCOMPARE(spy.count(), 1);
    }
    void stopTest_clearsRunning() {
        QtDemoEngine eng;
        eng.startTest();
        eng.stopTest();
        QVERIFY(!eng.isRunning());
    }
    void doubleStart_ignored() {
        QtDemoEngine eng;
        QSignalSpy spy(&eng, &QtIometerEngine::testStarted);
        eng.startTest();
        eng.startTest(); // second call should be a no-op
        QCOMPARE(spy.count(), 1);
        eng.stopTest();
    }

    // ── resultsUpdated fires during test run ─────────────────────────────────
    void resultsUpdated_emittedDuringTest() {
        QtDemoEngine eng;
        QSignalSpy spy(&eng, &QtIometerEngine::resultsUpdated);
        eng.startTest();
        // Wait up to 2000ms for at least one update (timer fires every 500ms)
        QVERIFY(spy.wait(2000));
        QVERIFY(spy.count() >= 1);
        eng.stopTest();
    }
    void resultsUpdated_nonZeroValues() {
        QtDemoEngine eng;
        QSignalSpy spy(&eng, &QtIometerEngine::resultsUpdated);
        eng.startTest();
        QVERIFY(spy.wait(2000));
        eng.stopTest();
        QVERIFY(!spy.isEmpty());
        const auto results = spy.last().first().value<QVector<WorkerResult>>();
        // Should have at least one result with non-zero IOps
        bool hasNonZero = false;
        for (const auto &r : results)
            if (r.iops > 0.0) { hasNonZero = true; break; }
        QVERIFY2(hasNonZero, "All IOps were zero after demo test started");
    }
    void resultsUpdated_hasAggregateRow() {
        QtDemoEngine eng;
        QSignalSpy spy(&eng, &QtIometerEngine::resultsUpdated);
        eng.startTest();
        QVERIFY(spy.wait(2000));
        eng.stopTest();
        const auto results = spy.last().first().value<QVector<WorkerResult>>();
        bool hasAggregate = std::any_of(results.begin(), results.end(),
            [](const WorkerResult &r){ return r.isAggregate; });
        QVERIFY2(hasAggregate, "No aggregate row in demo results");
    }

    // ── stopAll behaves same as stopTest ────────────────────────────────────
    void stopAll_stopsTest() {
        QtDemoEngine eng;
        eng.startTest();
        QSignalSpy spy(&eng, &QtIometerEngine::testStopped);
        eng.stopAll();
        QCOMPARE(spy.count(), 1);
        QVERIFY(!eng.isRunning());
    }

    // ── Access spec round-trip ───────────────────────────────────────────────
    void setAccessSpecs_roundTrip() {
        QtDemoEngine eng;
        QList<AccessSpec> custom;
        AccessSpec s;
        s.name = "TestSpec";
        AccessSpecLine l; l.sizeBytes = 4096; l.readPercent = 50; l.seqPercent = 100;
        s.lines.push_back(l);
        custom.append(s);
        eng.setAccessSpecs(custom);
        QCOMPARE(eng.accessSpecs().size(), 1);
        QCOMPARE(QString::fromStdString(eng.accessSpecs()[0].name), QString("TestSpec"));
    }

    // ── TestConfig round-trip ────────────────────────────────────────────────
    void testConfig_roundTrip() {
        QtDemoEngine eng;
        TestConfig cfg;
        cfg.runSeconds  = 30;
        cfg.rampSeconds = 5;
        cfg.description = "Hello";
        eng.setTestConfig(cfg);
        const TestConfig got = eng.testConfig();
        QCOMPARE(got.runSeconds,  30);
        QCOMPARE(got.rampSeconds, 5);
        QCOMPARE(got.description, QString("Hello"));
    }

    // ── Workers and managers ──────────────────────────────────────────────────
    void managers_notEmpty() {
        QtDemoEngine eng;
        QVERIFY(!eng.managers().isEmpty());
    }
    void workers_count() {
        QtDemoEngine eng;
        // QtDemoEngine simulates a quad-core system with 4 disk workers
        QCOMPARE(eng.managers().first().workers.size(), 4);
    }
};

QTEST_GUILESS_MAIN(DemoEngineTest)
#include "tst_demo.moc"
