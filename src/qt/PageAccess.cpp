// PageAccess.cpp -- "Access Specifications" tab
#include "PageAccess.h"
#include "IometerEngine.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QRandomGenerator>

static const QList<QPair<QString,int>> XFER_SIZES = {
    {"512 B",     512},   {"1 KiB",    1024},  {"2 KiB",    2048},
    {"4 KiB",    4096},   {"8 KiB",   8192},   {"16 KiB", 16384},
    {"32 KiB",  32768},   {"64 KiB", 65536},   {"128 KiB",131072},
    {"256 KiB", 262144},  {"512 KiB",524288},  {"1 MiB", 1048576},
    {"2 MiB", 2097152},   {"4 MiB", 4194304},  {"8 MiB", 8388608},
    {"16 MiB",16777216},  {"32 MiB",33554432}, {"64 MiB",67108864}
};

// =============================================================================

PageAccess::PageAccess(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    loadSpecList();
    connect(engine, &IometerEngine::configChanged, this, &PageAccess::loadSpecList);
}

// =============================================================================
// UI construction -- matches the original Access Specifications tab
// =============================================================================

void PageAccess::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // ---- Left: Assigned Access Specifications + Move Up/Down ----------------
    auto *leftGroup = new QGroupBox("Assigned Access Specifications");
    auto *leftLay   = new QVBoxLayout(leftGroup);
    leftLay->setContentsMargins(4, 8, 4, 4);
    m_assigned = new QListWidget;
    leftLay->addWidget(m_assigned, 1);
    // Move Up / Move Down at bottom
    auto *moveLay = new QHBoxLayout;
    m_upBtn   = new QPushButton("Move Up");
    m_downBtn = new QPushButton("Move Down");
    m_upBtn->setEnabled(false);
    m_downBtn->setEnabled(false);
    moveLay->addWidget(m_upBtn);
    moveLay->addWidget(m_downBtn);
    leftLay->addLayout(moveLay);
    root->addWidget(leftGroup, 2);

    // ---- Center: << Add / Remove >> buttons ---------------------------------
    auto *midLay = new QVBoxLayout;
    midLay->addStretch(1);
    m_addBtn = new QPushButton("<< Add");
    m_removeBtn = new QPushButton("Remove >>");
    m_addBtn->setEnabled(false);
    m_removeBtn->setEnabled(false);
    midLay->addWidget(m_addBtn);
    midLay->addSpacing(6);
    midLay->addWidget(m_removeBtn);
    midLay->addStretch(1);
    root->addLayout(midLay);

    // ---- Right: Global Access Specifications + New/Edit/Copy/Delete ----------
    auto *rightOuter = new QHBoxLayout;

    auto *rightGroup = new QGroupBox("Global Access Specifications");
    auto *rightLay   = new QVBoxLayout(rightGroup);
    rightLay->setContentsMargins(4, 8, 4, 4);
    m_global = new QListWidget;
    rightLay->addWidget(m_global, 1);
    rightOuter->addWidget(rightGroup, 1);

    // Buttons column to the right of the global list
    auto *btnLay = new QVBoxLayout;
    m_newBtn  = new QPushButton("New");
    m_editBtn = new QPushButton("Edit");
    m_copyBtn = new QPushButton("Edit Copy");
    m_delBtn  = new QPushButton("Delete");
    m_editBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);
    m_delBtn->setEnabled(false);
    btnLay->addWidget(m_newBtn);
    btnLay->addWidget(m_editBtn);
    btnLay->addWidget(m_copyBtn);
    btnLay->addWidget(m_delBtn);
    btnLay->addStretch();
    rightOuter->addLayout(btnLay);

    root->addLayout(rightOuter, 3);

    // ---- Connections ---------------------------------------------------------
    connect(m_addBtn,    &QPushButton::clicked, this, &PageAccess::onAddToAssigned);
    connect(m_removeBtn, &QPushButton::clicked, this, &PageAccess::onRemoveFromAssigned);
    connect(m_upBtn,     &QPushButton::clicked, this, &PageAccess::onMoveUp);
    connect(m_downBtn,   &QPushButton::clicked, this, &PageAccess::onMoveDown);
    connect(m_newBtn,    &QPushButton::clicked, this, &PageAccess::onNewSpec);
    connect(m_editBtn,   &QPushButton::clicked, this, &PageAccess::onEditSpec);
    connect(m_copyBtn,   &QPushButton::clicked, this, &PageAccess::onEditCopySpec);
    connect(m_delBtn,    &QPushButton::clicked, this, &PageAccess::onDeleteSpec);
    connect(m_global,   &QListWidget::itemSelectionChanged,
            this, &PageAccess::onGlobalSelectionChanged);
    connect(m_assigned, &QListWidget::itemSelectionChanged,
            this, &PageAccess::onAssignedSelectionChanged);
}

