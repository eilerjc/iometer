// Tests for iocore::ResultStatCatalog - the canonical statistic display names
// (the cross-GUI / ICF-compat contract; must match the MFC Galileo.rc menu and
// the strings written into ICF 'Bar chart N statistic lines).

#include <QtTest>
#include "core/ResultStatCatalog.h"
#include "core/IometerTypes.h"

using namespace iocore;

class TestResultStatCatalog : public QObject {
    Q_OBJECT
private slots:
    // The six default bar-chart statistics, exactly as the MFC GUI writes them
    // (verified against the save-golden corpus).
    void defaultBars_matchCanonicalIcfStrings() {
        QCOMPARE(QString(resultStatName(RESULT_IOPS)),          QString("Total I/Os per Second"));
        QCOMPARE(QString(resultStatName(RESULT_MBPS_DEC)),      QString("Total MBs per Second (Decimal)"));
        QCOMPARE(QString(resultStatName(RESULT_AVG_LATENCY_MS)),QString("Average I/O Response Time (ms)"));
        QCOMPARE(QString(resultStatName(RESULT_MAX_LATENCY_MS)),QString("Maximum I/O Response Time (ms)"));
        QCOMPARE(QString(resultStatName(RESULT_CPU_UTIL)),      QString("% CPU Utilization (total)"));
        QCOMPARE(QString(resultStatName(RESULT_ERRORS)),        QString("Total Error Count"));
    }

    // A few non-default names (read/write/network variants) pinned exactly.
    void variantNames_pinned() {
        QCOMPARE(QString(resultStatName(RESULT_AVG_READ_LATENCY_MS)), QString("Avg. Read Response Time (ms)"));
        QCOMPARE(QString(resultStatName(RESULT_MAX_CONN_LATENCY_MS)), QString("Max. Connection Time (ms)"));
        QCOMPARE(QString(resultStatName(RESULT_NET_RETRANS_PS)),      QString("TCP Segments Retrans. per Sec."));
        QCOMPARE(QString(resultStatName(RESULT_CPU_KERNEL)),          QString("% Privileged Time"));
        QCOMPARE(QString(resultStatName(RESULT_WRITE_ERRORS)),        QString("Write Error Count"));
    }

    // Every statistic has a non-empty name and round-trips name -> id -> name.
    void allNamesPresentAndRoundTrip() {
        for (int t = 0; t < NUM_RESULT_TYPES; ++t) {
            const char *n = resultStatName(t);
            QVERIFY2(n[0] != '\0', qPrintable(QString("empty name for type %1").arg(t)));
            QCOMPARE(resultStatByName(n), t);    // name -> id is the inverse
        }
    }

    void outOfRangeAndUnknown() {
        QCOMPARE(QString(resultStatName(-1)), QString(""));
        QCOMPARE(QString(resultStatName(NUM_RESULT_TYPES)), QString(""));
        QCOMPARE(resultStatByName("not a statistic"), -1);
        QCOMPARE(resultStatByName(nullptr), -1);
    }
};

QTEST_APPLESS_MAIN(TestResultStatCatalog)
#include "tst_resultstatcatalog.moc"
