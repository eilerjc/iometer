// tst_icffile.cpp — Direct unit tests for the pure-C++ core IcfFile parser.
//
// Calls IcfFile::load/save with std:: types directly (no QtDynamoEngine), proving
// the core ICF interface works standalone. tst_icf.cpp covers the same format
// through the Qt engine; this verifies the lower layer in isolation.
#include <QObject>
#include <QTest>
#include "../core/IcfFile.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

static std::string fixture(const std::string &name)
{
    return std::string(IOMETER_FIXTURE_DIR) + "/" + name;
}

static std::string tempPath(const std::string &leaf)
{
    auto p = std::filesystem::temp_directory_path() / ("iometer_core_" + leaf);
    return p.string();
}

// Find a spec by name in a parsed vector (returns nullptr if absent).
static const AccessSpec* findSpec(const std::vector<AccessSpec> &specs,
                                  const std::string &name)
{
    for (const auto &s : specs)
        if (s.name == name) return &s;
    return nullptr;
}

class IcfFileTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Version validation ───────────────────────────────────────────────────
    void isValidVersion_acceptsAndRejects() {
        QVERIFY(IcfFile::isValidVersion("Version 1.1.0"));
        QVERIFY(IcfFile::isValidVersion("1.1.0"));
        QVERIFY(!IcfFile::isValidVersion("2.0.0"));
        QVERIFY(!IcfFile::isValidVersion(""));
    }

    // ── Missing file ─────────────────────────────────────────────────────────
    void load_missingFileReturnsFalse() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(!IcfFile::load("/no/such/file.icf", cfg, specs, workers));
    }

    // ── minimal.icf ──────────────────────────────────────────────────────────
    void load_minimal_runTime() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(fixture("minimal.icf"), cfg, specs, workers));
        QCOMPARE(cfg.runHours,   0);
        QCOMPARE(cfg.runMinutes, 0);
        QCOMPARE(cfg.runSeconds, 5);
        QCOMPARE(cfg.rampSeconds, 0);
    }
    void load_minimal_specs() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("minimal.icf"), cfg, specs, workers);
        // Default + "64 KiB; 100% Read; 0% random"
        QCOMPARE(specs.size(), size_t(2));
        const AccessSpec *def = findSpec(specs, "Default");
        QVERIFY(def != nullptr);
        QCOMPARE(def->lines.size(), size_t(1));
        QCOMPARE(def->lines[0].sizeBytes, 65536);
        QCOMPARE(def->lines[0].readPercent, 100);
    }
    void load_minimal_blankDescription() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("minimal.icf"), cfg, specs, workers);
        // Blank description must not absorb the next numeric line
        QVERIFY(cfg.description.empty() ||
                cfg.description.find_first_not_of(" \t") == std::string::npos);
    }

    // ── multispec.icf — multi-line spec + batch workers ──────────────────────
    void load_multispec_runTimeAndRamp() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(fixture("multispec.icf"), cfg, specs, workers));
        QCOMPARE(cfg.runSeconds, 30);
        QCOMPARE(cfg.rampSeconds, 5);
    }
    void load_multispec_allInOneHas29Lines() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);
        const AccessSpec *aio = findSpec(specs, "All in one");
        QVERIFY(aio != nullptr);
        QCOMPARE(aio->lines.size(), size_t(29));
    }
    void load_multispec_randomPercentDecoded() {
        // "4 KiB; 50% Read; 100% random" → seqPercent = 100 - 100 = 0
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);
        const AccessSpec *rnd = findSpec(specs, "4 KiB; 50% Read; 100% random");
        QVERIFY(rnd != nullptr);
        QCOMPARE(rnd->lines[0].seqPercent, 0);
        QCOMPARE(rnd->lines[0].readPercent, 50);
        QCOMPARE(rnd->lines[0].sizeBytes, 4096);
    }
    void load_multispec_twoBatchWorkers() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);
        QCOMPARE(workers.size(), size_t(2));
        QCOMPARE(workers[0].name, std::string("Worker 1"));
        QVERIFY(!workers[0].assignedSpecs.empty());
        QCOMPARE(workers[0].assignedSpecs[0],
                 std::string("64 KiB; 100% Read; 0% random"));
        QVERIFY(!workers[0].targets.empty());
        // extractTarget() returns the quoted volume label (C: "System" → System),
        // matching the smoke-test contract (W: "nvme" → nvme).
        QCOMPARE(workers[0].targets[0], std::string("System"));
    }

    // ── smoke_test.icf — large spec library + one worker ─────────────────────
    void load_smoke_runTimeAndWorker() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(std::string(IOMETER_SMOKE_ICF), cfg, specs, workers));
        QCOMPARE(cfg.runSeconds, 10);
        QVERIFY(specs.size() >= 30);
        QCOMPARE(workers.size(), size_t(1));
        QVERIFY(!workers[0].targets.empty());
        QVERIFY(workers[0].targets[0].find("nvme") != std::string::npos);
    }

    // ── Worker type (DISK / TCP) round-trips ─────────────────────────────────
    void load_networkWorkerType() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(fixture("network_worker.icf"), cfg, specs, workers));
        QCOMPARE(workers.size(), size_t(1));
        QCOMPARE(workers[0].name, std::string("Worker 1"));
        QCOMPARE(workers[0].type, std::string("TCP"));   // not dropped to DISK
        QVERIFY(!workers[0].targets.empty());
        QCOMPARE(workers[0].targets[0], std::string("127.0.0.1"));
    }
    void load_diskWorkerTypeDefaults() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);
        QVERIFY(!workers.empty());
        QCOMPARE(workers[0].type, std::string("DISK"));
    }
    void roundTrip_workerType() {
        TestConfig cfg; cfg.runSeconds = 5;
        std::vector<AccessSpec> specs;
        AccessSpec s; s.name = "4 KiB; 100% Read; 0% random";
        AccessSpecLine l; l.sizeBytes = 4096; l.readPercent = 100; l.seqPercent = 100; l.ofSize = 100;
        s.lines.push_back(l); specs.push_back(s);
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::BatchWorker net; net.name = "Net 1"; net.type = "TCP";
        net.assignedSpecs.push_back(s.name); net.targets.push_back("127.0.0.1");
        workers.push_back(net);
        IcfFile::BatchWorker disk; disk.name = "Disk 1"; disk.type = "DISK";
        disk.assignedSpecs.push_back(s.name); disk.targets.push_back("C");
        workers.push_back(disk);

        const std::string out = tempPath("workertype.icf");
        QVERIFY(IcfFile::save(out, cfg, specs, workers));
        TestConfig cfg2; std::vector<AccessSpec> specs2;
        std::vector<IcfFile::BatchWorker> workers2;
        QVERIFY(IcfFile::load(out, cfg2, specs2, workers2));
        QCOMPARE(workers2.size(), size_t(2));
        QCOMPARE(workers2[0].type, std::string("TCP"));
        QCOMPARE(workers2[1].type, std::string("DISK"));
        std::filesystem::remove(out);
    }

    // ── Robustness: malformed numerics degrade gracefully (no throw) ─────────
    void load_garbledNumericsDoNotThrow() {
        // Non-numeric run-time and access-spec fields must parse to 0 rather
        // than throwing (exercises the parseInt/parseDouble catch paths).
        const std::string out = tempPath("garbled.icf");
        {
            std::ofstream f(out);
            f << "Version 1.1.0\n"
              << "'TEST SETUP ===\n"
              << "'Run Time\n'\thours      minutes    seconds\n"
              << "\tx          y          z\n"
              << "'Ramp Up Time (s)\n\tnope\n"
              << "'END test setup\n"
              << "'RESULTS DISPLAY ===\n'END results display\n"
              << "'ACCESS SPECIFICATIONS ===\n"
              << "'Access specification name,default assignment\n"
              << "\tGarbled,NONE\n"
              << "'size,% of size,% reads,% random,delay,burst,align,reply\n"
              << "\tbig,lots,half,some,wait,burst,align,reply\n"
              << "'END access specifications\n"
              << "'MANAGER LIST ===\n'END manager list\nVersion 1.1.0\n";
        }
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(out, cfg, specs, workers));   // graceful, no throw
        QCOMPARE(cfg.runHours, 0);
        QCOMPARE(cfg.runSeconds, 0);
        QCOMPARE(cfg.rampSeconds, 0);
        // The spec line still parsed structurally; numeric fields fell back to 0
        const AccessSpec *g = findSpec(specs, "Garbled");
        QVERIFY(g != nullptr);
        QCOMPARE(g->lines.size(), size_t(1));
        QCOMPARE(g->lines[0].sizeBytes, 0);
        std::filesystem::remove(out);
    }
    void load_truncatedMidSpecDoesNotCrash() {
        // File cut off mid-access-spec: parser must bail cleanly, not over-read.
        const std::string out = tempPath("trunc.icf");
        {
            std::ofstream f(out);
            f << "Version 1.1.0\n"
              << "'TEST SETUP ===\n'Run Time\n'\thours minutes seconds\n\t0 0 9\n"
              << "'END test setup\n"
              << "'RESULTS DISPLAY ===\n'END results display\n"
              << "'ACCESS SPECIFICATIONS ===\n"
              << "'Access specification name,default assignment\n"
              << "\tHalfWritten,NONE\n";   // abrupt EOF — no data rows, no END markers
        }
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(out, cfg, specs, workers));
        QCOMPARE(cfg.runSeconds, 9);   // earlier sections still parsed
        std::filesystem::remove(out);
    }

    // ── Round-trip: save → load preserves fields ─────────────────────────────
    void roundTrip_configAndSpecs() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        QVERIFY(IcfFile::load(std::string(IOMETER_SMOKE_ICF), cfg, specs, workers));

        const std::string out = tempPath("roundtrip.icf");
        QVERIFY(IcfFile::save(out, cfg, specs, workers));

        TestConfig cfg2;
        std::vector<AccessSpec> specs2;
        std::vector<IcfFile::BatchWorker> workers2;
        QVERIFY(IcfFile::load(out, cfg2, specs2, workers2));

        QCOMPARE(cfg2.runSeconds, cfg.runSeconds);
        QCOMPARE(cfg2.runMinutes, cfg.runMinutes);
        QCOMPARE(cfg2.runHours,   cfg.runHours);
        QCOMPARE(cfg2.rampSeconds, cfg.rampSeconds);
        QCOMPARE(specs2.size(), specs.size());
        QCOMPARE(workers2.size(), workers.size());

        std::filesystem::remove(out);
    }
    void roundTrip_specNamesAndLines() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);

        const std::string out = tempPath("roundtrip2.icf");
        IcfFile::save(out, cfg, specs, workers);

        TestConfig cfg2;
        std::vector<AccessSpec> specs2;
        std::vector<IcfFile::BatchWorker> workers2;
        IcfFile::load(out, cfg2, specs2, workers2);

        QCOMPARE(specs2.size(), specs.size());
        for (size_t i = 0; i < specs.size(); ++i) {
            QCOMPARE(specs2[i].name, specs[i].name);
            QCOMPARE(specs2[i].lines.size(), specs[i].lines.size());
        }
        // "All in one" must survive with all 29 lines
        const AccessSpec *aio = findSpec(specs2, "All in one");
        QVERIFY(aio != nullptr);
        QCOMPARE(aio->lines.size(), size_t(29));

        std::filesystem::remove(out);
    }
    void roundTrip_defaultSpecFlag() {
        // A spec flagged defaultSpec must be saved as ",DEFAULT" and re-parsed
        // back to defaultSpec=true (exercises both the save and load branches).
        TestConfig cfg; cfg.runSeconds = 7;
        std::vector<AccessSpec> specs;
        AccessSpec def; def.name = "MyDefault"; def.defaultSpec = true;
        AccessSpecLine l; l.sizeBytes = 4096; l.readPercent = 100; l.seqPercent = 100; l.ofSize = 100;
        def.lines.push_back(l);
        specs.push_back(def);
        AccessSpec plain; plain.name = "Plain"; plain.defaultSpec = false;
        plain.lines.push_back(l);
        specs.push_back(plain);
        std::vector<IcfFile::BatchWorker> workers;

        const std::string out = tempPath("defaultspec.icf");
        QVERIFY(IcfFile::save(out, cfg, specs, workers));

        TestConfig cfg2;
        std::vector<AccessSpec> specs2;
        std::vector<IcfFile::BatchWorker> workers2;
        QVERIFY(IcfFile::load(out, cfg2, specs2, workers2));

        const AccessSpec *d = findSpec(specs2, "MyDefault");
        const AccessSpec *p = findSpec(specs2, "Plain");
        QVERIFY(d != nullptr && p != nullptr);
        QVERIFY(d->defaultSpec);
        QVERIFY(!p->defaultSpec);

        std::filesystem::remove(out);
    }

    void roundTrip_batchWorkerTargets() {
        TestConfig cfg;
        std::vector<AccessSpec> specs;
        std::vector<IcfFile::BatchWorker> workers;
        IcfFile::load(fixture("multispec.icf"), cfg, specs, workers);

        const std::string out = tempPath("roundtrip3.icf");
        IcfFile::save(out, cfg, specs, workers);

        TestConfig cfg2;
        std::vector<AccessSpec> specs2;
        std::vector<IcfFile::BatchWorker> workers2;
        IcfFile::load(out, cfg2, specs2, workers2);

        QCOMPARE(workers2.size(), size_t(2));
        QCOMPARE(workers2[0].assignedSpecs[0], workers[0].assignedSpecs[0]);
        QCOMPARE(workers2[0].targets[0], workers[0].targets[0]);

        std::filesystem::remove(out);
    }
};

QTEST_GUILESS_MAIN(IcfFileTest)
#include "tst_icffile.moc"
