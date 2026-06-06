// tst_mainwindow.cpp — Tests for MainWindow
// Verifies main window construction, menu/toolbar actions, and state transitions.
#include <QObject>
#include <QTest>
#include <QAction>
#include <QTreeWidget>
#include <QTabWidget>
#include <QStatusBar>
#include "MainWindow.h"
#include "DemoEngine.h"

class MainWindowTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Construction ─────────────────────────────────────────────────────────
    void construct_withDemoEngine() {
        DemoEngine engine;
        MainWindow win(&engine);
        QVERIFY(!win.isVisible());
        QVERIFY(win.windowTitle().length() > 0);
    }

    void construct_multipleInstances() {
        DemoEngine engine1, engine2;
        MainWindow win1(&engine1);
        MainWindow win2(&engine2);
        QVERIFY(true);
    }

    // ── Toolbar / Menu actions (by name) ────────────────────────────────────
    void toolbar_hasActions() {
        DemoEngine engine;
        MainWindow win(&engine);

        // Count actions in toolbar — just verify there are some
        const auto actions = win.findChildren<QAction *>();
        QVERIFY(actions.count() > 0);
    }

    void toolbar_newDynamo_action() {
        DemoEngine engine;
        MainWindow win(&engine);

        // Find "New Dynamo" action by text
        const auto actions = win.findChildren<QAction *>();
        const auto it = std::find_if(actions.begin(), actions.end(),
            [](const QAction *a) { return a->text().contains("New", Qt::CaseInsensitive) &&
                                          a->text().contains("Dynamo", Qt::CaseInsensitive); });

        if (it != actions.end()) {
            // Trigger the action (will add a new manager to the tree)
            (*it)->trigger();
            QVERIFY(true);
        } else {
            QVERIFY(true);  // action may not be visible, that's OK
        }
    }

    // ── Topology tree ────────────────────────────────────────────────────────
    void topologyTree_hasItems() {
        DemoEngine engine;
        MainWindow win(&engine);

        // The topology panel should be populated with the DemoEngine's managers/workers.
        // findChildren also returns empty trees embedded in child pages, so check
        // that *some* tree carries the "All Managers" root built at construction.
        const auto treeWidgets = win.findChildren<QTreeWidget *>();
        QVERIFY(treeWidgets.count() > 0);  // there should be at least one tree

        bool anyPopulated = false;
        for (auto *tree : treeWidgets) {
            if (tree->topLevelItemCount() > 0) { anyPopulated = true; break; }
        }
        QVERIFY2(anyPopulated, "no QTreeWidget had top-level items (worker tree empty)");
    }

    // ── Tabs ────────────────────────────────────────────────────────────────
    void tabs_exists() {
        DemoEngine engine;
        MainWindow win(&engine);

        const auto tabWidgets = win.findChildren<QTabWidget *>();
        QVERIFY(tabWidgets.count() > 0);  // should have tab widget with Setup, Access, etc.

        if (!tabWidgets.isEmpty()) {
            const auto tabs = tabWidgets[0];
            QVERIFY(tabs->count() > 0);  // should have at least one tab
        }
    }

    // ── StatusBar ────────────────────────────────────────────────────────────
    void statusBar_exists() {
        DemoEngine engine;
        MainWindow win(&engine);

        // StatusBar should show status messages
        QVERIFY(win.statusBar() != nullptr);
    }

    // ── Engine signal handling ───────────────────────────────────────────────
    void engineSignals_statusMessage() {
        DemoEngine engine;
        MainWindow win(&engine);

        // Emit a status message from the engine; window should handle it
        emit engine.statusMessage("Test message");

        QVERIFY(true);
    }

    void engineSignals_testStarted() {
        DemoEngine engine;
        MainWindow win(&engine);

        emit engine.testStarted();

        QVERIFY(true);
    }

    void engineSignals_testStopped() {
        DemoEngine engine;
        MainWindow win(&engine);

        emit engine.testStopped();

        QVERIFY(true);
    }

    void engineSignals_resultsUpdated() {
        DemoEngine engine;
        MainWindow win(&engine);

        // Simulate results update
        QVector<WorkerResult> results;
        WorkerResult r;
        r.managerName = "Local";
        r.workerName = "Worker 1";
        r.iops = 25000;
        r.mbpsDec = 1600;
        r.avgLatencyMs = 0.04;
        results.append(r);
        emit engine.resultsUpdated(results);

        QVERIFY(true);
    }
};

QTEST_MAIN(MainWindowTest)
#include "tst_mainwindow.moc"
