// QtPageSetup.cpp -- "Disk Targets" tab
#include "QtPageSetup.h"
#include "QtIometerEngine.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QPen>
#include <QPixmap>

// =============================================================================
// Icon helpers
// =============================================================================

// Build the "unprepared logical disk" icon: take a standard HD icon and
// paint a red diagonal slash across it - matches the original MFC red-slash.
static QIcon makeUnpreparedDiskIcon()
{
    const int sz = 16;
    QPixmap px = QApplication::style()
                     ->standardIcon(QStyle::SP_DriveHDIcon)
                     .pixmap(sz, sz);
    if (px.isNull()) {
        px = QPixmap(sz, sz);
        px.fill(Qt::transparent);
    }
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    // Red slash from bottom-left to top-right (the "not ready" indicator)
    p.setPen(QPen(QColor(220, 0, 0), 2.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(2, sz - 3, sz - 3, 2);
    p.end();
    return QIcon(px);
}

// =============================================================================

QtPageSetup::QtPageSetup(QtIometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    connect(engine, &QtIometerEngine::configChanged,       this, &QtPageSetup::refreshForEngine);
    connect(engine, &QtIometerEngine::managerConnected,    this, [this](const ManagerInfo&){ refreshForEngine(); });
    connect(engine, &QtIometerEngine::managerDisconnected, this, [this](const QString&){ refreshForEngine(); });
}

// =============================================================================
// UI construction -- matches the original Disk Targets tab layout
// =============================================================================

void QtPageSetup::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // ---- Left: Targets group box -------------------------------------------
    auto *targGroup = new QGroupBox("Targets");
    auto *targLay   = new QVBoxLayout(targGroup);
    targLay->setContentsMargins(4, 8, 4, 4);
    m_targetList = new QListWidget;
    m_targetList->setUniformItemSizes(true);
    targLay->addWidget(m_targetList);
    root->addWidget(targGroup, 1);

    // ---- Right: disk parameters (stacked group boxes) -----------------------
    auto *rightLay = new QVBoxLayout;
    rightLay->setSpacing(4);

    // Maximum Disk Size
    auto *maxGroup = new QGroupBox("Maximum Disk Size");
    auto *maxLay   = new QHBoxLayout(maxGroup);
    maxLay->setContentsMargins(6, 8, 6, 4);
    m_maxDiskSize = new QSpinBox;
    m_maxDiskSize->setRange(0, 2147483647);
    m_maxDiskSize->setValue(0);
    m_maxDiskSize->setFixedWidth(80);
    maxLay->addWidget(m_maxDiskSize);
    maxLay->addWidget(new QLabel("Sectors"));
    maxLay->addStretch();
    rightLay->addWidget(maxGroup);

    // Starting Disk Sector
    auto *startGroup = new QGroupBox("Starting Disk Sector");
    auto *startLay   = new QHBoxLayout(startGroup);
    startLay->setContentsMargins(6, 8, 6, 4);
    m_startSector = new QSpinBox;
    m_startSector->setRange(0, 2147483647);
    m_startSector->setValue(0);
    m_startSector->setFixedWidth(80);
    startLay->addWidget(m_startSector);
    startLay->addStretch();
    rightLay->addWidget(startGroup);

    // # of Outstanding I/Os
    auto *qGroup = new QGroupBox("# of Outstanding I/Os");
    auto *qLay   = new QHBoxLayout(qGroup);
    qLay->setContentsMargins(6, 8, 6, 4);
    m_queueDepth = new QSpinBox;
    m_queueDepth->setRange(1, 256);
    m_queueDepth->setValue(1);
    m_queueDepth->setFixedWidth(80);
    qLay->addWidget(m_queueDepth);
    qLay->addWidget(new QLabel("per target"));
    qLay->addStretch();
    rightLay->addWidget(qGroup);

    // Use Fixed Seed
    auto *seedGroup = new QGroupBox("Use Fixed Seed");
    auto *seedLay   = new QHBoxLayout(seedGroup);
    seedLay->setContentsMargins(6, 8, 6, 4);
    m_fixedSeedChk  = new QCheckBox;
    m_fixedSeedEdit = new QLineEdit("0");
    m_fixedSeedEdit->setFixedWidth(80);
    m_fixedSeedEdit->setEnabled(false);
    seedLay->addWidget(m_fixedSeedChk);
    seedLay->addWidget(m_fixedSeedEdit);
    seedLay->addWidget(new QLabel("Fixed Seed Value"));
    seedLay->addStretch();
    rightLay->addWidget(seedGroup);

    // Test Connection Rate
    auto *connGroup = new QGroupBox("Test Connection Rate");
    auto *connLay   = new QHBoxLayout(connGroup);
    connLay->setContentsMargins(6, 8, 6, 4);
    m_connRateChk  = new QCheckBox;
    m_transPerConn = new QSpinBox;
    m_transPerConn->setRange(1, 100000);
    m_transPerConn->setValue(1);
    m_transPerConn->setFixedWidth(80);
    m_transPerConn->setEnabled(false);
    connLay->addWidget(m_connRateChk);
    connLay->addWidget(m_transPerConn);
    connLay->addWidget(new QLabel("Transactions per connection"));
    connLay->addStretch();
    rightLay->addWidget(connGroup);

    // Write IO Data Pattern
    auto *patGroup = new QGroupBox("Write IO Data Pattern");
    auto *patLay   = new QHBoxLayout(patGroup);
    patLay->setContentsMargins(6, 8, 6, 4);
    m_dataPattern = new QComboBox;
    m_dataPattern->addItems({"Repeating bytes", "Pseudo-random", "Full random"});
    patLay->addWidget(m_dataPattern);
    patLay->addStretch();
    rightLay->addWidget(patGroup);

    rightLay->addStretch();
    root->addLayout(rightLay, 1);

    // ---- Connections --------------------------------------------------------
    connect(m_targetList,   &QListWidget::itemChanged,
            this, &QtPageSetup::onTargetItemChanged);
    connect(m_queueDepth,   QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageSetup::onQueueDepthChanged);
    connect(m_maxDiskSize,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageSetup::onMaxDiskSizeChanged);
    connect(m_startSector,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageSetup::onStartSectorChanged);
    connect(m_fixedSeedChk, &QCheckBox::toggled,
            this, &QtPageSetup::onFixedSeedToggled);
    connect(m_fixedSeedEdit, &QLineEdit::editingFinished,
            this, &QtPageSetup::onFixedSeedValueChanged);
    connect(m_connRateChk,  &QCheckBox::toggled,
            this, &QtPageSetup::onConnRateToggled);
    connect(m_transPerConn, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QtPageSetup::onTransPerConnChanged);
    connect(m_dataPattern,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QtPageSetup::onDataPatternChanged);

    setParamsEnabled(false);
}

// =============================================================================
// Selection control (called by QtMainWindow::onWorkerTreeSelectionChanged)
// =============================================================================

void QtPageSetup::clearSelection()
{
    m_selManagerName.clear();
    m_selWorkerId.clear();
    m_targetList->clear();
    setParamsEnabled(false);
}

void QtPageSetup::setSelectedManager(const QString &mgrName)
{
    m_selManagerName = mgrName;
    m_selWorkerId.clear();
    populateTargetList();   // show available disks, none checked
    setParamsEnabled(false);
}

void QtPageSetup::setSelectedWorker(const QString &mgrName, const QString &workerId)
{
    m_selManagerName = mgrName;
    m_selWorkerId    = workerId;
    populateTargetList();
    loadWorkerParams();
    setParamsEnabled(true);
}

void QtPageSetup::refreshForEngine()
{
    // Ignore the configChanged our own edit just caused; rebuilding the target
    // list here would delete the item whose itemChanged signal is still running.
    if (m_applyingEdit) return;
    // Re-populate with the current selection intact (e.g. after manager connects)
    if (!m_selManagerName.isEmpty() && !m_selWorkerId.isEmpty())
        setSelectedWorker(m_selManagerName, m_selWorkerId);
    else if (!m_selManagerName.isEmpty())
        setSelectedManager(m_selManagerName);
    else
        clearSelection();
}

// =============================================================================
// Internal helpers
// =============================================================================

void QtPageSetup::populateTargetList()
{
    m_updating = true;
    m_targetList->clear();

    // Find the manager to get its availableTargets list
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;

        // Collect available targets, falling back to worker assignments if needed
        QList<TargetInfo> avail(mgr.availableTargets.begin(), mgr.availableTargets.end());
        if (avail.isEmpty()) {
            QStringList seen;
            for (const auto &w : mgr.workers) {
                for (const auto &t : w.targets) {
                    const QString tq = QString::fromStdString(t);
                    if (!seen.contains(tq)) {
                        seen.append(tq);
                        // Infer type from name: PhysicalDrive → physical, else logical
                        const bool isPhys = tq.contains("PhysicalDrive", Qt::CaseInsensitive)
                                         || tq.startsWith("\\\\.\\");
                        avail.append(TargetInfo{
                            t,
                            isPhys ? TargetKind::PhysicalDisk : TargetKind::LogicalDisk,
                            !isPhys   // assume logical disks from existing assignments are prepared
                        });
                    }
                }
            }
        }

        // Build the set of targets currently assigned to the selected worker
        QStringList assigned;
        if (!m_selWorkerId.isEmpty()) {
            for (const auto &w : mgr.workers)
                if (w.id == m_selWorkerId.toStdString()) {
                    for (const auto &t : w.targets) assigned << QString::fromStdString(t);
                    break;
                }
        }

        // Prepare the three possible icons
        const QIcon physDiskIcon    = QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon);
        const QIcon logReadyIcon    = QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon);
        static const QIcon logUnreadyIcon = makeUnpreparedDiskIcon();

        for (const auto &ti : avail) {
            QIcon icon;
            if (ti.kind == TargetKind::PhysicalDisk)
                icon = physDiskIcon;
            else if (ti.ready)
                icon = logReadyIcon;
            else
                icon = logUnreadyIcon;

            auto *item = new QListWidgetItem(icon, QString::fromStdString(ti.name));
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(assigned.contains(QString::fromStdString(ti.name)) ? Qt::Checked : Qt::Unchecked);
            m_targetList->addItem(item);
        }
        break;
    }
    m_updating = false;
}

