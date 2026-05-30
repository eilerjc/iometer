// BarChartWidget.cpp
#include "BarChartWidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <cmath>

BarChartWidget::BarChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(160, 90);
    setAttribute(Qt::WA_OpaquePaintEvent);

    auto *vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    vlay->setSpacing(2);

    // Worker name label (top)
    m_workerLabel = new QLabel("(unassigned)");
    m_workerLabel->setAlignment(Qt::AlignCenter);
    m_workerLabel->setStyleSheet("color:#aabbcc; font-size:10px; background:transparent;");
    m_workerLabel->setFixedHeight(LABEL_H);
    vlay->addWidget(m_workerLabel);

    // Spacer for the bar area (painted manually)
    vlay->addStretch(1);

    // Value label (bottom)
    m_valueLabel = new QLabel("0");
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setStyleSheet("color:#ffffff; font-size:11px; font-weight:bold; background:transparent;");
    m_valueLabel->setFixedHeight(LABEL_H);
    vlay->addWidget(m_valueLabel);

    // Metric combo
    m_metricCombo = new QComboBox;
    m_metricCombo->addItems(resultTypeNames());
    m_metricCombo->setCurrentIndex(RESULT_IOPS);
    m_metricCombo->setFixedHeight(COMBO_H);
    m_metricCombo->setStyleSheet(
        "QComboBox { background:#223; color:#aabbcc; border:1px solid #445; "
        "            border-radius:3px; font-size:10px; padding:1px 4px; }"
        "QComboBox QAbstractItemView { background:#223; color:#aabbcc; }");
    vlay->addWidget(m_metricCombo);

    connect(m_metricCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BarChartWidget::onMetricChanged);

    setStyleSheet("BarChartWidget { background:#0d1117; border:1px solid #2a3a4a; border-radius:4px; }");
}

void BarChartWidget::setWorker(const QString &mgrName, const QString &workerName)
{
    m_managerName = mgrName;
    m_workerName  = workerName;
    m_workerLabel->setText(workerName.isEmpty() ? "(unassigned)" : workerName);
    m_value = m_peak = m_maxRange = 0.0;
    update();
}

void BarChartWidget::setResultType(int rt)
{
    if (rt < 0 || rt >= NUM_RESULT_TYPES) return;
    m_resultType = rt;
    {
        QSignalBlocker blk(m_metricCombo);
        m_metricCombo->setCurrentIndex(rt);
    }
    m_value = m_peak = m_maxRange = 0.0;
    update();
}

void BarChartWidget::updateResults(const QVector<WorkerResult> &results)
{
    for (const auto &r : results) {
        if ((r.managerName == m_managerName || m_managerName.isEmpty()) &&
            r.workerName == m_workerName)
        {
            m_value = r.get(m_resultType);
            updatePeak();
            // Format the value string
            QString txt;
            if      (m_value >= 1e6)  txt = QString("%1 M").arg(m_value / 1e6, 0, 'f', 2);
            else if (m_value >= 1e3)  txt = QString("%1 K").arg(m_value / 1e3, 0, 'f', 1);
            else                      txt = QString::number(m_value, 'f', m_value < 10 ? 2 : 0);
            m_valueLabel->setText(txt);
            update();
            return;
        }
    }
}

void BarChartWidget::updatePeak()
{
    if (m_value > m_peak) m_peak = m_value;
    // Auto-range: ratchet up by decades
    if (m_peak <= 0) { m_maxRange = 100.0; return; }
    double range = std::pow(10.0, std::ceil(std::log10(m_peak)));
    if (range < 100.0) range = 100.0;
    m_maxRange = range;
}

void BarChartWidget::onMetricChanged(int index)
{
    m_resultType = index;
    m_value = m_peak = m_maxRange = 0.0;
    m_valueLabel->setText("0");
    update();
    emit resultTypeChanged(index);
}

void BarChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Bar area is between the worker label and the value label
    m_barTop    = MARGIN + LABEL_H + 4;
    m_barHeight = height() - m_barTop - LABEL_H - COMBO_H - MARGIN * 2 - 8;
    m_barLeft   = MARGIN + 6;
    m_barRight  = width() - MARGIN - 6;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0x0d, 0x11, 0x17));

    if (m_barHeight < 4 || m_barRight <= m_barLeft) return;

    const int barW = m_barRight - m_barLeft;

    // ── Background track ──
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x18, 0x24, 0x30));
    p.drawRoundedRect(m_barLeft, m_barTop, barW, m_barHeight, 3, 3);

    // ── Filled portion ──
    const double fraction = (m_maxRange > 0) ? std::min(1.0, m_value / m_maxRange) : 0.0;
    if (fraction > 0.001) {
        const int fillW = static_cast<int>(barW * fraction);
        // Gradient: dark blue → cyan → green (performance traffic-light style)
        QLinearGradient grad(m_barLeft, 0, m_barLeft + barW, 0);
        grad.setColorAt(0.0,  QColor(0x00, 0x55, 0xaa));
        grad.setColorAt(0.5,  QColor(0x00, 0xcc, 0x88));
        grad.setColorAt(0.85, QColor(0x88, 0xff, 0x44));
        grad.setColorAt(1.0,  QColor(0xff, 0x44, 0x00));
        p.setBrush(grad);
        p.drawRoundedRect(m_barLeft, m_barTop, fillW, m_barHeight, 3, 3);
    }

    // ── Peak marker ──
    if (m_peak > 0 && m_maxRange > 0) {
        const int peakX = m_barLeft + static_cast<int>(barW * std::min(1.0, m_peak / m_maxRange));
        p.setPen(QPen(QColor(0xff, 0xff, 0xff, 180), 2));
        p.drawLine(peakX, m_barTop, peakX, m_barTop + m_barHeight);
    }

    // ── Max range label (right side) ──
    p.setPen(QColor(0x44, 0x66, 0x88));
    QFont sf = p.font();
    sf.setPixelSize(9);
    p.setFont(sf);
    QString rangeStr;
    if      (m_maxRange >= 1e6) rangeStr = QString("%1M").arg(m_maxRange / 1e6, 0, 'f', 1);
    else if (m_maxRange >= 1e3) rangeStr = QString("%1K").arg(m_maxRange / 1e3, 0, 'f', 0);
    else                        rangeStr = QString::number(static_cast<int>(m_maxRange));
    p.drawText(m_barRight - 30, m_barTop - 1, 30, 12, Qt::AlignRight, rangeStr);

    // ── Border ──
    p.setPen(QColor(0x2a, 0x3a, 0x4a));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
}

void BarChartWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    // Double-click → pop up the BigMeter for this chart
    emit bigMeterRequested(0);   // slot index TBD by parent
}
