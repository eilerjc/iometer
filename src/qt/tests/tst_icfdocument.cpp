// tst_icfdocument - the iocore::IcfDocument section loaders (Phase 2:
// 'TEST SETUP and 'RESULTS DISPLAY), ports of CPageSetup/CPageDisplay::LoadConfig.
#include <QtTest>
#include <QTemporaryDir>
#include <fstream>
#include "IcfDocument.h"

using namespace iocore;

class TestIcfDocument : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    int           m_n = 0;

    std::string makeFile(const std::string &content) {
        const std::string p = QDir(m_dir.path())
            .filePath(QString("doc_%1.icf").arg(m_n++)).toStdString();
        std::ofstream f(p, std::ios::binary);
        f << content;
        return p;
    }

    // A minimal valid TEST SETUP section body (modern version line).
    std::string setupFile(const std::string &body) {
        return "Version 1.1.0 \n'TEST SETUP ====\n" + body + "'END test setup\n";
    }

private slots:
    // TEST SETUP
    void setup_allModernKeys();
    void setup_missingSection_okAndUntouched();
    void setup_numberOfCpusToken();
    void setup_ancientDefaultWorkers_zeroBecomesMinusOne();
    void setup_recordResults_tokensAndLegacyInt();
    void setup_testType_tokenAndLegacyInt();
    void setup_unrecognizedKey_modernFails();
    void setup_unrecognizedKey_oldVersionBreaks();
    void setup_badRunTime_failsWithMfcText();
    void setup_onlyPresentKeysOverwrite();
    // RESULTS DISPLAY
    void display_updateLineAndBars();
    void display_managerAndWorkerBars();
    void display_invalidBarNumber_ignoredWithError();
    void display_invalidBarItem_ignoredWithError();
    void display_badUpdateType_fails();
    void display_unrecognizedKey_fails();
    // ACCESS SPECIFICATIONS
    void specs_newFormat_basics();
    void specs_newFormat_assignmentTokens();
    void specs_newFormat_replaceByName();
    void specs_newFormat_percentagesMustSum();
    void specs_newFormat_truncatedFails();
    void specs_oldFormat_smartNamedAndAssignAll();
    void specs_oldFormat_multiLineStaysUntitled();
    void specs_oldFormat_alignReplyDefaults();
    void specs_oldFormat_uniqueAgainstExisting();
    void specs_missingSection_ok();
    // MANAGER LIST
    void mgr_singleDiskWorker();
    void mgr_richWorkerSettings();
    void mgr_twoWorkers();
    void mgr_twoManagers();
    void mgr_skipAspecsAndTargets();
    void mgr_diskSizeOnNetWorkerFails();
    void mgr_unknownWorkerType_fails();
    void mgr_unknownNetSubtype_recordsErrorButContinues();
    void mgr_badManagerId_fails();
    void mgr_networkTarget();
    void mgr_networkTarget_missingMgrLine_fails();
    void mgr_missingSection_ok();
    void mgr_realFixtureWorkerRich();
};

// Fixtures live in test/fixtures/icf_compat (the golden corpus).
static std::string compatFixture(const char *name) {
    return std::string(IOMETER_FIXTURE_DIR) + "/icf_compat/" + name;
}

