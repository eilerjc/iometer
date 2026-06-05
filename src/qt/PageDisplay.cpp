// PageDisplay.cpp -- "Results Display" tab
#include "PageDisplay.h"
#include "BigMeterWidget.h"
#include "IometerEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QFrame>
#include <QMenu>
#include <QCursor>
#include <cmath>

// =============================================================================

PageDisplay::PageDisplay(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    m_bigMeter = new BigMeterWidget(nullptr);
    m_bigMeter->hide();

    // Wire BigMeter test-control buttons to the engine
    connect(m_bigMeter, &BigMeterWidget::stopRequested,
            m_engine,   &IometerEngine::stopTest);
    connect(m_bigMeter, &BigMeterWidget::startRequested,
            m_engine,   &IometerEngine::startTest);
    // "Next >>" advances to the next assigned spec (handled by MainWindow)
    connect(m_bigMeter, &BigMeterWidget::nextRequested,
            this,       &PageDisplay::nextSpecRequested);
}

// =============================================================================
// UI construction -- matches the original Results Display tab layout:
//
//   [Metric button (tall)]  |  All Managers        <value text>
//                           |  [====bar fill====]  <scale>  [>]
//
// Each metric block uses a nested HBox (outer) / VBox (inner) structure
// so the text line sits naturally above the bar without grid-cell fighting.
// =============================================================================

void PageDisplay::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // ---- Top row: instructions + options ------------------------------------
    auto *topLay = new QHBoxLayout;

    auto *instLbl = new QLabel(
        "Drag managers and workers\nfrom the Topology window\n"
        "to the progress bar of your choice.");
    instLbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    topLay->addWidget(instLbl, 1);

    m_recordChk = new QCheckBox("Record last update\nresults to file");
    topLay->addWidget(m_recordChk);
    topLay->addSpacing(12);

    auto *sinceGroup = new QGroupBox("Results Since");
    auto *sinceLay   = new QVBoxLayout(sinceGroup);
    sinceLay->setSpacing(2);
    m_sinceStart  = new QRadioButton("Start of Test");
    m_sinceUpdate = new QRadioButton("Last Update");
    m_sinceUpdate->setChecked(true);
    sinceLay->addWidget(m_sinceStart);
    sinceLay->addWidget(m_sinceUpdate);
    topLay->addWidget(sinceGroup);
    topLay->addSpacing(8);

    auto *freqGroup = new QGroupBox("Update Frequency (seconds)");
    auto *freqLay   = new QVBoxLayout(freqGroup);
    m_freqCombo = new QComboBox;
    m_freqCombo->addItems({"1", "2", "5", "10", "30", "60"});
    freqLay->addWidget(m_freqCombo);
    topLay->addWidget(freqGroup);

    root->addLayout(topLay);

    // ---- Display group: one metric block per row ---------------------------
    auto *dispGroup = new QGroupBox("Display");
    auto *dispLay   = new QVBoxLayout(dispGroup);
    dispLay->setSpacing(3);
    dispLay->setContentsMargins(4, 8, 4, 4);

    const struct { int type; const char *label; } ROWS[] = {
        { RESULT_IOPS,          "Total I/Os per Second"          },
        { RESULT_MBPS_DEC,      "Total MBs per Second (Decimal)" },
        { RESULT_AVG_LATENCY_MS,"Average I/O Response Time (ms)" },
        { RESULT_MAX_LATENCY_MS,"Maximum I/O Response Time (ms)" },
        { RESULT_CPU_UTIL,      "% CPU Utilization (total)"      },
        { RESULT_ERRORS,        "Total Error Count"              },
    };

    for (int r = 0; r < NUM_ROWS; ++r) {
        DisplayRow &row = m_rows[r];
        row.resultType = ROWS[r].type;

        // -- Metric button (left, spans the full 2-line height) ------------
        row.metricBtn = new QPushButton(ROWS[r].label);
        row.metricBtn->setFlat(false);
        row.metricBtn->setFixedWidth(230);
        row.metricBtn->setToolTip("Click to change the metric displayed in this row");
        const int capturedRow = r;
        connect(row.metricBtn, &QPushButton::clicked, this,
                [this, capturedRow]{ onMetricButtonClicked(capturedRow); });

        // -- Inner VBox: text line above bar line --------------------------
        auto *innerLay = new QVBoxLayout;
        innerLay->setSpacing(2);
        innerLay->setContentsMargins(0, 0, 0, 0);

        // Top line: "All Managers"  <spacer>  <value>
        auto *topLine = new QHBoxLayout;
        topLine->setSpacing(6);

        row.workerLbl = new QLabel("All Managers");
        row.workerLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        topLine->addWidget(row.workerLbl);

        topLine->addStretch(1);   // push value to the right

        row.valueLbl = new QLabel("0");
        row.valueLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.valueLbl->setMinimumWidth(155);  // fits "995.04 MBPS (948.94 MiBPS)"
        topLine->addWidget(row.valueLbl);

        innerLay->addLayout(topLine);

        // Bottom line: [bar fills] [scale fixed] [>]
        auto *botLine = new QHBoxLayout;
        botLine->setSpacing(4);

        // Progress bar - solid blue fill, no text overlay.
        // A minimal stylesheet is needed: Windows 11 native Vista-style draws
        // a thin animated chunk regardless of widget height; this overrides it.
        row.bar = new QProgressBar;
        row.bar->setRange(0, 1000);
        row.bar->setValue(0);
        row.bar->setTextVisible(false);
        row.bar->setFixedHeight(20);
        row.bar->setStyleSheet(
            "QProgressBar { border: 1px solid #999; background: #f0f0f0; }"
            "QProgressBar::chunk { background: #0078d7; margin: 0px; }");
        botLine->addWidget(row.bar, 1);   // stretch = 1 so bar fills remaining width

        // Scale label - current power-of-10 ceiling, right of bar
        row.refLbl = new QLabel("-");
        row.refLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.refLbl->setFixedWidth(65);
        botLine->addWidget(row.refLbl);

        // Expand button - opens BigMeter for this metric
        row.expandBtn = new QPushButton(">");
        row.expandBtn->setFixedWidth(22);
        row.expandBtn->setFlat(false);
        const int capturedRow2 = r;
        connect(row.expandBtn, &QPushButton::clicked, this,
                [this, capturedRow2]{ onMetricRowClicked(capturedRow2); });  // opens BigMeter
        botLine->addWidget(row.expandBtn);

        innerLay->addLayout(botLine);

        // -- Outer HBox: metric button | inner ----------------------------
        auto *outerRow = new QHBoxLayout;
        outerRow->setSpacing(4);
        outerRow->addWidget(row.metricBtn);
        outerRow->addLayout(innerLay, 1);
        dispLay->addLayout(outerRow);

        // Horizontal separator between rows (not after the last one)
        if (r < NUM_ROWS - 1) {
            auto *sep = new QFrame;
            sep->setFrameShape(QFrame::HLine);
            sep->setFrameShadow(QFrame::Sunken);
            dispLay->addWidget(sep);
        }
    }

    root->addWidget(dispGroup, 1);
}

