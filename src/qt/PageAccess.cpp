// PageAccess.cpp
#include "PageAccess.h"
#include "IometerEngine.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

static const QList<QPair<QString,int>> XFER_SIZES = {
    {"512 B",    512},  {"1 KiB",    1024},  {"2 KiB",  2048},
    {"4 KiB",   4096},  {"8 KiB",   8192},   {"16 KiB", 16384},
    {"32 KiB", 32768},  {"64 KiB", 65536},   {"128 KiB",131072},
    {"256 KiB",262144}, {"512 KiB",524288},  {"1 MiB", 1048576}
};

static const QString STYLE =
    "PageAccess { background:#111820; color:#ccddff; }"
    "QGroupBox { border:1px solid #2a3a4a; border-radius:4px; margin-top:8px; "
    "            color:#7799bb; font-weight:bold; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 4px; }"
    "QListWidget { background:#0d1520; color:#ccddff; border:1px solid #2a3a4a; }"
    "QListWidget::item:selected { background:#1a3a6a; }"
    "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox "
    "    { background:#162030; color:#ccddff; border:1px solid #2a3a4a; "
    "      border-radius:3px; padding:2px 4px; }"
    "QCheckBox { color:#ccddff; }"
    "QLabel { color:#ccddff; }"
    "QPushButton { background:#2a3a5e; color:#aaddff; border:1px solid #3a5a8e; "
    "              border-radius:4px; padding:3px 8px; }"
    "QPushButton:hover { background:#3a5a8e; }"
    "QPushButton:disabled { background:#1a2a3e; color:#445566; }";

PageAccess::PageAccess(IometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    loadSpecList();
    connect(engine, &IometerEngine::configChanged, this, &PageAccess::loadSpecList);
}

void PageAccess::populateXferSizes(QComboBox *cb)
{
    cb->clear();
    for (const auto &p : XFER_SIZES) cb->addItem(p.first, p.second);
}

