// PageResults.h — "Results" tab: tabular live and saved results.
#pragma once
#include "IometerTypes.h"
#include <QWidget>
class IometerEngine;
class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;

class PageResults : public QWidget
{
    Q_OBJECT
public:
    explicit PageResults(IometerEngine *engine, QWidget *parent = nullptr);
public slots:
    void updateResults(const QVector<WorkerResult> &results);
    void onTestStopped();
private slots:
    void onClearSaved();
    void onExportCsv();
    void onViewChanged(int index);
private:
    void setupUi();
    void populateTable(const QVector<WorkerResult> &results);
    IometerEngine *m_engine     = nullptr;
    QTableWidget  *m_table      = nullptr;
    QPushButton   *m_clearBtn   = nullptr;
    QPushButton   *m_exportBtn  = nullptr;
    QComboBox     *m_viewCombo  = nullptr;
    QLabel        *m_statusLbl  = nullptr;
    QVector<WorkerResult> m_liveResults;
    bool  m_showLive = true;
};
