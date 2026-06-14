// QtPageResults.cpp -- "Test Setup" tab
#include "QtPageResults.h"
#include "QtIometerEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>
#include <QCheckBox>
#include <QFrame>

// =============================================================================

QtPageResults::QtPageResults(QtIometerEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    loadConfig();
    connect(engine, &QtIometerEngine::configChanged, this, &QtPageResults::onConfigChanged);
}

// =============================================================================
// UI construction -- matches the original Test Setup tab exactly
// =============================================================================

void QtPageResults::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // ---- Test Description ---------------------------------------------------
    auto *descGroup = new QGroupBox("Test Description");
    auto *descLay   = new QHBoxLayout(descGroup);
    m_description = new QLineEdit;
    descLay->addWidget(m_description);
    root->addWidget(descGroup);

    // ---- Row: Run Time | Ramp Up Time | Number of Workers -------------------
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(8);

    // Run Time
    auto *runGroup = new QGroupBox("Run Time");
    auto *runLay   = new QGridLayout(runGroup);
    runLay->setSpacing(4);
    m_runHours   = new QSpinBox; m_runHours->setRange(0, 99);    m_runHours->setFixedWidth(40);
    m_runMinutes = new QSpinBox; m_runMinutes->setRange(0, 59);  m_runMinutes->setFixedWidth(40);
    m_runSeconds = new QSpinBox; m_runSeconds->setRange(0, 59);  m_runSeconds->setFixedWidth(40);
    runLay->addWidget(m_runHours,   0, 0); runLay->addWidget(new QLabel("Hours"),   0, 1);
    runLay->addWidget(m_runMinutes, 1, 0); runLay->addWidget(new QLabel("Minutes"), 1, 1);
    runLay->addWidget(m_runSeconds, 2, 0); runLay->addWidget(new QLabel("Seconds"), 2, 1);
    row1->addWidget(runGroup);

    // Ramp Up Time + Record Results (combined column)
    auto *rampCol = new QVBoxLayout;
    rampCol->setSpacing(4);
    auto *rampGroup = new QGroupBox("Ramp Up Time");
    auto *rampLay   = new QHBoxLayout(rampGroup);
    m_rampSeconds = new QSpinBox; m_rampSeconds->setRange(0, 9999); m_rampSeconds->setFixedWidth(50);
    rampLay->addWidget(m_rampSeconds);
    rampLay->addWidget(new QLabel("Seconds"));
    rampLay->addStretch();
    rampCol->addWidget(rampGroup);
    auto *recGroup = new QGroupBox("Record Results");
    auto *recLay   = new QHBoxLayout(recGroup);
    m_recordResults = new QComboBox;
    m_recordResults->addItems({"All", "First Run", "Last Run", "None"});
    recLay->addWidget(m_recordResults);
    recLay->addStretch();
    rampCol->addWidget(recGroup);
    row1->addLayout(rampCol);

    row1->addStretch(1);
    root->addLayout(row1);

    // ---- Cycling Options ---------------------------------------------------
    auto *cycGroup = new QGroupBox("Cycling Options");
    auto *cycLay   = new QVBoxLayout(cycGroup);
    cycLay->setSpacing(4);

    m_cyclingMode = new QComboBox;
    m_cyclingMode->addItems({
        "Normal -- run all selected targets for all workers.",
        "Cycle worker count",
        "Cycle target count",
        "Cycle outstanding I/Os"
    });
    cycLay->addWidget(m_cyclingMode);

    // Sub-row: Workers | Targets | # Outstanding I/Os
    auto *cycRow = new QHBoxLayout;
    cycRow->setSpacing(12);

    // Workers sub-group
    auto *wGrp = new QGroupBox("Workers");
    auto *wLay = new QGridLayout(wGrp);
    wLay->setSpacing(3);
    m_workerStart    = new QSpinBox; m_workerStart->setRange(1, 9999);    m_workerStart->setFixedWidth(50);
    m_workerStep     = new QSpinBox; m_workerStep->setRange(1, 9999);     m_workerStep->setFixedWidth(50);
    m_workerStepping = new QComboBox; m_workerStepping->addItems({"Linear Stepping", "Exponential Stepping"});
    wLay->addWidget(new QLabel("Start"), 0, 0);
    wLay->addWidget(m_workerStart,       0, 1);
    wLay->addWidget(new QLabel("Step"),  1, 0);
    wLay->addWidget(m_workerStep,        1, 1);
    wLay->addWidget(m_workerStepping,    2, 0, 1, 2);
    cycRow->addWidget(wGrp);

    // Targets sub-group
    auto *tGrp = new QGroupBox("Targets");
    auto *tLay = new QGridLayout(tGrp);
    tLay->setSpacing(3);
    m_targetStart    = new QSpinBox; m_targetStart->setRange(1, 9999);    m_targetStart->setFixedWidth(50);
    m_targetStep     = new QSpinBox; m_targetStep->setRange(1, 9999);     m_targetStep->setFixedWidth(50);
    m_targetStepping = new QComboBox; m_targetStepping->addItems({"Linear Stepping", "Exponential Stepping"});
    tLay->addWidget(new QLabel("Start"), 0, 0);
    tLay->addWidget(m_targetStart,       0, 1);
    tLay->addWidget(new QLabel("Step"),  1, 0);
    tLay->addWidget(m_targetStep,        1, 1);
    tLay->addWidget(m_targetStepping,    2, 0, 1, 2);
    cycRow->addWidget(tGrp);

    // # Outstanding I/Os sub-group
    auto *iGrp = new QGroupBox("# of Outstanding I/Os");
    auto *iLay = new QGridLayout(iGrp);
    iLay->setSpacing(3);
    m_ioqStart   = new QSpinBox; m_ioqStart->setRange(1, 9999);   m_ioqStart->setFixedWidth(50);
    m_ioqEnd     = new QSpinBox; m_ioqEnd->setRange(1, 9999);     m_ioqEnd->setFixedWidth(50);
    m_ioqPower   = new QSpinBox; m_ioqPower->setRange(1, 64);     m_ioqPower->setFixedWidth(50);
    m_ioqStepping= new QComboBox; m_ioqStepping->addItems({"Linear Stepping", "Exponential Stepping"});
    iLay->addWidget(new QLabel("Start"), 0, 0); iLay->addWidget(m_ioqStart,   0, 1);
    iLay->addWidget(new QLabel("End"),   1, 0); iLay->addWidget(m_ioqEnd,     1, 1);
    iLay->addWidget(new QLabel("Power"), 2, 0); iLay->addWidget(m_ioqPower,   2, 1);
    iLay->addWidget(m_ioqStepping, 3, 0, 1, 2);
    cycRow->addWidget(iGrp);
    cycRow->addStretch();

    cycLay->addLayout(cycRow);
    root->addWidget(cycGroup);

    root->addStretch();

    // ---- Wire up all controls -----------------------------------------------
    auto save = [this](){ saveConfig(); };
    connect(m_description, &QLineEdit::editingFinished, this, save);
    connect(m_runHours,    QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_runMinutes,  QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_runSeconds,  QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_rampSeconds, QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_recordResults, QOverload<int>::of(&QComboBox::currentIndexChanged), this, save);
    connect(m_cyclingMode,   QOverload<int>::of(&QComboBox::currentIndexChanged), this, save);
    connect(m_workerStart,   QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_workerStep,    QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_workerStepping,QOverload<int>::of(&QComboBox::currentIndexChanged), this, save);
    connect(m_targetStart,   QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_targetStep,    QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_targetStepping,QOverload<int>::of(&QComboBox::currentIndexChanged), this, save);
    connect(m_ioqStart,  QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_ioqEnd,    QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_ioqPower,  QOverload<int>::of(&QSpinBox::valueChanged), this, save);
    connect(m_ioqStepping, QOverload<int>::of(&QComboBox::currentIndexChanged), this, save);
}