// =============================================================================
// Helpers
// =============================================================================

// Smallest power of 10 strictly greater than val.
// Examples:  0.0329 → 0.1,  30366 → 100000,  995 → 1000,  3.89 → 10
static double logCeiling(double val)
{
    if (val <= 0.0) return 1.0;
    return std::pow(10.0, std::floor(std::log10(val)) + 1.0);
}

// Format the scale ceiling for display
static QString scaleText(double ceiling)
{
    if (ceiling >= 1.0)   return QString::number(static_cast<qint64>(ceiling));
    if (ceiling >= 0.01)  return QString::number(ceiling, 'f', 1);   // "0.1"
    return                       QString::number(ceiling, 'f', 2);   // "0.01"
}

// =============================================================================
// Slots
// =============================================================================

void PageDisplay::updateResults(const QVector<WorkerResult> &results)
{
    WorkerResult agg;
    for (const auto &r : results)
        if (r.isAggregate) { agg = r; break; }

    QString activeBigMeterTxt;

    for (int i = 0; i < NUM_ROWS; ++i) {
        DisplayRow &row = m_rows[i];
        const double val = agg.get(row.resultType);

        // -- Format numeric value for the value label -----------------------
        QString txt;
        switch (row.resultType) {
        case RESULT_MBPS_DEC:
            txt = QString("%1 MBPS (%2 MiBPS)")
                      .arg(val, 0, 'f', 2)
                      .arg(agg.mbpsBin, 0, 'f', 2);
            break;
        case RESULT_READ_MBPS_DEC: case RESULT_WRITE_MBPS_DEC:
        case RESULT_MBPS_BIN: case RESULT_READ_MBPS_BIN: case RESULT_WRITE_MBPS_BIN:
            txt = QString::number(val, 'f', 2);
            break;
        case RESULT_CPU_UTIL: case RESULT_CPU_USER: case RESULT_CPU_KERNEL:
        case RESULT_CPU_DPC:  case RESULT_CPU_INT_TIME: case RESULT_CPU_EFFECTIVENESS:
            txt = QString("%1 %").arg(val, 0, 'f', 2);
            break;
        case RESULT_AVG_LATENCY_MS: case RESULT_AVG_READ_LATENCY_MS:
        case RESULT_AVG_WRITE_LATENCY_MS: case RESULT_AVG_TRANS_LATENCY_MS:
        case RESULT_AVG_CONN_LATENCY_MS:
        case RESULT_MAX_LATENCY_MS: case RESULT_MAX_READ_LATENCY_MS:
        case RESULT_MAX_WRITE_LATENCY_MS: case RESULT_MAX_TRANS_LATENCY_MS:
        case RESULT_MAX_CONN_LATENCY_MS:
            txt = QString::number(val, 'f', 4);
            break;
        case RESULT_ERRORS: case RESULT_READ_ERRORS: case RESULT_WRITE_ERRORS:
        case RESULT_NET_PACKET_ERRORS:
            txt = QString::number(static_cast<int>(val));
            break;
        default:
            txt = QString::number(val, 'f', 2);
        }
        row.valueLbl->setText(txt);

        // -- Units-qualified text for BigMeter ------------------------------
        QString bigTxt = txt;
        switch (row.resultType) {
        case RESULT_IOPS: case RESULT_READ_IOPS: case RESULT_WRITE_IOPS:
        case RESULT_TRANS_PS: case RESULT_CONN_PS:
            bigTxt = txt + " IOPS"; break;
        case RESULT_AVG_LATENCY_MS: case RESULT_AVG_READ_LATENCY_MS:
        case RESULT_AVG_WRITE_LATENCY_MS: case RESULT_MAX_LATENCY_MS:
        case RESULT_MAX_READ_LATENCY_MS: case RESULT_MAX_WRITE_LATENCY_MS:
            bigTxt = txt + " ms"; break;
        default: break;
        }
        if (i == m_activeMeterRow)
            activeBigMeterTxt = bigTxt;

        // -- Log-scale bar fill ---------------------------------------------
        if (val > 0.0) {
            const double needed = logCeiling(val);
            if (needed > row.maxSeen) row.maxSeen = needed;
        } else if (row.maxSeen <= 0.0) {
            row.maxSeen = 1.0;
        }
        const int pos = (row.maxSeen > 0.0)
                            ? static_cast<int>(val / row.maxSeen * 1000.0)
                            : 0;
        row.bar->setValue(pos);
        row.refLbl->setText(scaleText(row.maxSeen));
    }

    if (m_bigMeter && m_bigMeter->isVisible()) {
        const double v = agg.get(m_rows[m_activeMeterRow].resultType);
        m_bigMeter->updateDisplay(v, activeBigMeterTxt);
    }
}

