// tst_workerpool.cpp — Direct unit tests for the pure-C++ core WorkerPool.
//
// These tests deliberately include ONLY the core header and use ONLY std::
// types in the bodies — no Qt engine/GUI types — to prove the core API is
// usable (and correct) without the GUI layer. QtTest is used purely as the
// runner harness, consistent with the rest of the suite.
#include <QObject>
#include <QTest>
#include "../core/WorkerPool.h"

// ── Helpers (std:: only) ─────────────────────────────────────────────────────
static ManagerInfo makeManager(const std::string &name, int cpus = 4)
{
    ManagerInfo m;
    m.name = name;
    m.address = "127.0.0.1";
    m.processorCount = cpus;
    return m;
}

static WorkerInfo makeWorker(const std::string &id, const std::string &mgr,
                             const std::string &name = "")
{
    WorkerInfo w;
    w.id = id;
    w.name = name.empty() ? id : name;
    w.managerName = mgr;
    w.type = "Disk";
    return w;
}

class WorkerPoolTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Empty / initial state ────────────────────────────────────────────────
    void empty_initialState() {
        WorkerPool pool;
        QCOMPARE(pool.managerCount(), 0);
        QCOMPARE(pool.totalWorkerCount(), 0);
        QVERIFY(!pool.isValid());          // need ≥1 manager
        QVERIFY(!pool.hasManager("nope"));
    }

    // ── addManager ───────────────────────────────────────────────────────────
    void addManager_succeeds() {
        WorkerPool pool;
        QVERIFY(pool.addManager(makeManager("mgr1")));
        QCOMPARE(pool.managerCount(), 1);
        QVERIFY(pool.hasManager("mgr1"));
    }
    void addManager_duplicateFails() {
        WorkerPool pool;
        QVERIFY(pool.addManager(makeManager("mgr1")));
        QVERIFY(!pool.addManager(makeManager("mgr1")));
        QCOMPARE(pool.managerCount(), 1);
        QVERIFY(pool.lastError().find("already exists") != std::string::npos);
    }
    void addManager_multiple() {
        WorkerPool pool;
        QVERIFY(pool.addManager(makeManager("a")));
        QVERIFY(pool.addManager(makeManager("b")));
        QVERIFY(pool.addManager(makeManager("c")));
        QCOMPARE(pool.managerCount(), 3);
    }

    // ── removeManager ────────────────────────────────────────────────────────
    void removeManager_succeeds() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        QVERIFY(pool.removeManager("mgr1"));
        QCOMPARE(pool.managerCount(), 0);
        QVERIFY(!pool.hasManager("mgr1"));
    }
    void removeManager_missingFails() {
        WorkerPool pool;
        QVERIFY(!pool.removeManager("ghost"));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }
    void removeManager_dropsItsWorkers() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        pool.addWorker("mgr1", makeWorker("w1", "mgr1"));
        pool.addWorker("mgr1", makeWorker("w2", "mgr1"));
        QCOMPARE(pool.totalWorkerCount(), 2);
        pool.removeManager("mgr1");
        QCOMPARE(pool.totalWorkerCount(), 0);
    }

    // ── addWorker ────────────────────────────────────────────────────────────
    void addWorker_succeeds() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        QVERIFY(pool.addWorker("mgr1", makeWorker("w1", "mgr1")));
        QCOMPARE(pool.workerCount("mgr1"), 1);
        QVERIFY(pool.hasWorker("mgr1", "w1"));
    }
    void addWorker_toMissingManagerFails() {
        WorkerPool pool;
        QVERIFY(!pool.addWorker("ghost", makeWorker("w1", "ghost")));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }
    void addWorker_duplicateIdFails() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        QVERIFY(pool.addWorker("mgr1", makeWorker("w1", "mgr1")));
        QVERIFY(!pool.addWorker("mgr1", makeWorker("w1", "mgr1")));
        QCOMPARE(pool.workerCount("mgr1"), 1);
        QVERIFY(pool.lastError().find("already exists") != std::string::npos);
    }
    void addWorker_sameIdDifferentManagersOk() {
        WorkerPool pool;
        pool.addManager(makeManager("a"));
        pool.addManager(makeManager("b"));
        QVERIFY(pool.addWorker("a", makeWorker("w1", "a")));
        QVERIFY(pool.addWorker("b", makeWorker("w1", "b")));
        QCOMPARE(pool.totalWorkerCount(), 2);
    }

    // ── removeWorker ─────────────────────────────────────────────────────────
    void removeWorker_succeeds() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        pool.addWorker("mgr1", makeWorker("w1", "mgr1"));
        QVERIFY(pool.removeWorker("mgr1", "w1"));
        QVERIFY(!pool.hasWorker("mgr1", "w1"));
        QCOMPARE(pool.workerCount("mgr1"), 0);
    }
    void removeWorker_missingWorkerFails() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        QVERIFY(!pool.removeWorker("mgr1", "ghost"));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }
    void removeWorker_missingManagerFails() {
        WorkerPool pool;
        QVERIFY(!pool.removeWorker("ghost", "w1"));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }

    // ── updateWorker ─────────────────────────────────────────────────────────
    void updateWorker_succeeds() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        pool.addWorker("mgr1", makeWorker("w1", "mgr1"));
        WorkerInfo upd = makeWorker("w1", "mgr1", "Renamed");
        upd.queueDepth = 16;
        upd.targets.push_back("C:");
        QVERIFY(pool.updateWorker(upd));
        const auto &ws = pool.workers("mgr1");
        QCOMPARE(ws.size(), size_t(1));
        QCOMPARE(ws[0].name, std::string("Renamed"));
        QCOMPARE(ws[0].queueDepth, 16);
        QCOMPARE(ws[0].targets.size(), size_t(1));
    }
    void updateWorker_missingManagerFails() {
        WorkerPool pool;
        QVERIFY(!pool.updateWorker(makeWorker("w1", "ghost")));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }
    void updateWorker_missingWorkerFails() {
        WorkerPool pool;
        pool.addManager(makeManager("mgr1"));
        QVERIFY(!pool.updateWorker(makeWorker("ghost", "mgr1")));
        QVERIFY(pool.lastError().find("not found") != std::string::npos);
    }

    // ── Counts and queries ───────────────────────────────────────────────────
    void workerCount_perManagerAndTotal() {
        WorkerPool pool;
        pool.addManager(makeManager("a"));
        pool.addManager(makeManager("b"));
        pool.addWorker("a", makeWorker("a1", "a"));
        pool.addWorker("a", makeWorker("a2", "a"));
        pool.addWorker("b", makeWorker("b1", "b"));
        QCOMPARE(pool.workerCount("a"), 2);
        QCOMPARE(pool.workerCount("b"), 1);
        QCOMPARE(pool.totalWorkerCount(), 3);
        // workerCount("") aliases totalWorkerCount()
        QCOMPARE(pool.workerCount(), 3);
    }
    void workerCount_unknownManagerIsZero() {
        WorkerPool pool;
        QCOMPARE(pool.workerCount("ghost"), 0);
    }
    void workers_unknownManagerReturnsEmpty() {
        WorkerPool pool;
        const auto &ws = pool.workers("ghost");
        QVERIFY(ws.empty());
    }
    void managerInfos_returnsAll() {
        WorkerPool pool;
        pool.addManager(makeManager("a", 2));
        pool.addManager(makeManager("b", 8));
        const auto infos = pool.managerInfos();
        QCOMPARE(infos.size(), size_t(2));
        QCOMPARE(infos[0].name, std::string("a"));
        QCOMPARE(infos[0].processorCount, 2);
        QCOMPARE(infos[1].processorCount, 8);
    }

    // ── Validity ─────────────────────────────────────────────────────────────
    void isValid_requiresManagerWithWorker() {
        WorkerPool pool;
        QVERIFY(!pool.isValid());               // no managers
        pool.addManager(makeManager("mgr1"));
        QVERIFY(!pool.isValid());               // manager but no workers
        pool.addWorker("mgr1", makeWorker("w1", "mgr1"));
        QVERIFY(pool.isValid());                // now valid
    }
    void isValid_falseIfAnyManagerEmpty() {
        WorkerPool pool;
        pool.addManager(makeManager("a"));
        pool.addManager(makeManager("b"));
        pool.addWorker("a", makeWorker("a1", "a"));
        // 'b' has no workers → whole pool invalid
        QVERIFY(!pool.isValid());
        pool.addWorker("b", makeWorker("b1", "b"));
        QVERIFY(pool.isValid());
    }

    // ── clear ────────────────────────────────────────────────────────────────
    void clear_resetsEverything() {
        WorkerPool pool;
        pool.addManager(makeManager("a"));
        pool.addWorker("a", makeWorker("a1", "a"));
        pool.clear();
        QCOMPARE(pool.managerCount(), 0);
        QCOMPARE(pool.totalWorkerCount(), 0);
        QVERIFY(!pool.isValid());
    }
};

QTEST_GUILESS_MAIN(WorkerPoolTest)
#include "tst_workerpool.moc"
