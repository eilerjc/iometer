// QtPageNetwork.cpp -- "Network Targets" tab
#include "QtPageNetwork.h"
#include "QtIometerEngine.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QApplication>
#include <QStyle>

// =============================================================================

QtPageNetwork::QtPageNetwork(QtIometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    connect(engine, &QtIometerEngine::configChanged,       this, &QtPageNetwork::refreshForEngine);
    connect(engine, &QtIometerEngine::managerConnected,    this, &QtPageNetwork::onManagerConnected);
    connect(engine, &QtIometerEngine::managerDisconnected, this, &QtPageNetwork::onManagerDisconnected);
}

// =============================================================================
// UI construction -- matches the original Network Targets tab layout
// =============================================================================

void QtPageNetwork::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // ---- Left: Targets group box --------------------------------------------
    auto *targGroup = new QGroupBox("Targets");
    auto *targLay   = new QVBoxLayout(targGroup);
    targLay->setContentsMargins(4, 8, 4, 4);
    m_targetTree = new QTreeWidget;
    m_targetTree->setHeaderHidden(true);
    m_targetTree->setRootIsDecorated(true);
    m_targetTree->setUniformRowHeights(true);
    targLay->addWidget(m_targetTree);
    root->addWidget(targGroup, 1);

    // ---- Right: network parameters (stacked group boxes) --------------------
    auto *rightLay = new QVBoxLayout;
    rightLay->setSpacing(4);

    // Network Interface to Use for Connection
    auto *nicGroup = new QGroupBox("Network Interface to Use for Connection");
    auto *nicLay   = new QHBoxLayout(nicGroup);
    nicLay->setContentsMargins(6, 8, 6, 4);
    m_nicCombo = new QComboBox;
    m_nicCombo->setMinimumWidth(160);
    nicLay->addWidget(m_nicCombo);
    nicLay->addStretch();
    rightLay->addWidget(nicGroup);

    // Max # Outstanding Sends
    auto *sendsGroup = new QGroupBox("Max # Outstanding Sends");
    auto *sendsLay   = new QHBoxLayout(sendsGroup);
    sendsLay->setContentsMargins(6, 8, 6, 4);
    m_maxSends = new QSpinBox;
    m_maxSends->setRange(1, 256);
    m_maxSends->setValue(1);
    m_maxSends->setFixedWidth(60);
    sendsLay->addWidget(m_maxSends);
    sendsLay->addWidget(new QLabel("per VI target"));
    sendsLay->addStretch();
    rightLay->addWidget(sendsGroup);

    rightLay->addStretch(1);

    // Test Connection Rate
    auto *connGroup = new QGroupBox("Test Connection Rate");
    auto *connLay   = new QHBoxLayout(connGroup);
    connLay->setContentsMargins(6, 8, 6, 4);
    m_testConnRateChk = new QCheckBox;
    m_transPerConn    = new QSpinBox;
    m_transPerConn->setRange(1, 100000);
    m_transPerConn->setValue(1);
    m_transPerConn->setFixedWidth(60);
    m_transPerConn->setEnabled(false);
    connLay->addWidget(m_testConnRateChk);
    connLay->addWidget(m_transPerConn);
    connLay->addWidget(new QLabel("Transactions per connection"));
    connLay->addStretch();
    rightLay->addWidget(connGroup);

    root->addLayout(rightLay, 1);

    // ---- Connections --------------------------------------------------------
    connect(m_targetTree, &QTreeWidget::itemChanged,
            this, &QtPageNetwork::onTargetItemChanged);
    connect(m_nicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QtPageNetwork::onNicComboChanged);
    connect(m_maxSends, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageNetwork::onMaxSendsChanged);
    connect(m_testConnRateChk, &QCheckBox::toggled,
            this, &QtPageNetwork::onTestConnRateToggled);
    connect(m_transPerConn, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageNetwork::onTransPerConnChanged);

    setParamsEnabled(false);
}

// =============================================================================
// Selection control (called by QtMainWindow::onWorkerTreeSelectionChanged)
// =============================================================================

void QtPageNetwork::clearSelection()
{
    m_selManagerName.clear();
    m_selWorkerId.clear();
    m_targetTree->clear();
    setParamsEnabled(false);
}

void QtPageNetwork::setSelectedManager(const QString &mgrName)
{
    m_selManagerName = mgrName;
    m_selWorkerId.clear();
    populateTargetTree();
    rebuildNicCombo();
    setParamsEnabled(false);
}

void QtPageNetwork::setSelectedWorker(const QString &mgrName, const QString &workerId)
{
    m_selManagerName = mgrName;
    m_selWorkerId    = workerId;
    populateTargetTree();
    rebuildNicCombo();
    loadWorkerParams();
    setParamsEnabled(true);
}

void QtPageNetwork::refreshForEngine()
{
    // Ignore the configChanged our own edit just caused (we already hold the
    // correct state); rebuilding here would delete the tree item / combo entry
    // whose signal is still being processed.
    if (m_applyingEdit) return;
    if (!m_selManagerName.isEmpty() && !m_selWorkerId.isEmpty())
        setSelectedWorker(m_selManagerName, m_selWorkerId);
    else if (!m_selManagerName.isEmpty())
        setSelectedManager(m_selManagerName);
    else
        clearSelection();
}

void QtPageNetwork::onManagerConnected(const ManagerInfo &)    { refreshForEngine(); }
void QtPageNetwork::onManagerDisconnected(const QString &)     { refreshForEngine(); }