void PageDisplay::onTestStarted()
{
    m_running = true;
    for (int i = 0; i < NUM_ROWS; ++i) {
        m_rows[i].refValue = 0.0;
        m_rows[i].maxSeen  = 0.0;
        m_rows[i].valueLbl->setText("0");
        m_rows[i].refLbl->setText("-");
        m_rows[i].bar->setValue(0);
    }
    if (m_bigMeter) m_bigMeter->setButtonState(false, true, true);
}

void PageDisplay::onTestStopped()
{
    m_running = false;
    if (m_bigMeter) m_bigMeter->setButtonState(true, false, false);
}

void PageDisplay::onBigMeterClicked()
{
    m_bigMeter->setTitle("Iometer Presentation Meter");
    m_bigMeter->setWorkerResult("All Managers",
                                m_rows[m_activeMeterRow].metricBtn->text());
    m_bigMeter->show();
    m_bigMeter->raise();
    m_bigMeter->activateWindow();
}

void PageDisplay::onMetricButtonClicked(int row)
{
    // Build the exact 2-level cascading menu from IDR_POPUP_DISPLAY_LIST.
    const int cur = m_rows[row].resultType;

    QMenu menu(this);

    auto add = [&](QMenu *m, const QString &label, int type) {
        QAction *act = m->addAction(label);
        act->setData(type);
        act->setCheckable(true);
        act->setChecked(cur == type);
    };

    // -- Operations per Second ----------------------------------------------
    QMenu *ops = menu.addMenu("Operations per Second");
    add(ops, "Total I/Os per Second",       RESULT_IOPS);
    add(ops, "Read I/Os per Second",         RESULT_READ_IOPS);
    add(ops, "Write I/Os per Second",        RESULT_WRITE_IOPS);
    ops->addSeparator();
    add(ops, "Transactions per Second",      RESULT_TRANS_PS);
    add(ops, "Connections per Second",       RESULT_CONN_PS);

    // -- Megabytes per Second -----------------------------------------------
    QMenu *mbs = menu.addMenu("Megabytes per Second");
    add(mbs, "Total MBs per Second (Decimal)",  RESULT_MBPS_DEC);
    add(mbs, "Read MBs per Second (Decimal)",   RESULT_READ_MBPS_DEC);
    add(mbs, "Write MBs per Second (Decimal)",  RESULT_WRITE_MBPS_DEC);
    mbs->addSeparator();
    add(mbs, "Total MBs per Second (Binary)",   RESULT_MBPS_BIN);
    add(mbs, "Read MBs per Second (Binary)",    RESULT_READ_MBPS_BIN);
    add(mbs, "Write MBs per Second (Binary)",   RESULT_WRITE_MBPS_BIN);

    // -- Average Latency ----------------------------------------------------
    QMenu *avg = menu.addMenu("Average Latency");
    add(avg, "Average I/O Response Time (ms)",  RESULT_AVG_LATENCY_MS);
    add(avg, "Avg. Read Response Time (ms)",    RESULT_AVG_READ_LATENCY_MS);
    add(avg, "Avg. Write Response Time (ms)",   RESULT_AVG_WRITE_LATENCY_MS);
    avg->addSeparator();
    add(avg, "Avg. Transaction Time (ms)",      RESULT_AVG_TRANS_LATENCY_MS);
    add(avg, "Avg. Connection Time (ms)",       RESULT_AVG_CONN_LATENCY_MS);

    // -- Maximum Latency ----------------------------------------------------
    QMenu *mx = menu.addMenu("Maximum Latency");
    add(mx, "Maximum I/O Response Time (ms)",   RESULT_MAX_LATENCY_MS);
    add(mx, "Max. Read Response Time (ms)",     RESULT_MAX_READ_LATENCY_MS);
    add(mx, "Max. Write Response Time (ms)",    RESULT_MAX_WRITE_LATENCY_MS);
    mx->addSeparator();
    add(mx, "Max. Transaction Time (ms)",       RESULT_MAX_TRANS_LATENCY_MS);
    add(mx, "Max. Connection Time (ms)",        RESULT_MAX_CONN_LATENCY_MS);

    // -- CPU ----------------------------------------------------------------
    QMenu *cpu = menu.addMenu("CPU");
    add(cpu, "% CPU Utilization (total)",   RESULT_CPU_UTIL);
    add(cpu, "% User Time",                 RESULT_CPU_USER);
    add(cpu, "% Privileged Time",           RESULT_CPU_KERNEL);
    add(cpu, "% DPC Time",                  RESULT_CPU_DPC);
    add(cpu, "% Interrupt Time",            RESULT_CPU_INT_TIME);
    cpu->addSeparator();
    add(cpu, "Interrupts per Second",       RESULT_CPU_INTERRUPTS_PS);
    add(cpu, "CPU Effectiveness",           RESULT_CPU_EFFECTIVENESS);

    // -- Network ------------------------------------------------------------
    QMenu *net = menu.addMenu("Network");
    add(net, "Network Packets per Second",      RESULT_NET_PACKETS_PS);
    net->addSeparator();
    add(net, "Packet Errors",                   RESULT_NET_PACKET_ERRORS);
    add(net, "TCP Segments Retrans. per Sec.",  RESULT_NET_RETRANS_PS);

    // -- Errors -------------------------------------------------------------
    QMenu *err = menu.addMenu("Errors");
    add(err, "Total Error Count",   RESULT_ERRORS);
    add(err, "Read Error Count",    RESULT_READ_ERRORS);
    add(err, "Write Error Count",   RESULT_WRITE_ERRORS);

    QAction *chosen = menu.exec(QCursor::pos());
    if (!chosen || !chosen->data().isValid()) return;

    const int newType = chosen->data().toInt();
    if (newType == cur) return;

    m_rows[row].resultType = newType;
    m_rows[row].metricBtn->setText(chosen->text());
    m_rows[row].maxSeen  = 0.0;
    m_rows[row].valueLbl->setText("0");
    m_rows[row].refLbl->setText("-");
    m_rows[row].bar->setValue(0);
}

void PageDisplay::onMetricRowClicked(int row)
{
    // ">" button - open BigMeter for this row's current metric
    m_activeMeterRow = row;
    onBigMeterClicked();
}

void PageDisplay::refreshWorkerAssignments()
{
    // Could update worker labels in each row; currently all show "All Managers"
}
