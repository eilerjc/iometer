// tst_results.cpp — Tests for the results CSV writer
// Verifies column layout matches the original Iometer format so that
// smoke_test.ps1 and other tools can parse the output identically.
#include <QObject>
#include <QTest>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "DynamoEngine.h"

// Helper: build a minimal WorkerResult
static WorkerResult makeResult(const QString &manager, const QString &worker,
                               double iops, double mbpsDec, double mbpsBin,
                               double avgLat, double maxLat, int errors,
                               bool aggregate = false)
{
    WorkerResult r;
    r.managerName    = manager;
    r.workerName     = worker;
    r.isAggregate    = aggregate;
    r.iops           = iops;
    r.readIops       = iops * 0.9;
    r.writeIops      = iops * 0.1;
    r.mbpsDec        = mbpsDec;
    r.readMbpsDec    = mbpsDec * 0.9;
    r.writeMbpsDec   = mbpsDec * 0.1;
    r.mbpsBin        = mbpsBin;
    r.avgLatencyMs   = avgLat;
    r.maxLatencyMs   = maxLat;
    r.errors         = errors;
    return r;
}

// Helper: parse the ALL row from a CSV file
static QStringList parseAllRow(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&f);
    bool inResults = false;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith("'Results")) { inResults = true; continue; }
        if (!inResults || line.startsWith("'")) continue;
        const QStringList fields = line.split(',');
        if (fields.value(0).trimmed() == "ALL" && fields.size() > 27)
            return fields;
    }
    return {};
}

class ResultsTest : public QObject
{
    Q_OBJECT
private:
    // unused helper — tests open their own tmp files inline

private slots:

    // ── File structure ───────────────────────────────────────────────────────
    void csv_hasResultsMarker() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1",1000,100,95,0.04,15,0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open());
        tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        bool found = false;
        QTextStream in(&f);
        while (!in.atEnd()) { if (in.readLine().startsWith("'Results")) { found=true; break; } }
        QVERIFY2(found, "CSV missing 'Results marker");
    }

    void csv_writesToFile() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1",1000,100,95,0.04,15,0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open());
        tmp.close();
        QVERIFY(DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{}));
        QVERIFY(QFile::exists(tmp.fileName()));
        QVERIFY(QFileInfo(tmp.fileName()).size() > 0);
    }

    // ── ALL row column layout — must match smoke test validator ───────────────
    void allRow_field0_isALL() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1",1000,100,95,0.04,15,0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList fields = parseAllRow(tmp.fileName());
        QVERIFY2(!fields.isEmpty(), "No ALL row found");
        QCOMPARE(fields[0].trimmed(), QString("ALL"));
    }

    void allRow_field6_isIops() {
        const double testIops = 25000.5;
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", testIops, 1600, 1525, 0.04, 15, 0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 6);
        QCOMPARE(f[6].trimmed().toDouble(), testIops);
    }

    void allRow_field12_isMbpsDec() {
        const double testMbps = 1600.75;
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 25000, testMbps, 1525, 0.04, 15, 0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 12);
        QCOMPARE(f[12].trimmed().toDouble(), testMbps);
    }

    void allRow_field27_isErrors() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 25000, 1600, 1525, 0.04, 15, 0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 27);
        QCOMPARE(f[27].trimmed().toInt(), 0);
    }

    void allRow_field27_errors_nonzero() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 0, 0, 0, 0, 0, 42));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 27);
        QCOMPARE(f[27].trimmed().toInt(), 42);
    }

    // ── Aggregate correctness ────────────────────────────────────────────────
    void aggregate_sumsIops() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 10000, 800, 762, 0.04, 10, 0));
        r.append(makeResult("mgr","w2", 15000, 800, 762, 0.04, 12, 0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 6);
        QCOMPARE(f[6].trimmed().toDouble(), 25000.0);
    }

    void aggregate_workerCount() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 10000, 800, 762, 0.04, 10, 0));
        r.append(makeResult("mgr","w2", 10000, 800, 762, 0.04, 10, 0));
        r.append(makeResult("mgr","w3", 10000, 800, 762, 0.04, 10, 0));
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 3);
        QCOMPARE(f[3].trimmed().toInt(), 3); // workers field
    }

    void aggregate_skipsExistingAggregateRows() {
        QVector<WorkerResult> r;
        r.append(makeResult("mgr","w1", 1000, 100, 95, 0.04, 10, 0, false));
        r.append(makeResult("ALL","ALL", 9999, 999, 900, 0.01, 1, 0, true)); // existing aggregate
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), r, TestConfig{});
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(f.size() > 6);
        // The recomputed aggregate should be 1000 (w1 only), not 9999
        QCOMPARE(f[6].trimmed().toDouble(), 1000.0);
    }

    // ── Empty results ────────────────────────────────────────────────────────
    void emptyResults_writesZeroRow() {
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        QVERIFY(DynamoEngine::writeBatchResultsCsv(tmp.fileName(), {}, TestConfig{}));
        const QStringList f = parseAllRow(tmp.fileName());
        QVERIFY(!f.isEmpty());
        QCOMPARE(f[6].trimmed().toDouble(), 0.0); // IOps = 0
        QCOMPARE(f[27].trimmed().toInt(),   0);   // Errors = 0
    }

    // ── Config metadata in header ────────────────────────────────────────────
    void header_includesRunTime() {
        TestConfig cfg;
        cfg.runHours = 1; cfg.runMinutes = 30; cfg.runSeconds = 0;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_XXXXXX.csv");
        QVERIFY(tmp.open()); tmp.close();
        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), {}, cfg);
        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());
        QVERIFY(content.contains("1"));  // runHours
        QVERIFY(content.contains("30")); // runMinutes
    }
};

QTEST_GUILESS_MAIN(ResultsTest)
#include "tst_results.moc"
