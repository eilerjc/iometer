// tst_icf_fixtures.cpp — Parse-validation for the focused smoke-test ICF set.
//
// Each fixture isolates one piece of "full Iometer" config so a parser
// regression points straight at the feature that broke:
//   two_managers.icf     — multiple managers, one worker each
//   multi_target.icf     — one worker striped across several targets
//   multispec_worker.icf — one worker with several assigned access specs
//   default_assign.icf   — a DEFAULT-flagged spec + an unquoted raw-disk target
#include <QObject>
#include <QTest>
#include "../core/IcfFile.h"
#include <string>
#include <vector>

static std::string fixture(const std::string &name)
{
    return std::string(IOMETER_FIXTURE_DIR) + "/" + name;
}

struct Parsed {
    TestConfig cfg;
    std::vector<AccessSpec> specs;
    std::vector<IcfFile::BatchWorker> workers;
    bool ok = false;
};

static Parsed parse(const std::string &name)
{
    Parsed p;
    p.ok = IcfFile::load(fixture(name), p.cfg, p.specs, p.workers);
    return p;
}

static const AccessSpec* findSpec(const std::vector<AccessSpec> &v, const std::string &n)
{
    for (const auto &s : v) if (s.name == n) return &s;
    return nullptr;
}

class IcfFixturesTest : public QObject
{
    Q_OBJECT
private slots:

    // ── two_managers.icf ─────────────────────────────────────────────────────
    void twoManagers_loads() {
        QVERIFY(parse("two_managers.icf").ok);
    }
    void twoManagers_runTime() {
        QCOMPARE(parse("two_managers.icf").cfg.runSeconds, 8);
    }
    void twoManagers_twoWorkersDistinct() {
        auto p = parse("two_managers.icf");
        QCOMPARE(p.workers.size(), size_t(2));
        QCOMPARE(p.workers[0].name, std::string("Worker A1"));
        QCOMPARE(p.workers[1].name, std::string("Worker B1"));
    }
    void twoManagers_targetsPerWorker() {
        auto p = parse("two_managers.icf");
        QVERIFY(!p.workers[0].targets.empty());
        QVERIFY(!p.workers[1].targets.empty());
        QCOMPARE(p.workers[0].targets[0], std::string("SystemA"));
        QCOMPARE(p.workers[1].targets[0], std::string("SystemB"));
    }

    // ── multi_target.icf ─────────────────────────────────────────────────────
    void multiTarget_loads() {
        QVERIFY(parse("multi_target.icf").ok);
    }
    void multiTarget_rampAndRunTime() {
        auto p = parse("multi_target.icf");
        QCOMPARE(p.cfg.runSeconds, 12);
        QCOMPARE(p.cfg.rampSeconds, 2);
    }
    void multiTarget_oneWorkerThreeTargets() {
        auto p = parse("multi_target.icf");
        QCOMPARE(p.workers.size(), size_t(1));
        QCOMPARE(p.workers[0].targets.size(), size_t(3));
        QCOMPARE(p.workers[0].targets[0], std::string("Vol0"));
        QCOMPARE(p.workers[0].targets[1], std::string("Vol1"));
        QCOMPARE(p.workers[0].targets[2], std::string("Vol2"));
    }

    // ── multispec_worker.icf ─────────────────────────────────────────────────
    void multispecWorker_loads() {
        QVERIFY(parse("multispec_worker.icf").ok);
    }
    void multispecWorker_runTimeMinutes() {
        QCOMPARE(parse("multispec_worker.icf").cfg.runMinutes, 1);
    }
    void multispecWorker_threeSpecsInLibrary() {
        QCOMPARE(parse("multispec_worker.icf").specs.size(), size_t(3));
    }
    void multispecWorker_threeAssignedSpecs() {
        auto p = parse("multispec_worker.icf");
        QCOMPARE(p.workers.size(), size_t(1));
        QCOMPARE(p.workers[0].assignedSpecs.size(), size_t(3));
        QCOMPARE(p.workers[0].assignedSpecs[0], std::string("512 B; 100% Read; 0% random"));
        QCOMPARE(p.workers[0].assignedSpecs[2], std::string("64 KiB; 0% Read; 0% random"));
    }
    void multispecWorker_randomSpecDecoded() {
        auto p = parse("multispec_worker.icf");
        const AccessSpec *rnd = findSpec(p.specs, "4 KiB; 50% Read; 100% random");
        QVERIFY(rnd != nullptr);
        QCOMPARE(rnd->lines[0].seqPercent, 0);     // 100% random → 0% sequential
        QCOMPARE(rnd->lines[0].alignBytes, 4096);
    }

    // ── default_assign.icf ───────────────────────────────────────────────────
    void defaultAssign_loads() {
        QVERIFY(parse("default_assign.icf").ok);
    }
    void defaultAssign_specFlaggedDefault() {
        auto p = parse("default_assign.icf");
        const AccessSpec *s = findSpec(p.specs, "2 KiB; 67% Read; 100% random");
        QVERIFY(s != nullptr);
        QVERIFY(s->defaultSpec);
    }
    void defaultAssign_unquotedTargetKept() {
        // Raw-disk targets have no quoted volume label; extractTarget returns
        // the bare token unchanged.
        auto p = parse("default_assign.icf");
        QCOMPARE(p.workers.size(), size_t(1));
        QCOMPARE(p.workers[0].targets[0], std::string("PHYSICALDRIVE0"));
    }
    void defaultAssign_rampUp() {
        QCOMPARE(parse("default_assign.icf").cfg.rampSeconds, 3);
    }
};

QTEST_GUILESS_MAIN(IcfFixturesTest)
#include "tst_icf_fixtures.moc"