void TestIcfDocument::setup_allModernKeys() {
    IcfDocument doc(makeFile(setupFile(
        "'Test Description\n\tmy test\n"
        "'Run Time\n'\thours      minutes    seconds\n\t1          2          3\n"
        "'Ramp Up Time (s)\n\t15\n"
        "'Default Disk Workers to Spawn\n\t4\n"
        "'Default Network Workers to Spawn\n\t2\n"
        "'Record Results\n\tALL\n"
        "'Worker Cycling\n'\tstart      step       step type\n\t1          2          LINEAR\n"
        "'Disk Cycling\n'\tstart      step       step type\n\t3          4          EXPONENTIAL\n"
        "'Queue Depth Cycling\n'\tstart      end        step       step type\n\t1          32         2          EXPONENTIAL\n"
        "'Test Type\n\tNORMAL\n")));
    IcfTestSetup ts;
    QVERIFY(doc.loadTestSetup(ts));
    QCOMPARE(ts.testName, std::string("my test"));
    QCOMPARE(ts.hours, 1); QCOMPARE(ts.minutes, 2); QCOMPARE(ts.seconds, 3);
    QCOMPARE(ts.rampTime, 15);
    QCOMPARE(ts.diskWorkerCount, 4);
    QCOMPARE(ts.netWorkerCount, 2);
    QCOMPARE(ts.resultType, 0);                      // RecordAll
    QCOMPARE(ts.workerCycling.start, 1); QCOMPARE(ts.workerCycling.step, 2);
    QCOMPARE(ts.workerCycling.stepType, 0);          // LINEAR
    QCOMPARE(ts.targetCycling.stepType, 1);          // EXPONENTIAL
    QCOMPARE(ts.queueCycling.end, 32);
    QCOMPARE(ts.testType, 0);
    QVERIFY(doc.errors().empty());
}

void TestIcfDocument::setup_missingSection_okAndUntouched() {
    IcfDocument doc(makeFile("Version 1.1.0 \n'MANAGER LIST ====\n"));
    IcfTestSetup ts;
    ts.testName = "keep"; ts.seconds = 42;
    QVERIFY(doc.loadTestSetup(ts));     // "this is OK"
    QCOMPARE(ts.testName, std::string("keep"));
    QCOMPARE(ts.seconds, 42);
}

void TestIcfDocument::setup_numberOfCpusToken() {
    IcfDocument doc(makeFile(setupFile(
        "'Default Disk Workers to Spawn\n\tNUMBER_OF_CPUS\n"
        "'Default Network Workers to Spawn\n\tnumber_of_cpus\n")));
    IcfTestSetup ts;
    QVERIFY(doc.loadTestSetup(ts));
    QCOMPARE(ts.diskWorkerCount, -1);
    QCOMPARE(ts.netWorkerCount, -1);
}

void TestIcfDocument::setup_ancientDefaultWorkers_zeroBecomesMinusOne() {
    IcfDocument doc(makeFile(setupFile("'Default Workers to Spawn\n\t0\n")));
    IcfTestSetup ts;
    QVERIFY(doc.loadTestSetup(ts));
    QCOMPARE(ts.diskWorkerCount, -1);   // legacy: 0 meant "# of CPUs"
}

void TestIcfDocument::setup_recordResults_tokensAndLegacyInt() {
    {
        IcfDocument doc(makeFile(setupFile("'Record Results\n\tNO_WORKERS\n")));
        IcfTestSetup ts;
        QVERIFY(doc.loadTestSetup(ts));
        QCOMPARE(ts.resultType, 2);
    }
    {
        IcfDocument doc(makeFile(setupFile("'Record Results\n\t3\n")));
        IcfTestSetup ts;
        QVERIFY(doc.loadTestSetup(ts));
        QCOMPARE(ts.resultType, 3);      // legacy integer
    }
}

void TestIcfDocument::setup_testType_tokenAndLegacyInt() {
    {
        IcfDocument doc(makeFile(setupFile("'Test Type\n\tCYCLE_OUTSTANDING_IOS_AND_TARGETS\n")));
        IcfTestSetup ts;
        QVERIFY(doc.loadTestSetup(ts));
        QCOMPARE(ts.testType, 7);
    }
    {
        IcfDocument doc(makeFile(setupFile("'Test Type\n\t5\n")));
        IcfTestSetup ts;
        QVERIFY(doc.loadTestSetup(ts));
        QCOMPARE(ts.testType, 5);
    }
}