void QtPageSetup::loadWorkerParams()
{
    if (m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    m_updating = true;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName) continue;
        for (const auto &w : mgr.workers) {
            if (w.id != m_selWorkerId) continue;
            m_queueDepth->setValue(w.queueDepth);
            m_maxDiskSize->setValue((int)qMin(w.maxDiskSize, (qint64)2147483647));
            m_startSector->setValue((int)qMin(w.startingSector, (qint64)2147483647));
            m_fixedSeedChk->setChecked(w.useFixedSeed);
            m_fixedSeedEdit->setText(QString::number(w.fixedSeedValue));
            m_fixedSeedEdit->setEnabled(w.useFixedSeed);
            m_connRateChk->setChecked(w.testConnRate);
            m_transPerConn->setValue(w.transPerConn);
            m_transPerConn->setEnabled(w.testConnRate);
            m_dataPattern->setCurrentIndex(w.dataPattern);
            break;
        }
        break;
    }
    m_updating = false;
}

void QtPageSetup::saveWorkerParams()
{
    if (m_updating || m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName) continue;
        for (auto w : mgr.workers) {
            if (w.id != m_selWorkerId) continue;
            w.queueDepth    = m_queueDepth->value();
            w.maxDiskSize   = m_maxDiskSize->value();
            w.startingSector= m_startSector->value();
            w.useFixedSeed  = m_fixedSeedChk->isChecked();
            w.fixedSeedValue= m_fixedSeedEdit->text().toLongLong();
            w.testConnRate  = m_connRateChk->isChecked();
            w.transPerConn  = m_transPerConn->value();
            w.dataPattern   = m_dataPattern->currentIndex();
            m_applyingEdit = true;
            m_engine->updateWorker(w);
            m_applyingEdit = false;
            return;
        }
    }
}

