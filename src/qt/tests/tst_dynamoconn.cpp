// tst_dynamoconn.cpp — Unit tests for the core DynamoConnection.
//
// DynamoConnection is currently a platform-agnostic stub (the real socket I/O
// lives in the Qt QtDySession). These tests pin its observable contract — state
// transitions, the guard on sendMessage, default port, and the disconnect
// callback — so the behaviour can't silently regress when real socket handling
// is added later.
#include <QObject>
#include <QTest>
#include "../core/DynamoConnection.h"
#include "../QtDyProto.h"   // complete DyMsg for sendMessage()

using State = DynamoConnection::State;

class DynamoConnTest : public QObject
{
    Q_OBJECT
private slots:

    void initial_isDisconnected() {
        DynamoConnection c;
        QVERIFY(c.state() == State::Disconnected);
        QVERIFY(!c.isConnected());
    }

    void defaultPort_is1066() {
        DynamoConnection c;
        QCOMPARE(c.remotePort(), 1066);
    }

    void remoteAddress_emptyWhenDisconnected() {
        DynamoConnection c;
        QVERIFY(c.remoteAddress().empty());
    }

    void startLogin_goesConnecting() {
        DynamoConnection c;
        QVERIFY(c.startLogin("TestManager"));
        QVERIFY(c.state() == State::Connecting);
        QVERIFY(!c.isConnected());     // Connecting is not yet Connected
    }

    void sendMessage_failsWhenNotConnected() {
        DynamoConnection c;
        DyMsg msg{}; msg.purpose = 1; msg.data = 0;
        QVERIFY(!c.sendMessage(msg));          // Disconnected
        c.startLogin("m");
        QVERIFY(!c.sendMessage(msg));          // Connecting, still not Connected
    }

    void receiveDataMessage_nullWhenStub() {
        DynamoConnection c;
        QVERIFY(c.receiveDataMessage(10) == nullptr);
    }

    void disconnect_resetsToDisconnected() {
        DynamoConnection c;
        c.startLogin("m");
        QVERIFY(c.state() == State::Connecting);
        c.disconnect();
        QVERIFY(c.state() == State::Disconnected);
        QVERIFY(!c.isConnected());
    }

    void disconnect_firesCallback() {
        DynamoConnection c;
        int fired = 0;
        c.onDisconnected = [&]{ ++fired; };
        c.startLogin("m");
        c.disconnect();
        QCOMPARE(fired, 1);
    }

    void disconnect_idempotent() {
        DynamoConnection c;
        int fired = 0;
        c.onDisconnected = [&]{ ++fired; };
        c.disconnect();
        c.disconnect();
        QCOMPARE(fired, 2);            // callback fires each call; state stays Disconnected
        QVERIFY(c.state() == State::Disconnected);
    }
};

QTEST_GUILESS_MAIN(DynamoConnTest)
#include "tst_dynamoconn.moc"
