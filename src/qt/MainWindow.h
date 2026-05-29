// MainWindow.h -- Top-level application window for the Iometer Qt GUI.
// Equivalent to CMainFrame + CGalileoView in the MFC original.
#pragma once

#include "IometerTypes.h"
#include <QMainWindow>
#include <QVector>

class IometerEngine;
class PageDisplay;
class PageSetup;
class PageAccess;
class PageResults;
class PageNetwork;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QSplitter;
class QLabel;
class QAction;
class QGroupBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(IometerEngine *engine, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onStart();
    void onStop();
    void onStopAll();
    void onAbout();

    void onTestStarted();
    void onTestStopped();
    void onResultsUpdated(QVector<WorkerResult> results);
    void onStatusMessage(QString msg);
    void onManagerConnected(ManagerInfo mgr);
    void onManagerDisconnected(QString name);
    void onConfigChanged();

    void onWorkerTreeSelectionChanged();

private:
    void setupToolBar();
    void setupTopologyPanel();
    void setupTabs();
    void setupStatusBar();
    void rebuildWorkerTree();
    void setRunningState(bool running);

    // Icon helpers
    static QIcon makeOpenIcon();
    static QIcon makeSaveIcon();
    static QIcon makeNewIcon();
    static QIcon makeStartIcon();
    static QIcon makeStopIcon(bool all);
    static QIcon makeMeterIcon();

    // ---- Engine --------------------------------------------------------------
    IometerEngine *m_engine = nullptr;

    // ---- Central area: topology panel (left) + tabs (right) -----------------
    QSplitter   *m_splitter      = nullptr;
    QGroupBox   *m_topoGroup     = nullptr;   // "Topology" group box
    QTreeWidget *m_workerTree    = nullptr;
    QTabWidget  *m_tabs          = nullptr;

    // ---- Tab pages ----------------------------------------------------------
    PageSetup   *m_pageSetup     = nullptr;   // tab 0: Disk Targets
    PageNetwork *m_pageNetwork   = nullptr;   // tab 1: Network Targets
    PageAccess  *m_pageAccess    = nullptr;   // tab 2: Access Specifications
    PageDisplay *m_pageDisplay   = nullptr;   // tab 3: Results Display
    PageResults *m_pageResults   = nullptr;   // tab 4: Test Setup

    // ---- Toolbar actions ----------------------------------------------------
    QAction *m_actNew      = nullptr;
    QAction *m_actOpen     = nullptr;
    QAction *m_actSave     = nullptr;
    QAction *m_actStart    = nullptr;
    QAction *m_actStop     = nullptr;
    QAction *m_actStopAll  = nullptr;
    QAction *m_actBigMeter = nullptr;

    // ---- Status bar ---------------------------------------------------------
    QLabel *m_statusLeft   = nullptr;
    QLabel *m_statusRight  = nullptr;

    bool m_running = false;
};
