// QtPageResults.h -- "Test Setup" tab.
// Matches the original Test Setup tab layout:
//   Test Description, Run Time, Ramp Up Time, Record Results,
//   Number of Workers to Spawn, Cycling Options.
#pragma once
#include "QtIometerTypes.h"
#include <QWidget>
class QtIometerEngine;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QRadioButton;
class QGroupBox;
class QCheckBox;

class QtPageResults : public QWidget
{
    Q_OBJECT
public:
    explicit QtPageResults(QtIometerEngine *engine, QWidget *parent = nullptr);

    // Still connected from QtMainWindow for compatibility
    void updateResults(const QVector<WorkerResult> &) {}
    void onTestStopped() {}

private slots:
    void onConfigChanged();
    void saveConfig();

private:
    void setupUi();
    void loadConfig();

    QtIometerEngine *m_engine        = nullptr;
    bool           m_updating      = false;

    // Test Description
    QLineEdit    *m_description    = nullptr;

    // Run Time
    QSpinBox     *m_runHours       = nullptr;
    QSpinBox     *m_runMinutes     = nullptr;
    QSpinBox     *m_runSeconds     = nullptr;

    // Ramp Up Time
    QSpinBox     *m_rampSeconds    = nullptr;

    // Record Results
    QComboBox    *m_recordResults  = nullptr;

    // Cycling Options
    QComboBox    *m_cyclingMode    = nullptr;
    QSpinBox     *m_workerStart    = nullptr;
    QSpinBox     *m_workerStep     = nullptr;
    QComboBox    *m_workerStepping = nullptr;
    QSpinBox     *m_targetStart    = nullptr;
    QSpinBox     *m_targetStep     = nullptr;
    QComboBox    *m_targetStepping = nullptr;
    QSpinBox     *m_ioqStart       = nullptr;
    QSpinBox     *m_ioqEnd         = nullptr;
    QSpinBox     *m_ioqPower       = nullptr;
    QComboBox    *m_ioqStepping    = nullptr;
};
