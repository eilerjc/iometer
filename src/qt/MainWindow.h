// MainWindow.h — Top-level application window for the Iometer Qt GUI.
// Equivalent to CMainFrame + CGalileoView in the MFC GUI.
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
class QToolBar;

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
    void setupWorkerTree();
    void setupTabs();
    void setupStatusBar();
    void rebuildWorkerTree();
    void setRunningState(bool running);

    // ── Engine ──────────────────────────────────────────────────────────
    IometerEngine *m_engine = nullptr;

    // ── Central area: splitter → worker-tree + tabs ──────────────────────
    QSplitter   *m_splitter   = nullptr;
    QTreeWidget *m_workerTree = nullptr;
    QTabWidget  *m_tabs       = nullptr;

    // ── Tab pages ─────────────────────────────────────────────────────────
    PageSetup   *m_pageSetup    = nullptr;
    PageAccess  *m_pageAccess   = nullptr;
    PageDisplay *m_pageDisplay  = nullptr;
    PageResults *m_pageResults  = nullptr;
    PageNetwork *m_pageNetwork  = nullptr;

    // ── Toolbar actions ───────────────────────────────────────────────────
    QAction *m_actNew     = nullptr;
    QAction *m_actOpen    = nullptr;
    QAction *m_actSave    = nullptr;
    QAction *m_actStart   = nullptr;
    QAction *m_actStop    = nullptr;
    QAction *m_actStopAll = nullptr;
    QAction *m_actBigMeter = nullptr;

    // ── Status bar ─────────────────────────────────────────────────────────
    QLabel *m_statusLeft  = nullptr;
    QLabel *m_statusRight = nullptr;

    bool m_running = false;
};
