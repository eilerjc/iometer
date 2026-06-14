// tst_resultsaggregator - the shared "ALL"-row aggregation (iocore::aggregateResults),
// the single implementation behind the CSV writer and both GUIs' live displays.
#include <QtTest>
#include "ResultsAggregator.h"

using iocore::aggregateResults;

static WorkerResult worker(const char *mgr, const char *name,
                           double iops, double mbps, double avgLat, double maxLat,
                           double cpu, int errors)
{
    WorkerResult r;
    r.managerName = mgr;  r.workerName = name;
    r.iops = iops;        r.readIops = iops;
    r.mbpsDec = mbps;     r.readMbpsDec = mbps;  r.mbpsBin = mbps / 1.048576;
    r.avgLatencyMs = avgLat;  r.maxLatencyMs = maxLat;
    r.cpuUtil = cpu;  r.cpuUser = cpu * 0.4;  r.cpuKernel = cpu * 0.6;
    r.errors = errors;
    return r;
}

class TestResultsAggregator : public QObject {
    Q_OBJECT
private slots:
    void empty_returnsZeroCounts();
    void sums_throughputAndErrors();
    void avgLatency_isMean_maxLatency_isMax();
    void cpu_isMeanNotSum();
    void skipsExistingAggregates();
    void countsDistinctManagers();
    void aggregateRow_identity();
};

void TestResultsAggregator::empty_returnsZeroCounts() {
    auto a = aggregateResults({});
    QCOMPARE(a.workerCount, 0);
    QCOMPARE(a.managerCount, 0);
    QCOMPARE(a.all.iops, 0.0);
    QVERIFY(a.all.isAggregate);
}

void TestResultsAggregator::sums_throughputAndErrors() {
    auto a = aggregateResults({ worker("M1","w1", 1000, 50, 1, 2, 10, 2),
                                worker("M1","w2", 2000, 70, 3, 4, 20, 3) });
    QCOMPARE(a.all.iops, 3000.0);
    QCOMPARE(a.all.mbpsDec, 120.0);
    QCOMPARE(a.all.errors, 5);
    QCOMPARE(a.workerCount, 2);
}

void TestResultsAggregator::avgLatency_isMean_maxLatency_isMax() {
    auto a = aggregateResults({ worker("M1","w1", 1, 1, 2.0, 10.0, 0, 0),
                                worker("M1","w2", 1, 1, 4.0, 50.0, 0, 0) });
    QCOMPARE(a.all.avgLatencyMs, 3.0);    // mean
    QCOMPARE(a.all.maxLatencyMs, 50.0);   // max, not sum
}

void TestResultsAggregator::cpu_isMeanNotSum() {
    // CPU is reported per manager and mirrored onto workers; the ALL row must
    // average it (summing would overcount on multi-worker managers).
    auto a = aggregateResults({ worker("M1","w1", 1, 1, 1, 1, 30.0, 0),
                                worker("M1","w2", 1, 1, 1, 1, 30.0, 0) });
    QCOMPARE(a.all.cpuUtil, 30.0);
    QCOMPARE(a.all.cpuUser, 12.0);    // 30*0.4
    QCOMPARE(a.all.cpuKernel, 18.0);  // 30*0.6
}

void TestResultsAggregator::skipsExistingAggregates() {
    WorkerResult oldAgg = worker("ALL","All", 9999, 9999, 9, 9, 99, 99);
    oldAgg.isAggregate = true;
    auto a = aggregateResults({ oldAgg, worker("M1","w1", 100, 10, 1, 1, 5, 0) });
    QCOMPARE(a.all.iops, 100.0);
    QCOMPARE(a.workerCount, 1);
}

void TestResultsAggregator::countsDistinctManagers() {
    auto a = aggregateResults({ worker("M1","w1", 1, 1, 1, 1, 0, 0),
                                worker("M1","w2", 1, 1, 1, 1, 0, 0),
                                worker("M2","w1", 1, 1, 1, 1, 0, 0) });
    QCOMPARE(a.managerCount, 2);
    QCOMPARE(a.workerCount, 3);
}

void TestResultsAggregator::aggregateRow_identity() {
    auto a = aggregateResults({ worker("M1","w1", 1, 1, 1, 1, 0, 0) });
    QCOMPARE(a.all.managerName, std::string("ALL"));
    QCOMPARE(a.all.workerName,  std::string("All"));
    QVERIFY(a.all.isAggregate);
}

QTEST_APPLESS_MAIN(TestResultsAggregator)
#include "tst_resultsaggregator.moc"
