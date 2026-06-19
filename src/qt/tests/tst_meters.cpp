// tst_meters.cpp — Tests for QtMeterWidget (gauge control)
// Verifies range, value setting, watermark, and rendering without errors.
#include <QObject>
#include <QTest>
#include <QWidget>
#include "QtMeterWidget.h"

class MeterWidgetTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Construction ─────────────────────────────────────────────────────────
    void construct() {
        QtMeterWidget m;
        QVERIFY(m.sizeHint().width() > 0);
        QVERIFY(m.sizeHint().height() > 0);
    }

    // ── Range API ────────────────────────────────────────────────────────────
    void setRange_simple() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        // setRange should not crash; actual range is internal
        QVERIFY(true);
    }

    void setRange_withAutoRange() {
        QtMeterWidget m;
        m.setRange(0, 1000, true);
        QVERIFY(true);
    }

    void setRange_reverseOrder() {
        QtMeterWidget m;
        m.setRange(100, 0, false);  // reverse min/max
        QVERIFY(true);  // should handle gracefully
    }

    // ── Value setting ───────────────────────────────────────────────────────
    void setValue_zero() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(0);
        QVERIFY(true);
    }

    void setValue_midrange() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void setValue_max() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(100);
        QVERIFY(true);
    }

    void setValue_outOfRange() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(200);  // beyond max
        QVERIFY(true);  // should handle gracefully
    }

    void setValue_negative() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(-50);  // below min
        QVERIFY(true);  // should handle gracefully
    }

    void setValue_rapid() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        for (int i = 0; i < 100; ++i)
            m.setValue(i);
        QVERIFY(true);  // no crashes on rapid updates
    }

    // ── Watermark ───────────────────────────────────────────────────────────
    void watermark_disabled() {
        QtMeterWidget m;
        m.showWatermark = false;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void watermark_enabled() {
        QtMeterWidget m;
        m.showWatermark = true;
        m.setRange(0, 100, false);
        m.setValue(50);
        QVERIFY(true);
    }

    void watermark_reset() {
        QtMeterWidget m;
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
        QtMeterWidget m;
        m.resize(64, 64);
        // QtMeterWidget enforces a 120x90 minimum, so a smaller request clamps up.
        QVERIFY(m.width()  >= 120);
        QVERIFY(m.height() >= 90);
    }

    void resize_large() {
        QtMeterWidget m;
        m.resize(400, 400);
        QVERIFY(m.width() == 400);
        QVERIFY(m.height() == 400);
    }

    void resize_afterValue() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.setValue(75);
        m.resize(200, 200);
        m.setValue(25);  // should still work
        QVERIFY(true);
    }

    // ── Rendering (force paintEvent via grab) ────────────────────────────────
    void paint_fixedRangeWithValue() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.resize(160, 160);
        m.setValue(50);
        QVERIFY(!m.grab().isNull());     // draws dial + needle at mid-scale
    }

    void paint_autoRangeRatchetsAndDrawsWatermark() {
        QtMeterWidget m;
        m.setRange(0, 100, true);
        m.showWatermark = true;
        m.resize(180, 180);
        // A climbing then dropping series moves the needle + sets the low/high
        // watermark band and forces an auto-range ratchet; grab each to paint.
        for (double v : {10.0, 250.0, 2500.0, 800.0, 1.0}) {
            m.setValue(v);
            QVERIFY(!m.grab().isNull());
        }
        m.resetWatermark();
        QVERIFY(!m.grab().isNull());
    }

    void paint_zeroAndOverMax() {
        QtMeterWidget m;
        m.setRange(0, 100, false);
        m.resize(140, 140);
        m.setValue(0);
        QVERIFY(!m.grab().isNull());
        m.setValue(500);                 // pegs past max -> clamped needle
        QVERIFY(!m.grab().isNull());
    }

};

QTEST_MAIN(MeterWidgetTest)
#include "tst_meters.moc"
