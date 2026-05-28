// PageDisplay.cpp
#include "PageDisplay.h"
#include "BarChartWidget.h"
#include "BigMeterWidget.h"
#include "MeterWidget.h"
#include "IometerEngine.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

PageDisplay::PageDisplay(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    populateCharts();
}

void PageDisplay::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ── Top bar: status + BigMeter button ─────────────────────────────────
    auto *topRow = new QHBoxLayout;
    m_statusLabel = new QLabel("Idle — start a test to see live results");
    m_statusLabel->setStyleSheet("color:#6688aa; font-style:italic;");
    topRow->addWidget(m_statusLabel, 1);

    m_bigMeterBtn = new QPushButton("⚡ BigMeter");
    m_bigMeterBtn->setToolTip("Open the Presentation Meter (speedometer view)");
    m_bigMeterBtn->setStyleSheet(
        "QPushButton { background:#2a3a5e; color:#aaddff; border:1px solid #4466aa;"
        "              border-radius:4px; padding:4px 12px; }"
        "QPushButton:hover { background:#3a5a8e; }");
    topRow->addWidget(m_bigMeterBtn);
    root->addLayout(topRow);

    // ── 4×2 grid of bar charts ────────────────────────────────────────────
    auto *grid = new QGridLayout;
    grid->setSpacing(6);
    for (int i = 0; i < NUM_CHARTS; ++i) {
        m_charts[i] = new BarChartWidget;
        grid->addWidget(m_charts[i], i / 2, i % 2);
        connect(m_charts[i], &BarChartWidget::bigMeterRequested,
                this, &PageDisplay::onChartBigMeterRequested);
    }
    root->addLayout(grid, 1);

    // ── BigMeter (hidden by default) ──────────────────────────────────────
    m_bigMeter = new BigMeterWidget(nullptr);   // top-level window
    m_bigMeter->hide();

    connect(m_bigMeterBtn, &QPushButton::clicked, this, &PageDisplay::onBigMeterClicked);

    setStyleSheet("PageDisplay { background:#111820; }");
}

void PageDisplay::populateCharts()
{
    if (!m_engine) return;
    const auto &mgrs = m_engine->managers();
    int slot = 0;
    for (const auto &mgr : mgrs) {
        for (const auto &w : mgr.workers) {
            if (slot >= NUM_CHARTS) break;
            m_charts[slot]->setWorker(mgr.name, w.name);
            ++slot;
        }
        if (slot >= NUM_CHARTS) break;
    }
    // Assign the first chart to "ALL" aggregate
    if (slot == 0) {
        m_charts[0]->setWorker("ALL", "ALL");
    }
}

void PageDisplay::refreshWorkerAssignments()
{
    // Clear then re-populate
    for (auto *c : m_charts) c->setWorker({}, {});
    populateCharts();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void PageDisplay::updateResults(const QVector<WorkerResult> &results)
{
    for (auto *c : m_charts)
        c->updateResults(results);

    // Feed the BigMeter the aggregate
    for (const auto &r : results) {
        if (r.isAggregate) {
            const double v = r.get(m_bigMeter->meter()->showWatermark
                                   ? RESULT_MBPS_DEC : RESULT_MBPS_DEC);
            m_bigMeter->updateDisplay(r.mbpsDec, QString("%1 MBps").arg(r.mbpsDec, 0, 'f', 1));
            break;
        }
    }
}

void PageDisplay::onTestStarted()
{
    m_running = true;
    m_statusLabel->setText("● Running...");
    m_statusLabel->setStyleSheet("color:#44ff88; font-weight:bold;");
    m_bigMeter->setButtonState(false, true, true);
}

void PageDisplay::onTestStopped()
{
    m_running = false;
    m_statusLabel->setText("■ Stopped");
    m_statusLabel->setStyleSheet("color:#ffaa44; font-style:italic;");
    m_bigMeter->setButtonState(true, false, false);
}

void PageDisplay::onBigMeterClicked()
{
    m_bigMeter->setTitle("Iometer Presentation Meter");
    m_bigMeter->setWorkerResult("All Workers", "MBps (Decimal)");
    m_bigMeter->show();
    m_bigMeter->raise();
    m_bigMeter->activateWindow();
}

void PageDisplay::onChartBigMeterRequested(int /*slotIndex*/)
{
    onBigMeterClicked();
}
