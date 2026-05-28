// BigMeterWidget.h
// Qt port of CBigMeter — the Iometer "Presentation Meter" window.
//
// Displays a large speedometer (MeterWidget) together with:
//   • test title and worker/result labels
//   • large numeric readout
//   • result-type selector (combo box)
//   • max-range spin box (0 = auto)
//   • watermark check box
//   • Start / Next>>  and  Stop  buttons
//
// Signals mirror the CBigMeter callbacks so the rest of the Qt GUI can
// attach the same logic as the MFC version.
#pragma once

#include <QWidget>
#include <QString>

class MeterWidget;
class QLabel;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QComboBox;

class BigMeterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BigMeterWidget(QWidget *parent = nullptr);

    // ── Display control ───────────────────────────────────────────────────
    void setTitle(const QString &testTitle);
    void setWorkerResult(const QString &workerName, const QString &resultName);
    void updateDisplay(double value, const QString &formattedText = {});
    void resetWatermark();

    // ── Button state (mirrors CBigMeter::SetButtonState) ─────────────────
    //   canStart  → show "Start" button enabled
    //   canStop   → show "Stop" button enabled
    //   canStopAll→ "Next >>" button enabled (when running, there are more tests)
    void setButtonState(bool canStart, bool canStop, bool canStopAll);

    // ── Access to the embedded gauge ──────────────────────────────────────
    MeterWidget *meter() const { return m_meter; }

signals:
    void startRequested();
    void stopRequested();
    void nextRequested();
    void resultTypeChanged(const QString &resultName);

private slots:
    void onStartNextClicked();
    void onStopClicked();
    void onWatermarkToggled(bool checked);
    void onMaxRangeChanged(int value);
    void onResultTypeChanged(const QString &text);

private:
    void setupUi();
    void updateButtons();

    MeterWidget  *m_meter        = nullptr;
    QLabel       *m_titleLabel   = nullptr;
    QLabel       *m_workerLabel  = nullptr;
    QLabel       *m_valueLabel   = nullptr;
    QPushButton  *m_startNextBtn = nullptr;
    QPushButton  *m_stopBtn      = nullptr;
    QCheckBox    *m_watermarkChk = nullptr;
    QSpinBox     *m_maxRangeSpin = nullptr;
    QComboBox    *m_resultType   = nullptr;

    bool    m_canStart   = false;
    bool    m_canStop    = false;
    bool    m_canStopAll = false;
    QString m_workerName;
    QString m_resultName;
};
