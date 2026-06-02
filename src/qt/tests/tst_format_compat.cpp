// tst_format_compat.cpp — Format compatibility tests
// Verifies that Qt implementation matches ICF format spec and handles all
// valid format variations that the original MFC implementation supports.
// This ensures both implementations can read each other's output.
#include <QObject>
#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include "DynamoEngine.h"
#include "IometerTypes.h"

class FormatCompatTest : public QObject
{
    Q_OBJECT
private slots:

    // ── ICF Format Version ───────────────────────────────────────────────
    void icf_version_string() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);
        QString firstLine = in.readLine();

        // Must start with "Version 1.1.0"
        QCOMPARE(firstLine, QString("Version 1.1.0"));
    }

    void icf_version_at_end() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);
        QString lastLine;
        while (!in.atEnd()) lastLine = in.readLine();

        // Must end with "Version 1.1.0"
        QCOMPARE(lastLine, QString("Version 1.1.0"));
    }

    // ── ICF Section Headers ──────────────────────────────────────────────
    void icf_required_sections() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());

        // Must have all required sections
        QVERIFY(content.contains("'TEST SETUP"));
        QVERIFY(content.contains("'RESULTS DISPLAY"));
        QVERIFY(content.contains("'ACCESS SPECIFICATIONS"));
        QVERIFY(content.contains("'MANAGER LIST"));
    }

    void icf_section_endings() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());

        // Must have corresponding END markers
        QVERIFY(content.contains("'END test setup"));
        QVERIFY(content.contains("'END results display"));
        QVERIFY(content.contains("'END access specifications"));
        QVERIFY(content.contains("'END manager list"));
    }

    // ── Run Time / Ramp Fields ───────────────────────────────────────────
    void icf_runtime_format() {
        DemoEngine engine;
        TestConfig cfg;
        cfg.runHours = 2;
        cfg.runMinutes = 30;
        cfg.runSeconds = 45;
        cfg.rampSeconds = 10;
        engine.setTestConfig(cfg);

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        // Load it back
        DemoEngine engine2;
        QVERIFY(engine2.loadConfig(tmp.fileName()));

        const auto cfg2 = engine2.testConfig();
        QCOMPARE(cfg2.runHours, 2);
        QCOMPARE(cfg2.runMinutes, 30);
        QCOMPARE(cfg2.runSeconds, 45);
        QCOMPARE(cfg2.rampSeconds, 10);
    }

    // ── Comment Format (lines starting with ') ───────────────────────────
    void icf_comment_preservation() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);

        // Count lines that start with '
        int commentCount = 0;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith("'")) commentCount++;
        }

        // Should have many comment lines (headers, field descriptions)
        QVERIFY(commentCount > 10);
    }

    // ── CSV Output Format (Results) ──────────────────────────────────────
    void csv_header_row() {
        QVector<WorkerResult> results;
        WorkerResult r;
        r.managerName = "MGR";
        r.workerName = "W1";
        r.iops = 1000;
        r.mbpsDec = 100;
        r.avgLatencyMs = 0.1;
        r.errors = 0;
        results.append(r);

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.csv");
        QVERIFY(tmp.open());
        tmp.close();

        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), results, TestConfig{});

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);

        // Find the header line
        QString headerLine;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.contains("Target Type") && line.contains("IOps")) {
                headerLine = line;
                break;
            }
        }

        QVERIFY(!headerLine.isEmpty());
        // Header must have standard columns
        QVERIFY(headerLine.contains("Target Type"));
        QVERIFY(headerLine.contains("Target Name"));
        QVERIFY(headerLine.contains("IOps"));
        QVERIFY(headerLine.contains("MBps (Decimal)"));
        QVERIFY(headerLine.contains("Errors"));
    }

    void csv_data_row_count() {
        QVector<WorkerResult> results;
        for (int i = 0; i < 5; ++i) {
            WorkerResult r;
            r.managerName = QString("MGR%1").arg(i);
            r.workerName = QString("W%1").arg(i);
            r.iops = 1000 + i * 100;
            r.mbpsDec = 100;
            r.avgLatencyMs = 0.1;
            r.errors = 0;
            results.append(r);
        }

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.csv");
        QVERIFY(tmp.open());
        tmp.close();

        DynamoEngine::writeBatchResultsCsv(tmp.fileName(), results, TestConfig{});

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);

        // Skip to results section, count data rows
        int dataCount = 0;
        bool inResults = false;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.contains("'Results")) { inResults = true; continue; }
            if (!inResults || line.startsWith("'")) continue;
            if (!line.isEmpty()) dataCount++;
        }

        // Should have: 1 ALL aggregate + 5 worker rows = 6
        QCOMPARE(dataCount, 6);
    }

    // ── Access Spec Format ───────────────────────────────────────────────
    void icf_accessspec_line_format() {
        DemoEngine engine;
        TestConfig cfg;
        cfg.description = "Test with custom spec";
        engine.setTestConfig(cfg);

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());

        QFile f(tmp.fileName());
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());

        // Each access spec should have a name line and one or more data lines
        // Data line format: size,% of size,% reads,% random,delay,burst,align,reply
        QVERIFY(content.contains("'size,% of size,% reads,% random"));
    }

    // ── Blank Lines and Formatting ───────────────────────────────────────
    void icf_blank_line_handling() {
        DemoEngine engine;
        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.close();

        engine.saveConfig(tmp.fileName());
        DemoEngine engine2;
        QVERIFY(engine2.loadConfig(tmp.fileName()));

        // Should load successfully despite blank lines in various places
        QVERIFY(engine2.accessSpecs().size() > 0);
    }

    // ── Whitespace in Numeric Values ─────────────────────────────────────
    void icf_numeric_parsing_robust() {
        // Create an ICF with various whitespace patterns
        const char* icfContent = R"(Version 1.1.0
'TEST SETUP ====================================================================
'Test Description

'Run Time
'	hours      minutes    seconds
	  1    	    2    	    3
'Ramp Up Time (s)
	  5
'Default Disk Workers to Spawn
	1
'END test setup
'RESULTS DISPLAY ===============================================================
'END results display
'ACCESS SPECIFICATIONS =========================================================
'END access specifications
'MANAGER LIST ==================================================================
'END manager list
Version 1.1.0
)";

        QTemporaryFile tmp;
        tmp.setFileTemplate(QDir::tempPath() + "/compat_XXXXXX.icf");
        QVERIFY(tmp.open());
        tmp.write(icfContent);
        tmp.close();

        DemoEngine eng;
        QVERIFY(eng.loadConfig(tmp.fileName()));

        const auto cfg = eng.testConfig();
        QCOMPARE(cfg.runHours, 1);
        QCOMPARE(cfg.runMinutes, 2);
        QCOMPARE(cfg.runSeconds, 3);
        QCOMPARE(cfg.rampSeconds, 5);
    }
};

QTEST_GUILESS_MAIN(FormatCompatTest)
#include "tst_format_compat.moc"
