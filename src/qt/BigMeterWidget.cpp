// BigMeterWidget.cpp
// Qt port of CBigMeter — Iometer "Presentation Meter" window.

#include "BigMeterWidget.h"
#include "MeterWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QFont>
#include <QSizePolicy>
#include <QFrame>

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

BigMeterWidget::BigMeterWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setWindowTitle("Iometer — Presentation Meter");
    resize(440, 520);
}

void BigMeterWidget::setupUi()
{
    // Dark background to match the speedometer aesthetic
    setStyleSheet(
        "BigMeterWidget { background: #1a1a2e; }"
        "QLabel { color: #ccddff; }"
        "QPushButton { background: #2a3a5e; color: #ccddff; border: 1px solid #4a6a8e;"
        "              border-radius: 4px; padding: 4px 12px; min-width: 70px; }"
        "QPushButton:hover    { background: #3a5a8e; }"
        "QPushButton:disabled { background: #1a2a3e; color: #556677; }"
        "QComboBox { background: #2a3a5e; color: #ccddff; border: 1px solid #4a6a8e;"
        "            border-radius: 4px; padding: 2px 6px; }"
        "QComboBox QAbstractItemView { background: #2a3a5e; color: #ccddff; }"
        "QSpinBox  { background: #2a3a5e; color: #ccddff; border: 1px solid #4a6a8e;"
        "            border-radius: 4px; padding: 2px 6px; min-width: 70px; }"
        "QCheckBox { color: #ccddff; spacing: 6px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }"
    );

    auto *root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(10, 10, 10, 10);

    // ── Test title ──────────────────────────────────────────────────────
    m_titleLabel = new QLabel("(No test running)");
    QFont tf("Arial", 14, QFont::Bold);
    m_titleLabel->setFont(tf);
    m_titleLabel->setStyleSheet("color: #5599ff; padding: 2px 0;");
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    root->addWidget(m_titleLabel);

    // ── Worker / result name ─────────────────────────────────────────────
    m_workerLabel = new QLabel;
    QFont wf("Arial", 11);
    m_workerLabel->setFont(wf);
    m_workerLabel->setStyleSheet("color: #88aadd;");
    m_workerLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_workerLabel);

    // ── Speedometer (takes up most of the space) ─────────────────────────
    m_meter = new MeterWidget;
    m_meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_meter, 1);

    // ── Large numeric readout ────────────────────────────────────────────
    m_valueLabel = new QLabel("0");
    QFont vf("Arial", 22, QFont::Bold);
    m_valueLabel->setFont(vf);
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setStyleSheet("color: #ffffff; padding: 2px 0;");
    root->addWidget(m_valueLabel);

    // ── Separator ────────────────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #334455;");
    root->addWidget(sep);

    // ── Metric selector ──────────────────────────────────────────────────
    auto *metricRow = new QHBoxLayout;
    auto *metricLbl = new QLabel("Metric:");
    metricLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
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
    metricRow->addWidget(metricLbl);
    metricRow->addWidget(m_resultType, 1);
    root->addLayout(metricRow);

    // ── Max range + watermark ─────────────────────────────────────────────
    auto *optRow = new QHBoxLayout;
    auto *maxLbl = new QLabel("Max:");
    maxLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    m_maxRangeSpin = new QSpinBox;
    m_maxRangeSpin->setRange(0, 10000000);
    m_maxRangeSpin->setValue(0);
    m_maxRangeSpin->setSpecialValueText("Auto");
    m_maxRangeSpin->setSingleStep(1000);
    m_watermarkChk = new QCheckBox("Show Watermark");
    optRow->addWidget(maxLbl);
    optRow->addWidget(m_maxRangeSpin);
    optRow->addSpacing(12);
    optRow->addWidget(m_watermarkChk);
    optRow->addStretch();
    root->addLayout(optRow);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto *btnRow = new QHBoxLayout;
    m_startNextBtn = new QPushButton("Start");
    m_stopBtn      = new QPushButton("Stop");
    btnRow->addStretch();
    btnRow->addWidget(m_startNextBtn);
    btnRow->addWidget(m_stopBtn);
    root->addLayout(btnRow);

    // ── Wire up signals ───────────────────────────────────────────────────
    connect(m_startNextBtn, &QPushButton::clicked,
            this,           &BigMeterWidget::onStartNextClicked);
    connect(m_stopBtn,      &QPushButton::clicked,
            this,           &BigMeterWidget::onStopClicked);
    connect(m_watermarkChk, &QCheckBox::toggled,
            this,           &BigMeterWidget::onWatermarkToggled);
    connect(m_maxRangeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this,           &BigMeterWidget::onMaxRangeChanged);
    connect(m_resultType, &QComboBox::currentTextChanged,
            this,         &BigMeterWidget::onResultTypeChanged);

    // Initialise the meter and button states
    m_meter->setRange(0, 100, true);
    updateButtons();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void BigMeterWidget::setTitle(const QString &testTitle)
{
    m_titleLabel->setText(testTitle.isEmpty() ? "(No test running)" : testTitle);
}

void BigMeterWidget::setWorkerResult(const QString &workerName,
                                     const QString &resultName)
{
    m_workerName = workerName;
    m_resultName = resultName;

    if (workerName.isEmpty())
        m_workerLabel->setText(resultName);
    else if (resultName.isEmpty())
        m_workerLabel->setText(workerName);
    else
        m_workerLabel->setText(workerName + " — " + resultName);  // em-dash

    // Sync the combo box without triggering the resultTypeChanged signal
    if (!resultName.isEmpty()) {
        const int idx = m_resultType->findText(resultName);
        if (idx >= 0) {
            QSignalBlocker guard(m_resultType);
            m_resultType->setCurrentIndex(idx);
        }
    }
}

void BigMeterWidget::updateDisplay(double value, const QString &formattedText)
{
    m_meter->setValue(value);
    m_valueLabel->setText(
        formattedText.isEmpty()
            ? QString::number(value, 'f', (value < 100.0 ? 1 : 0))
            : formattedText);
}

void BigMeterWidget::resetWatermark()
{
    m_meter->resetWatermark();
}

void BigMeterWidget::setButtonState(bool canStart, bool canStop, bool canStopAll)
{
    m_canStart   = canStart;
    m_canStop    = canStop;
    m_canStopAll = canStopAll;
    updateButtons();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void BigMeterWidget::updateButtons()
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

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void BigMeterWidget::onStartNextClicked()
{
    if (m_canStart) {
        m_meter->resetWatermark();
        emit startRequested();
    } else {
        emit nextRequested();
    }
}

void BigMeterWidget::onStopClicked()
{
    emit stopRequested();
}

void BigMeterWidget::onWatermarkToggled(bool checked)
{
    m_meter->showWatermark = checked;
    m_meter->resetWatermark();
    m_meter->update();
}

void BigMeterWidget::onMaxRangeChanged(int value)
{
    if (value == 0)
        m_meter->setRange(0, 100, /*auto=*/true);
    else
        m_meter->setRange(0, value, /*auto=*/false);
}

void BigMeterWidget::onResultTypeChanged(const QString &text)
{
    m_resultName = text;
    setWorkerResult(m_workerName, text);   // update the label
    emit resultTypeChanged(text);
}