void TestIcfDocument::setup_unrecognizedKey_modernFails() {
    IcfDocument doc(makeFile(setupFile("'Bogus Key\n\t1\n")));
    IcfTestSetup ts;
    QVERIFY(!doc.loadTestSetup(ts));    // version 1.1.0 >= 19990217
    QCOMPARE(doc.errors().back(), std::string("File is improperly formatted.  "
        "TEST SETUP section contained an unrecognized \"'Bogus Key\" comment."));
}

void TestIcfDocument::setup_unrecognizedKey_oldVersionBreaks() {
    // Pre-1999.02.17 files have no 'End test setup convention: an unknown key
    // just terminates the section, successfully.
    IcfDocument doc(makeFile(
        "Version 1998.10.08 \n'TEST SETUP ====\n"
        "'Test Description\n\told file\n"
        "'Something Unknown\n\tx\n"));
    IcfTestSetup ts;
    QVERIFY(doc.loadTestSetup(ts));
    QCOMPARE(ts.testName, std::string("old file"));
    QVERIFY(doc.errors().empty());
}

void TestIcfDocument::setup_badRunTime_failsWithMfcText() {
    IcfDocument doc(makeFile(setupFile("'Run Time\n\t1 2\n'Test Type\n\tNORMAL\n")));
    IcfTestSetup ts;
    QVERIFY(!doc.loadTestSetup(ts));
    QCOMPARE(doc.errors().back(), std::string("Error while reading file.  "
        "\"Run time\" should be specified as three integer values "
        "(hours, minutes, seconds)."));
}

void TestIcfDocument::setup_onlyPresentKeysOverwrite() {
    IcfDocument doc(makeFile(setupFile("'Ramp Up Time (s)\n\t9\n")));
    IcfTestSetup ts;
    ts.testName = "prior"; ts.seconds = 30; ts.testType = 6;
    QVERIFY(doc.loadTestSetup(ts));
    QCOMPARE(ts.rampTime, 9);                      // present -> overwritten
    QCOMPARE(ts.testName, std::string("prior"));   // absent -> retained
    QCOMPARE(ts.seconds, 30);
    QCOMPARE(ts.testType, 6);
}

// ---- RESULTS DISPLAY ----------------------------------------------------------

static std::string displayFile(const std::string &body) {
    return "Version 1.1.0 \n'RESULTS DISPLAY ====\n" + body + "'END results display\n";
}

void TestIcfDocument::display_updateLineAndBars() {
    IcfDocument doc(makeFile(displayFile(
        "'Record Last Update Results,Update Frequency,Update Type\n\tENABLED,5,WHOLE_TEST\n"
        "'Bar chart 1 statistic\n\tTotal I/Os per Second\n")));
    IcfDisplaySettings ds;
    QVERIFY(doc.loadResultsDisplay(ds));
    QVERIFY(ds.updateLinePresent);
    QVERIFY(ds.recordLastUpdate);
    QCOMPARE(ds.updateFrequency, 5);
    QCOMPARE(ds.whichPerf, 0);                     // WHOLE_TEST_PERF
    QCOMPARE(ds.bars.size(), size_t(1));
    QCOMPARE(ds.bars[0].index, 0);
    QCOMPARE((int)ds.bars[0].kind, (int)IcfDisplaySettings::Bar::Statistic);
    QCOMPARE(ds.bars[0].name, std::string("Total I/Os per Second"));
}

void TestIcfDocument::display_managerAndWorkerBars() {
    IcfDocument doc(makeFile(displayFile(
        "'Bar chart 2 manager ID, manager name\n\t3,MyManager\n"
        "'Bar chart 3 worker ID, worker name\n\t7,Worker 1\n")));
    IcfDisplaySettings ds;
    QVERIFY(doc.loadResultsDisplay(ds));
    QCOMPARE(ds.bars.size(), size_t(2));
    QCOMPARE((int)ds.bars[0].kind, (int)IcfDisplaySettings::Bar::Manager);
    QCOMPARE(ds.bars[0].index, 1);
    QCOMPARE(ds.bars[0].id, 3);
    QCOMPARE(ds.bars[0].name, std::string("MyManager"));
    QCOMPARE((int)ds.bars[1].kind, (int)IcfDisplaySettings::Bar::Worker);
    QCOMPARE(ds.bars[1].id, 7);
    QCOMPARE(ds.bars[1].name, std::string("Worker 1"));
}

