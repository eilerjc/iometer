// MainWindow.h -- Top-level application window for the Iometer Qt GUI.
// Equivalent to CMainFrame + CGalileoView in the MFC original.
#pragma once

#include "IometerTypes.h"
#include <QMainWindow>
#include <QVector>
#include <QFile>

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
    // File
    void onNew();
    void onOpen();
    void onSave();

    // Topology operations (map to original BNewDynamo, BNewDiskWorker, etc.)
    void onNewDynamo();
    void onNewDiskWorker();
    void onNewNetWorker();
    void onCopyWorker();
    void onExitOne();

    // Test control
    void onStart();
    void onStop();
    void onStopAll();

    // Help
    void onAbout();

    // Engine signals
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
    void updateTopologyButtons();   // enable/disable per-selection buttons

    // Icon helpers — pixel-faithful replicas of the original toolbar bitmaps
    static QIcon makeOpenIcon();
    static QIcon makeSaveIcon();
    static QIcon makeNewDynamoIcon();
    static QIcon makeNewDiskWorkerIcon();
    static QIcon makeNewNetWorkerIcon();
    static QIcon makeCopyWorkerIcon();
    static QIcon makeStartIcon();
    static QIcon makeStopIcon(bool all);
    static QIcon makeResetIcon();        // tlb10 — curved yellow arrow
    static QIcon makeExitOneIcon();      // tlb11 — inward red arrows
    static QIcon makeExitIcon();         // tlb12 — green door
    static QIcon makeHelpIcon();         // tlb13 — yellow "?"

    // ---- Engine ---------------------------------------------------------------
    IometerEngine *m_engine = nullptr;

    // ---- Central area: topology panel (left) + tabs (right) ------------------
    QSplitter   *m_splitter      = nullptr;
    QGroupBox   *m_topoGroup     = nullptr;   // "Topology" group box
    QTreeWidget *m_workerTree    = nullptr;
    QTabWidget  *m_tabs          = nullptr;

    // ---- Tab pages -----------------------------------------------------------
    PageSetup   *m_pageSetup     = nullptr;   // tab 0: Disk Targets
    PageNetwork *m_pageNetwork   = nullptr;   // tab 1: Network Targets
    PageAccess  *m_pageAccess    = nullptr;   // tab 2: Access Specifications
    PageDisplay *m_pageDisplay   = nullptr;   // tab 3: Results Display
    PageResults *m_pageResults   = nullptr;   // tab 4: Test Setup

    // ---- Toolbar actions (in original toolbar order) -------------------------
    QAction *m_actOpen           = nullptr;   // ID_FILE_OPEN
    QAction *m_actSave           = nullptr;   // ID_FILE_SAVE
    QAction *m_actNewDynamo      = nullptr;   // BNewDynamo
    QAction *m_actNewDiskWorker  = nullptr;   // BNewDiskWorker
    QAction *m_actNewNetWorker   = nullptr;   // BNewNetWorker
    QAction *m_actCopyWorker     = nullptr;   // BCopyWorker
    QAction *m_actStart          = nullptr;   // BStart
    QAction *m_actStop           = nullptr;   // BStop
    QAction *m_actStopAll        = nullptr;   // BStopAll
    QAction *m_actNew            = nullptr;   // BReset
    QAction *m_actExitOne        = nullptr;   // BExitOne
    QAction *m_actExit           = nullptr;   // Exit Iometer
    QAction *m_actHelp           = nullptr;   // ID_APP_ABOUT
    QAction *m_actBigMeter       = nullptr;   // Presentation Meter (menu only)

    // ---- Status bar ----------------------------------------------------------
    QLabel *m_statusLeft         = nullptr;
    QLabel *m_statusRight        = nullptr;

    // ---- Current topology selection (drives button enable/disable) -----------
    QString m_selManagerName;
    QString m_selWorkerId;
    int     m_deletedWorkerPos = -1;

    bool    m_running      = false;
    QFile  *m_resultsFile  = nullptr;

    // Spec cycling state
    QList<AccessSpec> m_runQueue;        // specs to run in order this test session
    int               m_runQueueIdx  = 0;
    bool              m_specAdvancing = false;  // true = Stop was "advance", false = abort
};
