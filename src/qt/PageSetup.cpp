// PageSetup.cpp
#include "PageSetup.h"
#include "IometerEngine.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>
#include <QRandomGenerator>

// ─────────────────────────────────────────────────────────────────────────────

static const QString STYLE_DARK =
    "background:#111820; color:#ccddff;"
    "QGroupBox { border:1px solid #2a3a4a; border-radius:4px; "
    "            margin-top:8px; color:#7799bb; font-weight:bold; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 4px; }"
    "QTreeWidget, QTableWidget { background:#0d1520; color:#ccddff; "
    "    border:1px solid #2a3a4a; gridline-color:#1a2a3a; }"
    "QTreeWidget::item:selected, QTableWidget::item:selected "
    "    { background:#1a3a6a; }"
    "QHeaderView::section { background:#162030; color:#8899aa; "
    "    border:1px solid #2a3a4a; padding:2px 4px; }"
    "QSpinBox, QComboBox { background:#162030; color:#ccddff; "
    "    border:1px solid #2a3a4a; border-radius:3px; padding:2px 4px; }"
    "QPushButton { background:#2a3a5e; color:#aaddff; "
    "    border:1px solid #3a5a8e; border-radius:4px; padding:3px 10px; }"
    "QPushButton:hover { background:#3a5a8e; }"
    "QPushButton:disabled { background:#1a2a3e; color:#445566; }";

// ─────────────────────────────────────────────────────────────────────────────

PageSetup::PageSetup(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    refreshWorkers();
    connect(engine, &IometerEngine::configChanged, this, &PageSetup::refreshWorkers);
}

void PageSetup::setupUi()
{
    setStyleSheet(STYLE_DARK);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background:#2a3a4a; width:4px; }");

    // ── Left: worker tree + add/remove buttons ────────────────────────────
    auto *leftPanel = new QWidget;
    auto *leftLay   = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(4);

    m_workerTree = new QTreeWidget;
    m_workerTree->setHeaderLabels({"Worker", "Type", "Queue"});
    m_workerTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_workerTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_workerTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_workerTree->setIndentation(16);
    leftLay->addWidget(m_workerTree, 1);

    auto *btnRow = new QHBoxLayout;
    m_addWorkerBtn    = new QPushButton("+ Add Worker");
    m_removeWorkerBtn = new QPushButton("− Remove");
    m_removeWorkerBtn->setEnabled(false);
    btnRow->addWidget(m_addWorkerBtn);
    btnRow->addWidget(m_removeWorkerBtn);
    leftLay->addLayout(btnRow);
    splitter->addWidget(leftPanel);

    // ── Right: worker detail panel ────────────────────────────────────────
    auto *rightPanel  = new QWidget;
    auto *rightLay    = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(6);

    // Worker info header
    m_workerNameLbl = new QLabel("(select a worker)");
    m_workerNameLbl->setStyleSheet("color:#5599ff; font-size:13px; font-weight:bold; padding:2px;");
    rightLay->addWidget(m_workerNameLbl);

    // Queue depth
    auto *qGroup = new QGroupBox("Outstanding I/Os");
    auto *qLay   = new QHBoxLayout(qGroup);
    qLay->addWidget(new QLabel("Queue depth:"));
    m_queueDepth = new QSpinBox;
    m_queueDepth->setRange(1, 256);
    m_queueDepth->setValue(1);
    m_queueDepth->setEnabled(false);
    qLay->addWidget(m_queueDepth);
    m_workerTypeCmb = new QComboBox;
    m_workerTypeCmb->addItems({"Disk", "Network (TCP)"});
    m_workerTypeCmb->setEnabled(false);
    qLay->addWidget(new QLabel("Type:"));
    qLay->addWidget(m_workerTypeCmb);
    qLay->addStretch();
    rightLay->addWidget(qGroup);

    // Target table
    auto *tGroup = new QGroupBox("Assigned Targets");
    auto *tLay   = new QVBoxLayout(tGroup);
    m_targetTable = new QTableWidget(0, 3);
    m_targetTable->setHorizontalHeaderLabels({"Target", "Type", "Active"});
    m_targetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_targetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_targetTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_targetTable->verticalHeader()->setVisible(false);
    tLay->addWidget(m_targetTable);
    rightLay->addWidget(tGroup, 1);

    splitter->addWidget(rightPanel);
    splitter->setSizes({200, 400});

    root->addWidget(splitter);

    // ── Wire up ────────────────────────────────────────────────────────────
    connect(m_workerTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *, QTreeWidgetItem *) { onWorkerSelectionChanged(); });
    connect(m_queueDepth, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PageSetup::onQueueDepthChanged);
    connect(m_addWorkerBtn,    &QPushButton::clicked, this, &PageSetup::onAddWorker);
    connect(m_removeWorkerBtn, &QPushButton::clicked, this, &PageSetup::onRemoveWorker);
    connect(m_targetTable, &QTableWidget::cellClicked,
            this, &PageSetup::onTargetToggled);
}

