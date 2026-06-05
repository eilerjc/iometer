// MeterWidget.cpp
// Qt port of CMeterCtrl - Iometer speedometer gauge.
//
// Geometry notes
// --------------
// The sweep runs 270° from lower-left (value=0) clockwise through 12-o'clock
// (value=max/2) to lower-right (value=max).  The 90° "notch" at the bottom
// is where the scale text lives.
//
// CalculatePoint(θ, r):   real_angle = θ + 90 + PIVOT_ARC_ANGLE  (= θ + 135)
//   x = pivot.x + cos(real_angle°) · r
//   y = pivot.y + sin(real_angle°) · r     (y increases downward, same as MFC)
//
// Qt arc angles: 0° = east, positive = CCW, 1/16° units.
// Meter θ → Qt degrees: -(θ + 90 + PIVOT_ARC_ANGLE)
// CW sweep = negative spanAngle in Qt.

#include "MeterWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <cmath>

static constexpr double PI = 3.14159265358979323846;

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

MeterWidget::MeterWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);   // we fill the entire widget
    setMinimumSize(120, 90);
}

QSize MeterWidget::sizeHint() const
{
    return {320, 240};
}

// -----------------------------------------------------------------------------
// Geometry helpers
// -----------------------------------------------------------------------------

QPointF MeterWidget::calcPt(int angle, int radius) const
{
    // Matches CMeterCtrl::CalculatePoint exactly.
    const double a = (angle + 90 + PIVOT_ARC_ANGLE) * PI / 180.0;
    return { m_pivot.x() + std::cos(a) * radius,
             m_pivot.y() + std::sin(a) * radius };
}

void MeterWidget::recalcGeometry()
{
    const int w = width();
    const int h = height();
    if (w < 20 || h < 20) return;

    // Largest outer_radius that keeps the whole visible arc inside the widget.
    // The arc bottoms out at y = outer_radius * (1 + cos(PIVOT_ARC_ANGLE)).
    const double cosPA = std::cos(PIVOT_ARC_ANGLE * PI / 180.0);  // cos(45°) ≈ 0.7071
    m_outerRadius  = static_cast<int>(std::min(w / 2.0, h / (1.0 + cosPA)));
    m_innerRadius  = static_cast<int>(m_outerRadius * 0.70);
    m_pivotRadius  = std::max(3, m_innerRadius / 12);
    m_labelRadius  = static_cast<int>(m_outerRadius * 0.8275);
    m_tickRadius   = static_cast<int>(m_outerRadius * 0.96);
    m_labelBoxSize = std::max(6, static_cast<int>(m_outerRadius * 0.10));

    // Pivot at horizontal centre, one outer_radius down from the top.
    m_pivot = { w / 2.0, static_cast<double>(m_outerRadius) };

    updateScaleInfo();
    updateLabelInfo();
    setNeedlePoints();
}

// -----------------------------------------------------------------------------
// Scale text  (e.g. "x1,000")
// -----------------------------------------------------------------------------

void MeterWidget::updateScaleInfo()
{
    if (m_maxRange <= 10) { m_scaleText.clear(); return; }

    auto exp10 = static_cast<int>(std::floor(std::log10f(static_cast<float>(m_maxRange))));
    int  scale = static_cast<int>(std::pow(10.0f, static_cast<float>(exp10)));
    // If max_range is exactly a power of 10, show one decade smaller (matches MFC)
    if (m_maxRange == scale) scale /= 10;

    // Format with commas
    QString num = QString::number(scale);
    const int len = num.length();
    for (int p = 3; p < len; p += 4)
        num.insert(len - p, ',');

    m_scaleText = "x" + num;
}

// -----------------------------------------------------------------------------
// Label / tick-mark positions (matches CMeterCtrl::UpdateLabelInfo)
// -----------------------------------------------------------------------------

