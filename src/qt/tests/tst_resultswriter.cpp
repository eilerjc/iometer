// tst_resultswriter.cpp — Direct unit tests for the pure-C++ core ResultsWriter.
//
// Calls ResultsWriter::writeBatchResultsCsv with std:: types and parses the
// output with std::ifstream — no DynamoEngine, no Qt I/O. Verifies the original
// Iometer CSV layout (field[0]=ALL, [3]=workers, [6]=IOps, [12]=MBps(Dec),
// [27]=Errors) and aggregate correctness.
#include <QObject>
#include <QTest>
#include "../core/ResultsWriter.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

static std::string tempCsv(const std::string &leaf)
{
    auto p = std::filesystem::temp_directory_path() / ("iometer_rw_" + leaf);
    return p.string();
}

static std::vector<std::string> splitCsv(const std::string &line)
{
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) out.push_back(field);
    return out;
}

static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Parse the ALL aggregate row after the 'Results marker (mirrors smoke_test.ps1).
static std::vector<std::string> parseAllRow(const std::string &path)
{
    std::ifstream f(path);
    if (!f) return {};
    std::string line;
    bool inResults = false;
    while (std::getline(f, line)) {
        if (line.rfind("'Results", 0) == 0) { inResults = true; continue; }
        if (!inResults || (!line.empty() && line[0] == '\'')) continue;
        auto fields = splitCsv(line);
        if (!fields.empty() && trim(fields[0]) == "ALL" && fields.size() > 27)
            return fields;
    }
    return {};
}

