// Tests for iocore::decodeRawResult - the shared raw-counter -> rates/latency
// decode (faithful port of the MFC Worker::UpdateResults formula). A minimal
// fake raw struct stands in for Raw_Result / DyRawResult (identical field names).

#include <QtTest>
#include <cstdint>
#include "core/ResultDecode.h"

using namespace iocore;

// Mirrors the fields decodeRawResult reads (same names as Raw_Result/DyRawResult).
struct FakeRaw {
    uint64_t bytes_read = 0, bytes_written = 0;
    uint64_t read_count = 0, write_count = 0;
    uint64_t transaction_count = 0, connection_count = 0;
    unsigned read_errors = 0, write_errors = 0;
    uint64_t max_raw_read_latency = 0, read_latency_sum = 0;
    uint64_t max_raw_write_latency = 0, write_latency_sum = 0;
    uint64_t max_raw_transaction_latency = 0, transaction_latency_sum = 0;
    uint64_t max_raw_connection_latency = 0, connection_latency_sum = 0;
};

class TestResultDecode : public QObject {
    Q_OBJECT

    static bool close(double a, double b) { return std::abs(a - b) < 1e-9; }

private slots:
    void fullDecode_matchesHandComputedMfcFormula() {
        FakeRaw r;
        r.bytes_read = 2097152; r.bytes_written = 1048576;   // 2 MiB / 1 MiB
        r.read_count = 100; r.write_count = 50;
        r.transaction_count = 30; r.connection_count = 5;
        r.read_errors = 1; r.write_errors = 2;
        r.max_raw_read_latency = 5000; r.max_raw_write_latency = 3000;
        r.max_raw_transaction_latency = 4000; r.max_raw_connection_latency = 2000;
        r.read_latency_sum = 100000; r.write_latency_sum = 60000;
        r.transaction_latency_sum = 30000; r.connection_latency_sum = 10000;

        // runTime = 2 s, timerResolution = 1 MHz (so ticks->ms = ticks/1000).
        const ResultRates d = decodeRawResult(r, 2.0, 1.0e6);

        QVERIFY(close(d.readMbpsBin, 1.0));        // 2 MiB / 2 s
        QVERIFY(close(d.writeMbpsBin, 0.5));
        QVERIFY(close(d.mbpsBin, 1.5));
        QVERIFY(close(d.readMbpsDec, 2097152.0 / 1.0e6 / 2.0));
        QVERIFY(close(d.mbpsDec, (2097152.0 + 1048576.0) / 1.0e6 / 2.0));
        QVERIFY(close(d.readIops, 50.0));
        QVERIFY(close(d.writeIops, 25.0));
        QVERIFY(close(d.iops, 75.0));
        QVERIFY(close(d.transactionsPerSecond, 15.0));
        QVERIFY(close(d.connectionsPerSecond, 2.5));

        QVERIFY(close(d.maxReadLatency, 5.0));     // 5000 ticks / 1 MHz * 1000
        QVERIFY(close(d.maxWriteLatency, 3.0));
        QVERIFY(close(d.maxLatency, 5.0));         // max(read, write)
        QVERIFY(close(d.maxTransactionLatency, 4.0));
        QVERIFY(close(d.maxConnectionLatency, 2.0));

        QVERIFY(close(d.aveLatency, (100000.0 + 60000.0) * 1000.0 / 1.0e6 / 150.0));
        QVERIFY(close(d.aveReadLatency, 1.0));     // 100000/1e6*1000/100
        QVERIFY(close(d.aveWriteLatency, 1.2));
        QVERIFY(close(d.aveTransactionLatency, 1.0));
        QVERIFY(close(d.aveConnectionLatency, 2.0));

        QCOMPARE(d.totalErrors, 3u);
    }

    void zeroRunTime_ratesAreZeroButLatencyStillDecodes() {
        FakeRaw r;
        r.read_count = 10; r.read_latency_sum = 50000;
        r.max_raw_read_latency = 7000;
        const ResultRates d = decodeRawResult(r, 0.0, 1.0e6);
        QVERIFY(close(d.iops, 0.0));               // run time 0 -> no rate
        QVERIFY(close(d.mbpsDec, 0.0));
        QVERIFY(close(d.maxReadLatency, 7.0));     // latency still computed
        QVERIFY(close(d.aveReadLatency, 5.0));     // 50000/1e6*1000/10
    }

    void noIos_averageLatenciesAreZero() {
        FakeRaw r;                                 // all counts zero
        r.bytes_read = 0;
        const ResultRates d = decodeRawResult(r, 2.0, 1.0e6);
        QVERIFY(close(d.aveLatency, 0.0));
        QVERIFY(close(d.aveReadLatency, 0.0));
        QVERIFY(close(d.iops, 0.0));
        QCOMPARE(d.totalErrors, 0u);
    }

    void summingTargetsEqualsDecodingTheSum() {
        // Two targets; sum-of-rates must equal rate-of-sum (the property that lets
        // MFC sum per-target rates while Qt decodes the summed counters).
        FakeRaw a; a.bytes_read = 1000000; a.read_count = 40;
        FakeRaw b; b.bytes_read = 3000000; b.read_count = 60;
        const ResultRates da = decodeRawResult(a, 2.0, 1.0e6);
        const ResultRates db = decodeRawResult(b, 2.0, 1.0e6);
        FakeRaw sum; sum.bytes_read = 4000000; sum.read_count = 100;
        const ResultRates ds = decodeRawResult(sum, 2.0, 1.0e6);
        QVERIFY(close(da.readIops + db.readIops, ds.readIops));
        QVERIFY(close(da.readMbpsDec + db.readMbpsDec, ds.readMbpsDec));
    }
};

QTEST_APPLESS_MAIN(TestResultDecode)
#include "tst_resultdecode.moc"
