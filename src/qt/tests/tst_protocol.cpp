// tst_protocol.cpp — Protocol structure regression tests
// The DyProto.h static_assert guards already catch compile-time regressions.
// These runtime tests document the expected sizes and constant values, and
// serve as an explicit test record for compatibility audits.
// IMPORTANT: any test failure here means a breaking change to the wire protocol.
#include <QObject>
#include <QTest>
#include "DyProto.h"

class ProtocolTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Control message ───────────────────────────────────────────────────────
    void dyMsg_size() {
        QCOMPARE(static_cast<int>(sizeof(DyMsg)), 8);
    }
    void dyMsg_fieldOffsets() {
        DyMsg m{};
        QCOMPARE(static_cast<int>(offsetof(DyMsg, purpose)), 0);
        QCOMPARE(static_cast<int>(offsetof(DyMsg, data)),    4);
    }

    // ── Data message ─────────────────────────────────────────────────────────
    void dyDataMessage_size() {
        QCOMPARE(static_cast<int>(sizeof(DyDataMessage)), 888840);
    }

    // ── Key structure sizes ───────────────────────────────────────────────────
    void dyManagerInfo_size() {
        QCOMPARE(static_cast<int>(sizeof(DyManagerInfo)), 350);
    }
    void dyAccessSpec_size() {
        QCOMPARE(static_cast<int>(sizeof(DyAccessSpec)), 32);
    }
    void dyTestSpec_size() {
        QCOMPARE(static_cast<int>(sizeof(DyTestSpec)), 3332);
    }
    void dyTargetSpec_size() {
        QCOMPARE(static_cast<int>(sizeof(DyTargetSpec)), 434);
    }
    void dyRawResult_size() {
        QCOMPARE(static_cast<int>(sizeof(DyRawResult)), 296);
    }
    void dyWorkerResults_size() {
        QCOMPARE(static_cast<int>(sizeof(DyWorkerResults)), 606228);
    }
    void dyManagerResults_size() {
        QCOMPARE(static_cast<int>(sizeof(DyManagerResults)), 4640);
    }

    // ── Port and version ─────────────────────────────────────────────────────
    void port_is1066() {
        QCOMPARE(static_cast<int>(DY_PORT), 1066);
    }
    void version_major1_minor1() {
        // DY_VERSION = (1 << 24) | (1 << 8) = 0x01000100
        QCOMPARE(DY_VERSION, static_cast<int32_t>(0x01000100));
    }

    // ── Command code values (must not change — Dynamo and Iometer must agree) ─
    void commands_replyFilter() {
        QCOMPARE(DY_REPLY_FILTER, static_cast<int32_t>(0x10000000));
    }
    void commands_login()    { QCOMPARE(DY_LOGIN,          DY_REPLY_FILTER + 1);  }
    void commands_addWorkers(){ QCOMPARE(DY_ADD_WORKERS,   DY_REPLY_FILTER + 2);  }
    void commands_start()    { QCOMPARE(DY_START,          DY_REPLY_FILTER + 4);  }
    void commands_beginIO()  { QCOMPARE(DY_BEGIN_IO,       DY_REPLY_FILTER + 5);  }
    void commands_stop()     { QCOMPARE(DY_STOP,           DY_REPLY_FILTER + 7);  }
    void commands_setTargets(){ QCOMPARE(DY_SET_TARGETS,   DY_REPLY_FILTER + 11); }
    void commands_setAccess() { QCOMPARE(DY_SET_ACCESS,    DY_REPLY_FILTER + 12); }
    void commands_loginOk()  { QCOMPARE(DY_LOGIN_OK,       DY_REPLY_FILTER + 15); }
    void commands_ready()    { QCOMPARE(DY_READY,          DY_NO_REPLY + 1);      }
    void commands_exit()     { QCOMPARE(DY_EXIT,           DY_NO_REPLY + 4);      }

    // ── Capacity constants ────────────────────────────────────────────────────
    void constants_maxTargets()     { QCOMPARE(DY_MAX_TARGETS,      2048); }
    void constants_maxAccessSpecs() { QCOMPARE(DY_MAX_ACCESS_SPECS, 100);  }
    void constants_maxCpus()        { QCOMPARE(DY_MAX_CPUS,         64);   }
    void constants_maxName()        { QCOMPARE(DY_MAX_NAME,         80);   }
    void constants_latencyBins()    { QCOMPARE(DY_LATENCY_BINS,     21);   }

    // ── Target type values ────────────────────────────────────────────────────
    void targetType_physicalDisk() {
        QCOMPARE(DY_PHYSICAL_DISK, static_cast<int32_t>(0x8C000000));
    }
    void targetType_logicalDisk() {
        QCOMPARE(DY_LOGICAL_DISK,  static_cast<int32_t>(0x8A000000));
    }
    void targetType_tcpServer() {
        QCOMPARE(DY_TCP_SERVER,    static_cast<int32_t>(0x800C8000));
    }
};

QTEST_GUILESS_MAIN(ProtocolTest)
#include "tst_protocol.moc"
