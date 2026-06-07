// QtBigMeterWidget.cpp
// Qt port of CBigMeter - Iometer "Presentation Meter" window.
//
// Layout (top to bottom) matches the original:
//   1. Speedometer gauge - fills most of the window
//   2. Large numeric value with units  (blue, ~22pt bold)
//   3. Subtitle "All Managers - <metric name>"  (plain, smaller)
//   4. Separator
//   5. Two side-by-side groups: [Settings] | [Test Controls]

#include "QtBigMeterWidget.h"
#include "QtMeterWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFont>
#include <QSizePolicy>
#include <QFrame>
#include <QPalette>

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

QtBigMeterWidget::QtBigMeterWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setWindowTitle("Iometer - Presentation Meter");
    resize(600, 580);
}

void QtBigMeterWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(4);
    root->setContentsMargins(8, 8, 8, 8);

    // -- 1. Speedometer (takes up most of the space) ----------------------
    m_meter = new QtMeterWidget;
    m_meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_meter, 1);

    // -- 2. Large numeric readout with units ------------------------------
    m_valueLabel = new QLabel("0");
    QFont vf("Arial", 22, QFont::Bold);
    m_valueLabel->setFont(vf);
    m_valueLabel->setAlignment(Qt::AlignCenter);
    // Use palette for colour - avoids a global stylesheet
    QPalette vp = m_valueLabel->palette();
    vp.setColor(QPalette::WindowText, QColor(0x44, 0x88, 0xbb));
    m_valueLabel->setPalette(vp);
    root->addWidget(m_valueLabel);

    // -- 3. Subtitle: "All Managers - <metric>" ---------------------------
    // m_titleLabel kept as a (hidden) member for setTitle() back-compat;
    // setTitle() now routes to setWindowTitle() so we don't add it to the layout.
    m_titleLabel  = new QLabel;
    m_workerLabel = new QLabel;
    QFont wf("Arial", 10);
    m_workerLabel->setFont(wf);
    m_workerLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_workerLabel);

    // -- 4. Separator -----------------------------------------------------
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep);

    // -- 5. Bottom: [Settings] | [Test Controls] --------------------------
    auto *bottomRow = new QHBoxLayout;

    // Settings group
    auto *settingsGroup = new QGroupBox("Settings");
    auto *settingsLay   = new QVBoxLayout(settingsGroup);
    settingsLay->setSpacing(4);

    m_resultType = new QComboBox;
    m_resultType->addItems({
        "IOps",
        "MBps (Decimal)",
        "MBps (Binary)",
        "Read IOps",
        "Write IOps",
        "Read MBps (Dec)",
        "Write MBps (Dec)",
        "Average Latency (ms)",
        "Max Latency (ms)",
        "CPU Utilization (%)",
        "CPU User (%)",
        "CPU Kernel (%)",
    });
    settingsLay->addWidget(m_resultType);

    auto *rangeRow = new QHBoxLayout;
    auto *rangeLbl = new QLabel("Range");
    m_maxRangeSpin = new QSpinBox;
    m_maxRangeSpin->setRange(0, 10000000);
    m_maxRangeSpin->setValue(0);
    m_maxRangeSpin->setFixedWidth(80);
    m_watermarkChk = new QCheckBox("Show Trace");
    rangeRow->addWidget(rangeLbl);
    rangeRow->addWidget(m_maxRangeSpin);
    rangeRow->addSpacing(8);
    rangeRow->addWidget(m_watermarkChk);
    rangeRow->addStretch();
    settingsLay->addLayout(rangeRow);

    bottomRow->addWidget(settingsGroup, 1);

    // Test Controls group
    auto *ctrlGroup = new QGroupBox("Test Controls");
    auto *ctrlLay   = new QHBoxLayout(ctrlGroup);
    m_startNextBtn = new QPushButton("Start");
    m_stopBtn      = new QPushButton("Stop");
    ctrlLay->addStretch();
    ctrlLay->addWidget(m_startNextBtn);
    ctrlLay->addWidget(m_stopBtn);
    bottomRow->addWidget(ctrlGroup);

    root->addLayout(bottomRow);

    // -- Wire up signals ---------------------------------------------------
    connect(m_startNextBtn, &QPushButton::clicked,
            this,           &QtBigMeterWidget::onStartNextClicked);
    connect(m_stopBtn,      &QPushButton::clicked,
            this,           &QtBigMeterWidget::onStopClicked);
    connect(m_watermarkChk, &QCheckBox::toggled,
            this,           &QtBigMeterWidget::onWatermarkToggled);
    connect(m_maxRangeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this,           &QtBigMeterWidget::onMaxRangeChanged);
    connect(m_resultType,   &QComboBox::currentTextChanged,
            this,           &QtBigMeterWidget::onResultTypeChanged);

    // Initialise the meter and button states
    m_meter->setRange(0, 100, true);
    updateButtons();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void QtBigMeterWidget::setTitle(const QString &testTitle)
{
    // Route to the OS window title bar - no separate in-window title label.
    setWindowTitle(testTitle.isEmpty() ? "Iometer - Presentation Meter" : testTitle);
}

