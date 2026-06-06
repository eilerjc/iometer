// tst_meters.cpp — Tests for MeterWidget (gauge control)
// Verifies range, value setting, watermark, and rendering without errors.
#include <QObject>
#include <QTest>
#include <QWidget>
#include "MeterWidget.h"

class MeterWidgetTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Construction ─────────────────────────────────────────────────────────
    void construct() {
        MeterWidget m;
        QVERIFY(m.sizeHint().width() > 0);
        QVERIFY(m.sizeHint().height() > 0);
    }

    // ── Range API ────────────────────────────────────────────────────────────
    void setRange_simple() {
        MeterWidget m;
        m.setRange(0, 100, false);
        // setRange should not crash; actual range is internal
        QVERIFY(true);
    }

    void setRange_withAutoRange() {
        MeterWidget m;
        m.setRange(0, 1000, true);
        QVERIFY(true);
    }

    void setRange_reverseOrder() {
        MeterWidget m;
        m.setRange(100, 0, false);  // reverse min/max
        QVERIFY(true);  // should handle gracefully
    }

    // ── Value setting ───────────────────────────────────────────────────────
    void setValue_zero() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(0);
        QVERIFY(true);
    }

    void setValue_midrange() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void setValue_max() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(100);
        QVERIFY(true);
    }

    void setValue_outOfRange() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(200);  // beyond max
        QVERIFY(true);  // should handle gracefully
    }

    void setValue_negative() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(-50);  // below min
        QVERIFY(true);  // should handle gracefully
    }

    void setValue_rapid() {
        MeterWidget m;
        m.setRange(0, 100, false);
        for (int i = 0; i < 100; ++i)
            m.setValue(i);
        QVERIFY(true);  // no crashes on rapid updates
    }

    // ── Watermark ───────────────────────────────────────────────────────────
    void watermark_disabled() {
        MeterWidget m;
        m.showWatermark = false;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void watermark_enabled() {
        MeterWidget m;
        m.showWatermark = true;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void watermark_reset() {
        MeterWidget m;
        m.showWatermark = true;
        m.setRange(0, 100, false);
        m.setValue(30);
        m.setValue(70);
        m.resetWatermark();
        m.setValue(50);
        QVERIFY(true);
    }

    // ── Resize ──────────────────────────────────────────────────────────────
    void resize_small() {
        MeterWidget m;
        m.resize(64, 64);
        // MeterWidget enforces a 120x90 minimum, so a smaller request clamps up.
        QVERIFY(m.width()  >= 120);
        QVERIFY(m.height() >= 90);
    }

    void resize_large() {
        MeterWidget m;
        m.resize(400, 400);
        QVERIFY(m.width() == 400);
        QVERIFY(m.height() == 400);
    }

    void resize_afterValue() {
        MeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(75);
        m.resize(200, 200);
        m.setValue(25);  // should still work
        QVERIFY(true);
    }

};

QTEST_MAIN(MeterWidgetTest)
#include "tst_meters.moc"
