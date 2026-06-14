// tst_managermap - unit tests for iocore::ManagerMap (the extracted, pure-C++
// manager-restore decision logic shared by both GUIs).
#include <QtTest>
#include "ManagerMap.h"

using iocore::ManagerMap;

// Fake opaque manager handles (the front-end's running-manager pointers).
static ManagerMap::Handle H(int n) { return reinterpret_cast<ManagerMap::Handle>(static_cast<size_t>(n)); }

class TestManagerMap : public QObject {
    Q_OBJECT
private slots:
    void store_and_size();
    void retrieve_byNameAndId();
    void retrieve_localHostMatchesAnything();
    void managerLoggedIn_assignsFirstUnassignedMatch();
    void managerLoggedIn_noMatchReturnsFalse();
    void managerLoggedIn_localHostClause();
    void setIfOneManager_onlyWhenSingleUnassigned();
    void managerLoggedOut_returnsToWaiting();
    void isWaitingList_trueWhileAnyUnassigned();
    void isManagerNeeded();
    void spawn_localByEmptyAddressAndLocalName();
    void spawn_localByMachineName_noNameArg();
    void spawn_localByIsLocalAddress_withNameArg();
    void spawn_nonLocalManagerIsNotSpawned();   // the fix-B behavior
    void spawn_skipsAssignedEntries();
};

void TestManagerMap::store_and_size() {
    ManagerMap m;
    QCOMPARE((int)m.size(), 0);
    m.store("A", 1, "10.0.0.1", H(1));
    m.store("B", 2, "10.0.0.2");
    QCOMPARE((int)m.size(), 2);
    m.reset();
    QCOMPARE((int)m.size(), 0);
}

void TestManagerMap::retrieve_byNameAndId() {
    ManagerMap m;
    m.store("A", 1, "addr", H(7));
    m.store("A", 2, "addr", H(8));
    QCOMPARE(m.retrieve("A", 2), H(8));
    QCOMPARE(m.retrieve("a", 1), H(7));        // case-insensitive
    QCOMPARE(m.retrieve("A", 3), (ManagerMap::Handle)nullptr);
}

void TestManagerMap::retrieve_localHostMatchesAnything() {
    ManagerMap m;
    m.store(ManagerMap::LocalHostName, 1, "", H(5));
    // The local-host entry is returned for any name/id query.
    QCOMPARE(m.retrieve("whatever", 99), H(5));
}

void TestManagerMap::managerLoggedIn_assignsFirstUnassignedMatch() {
    ManagerMap m;
    m.store("MGR", 1, "1.2.3.4");              // unassigned
    QVERIFY(m.isWaitingList());
    QVERIFY(m.managerLoggedIn("mgr", "1.2.3.4", H(3)));   // case-insensitive match
    QVERIFY(!m.isWaitingList());
    QCOMPARE(m.entries()[0].handle, H(3));
}

void TestManagerMap::managerLoggedIn_noMatchReturnsFalse() {
    ManagerMap m;
    m.store("MGR", 1, "1.2.3.4");
    QVERIFY(!m.managerLoggedIn("MGR", "9.9.9.9", H(3)));  // address mismatch
    QVERIFY(m.isWaitingList());
}

void TestManagerMap::managerLoggedIn_localHostClause() {
    ManagerMap m;
    m.store(ManagerMap::LocalHostName, 1, "");           // empty addr + (Local)
    // Any login satisfies the local-host entry.
    QVERIFY(m.managerLoggedIn("4KMONSTER", "", H(9)));
    QCOMPARE(m.entries()[0].handle, H(9));
}

void TestManagerMap::setIfOneManager_onlyWhenSingleUnassigned() {
    ManagerMap m;
    m.store("A", 1, "addr");
    QVERIFY(m.setIfOneManager(H(2)));
    QCOMPARE(m.entries()[0].handle, H(2));
    QVERIFY(!m.setIfOneManager(H(3)));         // already assigned
    ManagerMap m2;
    m2.store("A", 1, "a"); m2.store("B", 2, "b");
    QVERIFY(!m2.setIfOneManager(H(1)));        // more than one
}

