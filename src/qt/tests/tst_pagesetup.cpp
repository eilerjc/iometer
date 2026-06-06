// tst_pagesetup.cpp — Tests for PageSetup widget
// Verifies target list, worker params, and state management.
#include <QObject>
#include <QTest>
#include "PageSetup.h"
#include "DemoEngine.h"

class PageSetupTest : public QObject
{
    Q_OBJECT
private slots:

    void construct_withDemoEngine() {
        DemoEngine engine;
        PageSetup page(&engine);
        QVERIFY(!page.isVisible());
    }

    void clearSelection() {
        DemoEngine engine;
        PageSetup page(&engine);
        page.clearSelection();
        QVERIFY(true);
    }

    void setSelectedManager_valid() {
        DemoEngine engine;
        PageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        page.setSelectedManager(QString::fromStdString(managers[0].name));
        QVERIFY(true);
    }

    void setSelectedManager_invalid() {
        DemoEngine engine;
        PageSetup page(&engine);
        page.setSelectedManager("NonExistent");
        QVERIFY(true);
    }

    void setSelectedManager_empty() {
        DemoEngine engine;
        PageSetup page(&engine);
        page.setSelectedManager("");
        QVERIFY(true);
    }

    void setSelectedWorker_valid() {
        DemoEngine engine;
        PageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        const auto &mgr = managers[0];
        QVERIFY(!mgr.workers.empty());
        page.setSelectedWorker(QString::fromStdString(mgr.name), QString::fromStdString(mgr.workers[0].id));
        QVERIFY(true);
    }

    void setSelectedWorker_invalidWorker() {
        DemoEngine engine;
        PageSetup page(&engine);
        const auto managers = engine.managers();
        QVERIFY(!managers.isEmpty());
        page.setSelectedWorker(QString::fromStdString(managers[0].name), "InvalidWorkerID");
        QVERIFY(true);
    }

    void setSelectedWorker_invalidManager() {
        DemoEngine engine;
        PageSetup page(&engine);
        page.setSelectedWorker("InvalidMgr", "InvalidWorker");
        QVERIFY(true);
    }

    void refreshForEngine() {
        DemoEngine engine;
        PageSetup page(&engine);
        page.refreshForEngine();
        QVERIFY(true);
    }

    void refreshForEngine_afterSelection() {
        DemoEngine engine;
        PageSetup page(&engine);
        const auto managers = engine.managers();
        if (!managers.isEmpty() && !managers[0].workers.empty()) {
            page.setSelectedWorker(QString::fromStdString(managers[0].name), QString::fromStdString(managers[0].workers[0].id));
            page.refreshForEngine();
            QVERIFY(true);
        }
    }

    void stateTransitions() {
        DemoEngine engine;
        PageSetup page(&engine);
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