// =============================================================================
// Population
// =============================================================================

static QString specLabel(const AccessSpec &s) {
    if (s.name == "Idle" || s.name == "Default") return s.name;
    return s.displayLabel();
}

void PageAccess::rebuildGlobalList()
{
    const int cur = m_global->currentRow();
    m_global->clear();
    for (const auto &s : m_engine->accessSpecs()) {
        auto *item = new QListWidgetItem(specLabel(s));
        item->setData(Qt::UserRole, s.name);
        if (s.defaultSpec) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
        m_global->addItem(item);
    }
    if (cur >= 0 && cur < m_global->count())
        m_global->setCurrentRow(cur);
}

void PageAccess::rebuildAssignedList()
{
    // The "assigned" list reflects the global assignment (assignedSpecs of all workers)
    // For now show a placeholder empty list -- workers are configured via the engine
    m_assigned->clear();
}

void PageAccess::loadSpecList()
{
    rebuildGlobalList();
    rebuildAssignedList();
    onGlobalSelectionChanged();
    onAssignedSelectionChanged();
}

// =============================================================================
// Slot implementations
// =============================================================================

void PageAccess::onGlobalSelectionChanged()
{
    const bool sel = m_global->currentRow() >= 0;
    m_addBtn->setEnabled(sel);
    m_editBtn->setEnabled(sel);
    m_copyBtn->setEnabled(sel);
    m_delBtn->setEnabled(sel);
}

void PageAccess::onAssignedSelectionChanged()
{
    const int row = m_assigned->currentRow();
    m_removeBtn->setEnabled(row >= 0);
    m_upBtn->setEnabled(row > 0);
    m_downBtn->setEnabled(row >= 0 && row < m_assigned->count() - 1);
}

void PageAccess::onAddToAssigned()
{
    auto *item = m_global->currentItem();
    if (!item) return;
    // Add spec name to assigned list if not already present
    const QString name = item->data(Qt::UserRole).toString();
    for (int i = 0; i < m_assigned->count(); ++i)
        if (m_assigned->item(i)->data(Qt::UserRole).toString() == name) return;
    auto *newItem = new QListWidgetItem(item->text());
    newItem->setData(Qt::UserRole, name);
    m_assigned->addItem(newItem);
    onAssignedSelectionChanged();
}

void PageAccess::onRemoveFromAssigned()
{
    const int row = m_assigned->currentRow();
    if (row < 0) return;
    delete m_assigned->takeItem(row);
    onAssignedSelectionChanged();
}

void PageAccess::onMoveUp()
{
    const int row = m_assigned->currentRow();
    if (row <= 0) return;
    auto *item = m_assigned->takeItem(row);
    m_assigned->insertItem(row - 1, item);
    m_assigned->setCurrentRow(row - 1);
    onAssignedSelectionChanged();
}

void PageAccess::onMoveDown()
{
    const int row = m_assigned->currentRow();
    if (row < 0 || row >= m_assigned->count() - 1) return;
    auto *item = m_assigned->takeItem(row);
    m_assigned->insertItem(row + 1, item);
    m_assigned->setCurrentRow(row + 1);
    onAssignedSelectionChanged();
}

// =============================================================================
// Spec editor dialog
// =============================================================================

