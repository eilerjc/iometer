// MeterWidget.h
// Qt port of CMeterCtrl — the Iometer speedometer gauge control.
//
// Drop-in replacement geometry: same NEEDLE_SWEEP, PIVOT_ARC_ANGLE, and
// CalculatePoint() math as the original MFC control, rewritten with QPainter.
//
// Usage:
//   MeterWidget *m = new MeterWidget(parent);
//   m->setRange(0, 100, /*autoRange=*/true);
//   m->setValue(42.0);
//   m->showWatermark = true;
#pragma once

#include <QWidget>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>

class MeterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeterWidget(QWidget *parent = nullptr);

    // ── API matching CMeterCtrl ───────────────────────────────────────────
    void setRange(int range1, int range2, bool autoRange = false);
    void setValue(double newValue);
    void resetWatermark();

    bool showWatermark = false;   // public, mirrors CMeterCtrl::show_watermark

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // ── Constants (match MeterCtrl.h) ────────────────────────────────────
    static constexpr int NEEDLE_SWEEP       = 270;                    // degrees
    static constexpr int PIVOT_ARC_ANGLE    = (360 - NEEDLE_SWEEP) / 2; // 45°
    static constexpr int NEEDLE_SENSITIVITY = 3;                      // degrees

    // ── Range / value ────────────────────────────────────────────────────
    int    m_minRange  = 0;
    int    m_maxRange  = 100;
    bool   m_autoRange = true;
    double m_value     = 0.0;

    // ── Watermark ────────────────────────────────────────────────────────
    double m_lowValue  = -1.0;   // < 0 means "not set yet"
    double m_highValue =  0.0;

    // ── Needle angles (meter-space: 0=min, NEEDLE_SWEEP=max) ─────────────
    int m_actualAngle = 0;
    int m_shownAngle  = 0;

    // ── Geometry (recalculated on resize) ─────────────────────────────────
    QPointF m_pivot;
    int     m_outerRadius  = 0;
    int     m_innerRadius  = 0;
    int     m_pivotRadius  = 0;
    int     m_labelRadius  = 0;
    int     m_tickRadius   = 0;
    int     m_labelBoxSize = 0;

    // ── Needle triangle (3 vertices) ─────────────────────────────────────
    QPointF m_needle[3];

    // ── Label / tick-mark cache ───────────────────────────────────────────
    struct LabelInfo {
        QRectF  box;
        QString text;
        QPointF tickInner;
        QPointF tickOuter;
    };
    std::vector<LabelInfo> m_labels;
    QString m_scaleText;          // e.g. "x1,000"

    // ── Private helpers ───────────────────────────────────────────────────
    void recalcGeometry();
    void updateScaleInfo();
    void updateLabelInfo();
    void setNeedlePoints();

    // Convert a meter-space angle (0…NEEDLE_SWEEP) to a screen point.
    // Exact translation of CMeterCtrl::CalculatePoint.
    QPointF calcPt(int angle, int radius) const;
};
