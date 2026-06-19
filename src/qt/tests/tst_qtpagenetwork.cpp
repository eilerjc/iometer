// tst_qtpagenetwork.cpp - QtPageNetwork ("Network Targets" tab). Adds a Network
// worker to a QtDemoEngine, then drives manager/worker selection (tree + NIC combo
// population, param enable/disable), the parameter controls (which save back to the
// engine via updateWorker), and the target-NIC checkboxes.
#include <QObject>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include "QtPageNetwork.h"
#include "QtDemoEngine.h"

class PageNetworkTest : public QObject
{
    Q_OBJECT

    static const char *MGR;        // demo engine's manager name
    static const char *WID;        // our network worker id

    static void addNetWorker(QtDemoEngine &eng) {
        WorkerInfo w;
        w.id          = WID;
        w.name        = "Net Worker";
        w.type        = "Network";
        w.managerName = MGR;
        w.networkInterface    = "192.168.1.100";   // matches a demo NIC
        w.maxOutstandingSends = 4;
        w.testConnRate        = false;
        w.transPerConn        = 5;
        eng.addWorker(MGR, w);
    }
    static WorkerInfo netWorker(QtIometerEngine &eng) {
        for (const auto &m : eng.managers())
            for (const auto &w : m.workers)
                if (w.id == WID) return w;
        return {};
    }
    // The two spinboxes are distinguishable by range (max sends 256, trans 100000).
    static void spinboxes(QWidget &p, QSpinBox *&maxSends, QSpinBox *&trans) {
        maxSends = trans = nullptr;
        for (auto *sb : p.findChildren<QSpinBox *>())
            (sb->maximum() == 256 ? maxSends : trans) = sb;
    }

private slots:
    void selectManager_populatesTreeAndNicCombo() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedManager(MGR);
        QVERIFY(p.findChild<QTreeWidget *>()->topLevelItemCount() >= 1);   // manager node
        QCOMPARE(p.findChild<QComboBox *>()->count(), 2);                  // the two demo NICs
        // No worker selected -> params disabled.
        QSpinBox *maxSends, *trans; spinboxes(p, maxSends, trans);
        QVERIFY(!maxSends->isEnabled());
    }

    void selectWorker_loadsParamsAndEnables() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedWorker(MGR, WID);
        QSpinBox *maxSends, *trans; spinboxes(p, maxSends, trans);
        QVERIFY(maxSends->isEnabled());
        QCOMPARE(maxSends->value(), 4);                                   // loaded from worker
        QCOMPARE(p.findChild<QComboBox *>()->currentText(), QString("192.168.1.100"));
        QVERIFY(!p.findChild<QCheckBox *>()->isChecked());
    }

    void editParams_saveBackToWorker() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedWorker(MGR, WID);
        QSpinBox *maxSends, *trans; spinboxes(p, maxSends, trans);

        maxSends->setValue(16);                          // onMaxSendsChanged -> updateWorker
        QCOMPARE(netWorker(eng).maxOutstandingSends, 16);

        p.findChild<QCheckBox *>()->setChecked(true);    // onTestConnRateToggled
        QVERIFY(trans->isEnabled());
        QVERIFY(netWorker(eng).testConnRate);

        trans->setValue(42);                             // onTransPerConnChanged
        QCOMPARE(netWorker(eng).transPerConn, 42);

        p.findChild<QComboBox *>()->setCurrentIndex(1);  // onNicComboChanged
        QCOMPARE(netWorker(eng).networkInterface, std::string("10.0.0.1"));
    }

    void checkTargetNic_assignsTargetToWorker() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedWorker(MGR, WID);
        auto *tree = p.findChild<QTreeWidget *>();
        QTreeWidgetItem *nic = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it)
            if ((*it)->flags() & Qt::ItemIsUserCheckable) { nic = *it; break; }
        QVERIFY(nic);
        nic->setCheckState(0, Qt::Checked);              // onTargetItemChanged -> updateWorker
        QVERIFY(!netWorker(eng).targets.empty());
        nic->setCheckState(0, Qt::Unchecked);
        QVERIFY(netWorker(eng).targets.empty());
    }

    void clearSelection_disablesParams() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedWorker(MGR, WID);
        p.clearSelection();
        QSpinBox *maxSends, *trans; spinboxes(p, maxSends, trans);
        QVERIFY(!maxSends->isEnabled());
        QCOMPARE(p.findChild<QTreeWidget *>()->topLevelItemCount(), 0);
    }

    void managerConnectDisconnect_refreshNoCrash() {
        QtDemoEngine eng; addNetWorker(eng);
        QtPageNetwork p(&eng);
        p.setSelectedWorker(MGR, WID);
        ManagerInfo m; m.name = MGR; m.connected = true;
        p.onManagerConnected(m);                         // -> refreshForEngine
        p.onManagerDisconnected(MGR);
        QVERIFY(true);
    }
};

const char *PageNetworkTest::MGR = "Local System";
const char *PageNetworkTest::WID = "net-w0";

QTEST_MAIN(PageNetworkTest)
#include "tst_qtpagenetwork.moc"
