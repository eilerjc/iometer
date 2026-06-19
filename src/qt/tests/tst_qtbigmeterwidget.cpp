// tst_qtbigmeterwidget.cpp - QtBigMeterWidget (the presentation-meter window:
// big gauge + title/worker/value labels + Start/Stop/Next buttons, watermark
// toggle and max-range spinner). Drives the display API, button states, and the
// interactive controls so the slots and paint are covered.
#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QPixmap>
#include "QtBigMeterWidget.h"
#include "QtMeterWidget.h"

class BigMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void construct_hasMeter() {
        QtBigMeterWidget w;
        QVERIFY(w.meter() != nullptr);
    }

    void display_setsTitleWorkerValueAndPaints() {
        QtBigMeterWidget w;
        w.resize(420, 520);
        w.setTitle("My Test");
        w.setWorkerResult("Worker 1", "IOps");
        w.updateDisplay(1234.5, "1.23 K");      // explicit formatted text
        QVERIFY(!w.grab().isNull());
        w.updateDisplay(42.0);                  // default formatting branch
        QVERIFY(!w.grab().isNull());
        w.resetWatermark();
    }

    void buttonStates_doNotCrash() {
        QtBigMeterWidget w;
        w.setButtonState(true,  false, false);  // Start
        w.setButtonState(false, true,  true);   // Stop + Next
        w.setButtonState(false, false, false);  // idle
        QVERIFY(true);
    }

    void interactions_driveSlotsAndSignals() {
        QtBigMeterWidget w;
        w.resize(420, 520);
        w.setButtonState(true, true, true);     // enable the buttons
        QSignalSpy startSpy(&w, &QtBigMeterWidget::startRequested);
        QSignalSpy stopSpy (&w, &QtBigMeterWidget::stopRequested);
        QSignalSpy nextSpy (&w, &QtBigMeterWidget::nextRequested);

        for (QPushButton *b : w.findChildren<QPushButton *>())
            if (b->isEnabled())
                QTest::mouseClick(b, Qt::LeftButton);   // onStartNextClicked / onStopClicked

        if (auto *chk = w.findChild<QCheckBox *>()) {   // onWatermarkToggled
            chk->setChecked(true);
            chk->setChecked(false);
        }
        if (auto *spin = w.findChild<QSpinBox *>())     // onMaxRangeChanged
            spin->setValue(spin->value() + 5000);

        QVERIFY(startSpy.count() + stopSpy.count() + nextSpy.count() >= 1);
    }
};

QTEST_MAIN(BigMeterTest)
#include "tst_qtbigmeterwidget.moc"
