// tst_types.cpp — Tests for IometerTypes.h utilities
// Covers: formatSizeCompact, AccessSpec::displayLabel, WorkerResult::get,
//         TestConfig defaults, ResultType enum ordering.
#include <QObject>
#include <QTest>
#include "IometerTypes.h"

class TypesTest : public QObject
{
    Q_OBJECT
private slots:

    // ── formatSizeCompact ────────────────────────────────────────────────────
    void formatSize_bytes() {
        QCOMPARE(formatSizeCompact(512),    QString("512 B"));
        QCOMPARE(formatSizeCompact(1),      QString("1 B"));
        QCOMPARE(formatSizeCompact(1023),   QString("1023 B"));
    }
    void formatSize_kib() {
        QCOMPARE(formatSizeCompact(1024),       QString("1 KiB"));
        QCOMPARE(formatSizeCompact(4096),       QString("4 KiB"));
        QCOMPARE(formatSizeCompact(65536),      QString("64 KiB"));
        QCOMPARE(formatSizeCompact(1023 * 1024), QString("1023 KiB"));
    }
    void formatSize_mib() {
        QCOMPARE(formatSizeCompact(1024 * 1024),     QString("1 MiB"));
        QCOMPARE(formatSizeCompact(16 * 1024 * 1024), QString("16 MiB"));
    }
    void formatSize_zero() {
        QCOMPARE(formatSizeCompact(0), QString("0 B"));
    }

    // ── AccessSpec::displayLabel ─────────────────────────────────────────────
    void displayLabel_emptySpec() {
        AccessSpec s;
        s.name = "TestSpec";
        QCOMPARE(s.displayLabel(), QString("TestSpec"));
    }
    void displayLabel_singleLine_sequential() {
        AccessSpec s;
        s.name = "64 KiB Seq Read";
        AccessSpecLine l;
        l.sizeBytes   = 65536;
        l.readPercent = 100;
        l.seqPercent  = 100;   // 100% sequential → 0% random
        s.lines.append(l);
        // Single-line → should produce formatted label
        const QString lbl = s.displayLabel();
        QVERIFY(lbl.contains("64 KiB"));
        QVERIFY(lbl.contains("100%"));
        QVERIFY(lbl.contains("0%"));    // 0% random
    }
    void displayLabel_singleLine_random() {
        AccessSpec s;
        s.name = "4K Random";
        AccessSpecLine l;
        l.sizeBytes   = 4096;
        l.readPercent = 50;
        l.seqPercent  = 0;     // 0% sequential → 100% random
        s.lines.append(l);
        const QString lbl = s.displayLabel();
        QVERIFY(lbl.contains("4 KiB"));
        QVERIFY(lbl.contains("50%"));
        QVERIFY(lbl.contains("100%")); // 100% random
    }
    void displayLabel_multiLine_returnsName() {
        AccessSpec s;
        s.name = "All in one";
        AccessSpecLine l;
        l.sizeBytes = 512; l.ofSize = 50; l.readPercent = 100; l.seqPercent = 100;
        s.lines.append(l);
        s.lines.append(l);  // two lines → fall back to name
        QCOMPARE(s.displayLabel(), QString("All in one"));
    }

    // ── WorkerResult::get ────────────────────────────────────────────────────
    void workerResultGet_iops() {
        WorkerResult r;
        r.iops = 12345.6;
        QCOMPARE(r.get(RESULT_IOPS), 12345.6);
    }
    void workerResultGet_readIops() {
        WorkerResult r;
        r.readIops = 8000.0;
        QCOMPARE(r.get(RESULT_READ_IOPS), 8000.0);
    }
    void workerResultGet_writeIops() {
        WorkerResult r;
        r.writeIops = 4000.0;
        QCOMPARE(r.get(RESULT_WRITE_IOPS), 4000.0);
    }
    void workerResultGet_mbpsDec() {
        WorkerResult r;
        r.mbpsDec = 1500.5;
        QCOMPARE(r.get(RESULT_MBPS_DEC), 1500.5);
    }
    void workerResultGet_mbpsBin() {
        WorkerResult r;
        r.mbpsBin = 1430.2;
        QCOMPARE(r.get(RESULT_MBPS_BIN), 1430.2);
    }
    void workerResultGet_avgLatency() {
        WorkerResult r;
        r.avgLatencyMs = 0.0452;
        QCOMPARE(r.get(RESULT_AVG_LATENCY_MS), 0.0452);
    }
    void workerResultGet_maxLatency() {
        WorkerResult r;
        r.maxLatencyMs = 15.7;
        QCOMPARE(r.get(RESULT_MAX_LATENCY_MS), 15.7);
    }
    void workerResultGet_cpuUtil() {
        WorkerResult r;
        r.cpuUtil = 42.3;
        QCOMPARE(r.get(RESULT_CPU_UTIL), 42.3);
    }
    void workerResultGet_errors() {
        WorkerResult r;
        r.errors = 7;
        QCOMPARE(r.get(RESULT_ERRORS), 7.0);
    }
    void workerResultGet_unknownType_returnsZero() {
        WorkerResult r;
        r.iops = 999.0;
        QCOMPARE(r.get(NUM_RESULT_TYPES), 0.0);
        QCOMPARE(r.get(-1), 0.0);
    }

    // ── TestConfig defaults ──────────────────────────────────────────────────
    void testConfigDefaults() {
        TestConfig cfg;
        QCOMPARE(cfg.runHours,   0);
        QCOMPARE(cfg.runMinutes, 0);
        QCOMPARE(cfg.runSeconds, 0);
        QCOMPARE(cfg.rampSeconds, 0);
    }
    void testConfigRoundTrip() {
        TestConfig cfg;
        cfg.runHours   = 1;
        cfg.runMinutes = 30;
        cfg.runSeconds = 45;
        cfg.rampSeconds = 10;
        cfg.description = "My test";
        QCOMPARE(cfg.runHours,   1);
        QCOMPARE(cfg.runMinutes, 30);
        QCOMPARE(cfg.runSeconds, 45);
        QCOMPARE(cfg.rampSeconds, 10);
        QCOMPARE(cfg.description, QString("My test"));
    }

    // ── ResultType enum sanity ───────────────────────────────────────────────
    void resultType_iopsIsFirst() {
        QCOMPARE(static_cast<int>(RESULT_IOPS), 0);
    }
    void resultType_errorsBeforeCount() {
        QVERIFY(RESULT_ERRORS < NUM_RESULT_TYPES);
        QVERIFY(RESULT_READ_ERRORS < NUM_RESULT_TYPES);
        QVERIFY(RESULT_WRITE_ERRORS < NUM_RESULT_TYPES);
    }
    void resultType_countIsPositive() {
        QVERIFY(NUM_RESULT_TYPES > 10);
    }
};

QTEST_GUILESS_MAIN(TypesTest)
#include "tst_types.moc"