void TestManagerMap::managerLoggedOut_returnsToWaiting() {
    ManagerMap m;
    m.store("MGR", 1, "1.2.3.4");
    QVERIFY(m.managerLoggedIn("MGR", "1.2.3.4", H(5)));
    QVERIFY(!m.isWaitingList());
    QVERIFY(m.managerLoggedOut(H(5)));         // disconnect
    QVERIFY(m.isWaitingList());                // waiting again
    QCOMPARE(m.entries()[0].handle, (ManagerMap::Handle)nullptr);
    QVERIFY(!m.managerLoggedOut(H(99)));       // unknown handle
}

void TestManagerMap::isWaitingList_trueWhileAnyUnassigned() {
    ManagerMap m;
    m.store("A", 1, "a", H(1));
    m.store("B", 2, "b");                      // unassigned
    QVERIFY(m.isWaitingList());
    QVERIFY(m.managerLoggedIn("B", "b", H(2)));
    QVERIFY(!m.isWaitingList());
}

void TestManagerMap::isManagerNeeded() {
    ManagerMap m;
    m.store("A", 1, "a", H(4));
    QVERIFY(m.isManagerNeeded(H(4)));
    QVERIFY(!m.isManagerNeeded(H(99)));
}

// --- spawn decision (the heart of the restore-vs-spawn behavior) -------------
static auto neverLocal = [](const std::string &) { return false; };

void TestManagerMap::spawn_localByEmptyAddressAndLocalName() {
    ManagerMap m;
    m.store(ManagerMap::LocalHostName, 1, "");
    auto reqs = m.localManagersToSpawn(neverLocal, "4KMONSTER");
    QCOMPARE((int)reqs.size(), 1);
    QVERIFY(reqs[0].nameArg.empty());          // default local name, no -n
}

void TestManagerMap::spawn_localByMachineName_noNameArg() {
    ManagerMap m;
    m.store("4KMONSTER", 1, "somewhere");      // name == this machine
    auto reqs = m.localManagersToSpawn(neverLocal, "4KMONSTER");
    QCOMPARE((int)reqs.size(), 1);
    QVERIFY(reqs[0].nameArg.empty());
}

void TestManagerMap::spawn_localByIsLocalAddress_withNameArg() {
    ManagerMap m;
    m.store("OTHERMGR", 1, "10.0.0.41");
    auto isLocal = [](const std::string &a) { return a == "10.0.0.41"; };
    auto reqs = m.localManagersToSpawn(isLocal, "4KMONSTER");
    QCOMPARE((int)reqs.size(), 1);
    QCOMPARE(QString::fromStdString(reqs[0].nameArg), QString("OTHERMGR"));  // -n OTHERMGR
}

void TestManagerMap::spawn_nonLocalManagerIsNotSpawned() {
    // A single non-local manager (e.g. TESTMGR @ localhost) must NOT be spawned -
    // it waits for an external Dynamo. This is the behavior fix B relies on.
    ManagerMap m;
    m.store("TESTMGR", 1, "localhost");
    auto reqs = m.localManagersToSpawn(neverLocal, "4KMONSTER");
    QCOMPARE((int)reqs.size(), 0);
    QVERIFY(m.isWaitingList());                // still waiting for it
}

void TestManagerMap::spawn_skipsAssignedEntries() {
    ManagerMap m;
    m.store("4KMONSTER", 1, "", H(1));         // already assigned
    auto reqs = m.localManagersToSpawn(neverLocal, "4KMONSTER");
    QCOMPARE((int)reqs.size(), 0);
}

QTEST_APPLESS_MAIN(TestManagerMap)
#include "tst_managermap.moc"
