// tst_qtbarchartwidget.cpp - QtBarChartWidget (one worker/metric performance bar,
// the Qt equivalent of MFC's CBargraph). Drives the public API + paint/interaction
// so the value formatting, auto-range, metric combo, and double-click are covered.
#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QComboBox>
#include <QPixmap>
#include "QtBarChartWidget.h"

class BarChartTest : public QObject
{
    Q_OBJECT

    static WorkerResult iopsResult(const char *mgr, const char *wkr, double iops) {
        WorkerResult r;
        r.managerName = mgr;
        r.workerName  = wkr;
        r.iops        = iops;   // tests use RESULT_IOPS unless noted
        return r;
    }

private slots:
    void construct_defaults() {
        QtBarChartWidget w;
        QCOMPARE(w.resultType(), int(RESULT_IOPS));
        QCOMPARE(w.currentValue(), 0.0);
        QVERIFY(w.sizeHint().width() > 0);
    }

    void setWorker_namesAndUnassignedLabel() {
        QtBarChartWidget w;
        w.setWorker("M1", "Worker A");
        QCOMPARE(w.managerName(), QString("M1"));
        QCOMPARE(w.workerName(), QString("Worker A"));
        w.setWorker("M1", "");                    // empty -> "(unassigned)" branch
        QCOMPARE(w.workerName(), QString());
    }

    void setResultType_validAndOutOfRangeIgnored() {
        QtBarChartWidget w;
        w.setResultType(RESULT_MBPS_DEC);
        QCOMPARE(w.resultType(), int(RESULT_MBPS_DEC));
        w.setResultType(-1);                      // ignored
        w.setResultType(NUM_RESULT_TYPES);        // ignored
        QCOMPARE(w.resultType(), int(RESULT_MBPS_DEC));
    }

    void updateResults_matchingUpdatesValue() {
        QtBarChartWidget w;
        w.setWorker("M1", "W1");
        w.updateResults({ iopsResult("M1", "W1", 1234.0) });
        QCOMPARE(w.currentValue(), 1234.0);
    }

    void updateResults_nonMatchingIgnored() {
        QtBarChartWidget w;
        w.setWorker("M1", "W1");
        w.updateResults({ iopsResult("M1", "Other", 50.0) });
        QCOMPARE(w.currentValue(), 0.0);
    }

    void updateResults_emptyManagerMatchesAnyManager() {
        QtBarChartWidget w;
        w.setWorker("", "W1");                    // empty manager -> matches any
        w.updateResults({ iopsResult("AnyMgr", "W1", 7.0) });
        QCOMPARE(w.currentValue(), 7.0);
    }

    void updateResults_valueFormatBranches() {
        QtBarChartWidget w;
        w.setWorker("M", "W");
        w.updateResults({ iopsResult("M", "W", 2.5e6) });   // ">= 1e6" -> "M" + big auto-range
        QCOMPARE(w.currentValue(), 2.5e6);
        w.updateResults({ iopsResult("M", "W", 5000.0) });  // ">= 1e3" -> "K"
        w.updateResults({ iopsResult("M", "W", 42.0) });    // plain, >= 10 (0 decimals)
        w.updateResults({ iopsResult("M", "W", 3.0) });     // plain, < 10 (2 decimals)
        QCOMPARE(w.currentValue(), 3.0);
    }

    void metricCombo_changesTypeAndEmits() {
        // The metric combo carries 13 display entries; onMetricChanged stores the
        // selected combo index as the result type, so use a valid combo index.
        QtBarChartWidget w;
        QSignalSpy spy(&w, &QtBarChartWidget::resultTypeChanged);
        auto *combo = w.findChild<QComboBox *>();
        QVERIFY(combo);
        const int idx = 5;
        combo->setCurrentIndex(idx);              // -> onMetricChanged
        QCOMPARE(w.resultType(), idx);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), idx);
    }

    void paint_rendersWithDataAndPeak() {
        QtBarChartWidget w;
        w.setWorker("M", "W");
        w.resize(220, 160);                       // resizeEvent computes bar geometry
        w.updateResults({ iopsResult("M", "W", 800.0) });   // value > 0 -> fill + peak marker
        const QPixmap pm = w.grab();              // forces paintEvent
        QVERIFY(!pm.isNull());
    }

    void doubleClick_requestsBigMeter() {
        QtBarChartWidget w;
        w.resize(200, 150);
        QSignalSpy spy(&w, &QtBarChartWidget::bigMeterRequested);
        QTest::mouseDClick(&w, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(BarChartTest)
#include "tst_qtbarchartwidget.moc"