void TestIcfDocument::display_invalidBarNumber_ignoredWithError() {
    IcfDocument doc(makeFile(displayFile(
        "'Bar chart 9 statistic\n\tErrors\n"
        "'Bar chart 1 statistic\n\tIOps\n")));
    IcfDisplaySettings ds;
    QVERIFY(doc.loadResultsDisplay(ds));           // continues like MFC
    QCOMPARE(ds.bars.size(), size_t(1));           // the bad one is dropped
    QCOMPARE(doc.errors().size(), size_t(1));
    QCOMPARE(doc.errors()[0], std::string("Invalid bar chart number in RESULTS "
        "DISPLAY section.  Ignoring this bar setting."));
}

void TestIcfDocument::display_invalidBarItem_ignoredWithError() {
    IcfDocument doc(makeFile(displayFile("'Bar chart 2 frobnicator\n\tx\n")));
    IcfDisplaySettings ds;
    QVERIFY(doc.loadResultsDisplay(ds));
    QVERIFY(ds.bars.empty());
    QCOMPARE(doc.errors()[0], std::string("Invalid bar chart item name "
        "\"frobnicator\" for bar #1 in RESULTS DISPLAY section.  "
        "Ignoring this bar setting."));            // zero-based # like MFC
}

void TestIcfDocument::display_badUpdateType_fails() {
    IcfDocument doc(makeFile(displayFile(
        "'Record Last Update Results,Update Frequency,Update Type\n\tDISABLED,1,SOMETIMES\n")));
    IcfDisplaySettings ds;
    QVERIFY(!doc.loadResultsDisplay(ds));
    QCOMPARE(doc.errors().back(), std::string("File is improperly formatted.  "
        "In RESULTS DISPLAY section, Update Frequency was not followed by "
        "an appropriate Update Type string."));
}

void TestIcfDocument::display_unrecognizedKey_fails() {
    IcfDocument doc(makeFile(displayFile("'Pie chart 1\n\tx\n")));
    IcfDisplaySettings ds;
    QVERIFY(!doc.loadResultsDisplay(ds));
    QCOMPARE(doc.errors().back(), std::string("File is improperly formatted.  "
        "RESULTS DISPLAY section contained an unrecognized \"'Pie chart 1\" comment."));
}

// ---- ACCESS SPECIFICATIONS ------------------------------------------------------

static std::string specsFile(const std::string &body, const char *version = "1.1.0") {
    return std::string("Version ") + version + " \n"
         + "'ACCESS SPECIFICATIONS ====\n" + body + "'END access specifications\n";
}

