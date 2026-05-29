// PageDisplay.cpp -- "Results Display" tab
#include "PageDisplay.h"
#include "BigMeterWidget.h"
#include "IometerEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QFrame>

// =============================================================================

PageDisplay::PageDisplay(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    m_bigMeter = new BigMeterWidget(nullptr);
    m_bigMeter->hide();
}

// =============================================================================
// UI construction -- matches the original Results Display tab
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

    // Results Since group
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

    // Update frequency group
    auto *freqGroup = new QGroupBox("Update Frequency (seconds)");
    auto *freqLay   = new QVBoxLayout(freqGroup);
    m_freqCombo = new QComboBox;
    m_freqCombo->addItems({"1", "2", "5", "10", "30", "60"});
    freqLay->addWidget(m_freqCombo);
    topLay->addWidget(freqGroup);

    root->addLayout(topLay);

    // ---- Display group: metric rows -----------------------------------------
    auto *dispGroup = new QGroupBox("Display");
    auto *grid      = new QGridLayout(dispGroup);
    grid->setSpacing(2);
    grid->setContentsMargins(4, 8, 4, 4);

    // Column stretch: metric btn | worker | value | ref | >
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);
    grid->setColumnStretch(4, 0);

    // The six standard metrics from the original
    const struct { int type; const char *label; } ROWS[] = {
        { RESULT_IOPS,          "Total I/Os per Second"         },
        { RESULT_MBPS_DEC,      "Total MBs per Second (Decimal)"},
        { RESULT_AVG_LATENCY_MS,"Average I/O Response Time (ms)"},
        { RESULT_MAX_LATENCY_MS,"Maximum I/O Response Time (ms)"},
        { RESULT_CPU_UTIL,      "% CPU Utilization (total)"     },
        { RESULT_ERRORS,        "Total Error Count"             },
    };

    for (int r = 0; r < NUM_ROWS; ++r) {
        DisplayRow &row = m_rows[r];
        row.resultType = ROWS[r].type;

        // Metric name button (left column)
        row.metricBtn = new QPushButton(ROWS[r].label);
        row.metricBtn->setFlat(false);
        row.metricBtn->setFixedWidth(230);
        row.metricBtn->setToolTip("Click to open Presentation Meter for this metric");
        const int capturedRow = r;
        connect(row.metricBtn, &QPushButton::clicked, this,
                [this, capturedRow]{ onMetricRowClicked(capturedRow); });

        // Worker label
        row.workerLbl = new QLabel("All Managers");
        row.workerLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Live value
        row.valueLbl = new QLabel("0");
        row.valueLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont vf = row.valueLbl->font();
        vf.setBold(true);
        row.valueLbl->setFont(vf);

        // Reference value
        row.refLbl = new QLabel("0");
        row.refLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Expand button
        row.expandBtn = new QPushButton(">");
        row.expandBtn->setFixedWidth(22);
        row.expandBtn->setFlat(false);
        const int capturedRow2 = r;
        connect(row.expandBtn, &QPushButton::clicked, this,
                [this, capturedRow2]{ onMetricRowClicked(capturedRow2); });

        grid->addWidget(row.metricBtn,  r, 0);
        grid->addWidget(row.workerLbl,  r, 1);
        grid->addWidget(row.valueLbl,   r, 2);
        grid->addWidget(row.refLbl,     r, 3);
        grid->addWidget(row.expandBtn,  r, 4);

        // Separator after each row except last
        if (r < NUM_ROWS - 1) {
            auto *line = new QFrame;
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            grid->addWidget(line, NUM_ROWS + r, 0, 1, 5);
        }
    }

    root->addWidget(dispGroup, 1);
}

// =============================================================================
// Slots
// =============================================================================

void PageDisplay::updateResults(const QVector<WorkerResult> &results)
{
    // Find the aggregate row
    WorkerResult agg;
    for (const auto &r : results)
        if (r.isAggregate) { agg = r; break; }

    for (int i = 0; i < NUM_ROWS; ++i) {
        DisplayRow &row = m_rows[i];
        const double val = agg.get(row.resultType);

        // Format value
        QString txt;
        switch (row.resultType) {
        case RESULT_MBPS_DEC:
            txt = QString("%1 MBPS (%2 MiBPS)")
                .arg(val, 0, 'f', 2)
                .arg(agg.mbpsBin, 0, 'f', 2);
            break;
        case RESULT_CPU_UTIL:
            txt = QString("%1 %").arg(val, 0, 'f', 2);
            break;
        case RESULT_AVG_LATENCY_MS:
        case RESULT_MAX_LATENCY_MS:
            txt = QString::number(val, 'f', 4);
            break;
        case RESULT_ERRORS:
            txt = QString::number((int)val);
            break;
        default:
            txt = QString::number(val, 'f', 2);
        }
        row.valueLbl->setText(txt);

        // Update reference if "since update" mode
        if (m_sinceUpdate && m_sinceUpdate->isChecked())
            row.refLbl->setText(txt);
    }

    // Feed BigMeter
    if (m_bigMeter && m_bigMeter->isVisible()) {
        const double v = agg.get(m_rows[m_activeMeterRow].resultType);
        m_bigMeter->updateDisplay(v, m_rows[m_activeMeterRow].metricBtn->text());
    }
}

void PageDisplay::onTestStarted()
{
    m_running = true;
    // Reset reference values
    for (int i = 0; i < NUM_ROWS; ++i) {
        m_rows[i].refValue = 0.0;
        m_rows[i].refLbl->setText("0");
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

void PageDisplay::onMetricRowClicked(int row)
{
    m_activeMeterRow = row;
    onBigMeterClicked();
}

void PageDisplay::refreshWorkerAssignments()
{
    // Could update worker labels in each row; currently all show "All Managers"
}