void PageAccess::setupUi()
{
    setStyleSheet(STYLE);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ── Left: spec list + buttons ─────────────────────────────────────────
    auto *leftGroup = new QGroupBox("Access Specifications");
    auto *leftLay   = new QVBoxLayout(leftGroup);

    m_specList = new QListWidget;
    m_specList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLay->addWidget(m_specList, 1);

    auto *btnRow = new QHBoxLayout;
    m_addBtn    = new QPushButton("+");
    m_removeBtn = new QPushButton("−");
    m_dupBtn    = new QPushButton("⎘");
    m_upBtn     = new QPushButton("↑");
    m_downBtn   = new QPushButton("↓");
    m_addBtn->setToolTip("New spec");
    m_removeBtn->setToolTip("Delete selected");
    m_dupBtn->setToolTip("Duplicate");
    m_upBtn->setToolTip("Move up");
    m_downBtn->setToolTip("Move down");
    for (auto *b : {m_addBtn, m_removeBtn, m_dupBtn, m_upBtn, m_downBtn})
        b->setFixedWidth(28);
    m_removeBtn->setEnabled(false);
    m_dupBtn->setEnabled(false);
    m_upBtn->setEnabled(false);
    m_downBtn->setEnabled(false);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addWidget(m_dupBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_upBtn);
    btnRow->addWidget(m_downBtn);
    leftLay->addLayout(btnRow);

    root->addWidget(leftGroup, 1);

    // ── Separator ─────────────────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color:#2a3a4a;");
    root->addWidget(sep);

    // ── Right: spec editor ────────────────────────────────────────────────
    auto *rightGroup = new QGroupBox("Specification Editor");
    auto *form       = new QFormLayout(rightGroup);
    form->setSpacing(6);
    form->setLabelAlignment(Qt::AlignRight);

    m_nameEdit   = new QLineEdit;
    m_xferSize   = new QComboBox; populateXferSizes(m_xferSize);
    m_alignSize  = new QComboBox; populateXferSizes(m_alignSize);
    m_readPct    = new QSpinBox;  m_readPct->setRange(0, 100);  m_readPct->setSuffix(" %");
    m_seqPct     = new QSpinBox;  m_seqPct->setRange(0, 100);   m_seqPct->setSuffix(" %");
    m_burstLen   = new QSpinBox;  m_burstLen->setRange(1, 1024);
    m_delayMs    = new QDoubleSpinBox; m_delayMs->setRange(0, 60000); m_delayMs->setSuffix(" ms");
    m_iterations = new QSpinBox;  m_iterations->setRange(0, 1000000);
    m_iterations->setSpecialValueText("∞ (unlimited)");
    m_defaultSpec = new QCheckBox("Default specification");

    m_descLabel = new QLabel;
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("color:#668899; font-style:italic; font-size:10px;");

    form->addRow("Name:",           m_nameEdit);
    form->addRow("Transfer size:",  m_xferSize);
    form->addRow("Alignment:",      m_alignSize);
    form->addRow("Read %:",         m_readPct);
    form->addRow("Sequential %:",   m_seqPct);
    form->addRow("Burst length:",   m_burstLen);
    form->addRow("Delay:",          m_delayMs);
    form->addRow("Iterations:",     m_iterations);
    form->addRow("",                m_defaultSpec);
    form->addRow("Description:",    m_descLabel);

    root->addWidget(rightGroup, 2);

    // ── Connections ────────────────────────────────────────────────────────
    connect(m_specList,  &QListWidget::itemClicked,     this, &PageAccess::onSpecSelected);
    connect(m_addBtn,    &QPushButton::clicked,         this, &PageAccess::onAddSpec);
    connect(m_removeBtn, &QPushButton::clicked,         this, &PageAccess::onRemoveSpec);
    connect(m_dupBtn,    &QPushButton::clicked,         this, &PageAccess::onDuplicateSpec);
    connect(m_upBtn,     &QPushButton::clicked,         this, &PageAccess::onMoveUp);
    connect(m_downBtn,   &QPushButton::clicked,         this, &PageAccess::onMoveDown);

    // Any change in the editor saves back
    connect(m_nameEdit,   &QLineEdit::textEdited,            this, &PageAccess::onSpecEdited);
    connect(m_xferSize,   QOverload<int>::of(&QComboBox::currentIndexChanged),  this, &PageAccess::onSpecEdited);
    connect(m_alignSize,  QOverload<int>::of(&QComboBox::currentIndexChanged),  this, &PageAccess::onSpecEdited);
    connect(m_readPct,    QOverload<int>::of(&QSpinBox::valueChanged),          this, &PageAccess::onSpecEdited);
    connect(m_seqPct,     QOverload<int>::of(&QSpinBox::valueChanged),          this, &PageAccess::onSpecEdited);
    connect(m_burstLen,   QOverload<int>::of(&QSpinBox::valueChanged),          this, &PageAccess::onSpecEdited);
    connect(m_delayMs,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageAccess::onSpecEdited);
    connect(m_iterations, QOverload<int>::of(&QSpinBox::valueChanged),          this, &PageAccess::onSpecEdited);
    connect(m_defaultSpec, &QCheckBox::toggled,                                 this, &PageAccess::onSpecEdited);
}

void PageAccess::loadSpecList()
{
    const int savedRow = m_specList->currentRow();
    m_specList->clear();
    for (const auto &s : m_engine->accessSpecs()) {
        auto *item = new QListWidgetItem(s.defaultSpec ? "[Default] " + s.name : s.name);
        if (s.defaultSpec) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            item->setForeground(QColor(0x44, 0xaa, 0xff));
        }
        m_specList->addItem(item);
    }
    const int row = qBound(0, savedRow, m_specList->count() - 1);
    if (m_specList->count() > 0) {
        m_specList->setCurrentRow(row);
        loadSpecEditor(row);
    }
}

