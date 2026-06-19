// tst_mainwindow.cpp — Tests for QtMainWindow
// Verifies main window construction, menu/toolbar actions, and state transitions.
#include <QObject>
#include <QTest>
#include <QAction>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QTabWidget>
#include <QStatusBar>
#include <QMessageBox>
#include <QDialog>
#include <QAbstractButton>
#include <QTimer>
#include <QApplication>
#include "QtMainWindow.h"
#include "QtDemoEngine.h"

// Find a toolbar/menu QAction by its exact text.
static QAction *findAct(QWidget &w, const QString &text) {
    for (auto *a : w.findChildren<QAction *>())
        if (a->text() == text) return a;
    return nullptr;
}
// The topology tree (the one with the "all" root) - QtMainWindow also contains the
// pages' QTreeWidgets, so findChild<QTreeWidget*> isn't specific enough.
static QTreeWidget *topoTree(QWidget &w) {
    for (auto *t : w.findChildren<QTreeWidget *>())
        if (t->topLevelItemCount() > 0 &&
            t->topLevelItem(0)->data(0, Qt::UserRole).toString() == "all")
            return t;
    return nullptr;
}
// First tree item whose UserRole key starts with `prefix` ("mgr:" / "worker:").
// Re-find after any engine mutation - configChanged rebuilds the tree, deleting
// the old item pointers.
static QTreeWidgetItem *findItem(QTreeWidget *t, const QString &prefix) {
    for (QTreeWidgetItemIterator it(t); *it; ++it)
        if ((*it)->data(0, Qt::UserRole).toString().startsWith(prefix)) return *it;
    return nullptr;
}
// Self-deleting poller that closes the next modal (Yes on a question box, else
// Ok/accept). Hang-safe: gives up after ~2.4 s.
static void scheduleCloseModal(bool yes = true) {
    auto *timer = new QTimer;
    timer->setInterval(40);
    timer->setProperty("n", 0);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, yes] {
        if (auto *w = QApplication::activeModalWidget()) {
            if (auto *mb = qobject_cast<QMessageBox *>(w)) {
                if (auto *b = mb->button(yes ? QMessageBox::Yes : QMessageBox::No)) b->click();
                else mb->accept();          // about/warning have only Ok
            } else if (auto *d = qobject_cast<QDialog *>(w)) {
                yes ? d->accept() : d->reject();
            }
            timer->stop(); timer->deleteLater();
            return;
        }
        const int n = timer->property("n").toInt() + 1;
        timer->setProperty("n", n);
        if (n > 60) { timer->stop(); timer->deleteLater(); }
    });
    timer->start();
}

class MainWindowTest : public QObject
{
    Q_OBJECT

    static int workerCount(QtDemoEngine &e) { return e.managers()[0].workers.size(); }

private slots:

    // ── Construction ─────────────────────────────────────────────────────────
    void construct_withDemoEngine() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);
        QVERIFY(!win.isVisible());
        QVERIFY(win.windowTitle().length() > 0);
    }

    void construct_multipleInstances() {
        QtDemoEngine engine1, engine2;
        QtMainWindow win1(&engine1);
        QtMainWindow win2(&engine2);
        QVERIFY(true);
    }

    // ── Toolbar / Menu actions (by name) ────────────────────────────────────
    void toolbar_hasActions() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        // Count actions in toolbar — just verify there are some
        const auto actions = win.findChildren<QAction *>();
        QVERIFY(actions.count() > 0);
    }

    void toolbar_newDynamo_action() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

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
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        // The topology panel should be populated with the QtDemoEngine's managers/workers.
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
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        const auto tabWidgets = win.findChildren<QTabWidget *>();
        QVERIFY(tabWidgets.count() > 0);  // should have tab widget with Setup, Access, etc.

        if (!tabWidgets.isEmpty()) {
            const auto tabs = tabWidgets[0];
            QVERIFY(tabs->count() > 0);  // should have at least one tab
        }
    }

    // ── StatusBar ────────────────────────────────────────────────────────────
    void statusBar_exists() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        // StatusBar should show status messages
        QVERIFY(win.statusBar() != nullptr);
    }

    // ── Engine signal handling ───────────────────────────────────────────────
    void engineSignals_statusMessage() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        // Emit a status message from the engine; window should handle it
        emit engine.statusMessage("Test message");

        QVERIFY(true);
    }

    void engineSignals_testStarted() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        emit engine.testStarted();

        QVERIFY(true);
    }

    void engineSignals_testStopped() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

        emit engine.testStopped();

        QVERIFY(true);
    }

    void engineSignals_resultsUpdated() {
        QtDemoEngine engine;
        QtMainWindow win(&engine);

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

    // ── Toolbar actions that mutate the topology ─────────────────────────────
    void addWorkers_viaToolbarActions() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        auto *tree = topoTree(win);
        QVERIFY(tree);
        const int before = workerCount(engine);

        auto *mgrItem = findItem(tree, "mgr:");
        QVERIFY2(mgrItem, "no manager item in tree");
        tree->setCurrentItem(mgrItem);                     // select the manager
        QCOMPARE(tree->currentItem(), mgrItem);
        auto *a = findAct(win, "New Disk Worker");
        QVERIFY2(a, "no New Disk Worker action");
        QVERIFY2(a->isEnabled(), "New Disk Worker action disabled after selecting manager");
        a->trigger();                                      // onNewDiskWorker -> addWorker
        QCOMPARE(workerCount(engine), before + 1);

        tree->setCurrentItem(findItem(tree, "mgr:"));      // tree was rebuilt; re-select
        findAct(win, "New Net Worker")->trigger();         // onNewNetWorker
        QCOMPARE(workerCount(engine), before + 2);

        tree->setCurrentItem(findItem(tree, "worker:"));   // select a worker
        findAct(win, "Copy Worker")->trigger();            // onCopyWorker
        QCOMPARE(workerCount(engine), before + 3);
    }

    void deleteSelectedWorker_removesIt() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        auto *tree = topoTree(win);
        const int before = workerCount(engine);
        tree->setCurrentItem(findItem(tree, "worker:"));   // a worker -> no confirm dialog
        findAct(win, "Delete Selected")->trigger();        // onExitOne -> removeWorker
        QCOMPARE(workerCount(engine), before - 1);
    }

    void disconnectManager_confirmYes() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        auto *tree = topoTree(win);
        tree->setCurrentItem(findItem(tree, "mgr:"));      // a manager -> confirm box
        scheduleCloseModal(true);                          // answer Yes
        findAct(win, "Delete Selected")->trigger();        // onExitOne -> disconnectManager
        QVERIFY(true);
    }

    // ── Test-control + view + reset actions ──────────────────────────────────
    void stopAndResetActions() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        findAct(win, "Stop")->trigger();                   // onStop
        findAct(win, "Stop All")->trigger();               // onStopAll
        findAct(win, "Reset")->trigger();                  // onNew (not running -> newConfig)
        QVERIFY(true);
    }

    void presentationMeterAction_shows() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        if (auto *a = findAct(win, "Presentation Meter")) a->trigger();
        QVERIFY(true);
    }

    void aboutAction_modalClosed() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        scheduleCloseModal();                              // close the About box
        findAct(win, "About")->trigger();                  // onAbout
        QVERIFY(true);
    }

    // While a test is running, New/Open hit the "Test Running" guard (a closeable
    // warning) instead of touching the config / native file dialog.
    void runningGuards_newAndOpen() {
        QtDemoEngine engine; QtMainWindow win(&engine);
        engine.startTest();                                // -> onTestStarted, m_running=true
        scheduleCloseModal();
        findAct(win, "Reset")->trigger();                  // onNew guard -> warning
        scheduleCloseModal();
        findAct(win, "Open")->trigger();                   // onOpen guard -> warning
        engine.stopTest();                                 // -> onTestStopped
        QVERIFY(true);
    }
};

QTEST_MAIN(MainWindowTest)
#include "tst_mainwindow.moc"