void QtBigMeterWidget::setWorkerResult(const QString &workerName,
                                     const QString &resultName)
{
    m_workerName = workerName;
    m_resultName = resultName;

    if (workerName.isEmpty())
        m_workerLabel->setText(resultName);
    else if (resultName.isEmpty())
        m_workerLabel->setText(workerName);
    else
        m_workerLabel->setText(workerName + " - " + resultName);  // em-dash

    // Sync the combo box without triggering the resultTypeChanged signal
    if (!resultName.isEmpty()) {
        const int idx = m_resultType->findText(resultName);
        if (idx >= 0) {
            QSignalBlocker guard(m_resultType);
            m_resultType->setCurrentIndex(idx);
        }
    }
}

void QtBigMeterWidget::updateDisplay(double value, const QString &formattedText)
{
    m_meter->setValue(value);
    m_valueLabel->setText(
        formattedText.isEmpty()
            ? QString::number(value, 'f', (value < 100.0 ? 1 : 0))
            : formattedText);
}

void QtBigMeterWidget::resetWatermark()
{
    m_meter->resetWatermark();
}

void QtBigMeterWidget::setButtonState(bool canStart, bool canStop, bool canStopAll)
{
    m_canStart   = canStart;
    m_canStop    = canStop;
    m_canStopAll = canStopAll;
    updateButtons();
}

// -----------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------

void QtBigMeterWidget::updateButtons()
{
    // Logic mirrors CBigMeter::UpdateButtons
    if (m_canStart) {
        m_startNextBtn->setText("Start");
        m_startNextBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
    } else if (m_canStop || m_canStopAll) {
        m_startNextBtn->setText("Next >>");
        m_startNextBtn->setEnabled(m_canStopAll);
        m_stopBtn->setEnabled(true);
    } else {
        m_startNextBtn->setText("Start");
        m_startNextBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
    }
}

// -----------------------------------------------------------------------------
// Slots
// -----------------------------------------------------------------------------

void QtBigMeterWidget::onStartNextClicked()
{
    if (m_canStart) {
        m_meter->resetWatermark();
        emit startRequested();
    } else {
        emit nextRequested();
    }
}

void QtBigMeterWidget::onStopClicked()
{
    emit stopRequested();
}

void QtBigMeterWidget::onWatermarkToggled(bool checked)
{
    m_meter->showWatermark = checked;
    m_meter->resetWatermark();
    m_meter->update();
}

void QtBigMeterWidget::onMaxRangeChanged(int value)
{
    if (value == 0)
        m_meter->setRange(0, 100, /*auto=*/true);
    else
        m_meter->setRange(0, value, /*auto=*/false);
}

void QtBigMeterWidget::onResultTypeChanged(const QString &text)
{
    m_resultName = text;
    setWorkerResult(m_workerName, text);   // update the subtitle label
    emit resultTypeChanged(text);
}