void PageAccess::loadSpecEditor(int index)
{
    const auto specs = m_engine->accessSpecs();
    if (index < 0 || index >= specs.size()) return;
    m_currentIndex = index;
    m_updating = true;

    const auto &s = specs[index];
    m_nameEdit->setText(s.name);

    // Find transfer size in combo
    for (int i = 0; i < XFER_SIZES.size(); ++i)
        if (XFER_SIZES[i].second == s.xferSizeBytes) { m_xferSize->setCurrentIndex(i); break; }
    for (int i = 0; i < XFER_SIZES.size(); ++i)
        if (XFER_SIZES[i].second == s.alignBytes)    { m_alignSize->setCurrentIndex(i); break; }

    m_readPct->setValue(s.readPercent);
    m_seqPct->setValue(s.seqPercent);
    m_burstLen->setValue(s.burstLength);
    m_delayMs->setValue(s.delayMs);
    m_iterations->setValue(s.iterations);
    m_defaultSpec->setChecked(s.defaultSpec);

    // Build a description
    const QString rwStr  = (s.readPercent == 100) ? "Read-only" :
                           (s.readPercent == 0)   ? "Write-only" :
                           QString("%1% Read / %2% Write").arg(s.readPercent).arg(100 - s.readPercent);
    const QString seqStr = (s.seqPercent == 100) ? "Sequential" :
                           (s.seqPercent == 0)   ? "Random" :
                           QString("%1% Sequential").arg(s.seqPercent);
    m_descLabel->setText(QString("%1 — %2 — %3")
        .arg(XFER_SIZES.isEmpty() ? "?" :
             [&]{ for (const auto &p : XFER_SIZES) if (p.second == s.xferSizeBytes) return p.first; return QString(); }())
        .arg(rwStr).arg(seqStr));

    m_updating = false;

    const bool enabled = (index >= 0);
    for (auto *w : QList<QWidget*>{m_nameEdit, m_xferSize, m_alignSize, m_readPct,
                                    m_seqPct, m_burstLen, m_delayMs, m_iterations, m_defaultSpec})
        w->setEnabled(enabled);
    m_removeBtn->setEnabled(enabled);
    m_dupBtn->setEnabled(enabled);
    m_upBtn->setEnabled(index > 0);
    m_downBtn->setEnabled(index < specs.size() - 1);
}

void PageAccess::onSpecSelected(QListWidgetItem *)
{
    loadSpecEditor(m_specList->currentRow());
}

void PageAccess::saveCurrentSpec()
{
    if (m_updating || m_currentIndex < 0) return;
    auto specs = m_engine->accessSpecs();
    if (m_currentIndex >= specs.size()) return;

    auto &s = specs[m_currentIndex];
    s.name        = m_nameEdit->text();
    s.xferSizeBytes = XFER_SIZES[m_xferSize->currentIndex()].second;
    s.alignBytes    = XFER_SIZES[m_alignSize->currentIndex()].second;
    s.readPercent   = m_readPct->value();
    s.seqPercent    = m_seqPct->value();
    s.burstLength   = m_burstLen->value();
    s.delayMs       = m_delayMs->value();
    s.iterations    = m_iterations->value();
    s.defaultSpec   = m_defaultSpec->isChecked();

    m_engine->setAccessSpecs(specs);
    // Update the list item label without re-loading
    auto *item = m_specList->item(m_currentIndex);
    if (item) item->setText(s.defaultSpec ? "[Default] " + s.name : s.name);
}

void PageAccess::onSpecEdited()
{
    saveCurrentSpec();
}

void PageAccess::onAddSpec()
{
    auto specs = m_engine->accessSpecs();
    AccessSpec s;
    s.name = QString("New Spec %1").arg(specs.size() + 1);
    specs.append(s);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_specList->setCurrentRow(specs.size() - 1);
    loadSpecEditor(specs.size() - 1);
}

void PageAccess::onRemoveSpec()
{
    if (m_currentIndex < 0) return;
    auto specs = m_engine->accessSpecs();
    specs.removeAt(m_currentIndex);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
}

void PageAccess::onDuplicateSpec()
{
    if (m_currentIndex < 0) return;
    auto specs = m_engine->accessSpecs();
    AccessSpec dup = specs[m_currentIndex];
    dup.name += " (copy)";
    dup.defaultSpec = false;
    specs.insert(m_currentIndex + 1, dup);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_specList->setCurrentRow(m_currentIndex + 1);
    loadSpecEditor(m_currentIndex + 1);
}

void PageAccess::onMoveUp()
{
    if (m_currentIndex <= 0) return;
    auto specs = m_engine->accessSpecs();
    specs.swapItemsAt(m_currentIndex, m_currentIndex - 1);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_specList->setCurrentRow(m_currentIndex - 1);
    loadSpecEditor(m_currentIndex - 1);
}

void PageAccess::onMoveDown()
{
    auto specs = m_engine->accessSpecs();
    if (m_currentIndex < 0 || m_currentIndex >= specs.size() - 1) return;
    specs.swapItemsAt(m_currentIndex, m_currentIndex + 1);
    m_engine->setAccessSpecs(specs);
    loadSpecList();
    m_specList->setCurrentRow(m_currentIndex + 1);
    loadSpecEditor(m_currentIndex + 1);
}
