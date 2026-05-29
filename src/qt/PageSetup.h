// PageSetup.h -- "Disk Targets" tab.
// Equivalent to CPageSetup / CDiskPageDlg in the MFC original.
//
// Layout matches the original:
//   Left:   "Targets" group box with a checkable target list
//   Right:  Maximum Disk Size, Starting Disk Sector, # Outstanding I/Os,
//           Use Fixed Seed, Test Connection Rate, Write IO Data Pattern
#pragma once
#include "IometerTypes.h"
#include <QWidget>

class IometerEngine;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QGroupBox;
class QLineEdit;

class PageSetup : public QWidget
{
    Q_OBJECT
public:
    explicit PageSetup(IometerEngine *engine, QWidget *parent = nullptr);

public slots:
    // Called by MainWindow when the Topology tree selection changes
    void clearSelection();
    void setSelectedManager(const QString &mgrName);
    void setSelectedWorker(const QString &mgrName, const QString &workerId);

    // Called when the engine reports new config (e.g. manager connected)
    void refreshForEngine();

private slots:
    void onTargetItemChanged(QListWidgetItem *item);
    void onQueueDepthChanged(int value);
    void onMaxDiskSizeChanged(int value);
    void onStartSectorChanged(int value);
    void onFixedSeedToggled(bool checked);
    void onFixedSeedValueChanged();
    void onConnRateToggled(bool checked);
    void onTransPerConnChanged(int value);
    void onDataPatternChanged(int index);

private:
    void setupUi();
    void populateTargetList();
    void loadWorkerParams();
    void saveWorkerParams();
    void setParamsEnabled(bool enabled);

    IometerEngine  *m_engine         = nullptr;
    QString         m_selManagerName;
    QString         m_selWorkerId;
    bool            m_updating        = false;

    // Left panel
    QListWidget    *m_targetList      = nullptr;

    // Right panel
    QSpinBox       *m_maxDiskSize     = nullptr;   // sectors
    QSpinBox       *m_startSector     = nullptr;   // sectors
    QSpinBox       *m_queueDepth      = nullptr;   // per target
    QCheckBox      *m_fixedSeedChk    = nullptr;
    QLineEdit      *m_fixedSeedEdit   = nullptr;
    QCheckBox      *m_connRateChk     = nullptr;
    QSpinBox       *m_transPerConn    = nullptr;
    QComboBox      *m_dataPattern     = nullptr;
};