void TestIcfDocument::specs_newFormat_basics() {
    IcfDocument doc(makeFile(specsFile(
        "'Access specification name,default assignment\n"
        "\t64 KiB; 100% Read; 0% random,NONE\n"
        "'size,% of size,% reads,% random,delay,burst,align,reply\n"
        "\t65536,100,100,0,0,1,0,0\n")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out.size(), size_t(1));
    QCOMPARE(out[0].name, std::string("64 KiB; 100% Read; 0% random"));
    QCOMPARE(out[0].defaultAssignment, 0);
    QCOMPARE(out[0].lines.size(), size_t(1));
    QCOMPARE(out[0].lines[0].size, 65536);
    QCOMPARE(out[0].lines[0].reads, 100);
}

void TestIcfDocument::specs_newFormat_assignmentTokens() {
    IcfDocument doc(makeFile(specsFile(
        "'Access specification name,default assignment\n\tA,ALL\n"
        "'size,...\n\t512,100,100,0,0,1,0,0\n"
        "'Access specification name,default assignment\n\tB,DISK\n"
        "'size,...\n\t512,100,100,0,0,1,0,0\n"
        "'Access specification name,default assignment\n\tC,NET\n"
        "'size,...\n\t512,100,100,0,0,1,0,0\n"
        "'Access specification name,default assignment\n\tD,7\n"
        "'size,...\n\t512,100,100,0,0,1,0,0\n")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out.size(), size_t(4));
    QCOMPARE(out[0].defaultAssignment, 1);   // ALL
    QCOMPARE(out[1].defaultAssignment, 2);   // DISK
    QCOMPARE(out[2].defaultAssignment, 3);   // NET
    QCOMPARE(out[3].defaultAssignment, 7);   // legacy integer
}

void TestIcfDocument::specs_newFormat_replaceByName() {
    IcfDocument doc(makeFile(specsFile(
        "'Access specification name,default assignment\n\tDupe,NONE\n"
        "'size,...\n\t512,100,100,0,0,1,0,0\n"
        "'Access specification name,default assignment\n\tDupe,ALL\n"
        "'size,...\n\t4096,100,50,0,0,1,0,0\n")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out.size(), size_t(1));         // second replaced the first
    QCOMPARE(out[0].defaultAssignment, 1);
    QCOMPARE(out[0].lines[0].size, 4096);
}

void TestIcfDocument::specs_newFormat_percentagesMustSum() {
    IcfDocument doc(makeFile(specsFile(
        "'Access specification name,default assignment\n\tShort,NONE\n"
        "'size,...\n\t512,60,100,0,0,1,0,0\n")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(!doc.loadAccessSpecs(out));
    QVERIFY(out.empty());                     // failing spec dropped (MFC Delete)
    QCOMPARE(doc.errors().back(), std::string("Error loading global access "
        "specifications.  Percentages don't add to 100."));
}

void TestIcfDocument::specs_newFormat_truncatedFails() {
    // Abrupt EOF mid-section (no END marker): MFC errors out.
    IcfDocument doc(makeFile(
        "Version 1.1.0 \n'ACCESS SPECIFICATIONS ====\n"
        "'Access specification name,default assignment\n\tHalfWritten,NONE\n"));
    std::vector<IcfAccessSpec> out;
    QVERIFY(!doc.loadAccessSpecs(out));
}

void TestIcfDocument::specs_oldFormat_smartNamedAndAssignAll() {
    IcfDocument doc(makeFile(specsFile(
        "'size, % of size, % reads, % random, delay, burst\n"
        "2048 100 67 100 0 1\n", "1998.02.01")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out.size(), size_t(1));
    QCOMPARE(out[0].name, std::string("2 KiB random 67% reads"));   // SmartName
    QCOMPARE(out[0].defaultAssignment, 1);                          // AssignAll
}

void TestIcfDocument::specs_oldFormat_multiLineStaysUntitled() {
    // Two 50% lines: multi-line specs can't be smart-named -> "Untitled 1".
    IcfDocument doc(makeFile(specsFile(
        "'size, % of size, % reads, % random, delay, burst\n"
        "512 50 100 0 0 1\n"
        "4096 50 0 100 0 1\n", "1998.02.01")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out.size(), size_t(1));
    QCOMPARE(out[0].name, std::string("Untitled 1"));
    QCOMPARE(out[0].lines.size(), size_t(2));
}

void TestIcfDocument::specs_oldFormat_alignReplyDefaults() {
    // The old format reads only six fields; align keeps the InitAccessSpecLine
    // default (2048) and reply stays 0 - a faithful MFC quirk.
    IcfDocument doc(makeFile(specsFile(
        "'size, % of size, % reads, % random, delay, burst\n"
        "65536 100 100 0 0 1\n", "1998.02.01")));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QCOMPARE(out[0].lines[0].align, 2048);
    QCOMPARE(out[0].lines[0].reply, 0);
}

void TestIcfDocument::specs_oldFormat_uniqueAgainstExisting() {
    // A live list already containing the smart name forces the " 2" suffix.
    IcfDocument doc(makeFile(specsFile(
        "'size, % of size, % reads, % random, delay, burst\n"
        "2048 100 67 100 0 1\n", "1998.02.01")));
    std::vector<IcfAccessSpec> out;
    const std::vector<std::string> existing = { "Idle", "2 KiB random 67% reads" };
    QVERIFY(doc.loadAccessSpecs(out, &existing));
    QCOMPARE(out[0].name, std::string("2 KiB random 67% reads 2"));
}

void TestIcfDocument::specs_missingSection_ok() {
    IcfDocument doc(makeFile("Version 1.1.0 \n'MANAGER LIST ====\n"));
    std::vector<IcfAccessSpec> out;
    QVERIFY(doc.loadAccessSpecs(out));
    QVERIFY(out.empty());
}

// ---- MANAGER LIST ---------------------------------------------------------------

static std::string mgrFile(const std::string &managerBody) {
    return "Version 1.1.0 \n'MANAGER LIST ====\n" + managerBody + "'END manager list\n";
}
// A standard disk worker block with the given name/settings/specs.
static std::string diskWorker(const std::string &name, const std::string &settings,
                              const std::string &disk, const std::string &spec,
                              const std::string &target = "TEST: Synthetic Disk") {
    return "'Worker\n\t" + name + "\n'Worker type\n\tDISK\n"
           "'Default target settings for worker\n"
           "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value\n\t"
           + settings + "\n"
           "'Disk maximum size,starting sector,Data pattern\n\t" + disk + "\n"
           "'End default target settings for worker\n"
           "'Assigned access specs\n\t" + spec + "\n'End assigned access specs\n"
           "'Target assignments\n'Target\n\t" + target + "\n'Target type\n\tDISK\n"
           "'End target\n'End target assignments\n'End worker\n";
}
static std::string mgrHdr(int id, const std::string &name, const std::string &addr) {
    return "'Manager ID, manager name\n\t" + std::to_string(id) + "," + name
         + "\n'Manager network address\n\t" + addr + "\n";
}

void TestIcfDocument::mgr_singleDiskWorker() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "TESTMGR", "localhost")
        + diskWorker("Worker 1", "4,DISABLED,1,DISABLED,0", "0,0,0", "64 KiB; 100% Read; 0% random")
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out.size(), size_t(1));
    QCOMPARE(out[0].name, std::string("TESTMGR"));
    QCOMPARE(out[0].id, 1);
    QCOMPARE(out[0].address, std::string("localhost"));
    QCOMPARE(out[0].workers.size(), size_t(1));
    const auto &w = out[0].workers[0];
    QCOMPARE(w.name, std::string("Worker 1"));
    QCOMPARE((int)w.kind, (int)IcfWorkerKind::Disk);
    QCOMPARE(w.queueDepth, 4);
    QCOMPARE(w.assignedSpecNames.size(), size_t(1));
    QCOMPARE(w.assignedSpecNames[0], std::string("64 KiB; 100% Read; 0% random"));
    QCOMPARE(w.targets.size(), size_t(1));
    QCOMPARE(w.targets[0].name, std::string("TEST: Synthetic Disk"));
    QCOMPARE((int)w.targets[0].kind, (int)IcfWorkerKind::Disk);
}

void TestIcfDocument::mgr_richWorkerSettings() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "TESTMGR", "localhost")
        + diskWorker("Rich", "8,ENABLED,5,ENABLED,12345", "1024,64,1", "64 KiB; 100% Read; 0% random")
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    const auto &w = out[0].workers[0];
    QVERIFY(w.hasDefaultSettings);
    QCOMPARE(w.queueDepth, 8);
    QVERIFY(w.testConnRate);
    QCOMPARE(w.transPerConn, 5);
    QVERIFY(w.useFixedSeed);
    QCOMPARE(w.fixedSeedValue, Q_UINT64_C(12345));
    QCOMPARE(w.diskMaxSize, Q_UINT64_C(1024));
    QCOMPARE(w.startingSector, Q_UINT64_C(64));
    QCOMPARE(w.dataPattern, 1);
}

void TestIcfDocument::mgr_twoWorkers() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "TESTMGR", "localhost")
        + diskWorker("Worker 1", "1,DISABLED,1,DISABLED,0", "0,0,0", "SpecA")
        + diskWorker("Worker 2", "2,DISABLED,1,DISABLED,0", "0,0,0", "SpecB")
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out[0].workers.size(), size_t(2));
    QCOMPARE(out[0].workers[1].name, std::string("Worker 2"));
    QCOMPARE(out[0].workers[1].queueDepth, 2);
    QCOMPARE(out[0].workers[1].assignedSpecNames[0], std::string("SpecB"));
}