// =============================================================================
// Internal helpers
// =============================================================================

void QtPageNetwork::populateTargetTree()
{
    m_updating = true;
    m_targetTree->clear();

    // Build the set of network targets assigned to the selected worker
    QStringList assigned;
    if (!m_selManagerName.isEmpty() && !m_selWorkerId.isEmpty()) {
        for (const auto &mgr : m_engine->managers()) {
            if (mgr.name != m_selManagerName.toStdString()) continue;
            for (const auto &w : mgr.workers) {
                if (w.id == m_selWorkerId.toStdString() && w.type == "Network") {
                    for (const auto &t : w.targets) assigned << QString::fromStdString(t);
                    break;
                }
            }
            break;
        }
    }

    const QIcon mgrIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    const QIcon nicIcon = QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon);

    // Populate tree: one top-level item per manager, NIC children with checkboxes
    for (const auto &mgr : m_engine->managers()) {
        if (!mgr.connected) continue;

        auto *mgrItem = new QTreeWidgetItem(m_targetTree);
        mgrItem->setText(0, QString::fromStdString(mgr.name));
        mgrItem->setIcon(0, mgrIcon);
        mgrItem->setFlags(mgrItem->flags() & ~Qt::ItemIsUserCheckable);

        for (const auto &nic : mgr.availableNetInterfaces) {
            auto *nicItem = new QTreeWidgetItem(mgrItem);
            nicItem->setText(0, QString::fromStdString(nic));
            nicItem->setIcon(0, nicIcon);
            nicItem->setFlags(nicItem->flags() | Qt::ItemIsUserCheckable);
            // Key: "managerName/nic"
            const QString key = QString::fromStdString(mgr.name + "/" + nic);
            nicItem->setCheckState(0, assigned.contains(key) ? Qt::Checked : Qt::Unchecked);
            nicItem->setData(0, Qt::UserRole, key);
        }

        mgrItem->setExpanded(true);
    }
    m_updating = false;
}

void QtPageNetwork::rebuildNicCombo()
{
    m_updating = true;
    m_nicCombo->clear();

    // Show NICs on the selected manager as candidates for outgoing connections
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;
        for (const auto &nic : mgr.availableNetInterfaces)
            m_nicCombo->addItem(QString::fromStdString(nic));
        break;
    }
    m_updating = false;
}

void QtPageNetwork::loadWorkerParams()
{
    if (m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    m_updating = true;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;
        for (const auto &w : mgr.workers) {
            if (w.id != m_selWorkerId.toStdString()) continue;
            // Set NIC combo to match worker's networkInterface
            const int idx = m_nicCombo->findText(QString::fromStdString(w.networkInterface));
            if (idx >= 0) m_nicCombo->setCurrentIndex(idx);

            m_maxSends->setValue(w.maxOutstandingSends);
            m_testConnRateChk->setChecked(w.testConnRate);
            m_transPerConn->setValue(w.transPerConn);
            m_transPerConn->setEnabled(w.testConnRate);
            break;
        }
        break;
    }
    m_updating = false;
}

void QtPageNetwork::saveWorkerParams()
{
    if (m_updating || m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;
        for (auto w : mgr.workers) {
            if (w.id != m_selWorkerId.toStdString()) continue;
            w.networkInterface    = m_nicCombo->currentText().toStdString();
            w.maxOutstandingSends = m_maxSends->value();
            w.testConnRate        = m_testConnRateChk->isChecked();
            w.transPerConn        = m_transPerConn->value();
            m_applyingEdit = true;
            m_engine->updateWorker(w);
            m_applyingEdit = false;
            return;
        }
    }
}

void QtPageNetwork::setParamsEnabled(bool enabled)
{
    m_nicCombo->setEnabled(enabled);
    m_maxSends->setEnabled(enabled);
    m_testConnRateChk->setEnabled(enabled);
    m_transPerConn->setEnabled(enabled && m_testConnRateChk->isChecked());
}

// =============================================================================
// Slots
// =============================================================================

void QtPageNetwork::onTargetItemChanged(QTreeWidgetItem *item, int /*column*/)
{
    if (m_updating || m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    if (!item || !(item->flags() & Qt::ItemIsUserCheckable)) return;

    // Rebuild the assigned network targets from all check states
    QStringList assigned;
    QTreeWidgetItemIterator it(m_targetTree);
    while (*it) {
        if (((*it)->flags() & Qt::ItemIsUserCheckable) &&
            (*it)->checkState(0) == Qt::Checked) {
            assigned.append((*it)->data(0, Qt::UserRole).toString());
        }
        ++it;
    }

    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;
        for (auto w : mgr.workers) {
            if (w.id != m_selWorkerId.toStdString()) continue;
            w.targets.clear();
            for (const auto &s : assigned) w.targets.push_back(s.toStdString());
            m_applyingEdit = true;
            m_engine->updateWorker(w);
            m_applyingEdit = false;
            return;
        }
    }
}

void QtPageNetwork::onNicComboChanged(int)       { saveWorkerParams(); }
void QtPageNetwork::onMaxSendsChanged(int)       { saveWorkerParams(); }

void QtPageNetwork::onTestConnRateToggled(bool checked)
{
    m_transPerConn->setEnabled(checked);
    saveWorkerParams();
}

void QtPageNetwork::onTransPerConnChanged(int)   { saveWorkerParams(); }
