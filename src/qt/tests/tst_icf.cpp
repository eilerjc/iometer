// tst_icf.cpp — ICF load/save round-trip tests
// Verifies that QtDynamoEngine correctly parses original Iometer 1.1.0 ICF files
// and writes them back in a format reloadable by both original and Qt Iometer.
#include <QObject>
#include <QTest>
#include <QTemporaryFile>
#include <QDir>
#include "QtDynamoEngine.h"
#include "QtIometerTypes.h"

class IcfTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Minimal ICF: parse run-time fields ───────────────────────────────────
    void parseRunTime() {
        QtDynamoEngine eng(false);
        QVERIFY(eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/minimal.icf"));
        const TestConfig cfg = eng.testConfig();
        QCOMPARE(cfg.runHours,   0);
        QCOMPARE(cfg.runMinutes, 0);
        QCOMPARE(cfg.runSeconds, 5);
    }
    void parseRampUp() {
        QtDynamoEngine eng(false);
        QVERIFY(eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/minimal.icf"));
        QCOMPARE(eng.testConfig().rampSeconds, 0);
    }
    void parseDescription_blank() {
        QtDynamoEngine eng(false);
        QVERIFY(eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/minimal.icf"));
        // Blank description must not bleed into run-time data
        const QString desc = QString::fromStdString(eng.testConfig().description);
        bool isNumeric = desc.toInt() != 0 && !desc.isEmpty();
        QVERIFY2(!isNumeric, qPrintable("Description '" + desc + "' looks like run-time data"));
    }

    // ── Smoke-test ICF: parse access specs ───────────────────────────────────
    void smokeIcf_loadsCleanly() {
        QtDynamoEngine eng(false);
        QVERIFY(eng.loadConfig(QString(IOMETER_SMOKE_ICF)));
    }
    void smokeIcf_specCount() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_SMOKE_ICF));
        // smoke_test.icf contains all built-in specs plus a modified Default
        QVERIFY(eng.accessSpecs().size() >= 30);
    }
    void smokeIcf_runTime() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_SMOKE_ICF));
        const TestConfig cfg = eng.testConfig();
        QCOMPARE(cfg.runHours,   0);
        QCOMPARE(cfg.runMinutes, 0);
        QCOMPARE(cfg.runSeconds, 10);
    }
    void smokeIcf_batchAssignedSpec() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_SMOKE_ICF));
        QCOMPARE(eng.batchAssignedSpec(), QString("64 KiB; 100% Read; 0% random"));
    }
    void smokeIcf_batchWorkerCount() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_SMOKE_ICF));
        QCOMPARE(eng.batchWorkers().size(), 1);
    }
    void smokeIcf_batchWorkerTarget() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_SMOKE_ICF));
        const auto workers = eng.batchWorkers();
        QVERIFY(!workers.isEmpty());
        QVERIFY(!workers[0].targets.isEmpty());
        QVERIFY(workers[0].targets[0].contains("nvme"));
    }

    // ── Multi-spec ICF fixture ───────────────────────────────────────────────
    void multispecIcf_loadsCleanly() {
        QtDynamoEngine eng(false);
        QVERIFY(eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/multispec.icf"));
    }
    void multispecIcf_runTime() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/multispec.icf");
        const TestConfig cfg = eng.testConfig();
        QCOMPARE(cfg.runHours,   0);
        QCOMPARE(cfg.runMinutes, 0);
        QCOMPARE(cfg.runSeconds, 30);
    }
    void multispecIcf_workerCount() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/multispec.icf");
        QCOMPARE(eng.batchWorkers().size(), 2);
    }
    void multispecIcf_allInOneSpec() {
        QtDynamoEngine eng(false);
        eng.loadConfig(QString(IOMETER_FIXTURE_DIR) + "/multispec.icf");
        // "All in one" is multi-line — verify it parsed all 29 lines
        for (const auto &spec : eng.accessSpecs()) {
            if (spec.name == "All in one") {
                QCOMPARE(spec.lines.size(), 29);
                return;
            }
        }
        QFAIL("All in one spec not found after loading multispec.icf");
    }

    // ── Missing file ──────────────────────────────────────────────────────────
    void missingFile_returnsFalse() {
        QtDynamoEngine eng(false);
        QVERIFY(!eng.loadConfig("/nonexistent/path/to/file.icf"));
    }

    // ── Round-trip: load → save → reload → compare ──────────────────────────
    void roundTrip_smokeIcf() {
        QtDynamoEngine eng1(false);
        QVERIFY(eng1.loadConfig(QString(IOMETER_SMOKE_ICF)));

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_test_XXXXXX.icf");
        QVERIFY(tmp.open());
        const QString tmpPath = tmp.fileName();
        tmp.close();

        QVERIFY(eng1.saveConfig(tmpPath));

        QtDynamoEngine eng2(false);
        QVERIFY(eng2.loadConfig(tmpPath));

        // Key fields must survive the round-trip
        QCOMPARE(eng2.testConfig().runSeconds, eng1.testConfig().runSeconds);
        QCOMPARE(eng2.testConfig().runMinutes, eng1.testConfig().runMinutes);
        QCOMPARE(eng2.testConfig().runHours,   eng1.testConfig().runHours);
        QCOMPARE(eng2.testConfig().rampSeconds, eng1.testConfig().rampSeconds);
        QCOMPARE(eng2.accessSpecs().size(),    eng1.accessSpecs().size());
        QCOMPARE(eng2.batchAssignedSpec(),     eng1.batchAssignedSpec());
    }

    void roundTrip_specNames() {
        QtDynamoEngine eng1(false);
        eng1.loadConfig(QString(IOMETER_SMOKE_ICF));

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_test_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        eng1.saveConfig(tmp.fileName());
        QtDynamoEngine eng2(false);
        eng2.loadConfig(tmp.fileName());

        const auto s1 = eng1.accessSpecs();
        const auto s2 = eng2.accessSpecs();
        QCOMPARE(s1.size(), s2.size());
        for (int i = 0; i < s1.size(); ++i)
            QCOMPARE(s2[i].name, s1[i].name);
    }

    void roundTrip_specLines() {
        QtDynamoEngine eng1(false);
        eng1.loadConfig(QString(IOMETER_SMOKE_ICF));

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/iometer_test_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        eng1.saveConfig(tmp.fileName());
        QtDynamoEngine eng2(false);
        eng2.loadConfig(tmp.fileName());

        // Spot-check: the Default spec (first after Idle) should have 1 line
        // with the correct size
        const auto s1 = eng1.accessSpecs();
        const auto s2 = eng2.accessSpecs();
        QVERIFY(s1.size() > 1 && s2.size() > 1);
        // Find the Default spec in both
        for (int i = 0; i < s1.size() && i < s2.size(); ++i) {
            if (s1[i].name == "Default") {
                QCOMPARE(s2[i].lines.size(), s1[i].lines.size());
                QCOMPARE(s2[i].lines[0].sizeBytes, s1[i].lines[0].sizeBytes);
                break;
            }
        }
    }
};

QTEST_GUILESS_MAIN(IcfTest)
#include "tst_icf.moc"
