// PageResults.cpp
#include "PageResults.h"
#include "IometerEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>

static const QStringList COLUMNS = {
    "Manager", "Worker",
    "IOps", "Read IOps", "Write IOps",
    "MBps (Dec)", "Read MBps", "Write MBps",
    "Avg Lat (ms)", "Max Lat (ms)",
    "CPU %", "Errors"
};

static const QString STYLE =
    "PageResults { background:#111820; color:#ccddff; }"
    "QTableWidget { background:#0d1520; color:#ccddff; border:1px solid #2a3a4a; "
    "               gridline-color:#1a2a3a; }"
    "QTableWidget::item:selected { background:#1a3a6a; }"
    "QHeaderView::section { background:#162030; color:#8899aa; border:1px solid #2a3a4a; padding:2px; }"
    "QComboBox { background:#162030; color:#ccddff; border:1px solid #2a3a4a; border-radius:3px; padding:2px; }"
    "QPushButton { background:#2a3a5e; color:#aaddff; border:1px solid #3a5a8e; border-radius:4px; padding:3px 10px; }"
    "QPushButton:hover { background:#3a5a8e; }"
    "QLabel { color:#ccddff; }";

PageResults::PageResults(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    connect(engine, &IometerEngine::configChanged, this, [this]{ m_table->setRowCount(0); });
}

void PageResults::setupUi()
{
    setStyleSheet(STYLE);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *topRow = new QHBoxLayout;
    m_viewCombo = new QComboBox;
    m_viewCombo->addItems({"Live Results", "Saved Results"});
    m_statusLbl = new QLabel("Waiting for test data...");
    m_statusLbl->setStyleSheet("color:#6688aa; font-style:italic;");
    m_clearBtn  = new QPushButton("Clear Saved");
    m_exportBtn = new QPushButton("Export CSV…");
    m_clearBtn->setEnabled(false);
    topRow->addWidget(new QLabel("View:"));
    topRow->addWidget(m_viewCombo);
    topRow->addWidget(m_statusLbl, 1);
    topRow->addWidget(m_clearBtn);
    topRow->addWidget(m_exportBtn);
    root->addLayout(topRow);

    m_table = new QTableWidget(0, COLUMNS.size());
    m_table->setHorizontalHeaderLabels(COLUMNS);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { alternate-background-color:#10181f; }"
        "QTableWidget::item { padding:2px; }");
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table, 1);

    connect(m_clearBtn,   &QPushButton::clicked,
            this, &PageResults::onClearSaved);
    connect(m_exportBtn,  &QPushButton::clicked,
            this, &PageResults::onExportCsv);
    connect(m_viewCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PageResults::onViewChanged);
}

void PageResults::updateResults(const QVector<WorkerResult> &results)
{
    m_liveResults = results;
    if (m_showLive) populateTable(results);
    if (!results.isEmpty())
        m_statusLbl->setText(QString("Last update: %1 workers").arg(results.size()));
}

void PageResults::onTestStopped()
{
    if (!m_liveResults.isEmpty()) {
        m_clearBtn->setEnabled(true);
        if (!m_showLive) populateTable(m_engine->savedResults());
    }
}

void PageResults::onViewChanged(int index)
{
    m_showLive = (index == 0);
    populateTable(m_showLive ? m_liveResults : m_engine->savedResults());
}

void PageResults::populateTable(const QVector<WorkerResult> &results)
{
    m_table->setRowCount(0);
    for (const auto &r : results) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto mkItem = [&](const QString &txt, bool bold = false) {
            auto *it = new QTableWidgetItem(txt);
            if (bold) {
                QFont f = it->font();
                f.setBold(true);
                it->setFont(f);
                it->setForeground(QColor(0x88, 0xcc, 0xff));
            }
            return it;
        };
        const bool agg = r.isAggregate;
        m_table->setItem(row, 0,  mkItem(r.managerName, agg));
        m_table->setItem(row, 1,  mkItem(r.workerName, agg));
        m_table->setItem(row, 2,  mkItem(QString::number(r.iops, 'f', 0), agg));
        m_table->setItem(row, 3,  mkItem(QString::number(r.readIops, 'f', 0)));
        m_table->setItem(row, 4,  mkItem(QString::number(r.writeIops, 'f', 0)));
        m_table->setItem(row, 5,  mkItem(QString::number(r.mbpsDec, 'f', 1), agg));
        m_table->setItem(row, 6,  mkItem(QString::number(r.readMbpsDec, 'f', 1)));
        m_table->setItem(row, 7,  mkItem(QString::number(r.writeMbpsDec, 'f', 1)));
        m_table->setItem(row, 8,  mkItem(QString::number(r.avgLatencyMs, 'f', 2)));
        m_table->setItem(row, 9,  mkItem(QString::number(r.maxLatencyMs, 'f', 2)));
        m_table->setItem(row, 10, mkItem(QString::number(r.cpuUtil, 'f', 1)));
        m_table->setItem(row, 11, mkItem(QString::number(r.errors)));

        if (agg) {
            for (int c = 0; c < m_table->columnCount(); ++c) {
                auto *it = m_table->item(row, c);
                if (it) it->setBackground(QColor(0x1a, 0x2a, 0x3a));
            }
        }
    }
}

void PageResults::onClearSaved()
{
    m_engine->clearSavedResults();
    if (!m_showLive) m_table->setRowCount(0);
    m_clearBtn->setEnabled(!m_engine->savedResults().isEmpty());
}

void PageResults::onExportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Export Results", "results.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not open file: " + path);
        return;
    }
    QTextStream ts(&f);
    ts << COLUMNS.join(",") << "\n";
    const auto &data = m_showLive ? m_liveResults : m_engine->savedResults();
    for (const auto &r : data) {
        ts << r.managerName   << "," << r.workerName  << ","
           << r.iops          << "," << r.readIops    << "," << r.writeIops   << ","
           << r.mbpsDec       << "," << r.readMbpsDec << "," << r.writeMbpsDec << ","
           << r.avgLatencyMs  << "," << r.maxLatencyMs << ","
           << r.cpuUtil       << "," << r.errors       << "\n";
    }
    QMessageBox::information(this, "Exported", "Results saved to:\n" + path);
}
