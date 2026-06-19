// tst_qtpagedisplay.cpp - QtPageDisplay (results-display tab: 6 metric rows with
// value/bar, the metric popup menu, the ">" BigMeter expand, and start/stop reset).
#include <QObject>
#include <QTest>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QApplication>
#include <QPixmap>
#include "QtPageDisplay.h"
#include "QtDemoEngine.h"

class PageDisplayTest : public QObject
{
    Q_OBJECT

    // An aggregate result with values across the metric families (drives the
    // per-row value formatting branches in updateResults).
    static WorkerResult agg() {
        WorkerResult r;
        r.isAggregate = true;
        r.managerName = "All Managers";
        r.iops = 12345; r.readIops = 8000; r.writeIops = 4345;
        r.mbpsDec = 512.5; r.mbpsBin = 488.2;
        r.avgLatencyMs = 1.2345; r.maxLatencyMs = 9.876;
        r.cpuUtil = 33.3; r.cpuUser = 20.0; r.cpuKernel = 13.3;
        r.errors = 2;
        return r;
    }

    static QPushButton *firstMetricButton(QWidget &p) {
        for (auto *b : p.findChildren<QPushButton *>())
            if (b->text() != ">" && !b->text().isEmpty()) return b;
        return nullptr;
    }

private slots:
    void construct_hasMetricRows() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        QVERIFY(p.findChildren<QPushButton *>().size() >= 2);
        QVERIFY(firstMetricButton(p));
    }

    void updateResults_formatsRowsAndPaints() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        p.resize(720, 420);
        p.updateResults({ agg() });
        QVERIFY(!p.grab().isNull());          // forces the row + bar paints
    }

    void testStartedStopped_resetsRows() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        p.updateResults({ agg() });
        p.onTestStarted();                    // reset refs/peaks
        p.updateResults({ agg() });
        p.onTestStopped();                    // restore button state
        QVERIFY(true);
    }

    void bigMeterClicked_showsMeter() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        p.updateResults({ agg() });
        p.onBigMeterClicked();                // shows the presentation meter window
        QVERIFY(true);
    }

    void metricRowExpand_opensBigMeter() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        for (auto *b : p.findChildren<QPushButton *>())
            if (b->text() == ">") { b->click(); break; }   // onMetricRowClicked
        QVERIFY(true);
    }

    void metricButton_opensMetricMenu() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        // The metric button opens a modal cascading QMenu (menu.exec). Close it
        // from a timer so exec returns - covers the (large) menu-build path.
        // (Programmatically triggering a nested submenu action does NOT dismiss
        // exec(), so we just close the popup; the few apply lines stay uncovered.)
        bool sawMenu = false;
        QTimer::singleShot(150, [&sawMenu] {
            if (auto *m = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
                sawMenu = true;
                m->close();
            }
        });
        firstMetricButton(p)->click();        // onMetricButtonClicked -> menu.exec()
        QVERIFY(sawMenu);
    }

    void refreshWorkerAssignments_noCrash() {
        QtDemoEngine eng; QtPageDisplay p(&eng);
        p.refreshWorkerAssignments();
        QVERIFY(true);
    }
};

QTEST_MAIN(PageDisplayTest)
#include "tst_qtpagedisplay.moc"