bool PageAccess::editSpecDialog(AccessSpec &spec, const QString &title)
{
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(360);

    auto *form = new QFormLayout(&dlg);
    form->setSpacing(6);

    auto *nameEdit   = new QLineEdit(spec.name);
    auto *xferCombo  = new QComboBox;
    auto *alignCombo = new QComboBox;
    for (const auto &p : XFER_SIZES) {
        xferCombo->addItem(p.first, p.second);
        alignCombo->addItem(p.first, p.second);
    }
    for (int i = 0; i < XFER_SIZES.size(); ++i)
        if (XFER_SIZES[i].second == spec.xferSizeBytes) { xferCombo->setCurrentIndex(i); break; }
    for (int i = 0; i < XFER_SIZES.size(); ++i)
        if (XFER_SIZES[i].second == spec.alignBytes)    { alignCombo->setCurrentIndex(i); break; }

    auto *readSpin  = new QSpinBox;  readSpin->setRange(0,100);  readSpin->setValue(spec.readPercent); readSpin->setSuffix(" %");
    auto *seqSpin   = new QSpinBox;  seqSpin->setRange(0,100);   seqSpin->setValue(spec.seqPercent);  seqSpin->setSuffix(" %");
    auto *burstSpin = new QSpinBox;  burstSpin->setRange(1,1024); burstSpin->setValue(spec.burstLength);
    auto *delaySpin = new QDoubleSpinBox; delaySpin->setRange(0,60000); delaySpin->setValue(spec.delayMs); delaySpin->setSuffix(" ms");
    auto *iterSpin  = new QSpinBox;  iterSpin->setRange(0,1000000); iterSpin->setValue(spec.iterations);
    iterSpin->setSpecialValueText("Unlimited");
    auto *defChk    = new QCheckBox; defChk->setChecked(spec.defaultSpec);

    form->addRow("Name:",             nameEdit);
    form->addRow("Transfer size:",    xferCombo);
    form->addRow("Alignment:",        alignCombo);
    form->addRow("Read %:",           readSpin);
    form->addRow("Sequential %:",     seqSpin);
    form->addRow("Burst length:",     burstSpin);
    form->addRow("Delay:",            delaySpin);
    form->addRow("Iterations:",       iterSpin);
    form->addRow("Default spec:",     defChk);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    spec.name          = nameEdit->text().trimmed();
    spec.xferSizeBytes = xferCombo->currentData().toInt();
    spec.alignBytes    = alignCombo->currentData().toInt();
    spec.readPercent   = readSpin->value();
    spec.seqPercent    = seqSpin->value();
    spec.burstLength   = burstSpin->value();
    spec.delayMs       = delaySpin->value();
    spec.iterations    = iterSpin->value();
    spec.defaultSpec   = defChk->isChecked();
    return true;
}

void PageAccess::onNewSpec()
{
    AccessSpec s;
    s.name = QString("New Spec %1").arg(m_engine->accessSpecs().size() + 1);
    if (!editSpecDialog(s, "New Access Specification")) return;
    auto specs = m_engine->accessSpecs();
    specs.append(s);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_global->setCurrentRow(m_global->count() - 1);
}

void PageAccess::onEditSpec()
{
    const int row = m_global->currentRow();
    if (row < 0) return;
    auto specs = m_engine->accessSpecs();
    if (row >= specs.size()) return;
    AccessSpec s = specs[row];
    if (!editSpecDialog(s, "Edit Access Specification")) return;
    specs[row] = s;
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_global->setCurrentRow(row);
}

void PageAccess::onEditCopySpec()
{
    const int row = m_global->currentRow();
    if (row < 0) return;
    auto specs = m_engine->accessSpecs();
    if (row >= specs.size()) return;
    AccessSpec s   = specs[row];
    s.name        += " (copy)";
    s.defaultSpec  = false;
    if (!editSpecDialog(s, "Edit Copy of Access Specification")) return;
    specs.insert(row + 1, s);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_global->setCurrentRow(row + 1);
}

void PageAccess::onDeleteSpec()
{
    const int row = m_global->currentRow();
    if (row < 0) return;
    auto specs = m_engine->accessSpecs();
    if (row >= specs.size()) return;
    const auto btn = QMessageBox::question(this, "Delete Spec",
        QString("Delete \"%1\"?").arg(specs[row].name));
    if (btn != QMessageBox::Yes) return;
    specs.removeAt(row);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
}
