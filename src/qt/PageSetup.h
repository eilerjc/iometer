// PageSetup.h — "Setup" tab: worker targets and test parameters.
// Equivalent to CPageSetup in the MFC GUI.
#pragma once
#include "IometerTypes.h"
#include <QWidget>

class IometerEngine;
class QTableWidget;
class QSpinBox;
class QLabel;
class QPushButton;
class QGroupBox;
class QComboBox;
class QTreeWidget;

class PageSetup : public QWidget
{
    Q_OBJECT
public:
    explicit PageSetup(IometerEngine *engine, QWidget *parent = nullptr);

public slots:
    void refreshWorkers();

private slots:
    void onWorkerSelectionChanged();
    void onQueueDepthChanged(int value);
    void onAddWorker();
    void onRemoveWorker();
    void onTargetToggled(int row, int col);

private:
    void setupUi();
    void populateTargetTable();
    void populateWorkerDetails(const WorkerInfo &w);

    IometerEngine  *m_engine         = nullptr;
    QTreeWidget    *m_workerTree     = nullptr;
    QTableWidget   *m_targetTable    = nullptr;
    QSpinBox       *m_queueDepth     = nullptr;
    QLabel         *m_workerNameLbl  = nullptr;
    QPushButton    *m_addWorkerBtn   = nullptr;
    QPushButton    *m_removeWorkerBtn = nullptr;
    QComboBox      *m_workerTypeCmb  = nullptr;

    QString m_selManagerName;
    QString m_selWorkerId;
};
