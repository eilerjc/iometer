// PageDisplay.h -- "Results Display" tab.
// Layout matches the original: metric rows showing live values + BigMeter access.
#pragma once

#include "IometerTypes.h"
#include <QWidget>
#include <QVector>

class BigMeterWidget;
class IometerEngine;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QRadioButton;
class QGroupBox;

class PageDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit PageDisplay(IometerEngine *engine, QWidget *parent = nullptr);

public slots:
    void updateResults(const QVector<WorkerResult> &results);
    void onTestStarted();
    void onTestStopped();
    void onBigMeterClicked();
    void refreshWorkerAssignments();

private slots:
    void onMetricRowClicked(int row);

private:
    // One display row (matches original "Total I/Os per Second" row etc.)
    struct DisplayRow {
        int         resultType;     // RESULT_* constant
        QPushButton *metricBtn;     // left button with metric name
        QLabel      *workerLbl;     // "All Managers"
        QLabel      *valueLbl;      // live value
        QLabel      *refLbl;        // reference (start-of-test) value
        QPushButton *expandBtn;     // ">" -- opens BigMeter
        double       refValue = 0.0;
    };

    void setupUi();
    void setupRow(QWidget *container, int row, int resultType, const QString &label);

    IometerEngine  *m_engine     = nullptr;
    BigMeterWidget *m_bigMeter   = nullptr;
    QCheckBox      *m_recordChk  = nullptr;
    QRadioButton   *m_sinceStart = nullptr;
    QRadioButton   *m_sinceUpdate= nullptr;
    QComboBox      *m_freqCombo  = nullptr;

    static constexpr int NUM_ROWS = 6;
    DisplayRow  m_rows[NUM_ROWS];
    int         m_activeMeterRow = 0;
    bool        m_running        = false;
};