static std::string readAll(const std::string &path)
{
    std::ifstream f(path);
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

static int countLinesStartingWith(const std::string &path, const std::string &prefix)
{
    std::ifstream f(path);
    std::string line; int n = 0;
    while (std::getline(f, line))
        if (line.rfind(prefix, 0) == 0) ++n;
    return n;
}

static WorkerResult makeResult(const std::string &mgr, const std::string &worker,
                               double iops, double mbpsDec, double mbpsBin,
                               double avgLat, double maxLat, int errors,
                               bool aggregate = false)
{
    WorkerResult r;
    r.managerName  = mgr;
    r.workerName   = worker;
    r.isAggregate  = aggregate;
    r.iops         = iops;
    r.readIops     = iops * 0.9;
    r.writeIops    = iops * 0.1;
    r.mbpsDec      = mbpsDec;
    r.readMbpsDec  = mbpsDec * 0.9;
    r.writeMbpsDec = mbpsDec * 0.1;
    r.mbpsBin      = mbpsBin;
    r.avgLatencyMs = avgLat;
    r.maxLatencyMs = maxLat;
    r.errors       = errors;
    return r;
}

class ResultsWriterTest : public QObject
{
    Q_OBJECT
private slots:

    // ── File structure ───────────────────────────────────────────────────────
    void writes_fileAndReturnsTrue() {
        const std::string p = tempCsv("write.csv");
        std::vector<WorkerResult> r{ makeResult("mgr","w1",1000,100,95,0.04,15,0) };
        QVERIFY(ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{}));
        QVERIFY(std::filesystem::exists(p));
        QVERIFY(std::filesystem::file_size(p) > 0);
        std::filesystem::remove(p);
    }
    void has_resultsMarker() {
        const std::string p = tempCsv("marker.csv");
        std::vector<WorkerResult> r{ makeResult("mgr","w1",1000,100,95,0.04,15,0) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        QVERIFY(countLinesStartingWith(p, "'Results") == 1);
        std::filesystem::remove(p);
    }

    // ── ALL row column positions ─────────────────────────────────────────────
    void allRow_field0_isALL() {
        const std::string p = tempCsv("f0.csv");
        std::vector<WorkerResult> r{ makeResult("mgr","w1",1000,100,95,0.04,15,0) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(!f.empty());
        QCOMPARE(trim(f[0]), std::string("ALL"));
        std::filesystem::remove(p);
    }
    void allRow_field6_isIops() {
        const std::string p = tempCsv("f6.csv");
        const double iops = 25000.5;
        std::vector<WorkerResult> r{ makeResult("mgr","w1",iops,1600,1525,0.04,15,0) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 6);
        QCOMPARE(std::stod(trim(f[6])), iops);
        std::filesystem::remove(p);
    }
    void allRow_field12_isMbpsDec() {
        const std::string p = tempCsv("f12.csv");
        const double mbps = 1600.75;
        std::vector<WorkerResult> r{ makeResult("mgr","w1",25000,mbps,1525,0.04,15,0) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 12);
        QCOMPARE(std::stod(trim(f[12])), mbps);
        std::filesystem::remove(p);
    }
    void allRow_field27_isErrors() {
        const std::string p = tempCsv("f27.csv");
        std::vector<WorkerResult> r{ makeResult("mgr","w1",25000,1600,1525,0.04,15,7) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 27);
        QCOMPARE(std::stoi(trim(f[27])), 7);
        std::filesystem::remove(p);
    }
    void allRow_hasAllColumns() {
        const std::string p = tempCsv("cols.csv");
        std::vector<WorkerResult> r{ makeResult("mgr","w1",1000,100,95,0.04,15,0) };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        // 37-column original layout
        QCOMPARE(f.size(), size_t(ResultsWriter::TOTAL_COLUMNS));
        std::filesystem::remove(p);
    }

    // ── Aggregate correctness ────────────────────────────────────────────────
    void aggregate_sumsIops() {
        const std::string p = tempCsv("agg.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",10000,800,762,0.04,10,0),
            makeResult("mgr","w2",15000,800,762,0.04,12,0)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 6);
        QCOMPARE(std::stod(trim(f[6])), 25000.0);
        std::filesystem::remove(p);
    }
    void aggregate_workerCountField3() {
        const std::string p = tempCsv("wc.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",1000,100,95,0.04,10,0),
            makeResult("mgr","w2",1000,100,95,0.04,10,0),
            makeResult("mgr","w3",1000,100,95,0.04,10,0)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 3);
        QCOMPARE(std::stoi(trim(f[3])), 3);
        std::filesystem::remove(p);
    }
    void aggregate_managerCountField4() {
        const std::string p = tempCsv("mc.csv");
        std::vector<WorkerResult> r{
            makeResult("mgrA","w1",1000,100,95,0.04,10,0),
            makeResult("mgrB","w2",1000,100,95,0.04,10,0)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 4);
        QCOMPARE(std::stoi(trim(f[4])), 2);   // two distinct managers
        std::filesystem::remove(p);
    }
    void aggregate_skipsExistingAggregateRows() {
        const std::string p = tempCsv("skip.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",1000,100,95,0.04,10,0,false),
            makeResult("ALL","ALL",9999,999,900,0.01,1,0,true)  // pre-existing aggregate
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 6);
        QCOMPARE(std::stod(trim(f[6])), 1000.0);   // not 1000+9999
        std::filesystem::remove(p);
    }
    void aggregate_sumsErrors() {
        const std::string p = tempCsv("errsum.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",0,0,0,0,0,3),
            makeResult("mgr","w2",0,0,0,0,0,4)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > 27);
        QCOMPARE(std::stoi(trim(f[27])), 7);
        std::filesystem::remove(p);
    }
    void aggregate_maxLatencyIsMaxNotSum() {
        const std::string p = tempCsv("maxlat.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",1000,100,95,0.04,10,0),
            makeResult("mgr","w2",1000,100,95,0.04,25,0)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        auto f = parseAllRow(p);
        QVERIFY(f.size() > ResultsWriter::COL_MAX_LATENCY_MS);
        QCOMPARE(std::stod(trim(f[ResultsWriter::COL_MAX_LATENCY_MS])), 25.0);
        std::filesystem::remove(p);
    }

    // ── Per-worker rows ──────────────────────────────────────────────────────
    void perWorker_diskRowsWritten() {
        const std::string p = tempCsv("disk.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr","w1",1000,100,95,0.04,10,0),
            makeResult("mgr","w2",2000,200,190,0.04,10,0)
        };
        ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{});
        QCOMPARE(countLinesStartingWith(p, "DISK"), 2);
        std::filesystem::remove(p);
    }

    void perWorker_nameWithCommaIsQuoted() {
        // A worker name containing a comma must be CSV-quoted so the column
        // layout stays intact (exercises escapeCsvField's quoting branch).
        const std::string p = tempCsv("comma.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr", "Worker 1, busy", 1000, 100, 95, 0.04, 10, 0)
        };
        QVERIFY(ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{}));
        const std::string content = readAll(p);
        QVERIFY(content.find("\"Worker 1, busy\"") != std::string::npos);
        std::filesystem::remove(p);
    }
    void perWorker_nameWithQuoteIsEscaped() {
        const std::string p = tempCsv("quote.csv");
        std::vector<WorkerResult> r{
            makeResult("mgr", "drive \"C\"", 1000, 100, 95, 0.04, 10, 0)
        };
        QVERIFY(ResultsWriter::writeBatchResultsCsv(p, r, TestConfig{}));
        // Embedded quotes are doubled per CSV rules
        QVERIFY(readAll(p).find("\"drive \"\"C\"\"\"") != std::string::npos);
        std::filesystem::remove(p);
    }

    // ── Empty results still produce a valid ALL row ──────────────────────────
    void empty_writesZeroRow() {
        const std::string p = tempCsv("empty.csv");
        QVERIFY(ResultsWriter::writeBatchResultsCsv(p, {}, TestConfig{}));
        auto f = parseAllRow(p);
        QVERIFY(!f.empty());
        QCOMPARE(std::stod(trim(f[6])), 0.0);
        QCOMPARE(std::stoi(trim(f[27])), 0);
        QCOMPARE(std::stoi(trim(f[3])), 0);   // zero workers
        std::filesystem::remove(p);
    }

    // ── Header metadata ──────────────────────────────────────────────────────
    void header_includesRunTime() {
        const std::string p = tempCsv("hdr.csv");
        TestConfig cfg; cfg.runHours = 1; cfg.runMinutes = 30; cfg.runSeconds = 0;
        ResultsWriter::writeBatchResultsCsv(p, {}, cfg);
        const std::string content = readAll(p);
        QVERIFY(content.find("Run Time") != std::string::npos);
        QVERIFY(content.find("30") != std::string::npos);   // runMinutes
        std::filesystem::remove(p);
    }
    void header_includesDescription() {
        const std::string p = tempCsv("desc.csv");
        TestConfig cfg; cfg.description = "my-benchmark-run";
        ResultsWriter::writeBatchResultsCsv(p, {}, cfg);
        QVERIFY(readAll(p).find("my-benchmark-run") != std::string::npos);
        std::filesystem::remove(p);
    }
};

QTEST_GUILESS_MAIN(ResultsWriterTest)
#include "tst_resultswriter.moc"