void MeterWidget::updateLabelInfo()
{
    m_labels.clear();
    if (m_outerRadius < 10 || m_maxRange <= m_minRange) return;

    const float rangeDiff    = static_cast<float>(m_maxRange - m_minRange);
    const float log10rd      = std::floor(std::log10f(rangeDiff));
    const float decade       = std::pow(10.0f, log10rd);

    float displayRange;
    if (rangeDiff == decade && rangeDiff != 1.0f)
        displayRange = 10.0f;
    else
        displayRange = rangeDiff / decade;

    float inc;
    if      (displayRange <= 1.0f)  inc = 0.1f;
    else if (displayRange <= 2.0f)  inc = 0.2f;
    else if (displayRange <= 5.0f)  inc = 0.5f;
    else                            inc = 1.0f;

    for (float v = 0.0f; v <= displayRange + 1e-5f; v += inc) {
        const int    labelAngle = static_cast<int>(v / displayRange * NEEDLE_SWEEP);
        const QPointF lp        = calcPt(labelAngle, m_labelRadius);

        LabelInfo li;
        li.box = QRectF(lp.x() - m_labelBoxSize, lp.y() - m_labelBoxSize,
                        m_labelBoxSize * 2.0,     m_labelBoxSize * 2.0);

        // Format: ".5" for 0 < v ≤ 0.9, "g" notation otherwise (matches MFC %.2g)
        if (v > 0.0f && v <= 0.9f)
            li.text = QString(".%1").arg(qRound(v * 10.0f));
        else
            li.text = QString::number(static_cast<double>(v), 'g', 2);

        li.tickInner = calcPt(labelAngle, m_tickRadius);
        li.tickOuter = calcPt(labelAngle, m_outerRadius);
        m_labels.push_back(std::move(li));
    }
}

// -----------------------------------------------------------------------------
// Needle geometry
// -----------------------------------------------------------------------------

void MeterWidget::setNeedlePoints()
{
    // Matches CMeterCtrl::SetNeedlePoints
    m_needle[0] = calcPt(m_actualAngle,             m_innerRadius);
    m_needle[1] = calcPt(180 + m_actualAngle - 20,  m_pivotRadius + 5);
    m_needle[2] = calcPt(180 + m_actualAngle + 20,  m_pivotRadius + 5);
    m_shownAngle = m_actualAngle;
}

// -----------------------------------------------------------------------------
// Qt events
// -----------------------------------------------------------------------------

void MeterWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    recalcGeometry();
    update();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void MeterWidget::setRange(int range1, int range2, bool autoRange)
{
    m_autoRange = autoRange;
    if (range1 == range2) return;

    m_minRange = std::min(range1, range2);
    m_maxRange = std::max(range1, range2);

    const double span = m_maxRange - m_minRange;
    if      (m_value < m_minRange) m_actualAngle = 0;
    else if (m_value > m_maxRange) m_actualAngle = NEEDLE_SWEEP;
    else                           m_actualAngle = static_cast<int>(m_value / span * NEEDLE_SWEEP);

    updateScaleInfo();
    updateLabelInfo();
    setNeedlePoints();
    update();
}

void MeterWidget::resetWatermark()
{
    m_lowValue  = -1.0;
    m_highValue =  0.0;
    update();
}

void MeterWidget::setValue(double newValue)
{
    if (newValue == m_value) return;

    bool wmChanged = false;
    if (newValue < m_lowValue || m_lowValue < 0.0) { m_lowValue  = newValue; wmChanged = true; }
    if (newValue > m_highValue)                     { m_highValue = newValue; wmChanged = true; }

    m_value = newValue;

    // Auto-ranging: ratchet up/down by decades (matches CMeterCtrl::SetValue)
    if (m_autoRange && m_value > 0.0) {
        const int tv = static_cast<int>(
            std::pow(10.0f, std::floor(std::log10f(static_cast<float>(m_value) * 10.0f))));
        if (tv > m_maxRange)
            setRange(m_minRange, tv, true);
        else if (tv <= m_maxRange / 10)
            setRange(m_minRange, m_maxRange / 10, true);
    }

    const double span = m_maxRange - m_minRange;
    if      (m_value < m_minRange) m_actualAngle = 0;
    else if (m_value > m_maxRange) m_actualAngle = NEEDLE_SWEEP;
    else                           m_actualAngle = static_cast<int>(m_value / span * NEEDLE_SWEEP);

    const bool needleMove = (std::abs(static_cast<double>(m_actualAngle) - m_shownAngle)
                             >= NEEDLE_SENSITIVITY);
    if (needleMove || (showWatermark && wmChanged)) {
        setNeedlePoints();
        update();
    }
}

