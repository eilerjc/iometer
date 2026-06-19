// tst_pagesetup.cpp — Tests for QtPageSetup widget
// Verifies target list, worker params, and state management.
#include <QObject>
#include <QTest>
#include <QListWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include "QtPageSetup.h"
#include "QtDemoEngine.h"

class PageSetupTest : public QObject
{
    Q_OBJECT

    static WorkerInfo worker0(QtDemoEngine &e) { return e.managers()[0].workers[0]; }
private slots:

    // Regression: editing a disk target / worker param calls updateWorker, which
    // emits configChanged; without the m_applyingEdit guard the page rebuilt the
    // target list out from under the itemChanged signal -> use-after-free crash.
    void editTargetAndParams_noReentrancyCrash() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto mgrs = engine.managers();
        page.setSelectedWorker(QString::fromStdString(mgrs[0].name),
                               QString::fromStdString(mgrs[0].workers[0].id));

        auto *list = page.findChild<QListWidget *>();
        QVERIFY(list);
        QVERIFY(list->count() > 0);
        auto *item = list->item(0);                       // toggle a disk target
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked
                                                              : Qt::Checked);
        for (auto *sb : page.findChildren<QSpinBox *>())  // edit the worker params
            sb->setValue(sb->value() + 1);
        QVERIFY(true);                                    // survived (no crash)
    }

    // Toggle the Use-Fixed-Seed / Test-Connection-Rate checkboxes and the Write IO
    // Data Pattern combo; each saves back to the worker (covers onFixedSeedToggled,
    // onConnRateToggled, onDataPatternChanged and their enable branches).
    void toggleSeedConnRateAndDataPattern_savesToWorker() {
        QtDemoEngine engine; QtPageSetup page(&engine);
        const auto mgrs = engine.managers();
        page.setSelectedWorker(QString::fromStdString(mgrs[0].name),
                               QString::fromStdString(mgrs[0].workers[0].id));

        auto chks = page.findChildren<QCheckBox *>();      // [0]=Fixed Seed, [1]=Conn Rate
        QVERIFY(chks.size() >= 2);
        chks[0]->setChecked(true);                         // onFixedSeedToggled
        chks[1]->setChecked(true);                         // onConnRateToggled
        QVERIFY(worker0(engine).useFixedSeed);
        QVERIFY(worker0(engine).testConnRate);

        auto *combo = page.findChild<QComboBox *>();        // Write IO Data Pattern
        QVERIFY(combo);
        const int dp = (combo->currentIndex() + 1) % combo->count();
        combo->setCurrentIndex(dp);                        // onDataPatternChanged
        QCOMPARE(worker0(engine).dataPattern, dp);

        chks[0]->setChecked(false);                        // un-toggle (other branch)
        chks[1]->setChecked(false);
        QVERIFY(!worker0(engine).useFixedSeed);
        QVERIFY(!worker0(engine).testConnRate);
    }

    void construct_withDemoEngine() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        QVERIFY(!page.isVisible());
    }

    void clearSelection() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        page.clearSelection();
        QVERIFY(true);
    }

    void setSelectedManager_valid() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        page.setSelectedManager(QString::fromStdString(managers[0].name));
        QVERIFY(true);
    }

    void setSelectedManager_invalid() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        page.setSelectedManager("NonExistent");
        QVERIFY(true);
    }

    void setSelectedManager_empty() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        page.setSelectedManager("");
        QVERIFY(true);
    }

    void setSelectedWorker_valid() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        const auto &mgr = managers[0];
        QVERIFY(!mgr.workers.empty());
        page.setSelectedWorker(QString::fromStdString(mgr.name), QString::fromStdString(mgr.workers[0].id));
        QVERIFY(true);
    }

    void setSelectedWorker_invalidWorker() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        page.setSelectedWorker(QString::fromStdString(managers[0].name), "InvalidWorkerID");
        QVERIFY(true);
    }

    void setSelectedWorker_invalidManager() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        page.setSelectedWorker("InvalidMgr", "InvalidWorker");
        QVERIFY(true);
    }

    void refreshForEngine() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        page.refreshForEngine();
        QVERIFY(true);
    }

    void refreshForEngine_afterSelection() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto managers = engine.managers();
        if (!managers.isEmpty() && !managers[0].workers.empty()) {
            page.setSelectedWorker(QString::fromStdString(managers[0].name), QString::fromStdString(managers[0].workers[0].id));
            page.refreshForEngine();
            QVERIFY(true);
        }
    }

    void stateTransitions() {
        QtDemoEngine engine;
        QtPageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());

        page.setSelectedManager(QString::fromStdString(managers[0].name));
        if (!managers[0].workers.empty()) {
            page.setSelectedWorker(QString::fromStdString(managers[0].name), QString::fromStdString(managers[0].workers[0].id));
        }
        page.refreshForEngine();
        page.clearSelection();

        QVERIFY(true);
    }
};

QTEST_MAIN(PageSetupTest)
#include "tst_pagesetup.moc"
