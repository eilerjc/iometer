// tst_testrunner.cpp — Direct unit tests for the pure-C++ core TestRunner.
//
// Exercises the state machine (Idle→Starting→Running→Stopping→Stopped / Error),
// guard conditions, and the std::function callbacks. Core header only; std::
// types only in the bodies.
#include <QObject>
#include <QTest>
#include "../core/TestRunner.h"

using State = TestRunner::State;

// ── Helpers (std:: only) ─────────────────────────────────────────────────────
static std::vector<WorkerInfo> oneWorker()
{
    WorkerInfo w;
    w.id = "w1";
    w.name = "Worker 1";
    w.managerName = "mgr1";
    w.type = "Disk";
    return { w };
}

static std::vector<AccessSpec> oneSpec()
{
    AccessSpec s;
    s.name = "64 KiB; 100% Read; 0% random";
    AccessSpecLine l;
    l.sizeBytes = 65536; l.readPercent = 100; l.seqPercent = 100; l.ofSize = 100;
    s.lines.push_back(l);
    return { s };
}

static TestConfig cfg10s()
{
    TestConfig c;
    c.runSeconds = 10;
    c.description = "unit-test run";
    return c;
}

class TestRunnerTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Initial state ────────────────────────────────────────────────────────
    void initial_isIdle() {
        TestRunner r;
        QVERIFY(r.isIdle());
        QVERIFY(!r.isRunning());
        QVERIFY(r.state() == State::Idle);
        QCOMPARE(r.activeWorkers(), 0);
        QVERIFY(r.results().empty());
    }

    // ── startTest happy path ─────────────────────────────────────────────────
    void start_validGoesRunning() {
        TestRunner r;
        QVERIFY(r.startTest(cfg10s(), oneWorker(), oneSpec()));
        QVERIFY(r.isRunning());
        QVERIFY(r.state() == State::Running);
        QCOMPARE(r.activeWorkers(), 1);
    }
    void start_storesConfigWorkersSpecs() {
        TestRunner r;
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        QCOMPARE(r.config().runSeconds, 10);
        QCOMPARE(r.config().description, std::string("unit-test run"));
        QCOMPARE(r.workers().size(), size_t(1));
        QCOMPARE(r.specs().size(), size_t(1));
        QCOMPARE(r.workers()[0].id, std::string("w1"));
    }
    void start_elapsedIsNonNegativeWhileRunning() {
        TestRunner r;
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        QVERIFY(r.elapsedSeconds() >= 0.0);
    }

    // ── startTest guard conditions ───────────────────────────────────────────
    void start_noWorkersFails() {
        TestRunner r;
        QVERIFY(!r.startTest(cfg10s(), {}, oneSpec()));
        QVERIFY(r.state() == State::Error);
        QVERIFY(r.lastError().find("no workers") != std::string::npos);
    }
    void start_noSpecsFails() {
        TestRunner r;
        QVERIFY(!r.startTest(cfg10s(), oneWorker(), {}));
        QVERIFY(r.state() == State::Error);
        QVERIFY(r.lastError().find("no access specs") != std::string::npos);
    }
    void start_whenNotIdleFails() {
        TestRunner r;
        QVERIFY(r.startTest(cfg10s(), oneWorker(), oneSpec()));
        // Second start while running must be rejected
        QVERIFY(!r.startTest(cfg10s(), oneWorker(), oneSpec()));
        QVERIFY(r.lastError().find("not idle") != std::string::npos);
    }

    // ── stopTest ─────────────────────────────────────────────────────────────
    void stop_fromRunningGoesStopped() {
        TestRunner r;
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        QVERIFY(r.stopTest());
        QVERIFY(r.state() == State::Stopped);
        QVERIFY(!r.isRunning());
        QCOMPARE(r.activeWorkers(), 0);
    }
    void stop_whenNotRunningFails() {
        TestRunner r;
        QVERIFY(!r.stopTest());          // idle → cannot stop
        QVERIFY(r.state() == State::Error);
        QVERIFY(r.lastError().find("not running") != std::string::npos);
    }

    // ── stopAll ──────────────────────────────────────────────────────────────
    void stopAll_whenIdleIsNoOp() {
        TestRunner r;
        QVERIFY(r.stopAll());            // already stopped → true
        QVERIFY(r.isIdle());             // state unchanged
    }
    void stopAll_fromRunningGoesStopped() {
        TestRunner r;
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        QVERIFY(r.stopAll());
        QVERIFY(r.state() == State::Stopped);
    }
    void stopAll_whenErrorIsNoOp() {
        TestRunner r;
        r.startTest(cfg10s(), {}, oneSpec());  // forces Error
        QVERIFY(r.state() == State::Error);
        QVERIFY(r.stopAll());                  // Error → returns true, stays Error
        QVERIFY(r.state() == State::Error);
    }

    // ── Callbacks ────────────────────────────────────────────────────────────
    //
    // NOTE: the capture target is declared BEFORE the TestRunner in every case.
    // ~TestRunner() calls stopAll() when the runner is still Running, which
    // fires the callbacks; if the captured local were declared after `r` it
    // would be destroyed first and the destructor's callback would touch freed
    // memory (a real use-after-free, surfaced by coverage instrumentation).
    void callback_stateChangedFiresOnStart() {
        std::vector<State> seen;
        TestRunner r;
        r.onStateChanged = [&](State s){ seen.push_back(s); };
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        // Expect Starting then Running
        QVERIFY(seen.size() >= 2);
        QVERIFY(seen.front() == State::Starting);
        QVERIFY(seen.back()  == State::Running);
    }
    void callback_statusMessageFires() {
        std::vector<std::string> msgs;
        TestRunner r;
        r.onStatusMessage = [&](const std::string &m){ msgs.push_back(m); };
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        QVERIFY(!msgs.empty());
        QVERIFY(msgs[0].find("Starting test") != std::string::npos);
    }
    void callback_errorOccurredFires() {
        std::string err;
        TestRunner r;
        r.onErrorOccurred = [&](const std::string &m){ err = m; };
        r.startTest(cfg10s(), {}, oneSpec());   // no workers → error
        QVERIFY(!err.empty());
        QVERIFY(err.find("no workers") != std::string::npos);
    }
    void callback_fullLifecycleStateSequence() {
        std::vector<State> seen;
        TestRunner r;
        r.onStateChanged = [&](State s){ seen.push_back(s); };
        r.startTest(cfg10s(), oneWorker(), oneSpec());
        r.stopTest();
        // Starting, Running, Stopping, Stopped
        QVERIFY(seen.size() >= 4);
        QVERIFY(seen[0] == State::Starting);
        QVERIFY(seen[1] == State::Running);
        QVERIFY(seen[2] == State::Stopping);
        QVERIFY(seen[3] == State::Stopped);
    }

    // ── setState dedup: no duplicate callback for same state ─────────────────
    void setState_noCallbackWhenUnchanged() {
        int calls = 0;
        TestRunner r;
        r.startTest(cfg10s(), oneWorker(), oneSpec());  // ends in Running
        r.onStateChanged = [&](State){ ++calls; };
        // stopAll from Running drives Stopping→Stopped (2 distinct changes)
        r.stopAll();
        QCOMPARE(calls, 2);
    }
};

QTEST_GUILESS_MAIN(TestRunnerTest)
#include "tst_testrunner.moc"