void PageSetup::refreshWorkers()
{
    m_workerTree->clear();
    if (!m_engine) return;

    for (const auto &mgr : m_engine->managers()) {
        auto *mgrItem = new QTreeWidgetItem(m_workerTree);
        mgrItem->setText(0, mgr.name + (mgr.connected ? "" : " (disconnected)"));
        mgrItem->setText(1, mgr.address);
        mgrItem->setData(0, Qt::UserRole, "manager:" + mgr.name);
        mgrItem->setExpanded(true);

        for (const auto &w : mgr.workers) {
            auto *wItem = new QTreeWidgetItem(mgrItem);
            wItem->setText(0, w.name);
            wItem->setText(1, w.type);
            wItem->setText(2, QString::number(w.queueDepth));
            wItem->setData(0, Qt::UserRole, "worker:" + mgr.name + ":" + w.id);
        }
    }
}

void PageSetup::onWorkerSelectionChanged()
{
    auto *item = m_workerTree->currentItem();
    if (!item) {
        m_removeWorkerBtn->setEnabled(false);
        m_queueDepth->setEnabled(false);
        m_workerTypeCmb->setEnabled(false);
        m_workerNameLbl->setText("(select a worker)");
        m_targetTable->setRowCount(0);
        return;
    }

    const QString key = item->data(0, Qt::UserRole).toString();
    if (key.startsWith("worker:")) {
        QStringList parts = key.mid(7).split(':');
        if (parts.size() < 2) return;
        m_selManagerName = parts[0];
        m_selWorkerId    = parts[1];

        // Find the worker
        for (const auto &mgr : m_engine->managers()) {
            if (mgr.name != m_selManagerName) continue;
            for (const auto &w : mgr.workers) {
                if (w.id != m_selWorkerId) continue;
                m_workerNameLbl->setText(mgr.name + " → " + w.name);
                m_queueDepth->setEnabled(true);
                m_workerTypeCmb->setEnabled(true);
                {
                    QSignalBlocker b(m_queueDepth);
                    m_queueDepth->setValue(w.queueDepth);
                }
                populateWorkerDetails(w);
                m_removeWorkerBtn->setEnabled(true);
                return;
            }
        }
    } else {
        m_removeWorkerBtn->setEnabled(false);
        m_queueDepth->setEnabled(false);
        m_workerTypeCmb->setEnabled(false);
        m_workerNameLbl->setText(item->text(0));
        m_targetTable->setRowCount(0);
    }
}

void PageSetup::populateWorkerDetails(const WorkerInfo &w)
{
    m_targetTable->setRowCount(0);
    const QStringList allTargets = {"C:", "D:", "W:", "X:", "Y:", "Z:",
                                    "\\Device\\Harddisk0\\DR0",
                                    "\\Device\\Harddisk1\\DR1"};
    for (const auto &t : allTargets) {
        const int row = m_targetTable->rowCount();
        m_targetTable->insertRow(row);
        m_targetTable->setItem(row, 0, new QTableWidgetItem(t));
        m_targetTable->setItem(row, 1, new QTableWidgetItem(t.startsWith('\\') ? "Raw" : "Drive"));
        auto *chk = new QTableWidgetItem;
        chk->setCheckState(w.targets.contains(t) ? Qt::Checked : Qt::Unchecked);
        chk->setTextAlignment(Qt::AlignCenter);
        m_targetTable->setItem(row, 2, chk);
    }
}

void PageSetup::onQueueDepthChanged(int value)
{
    if (m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName) continue;
        for (auto w : mgr.workers) {
            if (w.id != m_selWorkerId) continue;
            w.queueDepth = value;
            m_engine->updateWorker(w);
            return;
        }
    }
}

void PageSetup::onAddWorker()
{
    if (m_engine->managers().isEmpty()) return;
    bool ok;
    QString name = QInputDialog::getText(this, "Add Worker", "Worker name:",
                                         QLineEdit::Normal, "Worker", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const QString mgrName = m_engine->managers().first().name;
    WorkerInfo w;
    w.id          = mgrName + "-" + QString::number(QRandomGenerator::global()->bounded(100000));
    w.name        = name.trimmed();
    w.type        = "Disk";
    w.managerName = mgrName;
    w.queueDepth  = 1;
    m_engine->addWorker(mgrName, w);
}

void PageSetup::onRemoveWorker()
{
    if (m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    m_engine->removeWorker(m_selManagerName, m_selWorkerId);
    m_selManagerName.clear();
    m_selWorkerId.clear();
}

void PageSetup::onTargetToggled(int row, int col)
{
    if (col != 2) return;   // only the "Active" column
    // Update worker targets based on check states
    if (m_selManagerName.isEmpty()) return;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName) continue;
        for (auto w : mgr.workers) {
            if (w.id != m_selWorkerId) continue;
            w.targets.clear();
            for (int r = 0; r < m_targetTable->rowCount(); ++r) {
                auto *chkItem = m_targetTable->item(r, 2);
                if (chkItem && chkItem->checkState() == Qt::Checked)
                    w.targets.append(m_targetTable->item(r, 0)->text());
            }
            m_engine->updateWorker(w);
            return;
        }
    }
}