void TestIcfDocument::mgr_twoManagers() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M1", "localhost")
        + diskWorker("Worker A", "1,DISABLED,1,DISABLED,0", "0,0,0", "SpecA") + "'End manager\n"
        + mgrHdr(2, "M2", "localhost")
        + diskWorker("Worker B", "1,DISABLED,1,DISABLED,0", "0,0,0", "SpecB") + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out.size(), size_t(2));
    QCOMPARE(out[0].name, std::string("M1")); QCOMPARE(out[0].id, 1);
    QCOMPARE(out[1].name, std::string("M2")); QCOMPARE(out[1].id, 2);
    QCOMPARE(out[1].workers[0].name, std::string("Worker B"));
}

void TestIcfDocument::mgr_skipAspecsAndTargets() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "TESTMGR", "localhost")
        + diskWorker("Worker 1", "1,DISABLED,1,DISABLED,0", "0,0,0", "SpecA")
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out, /*aspecs*/false, /*targets*/false));
    const auto &w = out[0].workers[0];
    QVERIFY(w.assignedSpecNames.empty());   // skipped, but section consumed
    QVERIFY(w.targets.empty());
    QCOMPARE(w.queueDepth, 1);              // default settings still parsed
}

void TestIcfDocument::mgr_diskSizeOnNetWorkerFails() {
    // A TCP worker with a 'Disk maximum size key -> MFC type-validation error.
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M", "localhost")
        + "'Worker\n\tNetW\n'Worker type\n\tNETWORK,TCP\n"
          "'Default target settings for worker\n"
          "'Disk maximum size,starting sector\n\t0,0\n"
          "'End default target settings for worker\n'End worker\n"
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(!doc.loadManagerList(out));
    QCOMPARE(doc.errors().back(), std::string("Error restoring worker NetW.  "
        "Cannot specify \"Disk maximum size,starting sector\" for a non-disk worker."));
}

