// main.cpp — Iometer Qt entry point
//
// GUI mode (default):
//   IometerQt.exe            → DynamoEngine, full GUI
//   IometerQt.exe --demo     → DemoEngine, simulated data, no Dynamo
//
// Batch mode (backwards compatible with original IOmeter.exe /c /r /t flags):
//   IometerQt.exe /c config.icf /r results.csv /t 60
//   IometerQt.exe --icf config.icf --results results.csv --timeout 60
//
//   Loads config, waits up to <timeout> seconds for Dynamo to connect,
//   runs the test for the duration specified in the ICF, writes results.csv
//   in the original format, then exits 0 on success or 1 on failure.
//
#include "MainWindow.h"
#include "DemoEngine.h"
#include "DynamoEngine.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QCommandLineParser>
#include <QTimer>
#include <QDebug>

// ---------------------------------------------------------------------------
// Pre-scan argv for /c /r /t (original-style flags) and translate them to
// --icf --results --timeout so QCommandLineParser can handle them uniformly.
// ---------------------------------------------------------------------------
static void translateOriginalFlags(int argc, char *argv[],
                                   QString &icf, QString &results, int &timeout)
{
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if ((a == "/c" || a == "--icf") && i + 1 < argc)
            icf = QString::fromLocal8Bit(argv[++i]);
        else if ((a == "/r" || a == "--results") && i + 1 < argc)
            results = QString::fromLocal8Bit(argv[++i]);
        else if ((a == "/t" || a == "--timeout") && i + 1 < argc)
            timeout = QString::fromLocal8Bit(argv[++i]).toInt();
    }
}

// ---------------------------------------------------------------------------
// Batch mode — no window, QCoreApplication only.
// Mirrors the original IOmeter.exe /c /r /t behaviour exactly.
// ---------------------------------------------------------------------------
static int batchMain(const QString &icfFile,
                     const QString &resultsFile,
                     int loginTimeoutSec)
{
    // QCoreApplication is already set up by the caller.
    DynamoEngine engine;

    if (!engine.loadConfig(icfFile)) {
        qCritical("Batch: cannot open ICF file: %s", qPrintable(icfFile));
        return 1;
    }

    const TestConfig cfg = engine.testConfig();
    const int runMs = ((cfg.runHours * 60 + cfg.runMinutes) * 60 + cfg.runSeconds) * 1000;
    if (runMs <= 0) {
        qCritical("Batch: ICF run time is 0 seconds — nothing to do.");
        return 1;
    }

    qInfo("Batch: ICF loaded. Run time: %dh %dm %ds. Login timeout: %ds.",
          cfg.runHours, cfg.runMinutes, cfg.runSeconds, loginTimeoutSec);
    qInfo("Batch: Waiting for Dynamo on port 1066...");

    // State machine driven by signals:
    //   Idle → Connected → Running → Done
    bool success = false;

    QObject::connect(
        &engine, &IometerEngine::managerConnected,
        [&](ManagerInfo) {
            qInfo("Batch: Dynamo connected. Configuring test...");

            // Configure workers to match the ICF exactly:
            //   - Workers 0..N-1  get per-worker targets+spec from ICF
            //   - Workers N..end  get empty targets (no I/O — matches original)
            const auto batchWorkers = engine.batchWorkers();
            qInfo("Batch: ICF defines %d worker(s); Dynamo reported %d.",
                  batchWorkers.size(),
                  engine.managers().isEmpty() ? 0 : engine.managers().first().workers.size());

            int wi = 0;
            for (const auto &mgr : engine.managers()) {
                for (auto w : mgr.workers) {
                    if (wi < batchWorkers.size()) {
                        w.targets = batchWorkers[wi].targets;
                        qInfo("Batch:   Worker %d ('%s'): targets=[%s] spec='%s'",
                              wi, qPrintable(w.name),
                              qPrintable(w.targets.join(", ")),
                              qPrintable(batchWorkers[wi].assignedSpecs.value(0)));
                    } else {
                        w.targets.clear();   // no targets → Dynamo skips this worker
                    }
                    engine.updateWorker(w);
                    ++wi;
                }
            }

            // Apply the first ICF worker's spec globally (all active workers share it)
            const QString specName = engine.batchAssignedSpec();
            if (!specName.isEmpty()) {
                for (const auto &s : engine.accessSpecs()) {
                    if (s.name == specName) {
                        engine.setCurrentTestSpec(s);
                        qInfo("Batch: Using spec '%s'", qPrintable(s.name));
                        break;
                    }
                }
            }

            qInfo("Batch: Starting test (%d ms)...", runMs);
            engine.startTest();
            QTimer::singleShot(runMs, &engine, &IometerEngine::stopTest);
        });

    QObject::connect(
        &engine, &IometerEngine::testStopped,
        [&]() {
            qInfo("Batch: Test complete. Writing results to %s", qPrintable(resultsFile));
            success = engine.saveBatchResults(resultsFile);
            if (!success)
                qCritical("Batch: Failed to write results file.");
            QCoreApplication::instance()->exit(success ? 0 : 1);
        });

    // Overall login timeout
    QTimer::singleShot(loginTimeoutSec * 1000, [&]() {
        if (!engine.isRunning() && engine.managers().isEmpty()) {
            qCritical("Batch: Dynamo did not connect within %d seconds.", loginTimeoutSec);
            QCoreApplication::instance()->exit(1);
        }
    });

    return QCoreApplication::instance()->exec();
}

// ---------------------------------------------------------------------------
// GUI mode
// ---------------------------------------------------------------------------
static int guiMain(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Iometer");
    app.setApplicationDisplayName("Iometer Qt GUI");
    app.setOrganizationName("Iometer Project");
    app.setOrganizationDomain("github.com/eilerjc/iometer");
    app.setApplicationVersion("1.1.0");

    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);

    QCommandLineParser parser;
    parser.setApplicationDescription("Iometer Qt GUI — I/O benchmark controller");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption demoOpt("demo",
        "Use simulated DemoEngine instead of connecting to Dynamo.exe");
    parser.addOption(demoOpt);
    parser.process(app);

    IometerEngine *engine = nullptr;
    if (parser.isSet(demoOpt)) {
        engine = new DemoEngine;
    } else {
        engine = new DynamoEngine;
    }

    MainWindow win(engine);
    win.show();

    const int ret = app.exec();
    delete engine;
    return ret;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QString icf, results;
    int timeout = 60;
    translateOriginalFlags(argc, argv, icf, results, timeout);

    if (!icf.isEmpty()) {
        // Batch mode: no GUI needed, QCoreApplication suffices
        QCoreApplication app(argc, argv);
        app.setApplicationName("Iometer");
        app.setApplicationVersion("1.1.0");
        return batchMain(icf, results, timeout);
    }

    return guiMain(argc, argv);
}
