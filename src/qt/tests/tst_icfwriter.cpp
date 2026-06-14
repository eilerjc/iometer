// Tests for iocore::IcfWriter - the byte-for-byte counterpart of the section
// loaders. Strategy: load a CANONICAL corpus fixture's section via IcfDocument,
// write it back via IcfWriter, and assert the output equals the section bytes
// extracted from the fixture file. The corpus fixtures are in true MFC save
// format, so this proves the writer reproduces the canonical ICF format exactly,
// without needing to run the MFC GUI's save. (Version lines are excluded - the
// fixtures are hand-authored and inconsistent there.)

#include <QtTest>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include "core/IcfDocument.h"
#include "core/IcfWriter.h"

using namespace iocore;

class TestIcfWriter : public QObject {
    Q_OBJECT

    static std::string fixture(const char *name) {
        return std::string(IOMETER_FIXTURE_DIR) + "/icf_compat/" + name;
    }

    // Read a file as LF-terminated lines (corpus fixtures are LF-only; tolerate CR).
    static std::vector<std::string> readLines(const std::string &path) {
        std::ifstream f(path, std::ios::binary);
        const std::string content((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
        std::vector<std::string> lines;
        std::string cur;
        for (char c : content) {
            if (c == '\n') { lines.push_back(cur); cur.clear(); }
            else if (c != '\r') { cur += c; }
        }
        if (!cur.empty()) lines.push_back(cur);
        return lines;
    }

    // The inclusive [headerPrefix-line .. endLine] block, rejoined with '\n' and a
    // trailing '\n' - i.e. exactly what a section writer emits.
    static std::string section(const std::string &path,
                               const char *headerPrefix, const char *endLine) {
        const auto lines = readLines(path);
        std::string out;
        bool in = false;
        for (const auto &l : lines) {
            if (!in && l.rfind(headerPrefix, 0) == 0) in = true;
            if (in) { out += l; out += '\n'; }
            if (in && l == endLine) break;
        }
        return out;
    }

private slots:
    void testSetup_roundTripsToFixtureBytes() {
        const std::string fx = fixture("worker_rich.icf");
        IcfDocument doc(fx);
        IcfTestSetup ts;
        QVERIFY(doc.loadTestSetup(ts));
        std::ostringstream os;
        IcfWriter::writeTestSetup(os, ts);
        QCOMPARE(os.str(), section(fx, "'TEST SETUP", "'END test setup"));
    }

    void resultsDisplay_roundTripsToFixtureBytes() {
        const std::string fx = fixture("worker_rich.icf");
        IcfDocument doc(fx);
        IcfDisplaySettings ds;
        QVERIFY(doc.loadResultsDisplay(ds));
        std::ostringstream os;
        IcfWriter::writeResultsDisplay(os, ds);
        QCOMPARE(os.str(), section(fx, "'RESULTS DISPLAY", "'END results display"));
    }

    void accessSpecs_roundTripsToFixtureBytes() {
        const std::string fx = fixture("worker_rich.icf");
        IcfDocument doc(fx);
        std::vector<IcfAccessSpec> specs;
        QVERIFY(doc.loadAccessSpecs(specs));
        std::ostringstream os;
        IcfWriter::writeAccessSpecs(os, specs);
        QCOMPARE(os.str(), section(fx, "'ACCESS SPECIFICATIONS", "'END access specifications"));
    }

    void managerList_richWorker_roundTripsToFixtureBytes() {
        const std::string fx = fixture("worker_rich.icf");
        IcfDocument doc(fx);
        std::vector<IcfManagerConfig> mgrs;
        QVERIFY(doc.loadManagerList(mgrs));
        std::ostringstream os;
        IcfWriter::writeManagerList(os, mgrs, /*aspecs*/true, /*targets*/true);
        QCOMPARE(os.str(), section(fx, "'MANAGER LIST", "'END manager list"));
    }

    void managerList_twoWorkers_roundTripsToFixtureBytes() {
        const std::string fx = fixture("two_workers.icf");
        IcfDocument doc(fx);
        std::vector<IcfManagerConfig> mgrs;
        QVERIFY(doc.loadManagerList(mgrs));
        std::ostringstream os;
        IcfWriter::writeManagerList(os, mgrs, true, true);
        QCOMPARE(os.str(), section(fx, "'MANAGER LIST", "'END manager list"));
    }

    void managerList_twoManagers_roundTripsToFixtureBytes() {
        const std::string fx = fixture("two_managers_run.icf");
        IcfDocument doc(fx);
        std::vector<IcfManagerConfig> mgrs;
        QVERIFY(doc.loadManagerList(mgrs));
        std::ostringstream os;
        IcfWriter::writeManagerList(os, mgrs, true, true);
        QCOMPARE(os.str(), section(fx, "'MANAGER LIST", "'END manager list"));
    }

    // Authored byte checks for tokens the canonical fixtures don't exercise.
    void testSetup_numberOfCpus_and_tokens() {
        IcfTestSetup ts;
        ts.testName = "x";
        ts.diskWorkerCount = -1;   // NUMBER_OF_CPUS
        ts.netWorkerCount  = -1;
        ts.resultType = 4;         // NONE
        ts.testType   = 7;         // CYCLE_OUTSTANDING_IOS_AND_TARGETS
        ts.queueCycling.stepType = 1;   // EXPONENTIAL
        std::ostringstream os;
        IcfWriter::writeTestSetup(os, ts);
        const std::string s = os.str();
        QVERIFY(s.find("'Default Disk Workers to Spawn\n\tNUMBER_OF_CPUS\n") != std::string::npos);
        QVERIFY(s.find("'Default Network Workers to Spawn\n\tNUMBER_OF_CPUS\n") != std::string::npos);
        QVERIFY(s.find("'Record Results\n\tNONE\n") != std::string::npos);
        QVERIFY(s.find("\tCYCLE_OUTSTANDING_IOS_AND_TARGETS\n") != std::string::npos);
    }

    // A full save->load round-trip preserves the manager/worker data (semantic).
    void managerList_writeThenReadIsStable() {
        const std::string fx = fixture("worker_rich.icf");
        IcfDocument doc(fx);
        std::vector<IcfManagerConfig> a;
        QVERIFY(doc.loadManagerList(a));

        // Write a full file to a temp path, reload, compare key fields.
        const std::string tmp = std::string(IOMETER_FIXTURE_DIR) + "/icf_compat/_tmp_writer.icf";
        {
            std::ofstream out(tmp, std::ios::binary);
            IcfWriter::writeVersionLine(out, "1.1.0");
            IcfWriter::writeManagerList(out, a, true, true);
            IcfWriter::writeVersionLine(out, "1.1.0");
        }
        IcfDocument doc2(tmp);
        std::vector<IcfManagerConfig> b;
        QVERIFY(doc2.loadManagerList(b));
        std::remove(tmp.c_str());

        QCOMPARE(b.size(), a.size());
        QCOMPARE(b[0].name, a[0].name);
        QCOMPARE(b[0].workers.size(), a[0].workers.size());
        const auto &wa = a[0].workers[0];
        const auto &wb = b[0].workers[0];
        QCOMPARE(wb.queueDepth, wa.queueDepth);
        QCOMPARE(wb.fixedSeedValue, wa.fixedSeedValue);
        QCOMPARE(wb.diskMaxSize, wa.diskMaxSize);
        QCOMPARE(wb.targets.size(), wa.targets.size());
        QCOMPARE(wb.targets[0].name, wa.targets[0].name);
    }
};

QTEST_APPLESS_MAIN(TestIcfWriter)
#include "tst_icfwriter.moc"