// -----------------------------------------------------------------------------
// Paint
// -----------------------------------------------------------------------------

void MeterWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);
    if (m_outerRadius < 10) return;

    const QRectF meterBox(m_pivot.x() - m_outerRadius, m_pivot.y() - m_outerRadius,
                          m_outerRadius * 2.0,          m_outerRadius * 2.0);

    // -- 1. Dial circle (filled black) --
    p.setPen(Qt::black);
    p.setBrush(Qt::black);
    p.drawEllipse(meterBox);

    // -- 2. Scale text in the "notch" at the bottom  --
    if (!m_scaleText.isEmpty()) {
        // The notch spans horizontally between the two sweep endpoints
        // (angle=0 and angle=NEEDLE_SWEEP on the inner radius) and vertically
        // down to the outer-radius bottom.
        const QPointF leftPt  = calcPt(0,            m_innerRadius);
        const QPointF rightPt = calcPt(NEEDLE_SWEEP, m_innerRadius);
        const QPointF botPt   = calcPt(0,            m_outerRadius);
        const QRectF scaleRect(
            std::min(leftPt.x(), rightPt.x()),
            std::min(leftPt.y(), rightPt.y()),
            std::abs(rightPt.x() - leftPt.x()),
            std::abs(botPt.y()   - leftPt.y())
        );
        QFont sf("Arial", -1);
        sf.setPixelSize(std::max(8, static_cast<int>(m_outerRadius * 0.18)));
        p.setFont(sf);
        p.setPen(Qt::white);
        p.drawText(scaleRect, Qt::AlignCenter, m_scaleText);
    }

    // -- 3. Labels and tick marks --
    {
        QFont lf("Arial", -1);
        lf.setPixelSize(std::max(8, static_cast<int>(m_outerRadius * 0.16)));
        p.setFont(lf);
        p.setPen(QPen(Qt::white, 2));
        for (const auto &li : m_labels) {
            p.drawText(li.box, Qt::AlignCenter | Qt::TextDontClip, li.text);
            p.drawLine(li.tickInner, li.tickOuter);
        }
    }

    // -- 4. Watermark arc (red, on inner_radius) --
    if (showWatermark && m_lowValue >= 0.0 && m_highValue > m_lowValue) {
        const double span = m_maxRange - m_minRange;
        auto toAngle = [&](double v) -> int {
            if (v <= m_minRange) return 0;
            if (v >= m_maxRange) return NEEDLE_SWEEP;
            return static_cast<int>(v / span * NEEDLE_SWEEP);
        };
        const int la = toAngle(m_lowValue);
        const int ha = toAngle(m_highValue);
        if (la != ha) {
            const QRectF wmBox(
                m_pivot.x() - (m_innerRadius + 2), m_pivot.y() - (m_innerRadius + 2),
                (m_innerRadius + 2) * 2.0,          (m_innerRadius + 2) * 2.0);
            // Meter angle θ → Qt 1/16° = -(θ + 90 + PIVOT_ARC_ANGLE) * 16
            // CW sweep (value increases CW) = negative spanAngle in Qt
            const int startQt = -(la + 90 + PIVOT_ARC_ANGLE) * 16;
            const int spanQt  = -(ha - la) * 16;
            p.setPen(QPen(Qt::red, 3));
            p.setBrush(Qt::NoBrush);
            p.drawArc(wmBox, startQt, spanQt);
        }
    }

    // -- 5. Needle (blue triangle + grey pivot hub) --
    {
        const QPolygonF poly = QPolygonF()
            << m_needle[0] << m_needle[1] << m_needle[2];
        p.setPen(QPen(QColor(0, 0, 0xFF), 1));
        p.setBrush(QColor(0, 0, 0xFF));
        p.drawPolygon(poly);

        const QRectF pivotRect(
            m_pivot.x() - m_pivotRadius, m_pivot.y() - m_pivotRadius,
            m_pivotRadius * 2.0,          m_pivotRadius * 2.0);
        p.setPen(Qt::black);
        p.setBrush(QColor(170, 170, 170));
        p.drawEllipse(pivotRect);
    }
}
