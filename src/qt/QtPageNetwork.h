// QtPageNetwork.h -- "Network Targets" tab.
// Layout matches the original:
//   Left:   "Targets" group box with a checkable manager->NIC tree
//   Right:  Network Interface to Use for Connection,
//           Max # Outstanding Sends, Test Connection Rate
#pragma once
#include "QtIometerTypes.h"
#include <QWidget>

class QtIometerEngine;
class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;

class QtPageNetwork : public QWidget
{
    Q_OBJECT
public:
    explicit QtPageNetwork(QtIometerEngine *engine, QWidget *parent = nullptr);

public slots:
    // Called by QtMainWindow when Topology tree selection changes
    void clearSelection();
    void setSelectedManager(const QString &mgrName);
    void setSelectedWorker(const QString &mgrName, const QString &workerId);

    // Called when engine reports new config (e.g. manager connected)
    void refreshForEngine();

    // Called by QtMainWindow for compatibility
    void onManagerConnected(const ManagerInfo &mgr);
    void onManagerDisconnected(const QString &name);

private slots:
    void onTargetItemChanged(QTreeWidgetItem *item, int column);
    void onNicComboChanged(int index);
    void onMaxSendsChanged(int value);
    void onTestConnRateToggled(bool checked);
    void onTransPerConnChanged(int value);

private:
    void setupUi();
    void populateTargetTree();
    void loadWorkerParams();
    void saveWorkerParams();
    void setParamsEnabled(bool enabled);
    void rebuildNicCombo();

    QtIometerEngine   *m_engine          = nullptr;
    QString          m_selManagerName;
    QString          m_selWorkerId;
    bool             m_updating        = false;
    // Set while this page is writing its own edit back to the engine, so the
    // engine's resulting configChanged doesn't re-enter refreshForEngine and
    // rebuild the tree/combo out from under the widget signal in flight.
    bool             m_applyingEdit    = false;

    // Left panel
    QTreeWidget     *m_targetTree      = nullptr;

    // Right panel
    QComboBox       *m_nicCombo        = nullptr;
    QSpinBox        *m_maxSends        = nullptr;
    QCheckBox       *m_testConnRateChk = nullptr;
    QSpinBox        *m_transPerConn    = nullptr;
};
