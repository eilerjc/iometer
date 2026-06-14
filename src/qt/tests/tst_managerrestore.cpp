// tst_managerrestore - QtDynamoEngine's ICF manager-restore wiring over the
// shared iocore::ManagerMap: building the restore map from a parsed ICF and the
// expected/waiting query surface.
#include <QtTest>
#include "QtDynamoEngine.h"
#include "IcfFile.h"

class TestManagerRestore : public QObject {
    Q_OBJECT
    static QString fixture(const char *rel) {
        return QString(IOMETER_FIXTURE_DIR) + "/" + rel;
    }
private slots:
    void buildRestoreMap_distinctManagers();
    void buildRestoreMap_skipsEmptyManagerName();
    void loadConfig_populatesExpectedManagers();
    void loadConfig_waitingUntilManagersConnect();
    void workersForManager_groupsByManager();
};

void TestManagerRestore::buildRestoreMap_distinctManagers() {
    IcfFile::BatchWorker a;  a.managerName  = "HOST_A"; a.managerId = 1;
    IcfFile::BatchWorker a2; a2.managerName = "HOST_A"; a2.managerId = 1;  // same mgr, 2nd worker
    IcfFile::BatchWorker b;  b.managerName  = "HOST_B"; b.managerId = 2;
    std::vector<IcfFile::BatchWorker> ws { a, a2, b };

    auto map = QtDynamoEngine::buildRestoreMap(ws);
    QCOMPARE((int)map.size(), 2);     // distinct managers only
    QVERIFY(map.isWaitingList());     // all unassigned (waiting)
}

void TestManagerRestore::buildRestoreMap_skipsEmptyManagerName() {
    IcfFile::BatchWorker a;            // empty managerName
    std::vector<IcfFile::BatchWorker> ws { a };
    QCOMPARE((int)QtDynamoEngine::buildRestoreMap(ws).size(), 0);
}

void TestManagerRestore::loadConfig_populatesExpectedManagers() {
    QtDynamoEngine eng(false);        // false = don't bind the listen port
    QVERIFY(eng.loadConfig(fixture("two_managers.icf")));
    QStringList mgrs = eng.expectedManagers();
    QCOMPARE(mgrs.size(), 2);
    QVERIFY(mgrs.contains("HOST_A"));
    QVERIFY(mgrs.contains("HOST_B"));
}

void TestManagerRestore::loadConfig_waitingUntilManagersConnect() {
    QtDynamoEngine eng(false);
    QVERIFY(eng.loadConfig(fixture("nonlocal_manager.icf")));
    QVERIFY(eng.isWaitingForManagers());      // no Dynamo connected in a unit test
    QCOMPARE(eng.connectedManagerCount(), 0);
}

void TestManagerRestore::workersForManager_groupsByManager() {
    QtDynamoEngine eng(false);
    QVERIFY(eng.loadConfig(fixture("two_managers.icf")));
    auto a = eng.workersForManager("HOST_A");
    auto b = eng.workersForManager("HOST_B");
    QCOMPARE(a.size(), 1);
    QCOMPARE(a[0].name, QString("Worker A1"));
    QCOMPARE(b.size(), 1);
    QCOMPARE(b[0].name, QString("Worker B1"));
    QVERIFY(eng.workersForManager("NOPE").isEmpty());
}

QTEST_GUILESS_MAIN(TestManagerRestore)
#include "tst_managerrestore.moc"
