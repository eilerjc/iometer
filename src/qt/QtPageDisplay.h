// QtPageDisplay.h -- "Results Display" tab.
// Layout matches the original: metric rows showing live values + BigMeter access.
#pragma once

#include "QtIometerTypes.h"
#include <QWidget>
#include <QVector>
#include <QProgressBar>

class QtBigMeterWidget;
class QtIometerEngine;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QRadioButton;
class QGroupBox;

class QtPageDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit QtPageDisplay(QtIometerEngine *engine, QWidget *parent = nullptr);

public slots:
    void updateResults(const QVector<WorkerResult> &results);
    void onTestStarted();
    void onTestStopped();
    void onBigMeterClicked();
    void refreshWorkerAssignments();

signals:
    void nextSpecRequested();   // BigMeter "Next >>" button - advance to next spec

private slots:
    void onMetricButtonClicked(int row);  // popup menu to switch metric
    void onMetricRowClicked(int row);     // ">" button - open BigMeter

private:
    // One display row (matches original "Total I/Os per Second" row etc.)
    struct DisplayRow {
        int          resultType;    // RESULT_* constant
        QPushButton *metricBtn;     // left button with metric name
        QLabel      *workerLbl;     // "All Managers"
        QLabel      *valueLbl;      // live numeric value (separate from the bar)
        QProgressBar*bar;           // proportional fill indicator (no text)
        QLabel      *refLbl;        // scale ceiling (power of 10)
        QPushButton *expandBtn;     // ">" -- opens BigMeter
        double       refValue = 0.0;
        double       maxSeen  = 0.0; // 0 = not yet set; updated to pow-of-10 ceiling
    };

    void setupUi();
    void setupRow(QWidget *container, int row, int resultType, const QString &label);

    QtIometerEngine  *m_engine     = nullptr;
    QtBigMeterWidget *m_bigMeter   = nullptr;
    QCheckBox      *m_recordChk  = nullptr;
    QRadioButton   *m_sinceStart = nullptr;
    QRadioButton   *m_sinceUpdate= nullptr;
    QComboBox      *m_freqCombo  = nullptr;

    static constexpr int NUM_ROWS = 6;
    DisplayRow  m_rows[NUM_ROWS];
    int         m_activeMeterRow = 0;
    bool        m_running        = false;
};