// =============================================================================
// Config load/save
// =============================================================================

void QtPageResults::loadConfig()
{
    m_updating = true;
    const TestConfig &c = m_engine->testConfig();
    m_description->setText(QString::fromStdString(c.description));
    m_runHours->setValue(c.runHours);
    m_runMinutes->setValue(c.runMinutes);
    m_runSeconds->setValue(c.runSeconds);
    m_rampSeconds->setValue(c.rampSeconds);
    m_recordResults->setCurrentIndex(c.recordResults);
    m_cyclingMode->setCurrentIndex(c.cyclingMode);
    m_workerStart->setValue(c.workerStart);
    m_workerStep->setValue(c.workerStep);
    m_workerStepping->setCurrentIndex(c.workerStepping);
    m_targetStart->setValue(c.targetStart);
    m_targetStep->setValue(c.targetStep);
    m_targetStepping->setCurrentIndex(c.targetStepping);
    m_ioqStart->setValue(c.ioqStart);
    m_ioqEnd->setValue(c.ioqEnd);
    m_ioqPower->setValue(c.ioqPower);
    m_ioqStepping->setCurrentIndex(c.ioqStepping);
    m_updating = false;
}

void QtPageResults::saveConfig()
{
    if (m_updating) return;
    TestConfig c;
    c.description   = m_description->text().toStdString();
    c.runHours      = m_runHours->value();
    c.runMinutes    = m_runMinutes->value();
    c.runSeconds    = m_runSeconds->value();
    c.rampSeconds   = m_rampSeconds->value();
    c.recordResults = m_recordResults->currentIndex();
    c.cyclingMode   = m_cyclingMode->currentIndex();
    c.workerStart   = m_workerStart->value();
    c.workerStep    = m_workerStep->value();
    c.workerStepping= m_workerStepping->currentIndex();
    c.targetStart   = m_targetStart->value();
    c.targetStep    = m_targetStep->value();
    c.targetStepping= m_targetStepping->currentIndex();
    c.ioqStart      = m_ioqStart->value();
    c.ioqEnd        = m_ioqEnd->value();
    c.ioqPower      = m_ioqPower->value();
    c.ioqStepping   = m_ioqStepping->currentIndex();
    m_engine->setTestConfig(c);
}

void QtPageResults::onConfigChanged() { loadConfig(); }
