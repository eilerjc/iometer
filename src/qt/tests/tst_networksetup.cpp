// tst_networksetup.cpp - Direct unit tests for the pure-C++ core NetworkSetup.
//
// NetworkSetup is shared by both the Qt and MFC GUIs and depends on neither, so
// these tests use only std:: types and its self-contained Role/NetworkTarget.
#include <QObject>
#include <QTest>
#include "../core/NetworkSetup.h"
#include <string>
#include <vector>

using Role = NetworkSetup::Role;

class NetworkSetupTest : public QObject
{
    Q_OBJECT
private slots:

    // -- isNetworkWorker (type-string only; no domain types) ------------------
    void isNetwork_diskIsNotNetwork() {
        QVERIFY(!NetworkSetup::isNetworkWorker("Disk"));
        QVERIFY(!NetworkSetup::isNetworkWorker("DISK"));
        QVERIFY(!NetworkSetup::isNetworkWorker(""));
    }
    void isNetwork_networkVariants() {
        QVERIFY(NetworkSetup::isNetworkWorker("Network"));
        QVERIFY(NetworkSetup::isNetworkWorker("NETWORK,TCP"));
        QVERIFY(NetworkSetup::isNetworkWorker("TCP"));
        QVERIFY(NetworkSetup::isNetworkWorker("NETWORK,VI"));
    }

    // -- server / client target builders --------------------------------------
    void serverTarget_fields() {
        auto t = NetworkSetup::serverTarget("127.0.0.1");
        QVERIFY(t.role == Role::Server);
        QCOMPARE(t.localInterface, std::string("127.0.0.1"));
        QVERIFY(t.remoteAddress.empty());
    }
    void clientTarget_fields() {
        auto t = NetworkSetup::clientTarget("10.0.0.5", 12345);
        QVERIFY(t.role == Role::Client);
        QCOMPARE(t.remoteAddress, std::string("10.0.0.5"));
        QCOMPARE(t.port, 12345);
        QVERIFY(t.localInterface.empty());
    }

    // -- planLoopback ---------------------------------------------------------
    void loopback_pairOfWorkers() {
        auto plan = NetworkSetup::planLoopback(2);
        QCOMPARE(plan.size(), size_t(2));
        QVERIFY(plan[0].role == Role::Server);
        QCOMPARE(plan[0].localInterface, std::string("127.0.0.1"));
        QVERIFY(plan[1].role == Role::Client);
        QCOMPARE(plan[1].remoteAddress, std::string("127.0.0.1"));
    }
    void loopback_twoPairs() {
        auto plan = NetworkSetup::planLoopback(4);
        QCOMPARE(plan.size(), size_t(4));
        QVERIFY(plan[0].role == Role::Server);
        QVERIFY(plan[1].role == Role::Client);
        QVERIFY(plan[2].role == Role::Server);
        QVERIFY(plan[3].role == Role::Client);
    }
    void loopback_oddTrailingIsServer() {
        auto plan = NetworkSetup::planLoopback(1);
        QCOMPARE(plan.size(), size_t(1));
        QVERIFY(plan[0].role == Role::Server);
    }
    void loopback_zeroIsEmpty() {
        QVERIFY(NetworkSetup::planLoopback(0).empty());
    }
    void loopback_customInterface() {
        auto plan = NetworkSetup::planLoopback(2, "192.168.1.1");
        QCOMPARE(plan[0].localInterface, std::string("192.168.1.1"));
        QCOMPARE(plan[1].remoteAddress, std::string("192.168.1.1"));
    }
};

QTEST_GUILESS_MAIN(NetworkSetupTest)
#include "tst_networksetup.moc"
