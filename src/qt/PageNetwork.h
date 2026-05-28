// PageNetwork.h — "Network" tab: connect/disconnect remote Dynamo managers.
#pragma once
#include "IometerTypes.h"
#include <QWidget>
class IometerEngine;
class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

class PageNetwork : public QWidget
{
    Q_OBJECT
public:
    explicit PageNetwork(IometerEngine *engine, QWidget *parent = nullptr);
public slots:
    void onManagerConnected(const ManagerInfo &mgr);
    void onManagerDisconnected(const QString &name);
private slots:
    void onConnect();
    void onDisconnect();
    void onRefresh();
private:
    void setupUi();
    void rebuildTable();
    IometerEngine  *m_engine        = nullptr;
    QTableWidget   *m_managerTable  = nullptr;
    QLineEdit      *m_addrEdit      = nullptr;
    QLineEdit      *m_nameEdit      = nullptr;
    QPushButton    *m_connectBtn    = nullptr;
    QPushButton    *m_disconnectBtn = nullptr;
    QPushButton    *m_refreshBtn    = nullptr;
    QLabel         *m_statusLabel   = nullptr;
};