void TestIcfDocument::mgr_unknownWorkerType_fails() {
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M", "localhost")
        + "'Worker\n\tBad\n'Worker type\n\tBANANA\n'End worker\n'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(!doc.loadManagerList(out));
    QCOMPARE(doc.errors().back(), std::string("Unknown worker type encountered for "
        "worker \"Bad\": \"BANANA\".  Should be either DISK or NETWORK."));
}

void TestIcfDocument::mgr_unknownNetSubtype_recordsErrorButContinues() {
    // MFC ErrorMessage()s but does NOT abort on a bad NETWORK subtype; the worker
    // keeps InvalidType and parsing continues -> loadManagerList returns true.
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M", "localhost")
        + "'Worker\n\tNetW\n'Worker type\n\tNETWORK,BANANA\n'End worker\n'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out.size(), size_t(1));
    QCOMPARE(out[0].workers.size(), size_t(1));
    QCOMPARE((int)out[0].workers[0].kind, (int)IcfWorkerKind::Invalid);
    QCOMPARE(doc.errors().back(), std::string("Unknown network worker subtype "
        "encountered for worker \"NetW\": \"BANANA\".  Should be either TCP or VI."));
}

void TestIcfDocument::mgr_badManagerId_fails() {
    IcfDocument doc(makeFile(mgrFile(
        "'Manager ID, manager name\n\tNOTANUMBER,M\n"
        "'Manager network address\n\tlocalhost\n'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(!doc.loadManagerList(out));
    QCOMPARE(doc.errors().back(), std::string("File is improperly formatted.  "
        "Error retrieving manager ID.  This value must be an integer."));
}

void TestIcfDocument::mgr_networkTarget() {
    // A TCP worker with a TCP target: the target carries the remote manager's
    // ID + name ('Target manager ID, manager name) before 'End target.
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M", "localhost")
        + "'Worker\n\tNet 1\n'Worker type\n\tNETWORK,TCP\n"
          "'Assigned access specs\n\t4 KiB; 100% Read; 0% random\n'End assigned access specs\n"
          "'Target assignments\n'Target\n\t192.168.0.5\n'Target type\n\tNETWORK,TCP\n"
          "'Target manager ID, manager name\n\t2,REMOTE\n'End target\n"
          "'End target assignments\n'End worker\n"
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out[0].workers.size(), size_t(1));
    const auto &w = out[0].workers[0];
    QCOMPARE((int)w.kind, (int)IcfWorkerKind::NetTCP);
    QCOMPARE(w.targets.size(), size_t(1));
    QCOMPARE(w.targets[0].name, std::string("192.168.0.5"));
    QCOMPARE((int)w.targets[0].kind, (int)IcfWorkerKind::NetTCP);
    QCOMPARE(w.targets[0].targetManagerId, 2);
    QCOMPARE(w.targets[0].targetManagerName, std::string("REMOTE"));
}

void TestIcfDocument::mgr_networkTarget_missingMgrLine_fails() {
    // MFC requires the 'Target manager ID, manager name pair after a net target.
    IcfDocument doc(makeFile(mgrFile(
        mgrHdr(1, "M", "localhost")
        + "'Worker\n\tNet 1\n'Worker type\n\tNETWORK,TCP\n"
          "'Target assignments\n'Target\n\t192.168.0.5\n'Target type\n\tNETWORK,TCP\n"
          "'End target\n'End target assignments\n'End worker\n"
        + "'End manager\n")));
    std::vector<IcfManagerConfig> out;
    QVERIFY(!doc.loadManagerList(out));
    QCOMPARE(doc.errors().back(), std::string("File is improperly formatted.  Expected "
        "a \"Target manager ID, manager name\" comment after \"Target type\"."));
}

void TestIcfDocument::mgr_missingSection_ok() {
    IcfDocument doc(makeFile("Version 1.1.0 \n'TEST SETUP ====\n'END test setup\n"));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QVERIFY(out.empty());
}

void TestIcfDocument::mgr_realFixtureWorkerRich() {
    // The actual corpus fixture (also a golden), parsed through the loader.
    IcfDocument doc(compatFixture("worker_rich.icf"));
    std::vector<IcfManagerConfig> out;
    QVERIFY(doc.loadManagerList(out));
    QCOMPARE(out.size(), size_t(1));
    const auto &w = out[0].workers[0];
    QCOMPARE(w.queueDepth, 8);
    QVERIFY(w.useFixedSeed);
    QCOMPARE(w.fixedSeedValue, Q_UINT64_C(12345));
    QCOMPARE(w.diskMaxSize, Q_UINT64_C(1024));
    QCOMPARE(w.startingSector, Q_UINT64_C(64));
    QCOMPARE(w.dataPattern, 1);
}

QTEST_APPLESS_MAIN(TestIcfDocument)
#include "tst_icfdocument.moc"