void QtPageSetup::setParamsEnabled(bool enabled)
{
    m_maxDiskSize->setEnabled(enabled);
    m_startSector->setEnabled(enabled);
    m_queueDepth->setEnabled(enabled);
    m_fixedSeedChk->setEnabled(enabled);
    m_fixedSeedEdit->setEnabled(enabled && m_fixedSeedChk->isChecked());
    m_connRateChk->setEnabled(enabled);
    m_transPerConn->setEnabled(enabled && m_connRateChk->isChecked());
    m_dataPattern->setEnabled(enabled);
}

// =============================================================================
// Slots
// =============================================================================

void QtPageSetup::onTargetItemChanged(QListWidgetItem *item)
{
    if (m_updating || m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;
    if (!item) return;

    // Rebuild the assigned targets from all check states
    QStringList assigned;
    for (int r = 0; r < m_targetList->count(); ++r) {
        auto *it = m_targetList->item(r);
        if (it && it->checkState() == Qt::Checked)
            assigned.append(it->text());
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

void QtPageSetup::onQueueDepthChanged(int)    { saveWorkerParams(); }
void QtPageSetup::onMaxDiskSizeChanged(int)   { saveWorkerParams(); }
void QtPageSetup::onStartSectorChanged(int)   { saveWorkerParams(); }

void QtPageSetup::onFixedSeedToggled(bool checked)
{
    m_fixedSeedEdit->setEnabled(checked);
    saveWorkerParams();
}

void QtPageSetup::onFixedSeedValueChanged()   { saveWorkerParams(); }

void QtPageSetup::onConnRateToggled(bool checked)
{
    m_transPerConn->setEnabled(checked);
    saveWorkerParams();
}

void QtPageSetup::onTransPerConnChanged(int)  { saveWorkerParams(); }
void QtPageSetup::onDataPatternChanged(int)   { saveWorkerParams(); }
