// PageAccess.cpp -- "Access Specifications" tab
#include "PageAccess.h"
#include "IometerEngine.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
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
#include <QRadioButton>
#include <QSlider>
#include <QButtonGroup>
#include <QMessageBox>
#include <QStyleFactory>
#include <QPainter>
#include <QPixmap>


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
    m_global->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
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
    return s.name;   // all built-in specs carry their full original name
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

void PageAccess::setActiveSpecIndex(int idx)
{
    // Build a small green right-arrow icon for the running spec
    static QIcon runIcon, blankIcon;
    if (runIcon.isNull()) {
        QPixmap pm(12, 12);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(0x00, 0xaa, 0x00));
        p.setPen(Qt::NoPen);
        // Solid green right-pointing triangle
        p.drawPolygon(QPolygon({ QPoint(2,2), QPoint(2,10), QPoint(10,6) }));
        runIcon = QIcon(pm);

        QPixmap blank(12, 12);
        blank.fill(Qt::transparent);
        blankIcon = QIcon(blank);
    }

    for (int i = 0; i < m_assigned->count(); ++i)
        m_assigned->item(i)->setIcon(i == idx ? runIcon : blankIcon);
}

QList<AccessSpec> PageAccess::currentAssignedSpecs() const
{
    QList<AccessSpec> result;
    const auto &allSpecs = m_engine->accessSpecs();
    for (int i = 0; i < m_assigned->count(); ++i) {
        const QString name = m_assigned->item(i)->data(Qt::UserRole).toString();
        for (const auto &s : allSpecs) {
            if (s.name == name) { result.append(s); break; }
        }
    }
    return result;
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

// -- Helper --------------------------------------------------------------------
// Build three spinners (Megabytes / Kilobytes / Bytes) packed into an HBox.
// *mbSpin, *kbSpin, *bSpin are set to the created spinners.
static QHBoxLayout *makeSizeSpinners(QSpinBox **mbSpin, QSpinBox **kbSpin,
                                     QSpinBox **bSpin)
{
    *mbSpin = new QSpinBox; (*mbSpin)->setRange(0, 8192); (*mbSpin)->setSuffix("");
    *kbSpin = new QSpinBox; (*kbSpin)->setRange(0, 65535);
    *bSpin  = new QSpinBox; (*bSpin)->setRange(0, 65535);
    auto *lay = new QHBoxLayout;
    lay->setSpacing(4);
    lay->addWidget(*mbSpin); lay->addWidget(new QLabel("Megabytes"));
    lay->addWidget(*kbSpin); lay->addWidget(new QLabel("Kilobytes"));
    lay->addWidget(*bSpin);  lay->addWidget(new QLabel("Bytes"));
    return lay;
}
static void setSizeSpinners(QSpinBox *mb, QSpinBox *kb, QSpinBox *b, int bytes)
{
    mb->setValue(bytes / 1048576);
    kb->setValue((bytes % 1048576) / 1024);
    b ->setValue(bytes % 1024);
}
static int readSizeSpinners(QSpinBox *mb, QSpinBox *kb, QSpinBox *b)
{
    return mb->value() * 1048576 + kb->value() * 1024 + b->value();
}

// -----------------------------------------------------------------------------

bool PageAccess::editSpecDialog(AccessSpec &spec, const QString &title)
{
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setMinimumSize(730, 560);

    // Working copy of lines - edited locally until OK
    QList<AccessSpecLine> lines = spec.lines;
    if (lines.isEmpty()) {
        AccessSpecLine l; l.sizeBytes = 65536; l.ofSize = 100;
        lines.append(l);
    }

    bool updating = false;

    // -- Top row: Name + Default Assignment ----------------------------------
    auto *nameEdit    = new QLineEdit(spec.name);
    auto *defCombo    = new QComboBox;
    defCombo->addItems({"None", "All Managers"});
    defCombo->setCurrentIndex(spec.defaultSpec ? 1 : 0);

    // -- Access-line table ----------------------------------------------------
    auto *table = new QTableWidget(0, 8);
    table->setHorizontalHeaderLabels({"Size", "% Access", "% Read", "% Random",
                                      "Delay", "Burst", "Alignment", "Reply"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    // Compact row height: the original Windows list control uses ~14px rows.
    // Qt's default is ~28-30px because of cell padding; override both the
    // section size AND the item padding via stylesheet.
    table->verticalHeader()->hide();
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->verticalHeader()->setMinimumSectionSize(1);  // remove font-height floor
    table->verticalHeader()->setDefaultSectionSize(14);
    table->setStyleSheet("QTableWidget::item { padding: 0px 3px; }");
    table->setStyle(QStyleFactory::create("Fusion"));
    table->setFixedHeight(175);

    auto *insBefBtn = new QPushButton("Insert Before");
    auto *insAftBtn = new QPushButton("Insert After");
    auto *delBtn    = new QPushButton("Delete");

    // -- Transfer Request Size ---------------------------------------------
    QSpinBox *szMb, *szKb, *szB;
    auto *szLay = makeSizeSpinners(&szMb, &szKb, &szB);

    // -- % of Access Specification -----------------------------------------
    auto *ofSzSlider = new QSlider(Qt::Horizontal); ofSzSlider->setRange(0,100);
    auto *ofSzLbl    = new QLabel("100 Percent");

    // -- % Read/Write ------------------------------------------------------
    auto *rdSlider = new QSlider(Qt::Horizontal); rdSlider->setRange(0,100);

    // -- % Random/Sequential -----------------------------------------------
    auto *rndSlider = new QSlider(Qt::Horizontal); rndSlider->setRange(0,100);

    // -- Burstiness --------------------------------------------------------
    auto *delaySpin = new QDoubleSpinBox;
    delaySpin->setRange(0, 60000); delaySpin->setSuffix(" ms"); delaySpin->setFixedWidth(80);
    auto *burstSpin = new QSpinBox;
    burstSpin->setRange(1, 65536); burstSpin->setSuffix(" I/Os"); burstSpin->setFixedWidth(80);

    // -- Align I/Os on -----------------------------------------------------
    auto *alReqRad  = new QRadioButton("Request Size Boundaries");
    auto *alSectRad = new QRadioButton("Sector Boundaries");
    auto *alCustRad = new QRadioButton;
    auto *alBtnGrp  = new QButtonGroup(&dlg);
    alBtnGrp->addButton(alReqRad, 0);
    alBtnGrp->addButton(alSectRad, 1);
    alBtnGrp->addButton(alCustRad, 2);
    alSectRad->setChecked(true);
    QSpinBox *alMb, *alKb, *alB;
    auto *alLay = makeSizeSpinners(&alMb, &alKb, &alB);

    // -- Reply Size --------------------------------------------------------
    auto *rpNoneRad = new QRadioButton("No Reply");
    auto *rpCustRad = new QRadioButton;
    auto *rpBtnGrp  = new QButtonGroup(&dlg);
    rpBtnGrp->addButton(rpNoneRad, 0);
    rpBtnGrp->addButton(rpCustRad, 1);
    rpNoneRad->setChecked(true);
    QSpinBox *rpMb, *rpKb, *rpB;
    auto *rpLay = makeSizeSpinners(&rpMb, &rpKb, &rpB);

    // -- Helpers: populate table & sync controls ---------------------------
    auto populateTable = [&]() {
        bool wasBlocked = table->blockSignals(true);
        int cur = table->currentRow();
        table->setRowCount(lines.size());
        for (int r = 0; r < lines.size(); ++r) {
            const auto &l = lines[r];
            QString align;
            if      (l.alignBytes < 0) align = "req size";
            else if (l.alignBytes == 0) align = "sector";
            else    align = formatSizeCompact(l.alignBytes);
            table->setItem(r, 0, new QTableWidgetItem(formatSizeCompact(l.sizeBytes)));
            table->setItem(r, 1, new QTableWidgetItem(QString::number(l.ofSize)));
            table->setItem(r, 2, new QTableWidgetItem(QString::number(l.readPercent)));
            table->setItem(r, 3, new QTableWidgetItem(QString::number(100 - l.seqPercent)));
            table->setItem(r, 4, new QTableWidgetItem(QString::number(l.delayMs, 'f', 0)));
            table->setItem(r, 5, new QTableWidgetItem(QString::number(l.burstLength)));
            table->setItem(r, 6, new QTableWidgetItem(align));
            table->setItem(r, 7, new QTableWidgetItem(l.replyBytes == 0 ? "none"
                                     : formatSizeCompact(l.replyBytes)));
            table->setRowHeight(r, 14);
        }
        table->blockSignals(wasBlocked);
        if (cur >= 0 && cur < lines.size()) table->selectRow(cur);
    };

    auto loadControls = [&](int row) {
        if (updating || row < 0 || row >= lines.size()) return;
        updating = true;
        const auto &l = lines[row];
        setSizeSpinners(szMb, szKb, szB, l.sizeBytes);
        ofSzSlider->setValue(l.ofSize);
        ofSzLbl->setText(QString("%1 Percent").arg(l.ofSize));
        rdSlider->setValue(l.readPercent);
        rndSlider->setValue(100 - l.seqPercent);
        delaySpin->setValue(l.delayMs);
        burstSpin->setValue(l.burstLength);
        if      (l.alignBytes < 0)  alReqRad->setChecked(true);
        else if (l.alignBytes == 0) alSectRad->setChecked(true);
        else {
            alCustRad->setChecked(true);
            setSizeSpinners(alMb, alKb, alB, l.alignBytes);
        }
        rpNoneRad->setChecked(l.replyBytes == 0);
        rpCustRad->setChecked(l.replyBytes > 0);
        if (l.replyBytes > 0) setSizeSpinners(rpMb, rpKb, rpB, l.replyBytes);
        updating = false;
    };

    auto saveControls = [&](int row) {
        if (updating || row < 0 || row >= lines.size()) return;
        auto &l = lines[row];
        l.sizeBytes   = readSizeSpinners(szMb, szKb, szB);
        l.ofSize      = ofSzSlider->value();
        l.readPercent = rdSlider->value();
        l.seqPercent  = 100 - rndSlider->value();
        l.delayMs     = delaySpin->value();
        l.burstLength = burstSpin->value();
        if      (alReqRad->isChecked())  l.alignBytes = -1;
        else if (alSectRad->isChecked()) l.alignBytes =  0;
        else    l.alignBytes = readSizeSpinners(alMb, alKb, alB);
        if (rpNoneRad->isChecked()) l.replyBytes = 0;
        else    l.replyBytes = readSizeSpinners(rpMb, rpKb, rpB);
        populateTable();
    };

    // -- Connections -------------------------------------------------------
    QObject::connect(table, &QTableWidget::currentCellChanged,
        [&](int newRow, int, int oldRow, int) {
            if (!updating) saveControls(oldRow);
            loadControls(newRow);
        });

    // Any control change → save back to selected line
    auto onCtrlChanged = [&]() { saveControls(table->currentRow()); };
    QObject::connect(szMb,  QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(szKb,  QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(szB,   QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(ofSzSlider, &QSlider::valueChanged, &dlg, [&](int v) {
        ofSzLbl->setText(QString("%1 Percent").arg(v));
        onCtrlChanged();
    });
    QObject::connect(rdSlider,  &QSlider::valueChanged,  &dlg, onCtrlChanged);
    QObject::connect(rndSlider, &QSlider::valueChanged,  &dlg, onCtrlChanged);
    QObject::connect(delaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(burstSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(alReqRad,  &QRadioButton::toggled, &dlg, onCtrlChanged);
    QObject::connect(alSectRad, &QRadioButton::toggled, &dlg, onCtrlChanged);
    QObject::connect(alCustRad, &QRadioButton::toggled, &dlg, onCtrlChanged);
    QObject::connect(alMb, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(alKb, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(alB,  QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(rpNoneRad, &QRadioButton::toggled, &dlg, onCtrlChanged);
    QObject::connect(rpCustRad, &QRadioButton::toggled, &dlg, onCtrlChanged);
    QObject::connect(rpMb, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(rpKb, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);
    QObject::connect(rpB,  QOverload<int>::of(&QSpinBox::valueChanged), &dlg, onCtrlChanged);

    // Insert / Delete row buttons
    QObject::connect(insBefBtn, &QPushButton::clicked, &dlg, [&]() {
        int r = table->currentRow();
        if (r < 0) r = 0;
        AccessSpecLine l; l.ofSize = 100;
        lines.insert(r, l);
        populateTable();
        table->selectRow(r);
    });
    QObject::connect(insAftBtn, &QPushButton::clicked, &dlg, [&]() {
        int r = table->currentRow();
        int ins = (r < 0) ? lines.size() : r + 1;
        AccessSpecLine l; l.ofSize = 100;
        lines.insert(ins, l);
        populateTable();
        table->selectRow(ins);
    });
    QObject::connect(delBtn, &QPushButton::clicked, &dlg, [&]() {
        int r = table->currentRow();
        if (r < 0 || lines.size() <= 1) return;  // keep at least one line
        lines.removeAt(r);
        populateTable();
        table->selectRow(qMin(r, lines.size() - 1));
    });

    // -- Layout -----------------------------------------------------------
    auto *root = new QVBoxLayout(&dlg);
    root->setSpacing(6);

    // Top: Name + Default Assignment
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Name:"));
        row->addWidget(nameEdit, 1);
        row->addSpacing(12);
        row->addWidget(new QLabel("Default Assignment:"));
        row->addWidget(defCombo);
        root->addLayout(row);
    }

    // Table + buttons
    {
        auto *row = new QHBoxLayout;
        row->addWidget(table, 1);
        auto *btnCol = new QVBoxLayout;
        btnCol->addWidget(insBefBtn);
        btnCol->addWidget(insAftBtn);
        btnCol->addWidget(delBtn);
        btnCol->addStretch();
        row->addLayout(btnCol);
        root->addLayout(row);
    }

    // Editing controls - 3×2 grid of group boxes
    {
        auto *grid = new QGridLayout;
        grid->setSpacing(6);

        // -- Transfer Request Size ----------------------------------------
        auto *szGrp = new QGroupBox("Transfer Request Size");
        auto *szGL  = new QVBoxLayout(szGrp);
        szGL->addLayout(szLay);
        grid->addWidget(szGrp, 0, 0);

        // -- % of Access Specification ---------------------------------
        auto *ofGrp = new QGroupBox("Percent of Access Specification");
        auto *ofGL  = new QVBoxLayout(ofGrp);
        ofGL->addWidget(ofSzSlider);
        ofGL->addWidget(ofSzLbl, 0, Qt::AlignCenter);
        grid->addWidget(ofGrp, 0, 1);

        // -- % Read/Write Distribution ---------------------------------
        auto *rdGrp = new QGroupBox("Percent Read/Write Distribution");
        auto *rdGL  = new QVBoxLayout(rdGrp);
        rdGL->addWidget(rdSlider);
        {
            auto *rl = new QHBoxLayout;
            rl->addWidget(new QLabel("0%\nWrite"));
            rl->addStretch();
            rl->addWidget(new QLabel("100%\nRead"));
            rdGL->addLayout(rl);
        }
        grid->addWidget(rdGrp, 0, 2);

        // -- % Random/Sequential ---------------------------------------
        auto *rndGrp = new QGroupBox("Percent Random/Sequential Distribution");
        auto *rndGL  = new QVBoxLayout(rndGrp);
        rndGL->addWidget(rndSlider);
        {
            auto *rl = new QHBoxLayout;
            rl->addWidget(new QLabel("100%\nSequential"));
            rl->addStretch();
            rl->addWidget(new QLabel("0%\nRandom"));
            rndGL->addLayout(rl);
        }
        grid->addWidget(rndGrp, 1, 0);

        // -- Burstiness -----------------------------------------------
        auto *bstGrp = new QGroupBox("Burstiness");
        auto *bstGL  = new QFormLayout(bstGrp);
        bstGL->addRow("Transfer Delay:", delaySpin);
        bstGL->addRow("Burst Length:",   burstSpin);
        grid->addWidget(bstGrp, 1, 1);

        // -- Align I/Os on --------------------------------------------
        auto *alGrp = new QGroupBox("Align I/Os on");
        auto *alGL  = new QVBoxLayout(alGrp);
        alGL->addWidget(alReqRad);
        alGL->addWidget(alSectRad);
        {
            auto *cr = new QHBoxLayout;
            cr->addWidget(alCustRad);
            cr->addLayout(alLay);
            alGL->addLayout(cr);
        }
        grid->addWidget(alGrp, 1, 2);

        root->addLayout(grid);
    }

    // Reply Size (below grid, left-aligned)
    {
        auto *rpGrp = new QGroupBox("Reply Size");
        auto *rpGL  = new QVBoxLayout(rpGrp);
        rpGL->addWidget(rpNoneRad);
        {
            auto *cr = new QHBoxLayout;
            cr->addWidget(rpCustRad);
            cr->addLayout(rpLay);
            rpGL->addLayout(cr);
        }
        auto *row = new QHBoxLayout;
        row->addWidget(rpGrp);
        row->addStretch(1);
        root->addLayout(row);
    }

    // OK / Cancel
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    root->addWidget(buttons);

    // -- Initialise -------------------------------------------------------
    populateTable();
    if (!lines.isEmpty()) {
        table->selectRow(0);
        loadControls(0);
    }

    if (dlg.exec() != QDialog::Accepted) return false;

    // Save the currently selected row before accepting
    saveControls(table->currentRow());

    spec.name        = nameEdit->text().trimmed();
    spec.defaultSpec = (defCombo->currentIndex() > 0);
    spec.lines       = lines;
    return true;
}

void PageAccess::onNewSpec()
{
    AccessSpec s;
    s.name = QString("New Spec %1").arg(m_engine->accessSpecs().size() + 1);
    AccessSpecLine l; l.sizeBytes = 4096; l.ofSize = 100;
    s.lines.append(l);
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
