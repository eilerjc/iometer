// PageNetwork.cpp
#include "PageNetwork.h"
#include "IometerEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

static const QString STYLE =
    "PageNetwork { background:#111820; color:#ccddff; }"
    "QGroupBox { border:1px solid #2a3a4a; border-radius:4px; margin-top:8px; "
    "            color:#7799bb; font-weight:bold; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 4px; }"
    "QTableWidget { background:#0d1520; color:#ccddff; border:1px solid #2a3a4a; "
    "               gridline-color:#1a2a3a; }"
    "QTableWidget::item:selected { background:#1a3a6a; }"
    "QHeaderView::section { background:#162030; color:#8899aa; border:1px solid #2a3a4a; padding:2px; }"
    "QLineEdit { background:#162030; color:#ccddff; border:1px solid #2a3a4a; border-radius:3px; padding:2px 4px; }"
    "QPushButton { background:#2a3a5e; color:#aaddff; border:1px solid #3a5a8e; border-radius:4px; padding:3px 10px; }"
    "QPushButton:hover { background:#3a5a8e; }"
    "QPushButton:disabled { background:#1a2a3e; color:#445566; }"
    "QLabel { color:#ccddff; }";

PageNetwork::PageNetwork(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    rebuildTable();
    connect(engine, &IometerEngine::managerConnected,    this, &PageNetwork::onManagerConnected);
    connect(engine, &IometerEngine::managerDisconnected, this, &PageNetwork::onManagerDisconnected);
}

void PageNetwork::setupUi()
{
    setStyleSheet(STYLE);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // ── Info text ─────────────────────────────────────────────────────────
    auto *info = new QLabel(
        "Iometer (this GUI) listens on TCP port 1066 for incoming Dynamo connections.\n"
        "Dynamo instances connect automatically when launched with:  "
        "Dynamo.exe -i <this_machine_ip> -m <hostname>\n\n"
        "You can also manually specify an address below if Dynamo does not auto-connect.");
    info->setWordWrap(true);
    info->setStyleSheet("color:#6688aa; font-size:11px; "
                        "background:#0d1520; border:1px solid #2a3a4a; "
                        "border-radius:4px; padding:8px;");
    root->addWidget(info);

    // ── Connected managers table ─────────────────────────────────────────
    auto *tGroup = new QGroupBox("Connected Managers");
    auto *tLay   = new QVBoxLayout(tGroup);
    m_managerTable = new QTableWidget(0, 4);
    m_managerTable->setHorizontalHeaderLabels({"Name", "Address", "Workers", "Status"});
    m_managerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_managerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_managerTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_managerTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_managerTable->verticalHeader()->setVisible(false);
    m_managerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_managerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tLay->addWidget(m_managerTable);

    auto *tBtnRow = new QHBoxLayout;
    m_disconnectBtn = new QPushButton("Disconnect Selected");
    m_refreshBtn    = new QPushButton("⟳ Refresh");
    m_disconnectBtn->setEnabled(false);
    tBtnRow->addStretch();
    tBtnRow->addWidget(m_disconnectBtn);
    tBtnRow->addWidget(m_refreshBtn);
    tLay->addLayout(tBtnRow);
    root->addWidget(tGroup, 1);

    // ── Manual connect ────────────────────────────────────────────────────
    auto *cGroup = new QGroupBox("Manual Connect");
    auto *cLay   = new QHBoxLayout(cGroup);
    cLay->addWidget(new QLabel("Address:"));
    m_addrEdit = new QLineEdit("127.0.0.1");
    m_addrEdit->setPlaceholderText("hostname or IP");
    m_addrEdit->setFixedWidth(160);
    cLay->addWidget(m_addrEdit);
    cLay->addWidget(new QLabel("Name:"));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("(optional, auto-detected)");
    m_nameEdit->setFixedWidth(160);
    cLay->addWidget(m_nameEdit);
    m_connectBtn = new QPushButton("Connect");
    cLay->addWidget(m_connectBtn);
    cLay->addStretch();
    root->addWidget(cGroup);

    // ── Status ─────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Listening on port 1066...");
    m_statusLabel->setStyleSheet("color:#44aa88; font-style:italic;");
    root->addWidget(m_statusLabel);

    connect(m_connectBtn,    &QPushButton::clicked, this, &PageNetwork::onConnect);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &PageNetwork::onDisconnect);
    connect(m_refreshBtn,    &QPushButton::clicked, this, &PageNetwork::onRefresh);
    connect(m_managerTable,  &QTableWidget::itemSelectionChanged, this, [this]{
        m_disconnectBtn->setEnabled(!m_managerTable->selectedItems().isEmpty());
    });
}

void PageNetwork::rebuildTable()
{
    m_managerTable->setRowCount(0);
    for (const auto &mgr : m_engine->managers()) {
        const int row = m_managerTable->rowCount();
        m_managerTable->insertRow(row);
        m_managerTable->setItem(row, 0, new QTableWidgetItem(mgr.name));
        m_managerTable->setItem(row, 1, new QTableWidgetItem(mgr.address));
        m_managerTable->setItem(row, 2, new QTableWidgetItem(QString::number(mgr.workers.size())));
        auto *stItem = new QTableWidgetItem(mgr.connected ? "● Connected" : "○ Disconnected");
        stItem->setForeground(mgr.connected ? QColor(0x44, 0xff, 0x88) : QColor(0xaa, 0x44, 0x44));
        m_managerTable->setItem(row, 3, stItem);
    }
}

void PageNetwork::onManagerConnected(const ManagerInfo &)    { rebuildTable(); }
void PageNetwork::onManagerDisconnected(const QString &)     { rebuildTable(); }

void PageNetwork::onConnect()
{
    const QString addr = m_addrEdit->text().trimmed();
    const QString name = m_nameEdit->text().trimmed();
    if (addr.isEmpty()) return;
    m_engine->connectManager(addr, name);
    m_statusLabel->setText("Connecting to " + addr + "...");
}

void PageNetwork::onDisconnect()
{
    const auto rows = m_managerTable->selectionModel()->selectedRows();
    for (const auto &idx : rows) {
        const QString name = m_managerTable->item(idx.row(), 0)->text();
        m_engine->disconnectManager(name);
    }
}

void PageNetwork::onRefresh()
{
    rebuildTable();
    m_statusLabel->setText(QString("%1 manager(s) connected").arg(m_engine->managers().size()));
}
