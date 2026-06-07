// QtMainWindow.cpp
#include "QtMainWindow.h"
#include "QtIometerEngine.h"
#include "QtPageDisplay.h"
#include "QtPageSetup.h"
#include "QtPageAccess.h"
#include "QtPageResults.h"
#include "QtPageNetwork.h"

#include <QToolBar>
#include <QAction>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QHeaderView>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QStyle>
#include <QProcess>
#include <QCoreApplication>

// =============================================================================
// Icon helpers - load directly from the original Toolbar.bmp resource strip.
// The strip is 403×31 px, 13 icons each 31 px wide.
// Background colour (Windows button-face grey) is made transparent.
// =============================================================================

// Indices into the 13-icon strip (left to right, 0-based)
enum ToolbarIcon {
    TBI_OPEN        = 0,
    TBI_SAVE        = 1,
    TBI_NEW_DYNAMO  = 2,
    TBI_NEW_DISK    = 3,
    TBI_NEW_NET     = 4,
    TBI_COPY_WORKER = 5,
    TBI_START       = 6,
    TBI_STOP        = 7,
    TBI_STOP_ALL    = 8,
    TBI_RESET       = 9,
    TBI_EXIT_ONE    = 10,
    TBI_EXIT        = 11,
    TBI_HELP        = 12,
};

static QIcon toolbarIcon(int idx)
{
    // Function-local static: only initialized on first call, after QApplication exists.
    static QPixmap strip;
    if (strip.isNull()) {
        strip.load(":/Toolbar.bmp");
        if (strip.isNull()) return {};
    }

    const int w = strip.width() / 13;   // 31 px per icon
    const int h = strip.height();        // 31 px

    QPixmap cell = strip.copy(idx * w, 0, w, h);

    // Make the Windows button-face background transparent.
    // Original BMP uses the standard Windows grey 0xC0C0C0 as background.
    QImage img = cell.toImage().convertToFormat(QImage::Format_ARGB32);
    const QRgb bgKey = qRgb(0xC0, 0xC0, 0xC0);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if ((img.pixel(x, y) & 0x00FFFFFF) == (bgKey & 0x00FFFFFF))
                img.setPixel(x, y, 0x00000000);

    return QIcon(QPixmap::fromImage(img));
}

QIcon QtMainWindow::makeOpenIcon()        { return toolbarIcon(TBI_OPEN);        }
QIcon QtMainWindow::makeSaveIcon()        { return toolbarIcon(TBI_SAVE);        }
QIcon QtMainWindow::makeNewDynamoIcon()   { return toolbarIcon(TBI_NEW_DYNAMO);  }
QIcon QtMainWindow::makeNewDiskWorkerIcon(){ return toolbarIcon(TBI_NEW_DISK);   }
QIcon QtMainWindow::makeNewNetWorkerIcon(){ return toolbarIcon(TBI_NEW_NET);     }
QIcon QtMainWindow::makeCopyWorkerIcon()  { return toolbarIcon(TBI_COPY_WORKER); }
QIcon QtMainWindow::makeStartIcon()       { return toolbarIcon(TBI_START);       }
QIcon QtMainWindow::makeStopIcon(bool all){ return toolbarIcon(all ? TBI_STOP_ALL : TBI_STOP); }
QIcon QtMainWindow::makeResetIcon()       { return toolbarIcon(TBI_RESET);       }
QIcon QtMainWindow::makeExitOneIcon()     { return toolbarIcon(TBI_EXIT_ONE);    }
QIcon QtMainWindow::makeExitIcon()        { return toolbarIcon(TBI_EXIT);        }
QIcon QtMainWindow::makeHelpIcon()        { return toolbarIcon(TBI_HELP);        }

// =============================================================================
// QtMainWindow
// =============================================================================

QtMainWindow::QtMainWindow(QtIometerEngine *engine, QWidget *parent)
    : QMainWindow(parent), m_engine(engine)
{
    setWindowTitle(QString("Iometer 1.1.0 [Built: %1 %2]").arg(__DATE__, __TIME__));
    resize(860, 600);
    // No custom stylesheet -- use native Windows look

    setupToolBar();
    setupTopologyPanel();
    setupTabs();
    setupStatusBar();
    rebuildWorkerTree();
    setRunningState(false);

    connect(engine, &QtIometerEngine::testStarted,         this, &QtMainWindow::onTestStarted);
    connect(engine, &QtIometerEngine::testStopped,         this, &QtMainWindow::onTestStopped);
    connect(engine, &QtIometerEngine::resultsUpdated,      this, &QtMainWindow::onResultsUpdated);
    connect(engine, &QtIometerEngine::statusMessage,       this, &QtMainWindow::onStatusMessage);
    connect(engine, &QtIometerEngine::managerConnected,    this, &QtMainWindow::onManagerConnected);
    connect(engine, &QtIometerEngine::managerDisconnected, this, &QtMainWindow::onManagerDisconnected);
    connect(engine, &QtIometerEngine::configChanged,       this, &QtMainWindow::onConfigChanged);
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void QtMainWindow::setupToolBar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(24, 24));
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // -- Group 1: File operations (Open / Save) --------------------------------
    m_actOpen            = tb->addAction(makeOpenIcon(),           "Open");
    m_actSave            = tb->addAction(makeSaveIcon(),           "Save");
    tb->addSeparator();

    // -- Group 2: Topology operations (matches original BNew*/BCopyWorker) -----
    m_actNewDynamo       = tb->addAction(makeNewDynamoIcon(),      "New Manager");
    m_actNewDiskWorker   = tb->addAction(makeNewDiskWorkerIcon(),  "New Disk Worker");
    m_actNewNetWorker    = tb->addAction(makeNewNetWorkerIcon(),   "New Net Worker");
    m_actCopyWorker      = tb->addAction(makeCopyWorkerIcon(),     "Copy Worker");
    tb->addSeparator();

    // -- Group 3: Test control (BStart / BStop / BStopAll) ---------------------
    m_actStart           = tb->addAction(makeStartIcon(),          "Start");
    m_actStop            = tb->addAction(makeStopIcon(false),      "Stop");
    m_actStopAll         = tb->addAction(makeStopIcon(true),       "Stop All");
    tb->addSeparator();

    // -- Group 4: Config reset / remove item / exit / help --------------------
    m_actNew             = tb->addAction(makeResetIcon(),          "Reset");
    m_actExitOne         = tb->addAction(makeExitOneIcon(),        "Delete Selected");
    m_actExit            = tb->addAction(makeExitIcon(),           "Exit");
    m_actHelp            = tb->addAction(makeHelpIcon(),           "About");

    // -- Tooltips --------------------------------------------------------------
    m_actOpen->setToolTip("Open .icf configuration file (Ctrl+O)");
    m_actSave->setToolTip("Save .icf configuration file (Ctrl+S)");
    m_actNewDynamo->setToolTip("Launch a new local Dynamo manager");
    m_actNewDiskWorker->setToolTip("Add a disk worker to selected manager");
    m_actNewNetWorker->setToolTip("Add a network worker to selected manager");
    m_actCopyWorker->setToolTip("Copy selected worker");
    m_actStart->setToolTip("Start test (F5)");
    m_actStop->setToolTip("Stop current test (F6)");
    m_actStopAll->setToolTip("Stop all tests and reset");
    m_actNew->setToolTip("Reset - clear all workers and start over (Ctrl+N)");
    m_actExitOne->setToolTip("Delete selected worker or disconnect selected manager");
    m_actExit->setToolTip("Exit Iometer");
    m_actHelp->setToolTip("About Iometer");

    // -- Keyboard shortcuts ----------------------------------------------------
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actSave->setShortcut(QKeySequence::Save);
    m_actStart->setShortcut(Qt::Key_F5);
    m_actStop->setShortcut(Qt::Key_F6);
    m_actNew->setShortcut(QKeySequence::New);

    // -- Connections -----------------------------------------------------------
    connect(m_actOpen,          &QAction::triggered, this, &QtMainWindow::onOpen);
    connect(m_actSave,          &QAction::triggered, this, &QtMainWindow::onSave);
    connect(m_actNewDynamo,     &QAction::triggered, this, &QtMainWindow::onNewDynamo);
    connect(m_actNewDiskWorker, &QAction::triggered, this, &QtMainWindow::onNewDiskWorker);
    connect(m_actNewNetWorker,  &QAction::triggered, this, &QtMainWindow::onNewNetWorker);
    connect(m_actCopyWorker,    &QAction::triggered, this, &QtMainWindow::onCopyWorker);
    connect(m_actStart,         &QAction::triggered, this, &QtMainWindow::onStart);
    connect(m_actStop,          &QAction::triggered, this, &QtMainWindow::onStop);
    connect(m_actStopAll,       &QAction::triggered, this, &QtMainWindow::onStopAll);
    connect(m_actNew,           &QAction::triggered, this, &QtMainWindow::onNew);
    connect(m_actExitOne,       &QAction::triggered, this, &QtMainWindow::onExitOne);
    connect(m_actExit,          &QAction::triggered, this, [this]{
        close();
    });
    connect(m_actHelp,          &QAction::triggered, this, &QtMainWindow::onAbout);

    // -- Menu bar -------------------------------------------------------------
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QApplication::instance(), &QApplication::quit,
                        QKeySequence::Quit);

    auto *workerMenu = menuBar()->addMenu("&Worker");
    workerMenu->addAction(m_actNewDynamo);
    workerMenu->addSeparator();
    workerMenu->addAction(m_actNewDiskWorker);
    workerMenu->addAction(m_actNewNetWorker);
    workerMenu->addAction(m_actCopyWorker);
    workerMenu->addSeparator();
    workerMenu->addAction(m_actExitOne);

    auto *testMenu = menuBar()->addMenu("&Test");
    testMenu->addAction(m_actStart);
    testMenu->addAction(m_actStop);
    testMenu->addAction(m_actStopAll);

    // BigMeter is accessible from Results Display; also provide a menu entry
    m_actBigMeter = new QAction("Presentation Meter", this);
    connect(m_actBigMeter, &QAction::triggered, this, [this]{
        m_tabs->setCurrentIndex(3);
        m_pageDisplay->onBigMeterClicked();
    });

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_actBigMeter);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(m_actHelp);
}

void QtMainWindow::setupTopologyPanel()
{
    // QGroupBox provides the "Topology" header label + sunken border,
    // matching the look of the original panel.
    m_topoGroup = new QGroupBox("Topology");
    auto *lay   = new QVBoxLayout(m_topoGroup);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(0);

    m_workerTree = new QTreeWidget;
    m_workerTree->setHeaderHidden(true);
    m_workerTree->setIndentation(16);
    m_workerTree->setMinimumWidth(140);

    lay->addWidget(m_workerTree);

    connect(m_workerTree, &QTreeWidget::currentItemChanged,
            this, &QtMainWindow::onWorkerTreeSelectionChanged);
}

void QtMainWindow::setupTabs()
{
    m_pageSetup   = new QtPageSetup(m_engine);
    m_pageNetwork = new QtPageNetwork(m_engine);
    m_pageAccess  = new QtPageAccess(m_engine);
    m_pageDisplay = new QtPageDisplay(m_engine);
    m_pageResults = new QtPageResults(m_engine);

    m_tabs = new QTabWidget;
    // Tab order and names match the original exactly:
    m_tabs->addTab(m_pageSetup,   "Disk Targets");
    m_tabs->addTab(m_pageNetwork, "Network Targets");
    m_tabs->addTab(m_pageAccess,  "Access Specifications");
    m_tabs->addTab(m_pageDisplay, "Results Display");
    m_tabs->addTab(m_pageResults, "Test Setup");
    m_tabs->setCurrentIndex(0);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(m_topoGroup);
    m_splitter->addWidget(m_tabs);
    m_splitter->setSizes({170, 690});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    setCentralWidget(m_splitter);

    // Forward engine signals to pages
    connect(m_engine, &QtIometerEngine::resultsUpdated,
            m_pageDisplay, &QtPageDisplay::updateResults);
    connect(m_engine, &QtIometerEngine::testStarted,
            m_pageDisplay, &QtPageDisplay::onTestStarted);
    connect(m_engine, &QtIometerEngine::testStopped,
            m_pageDisplay, &QtPageDisplay::onTestStopped);
    // BigMeter "Next >>" = advance to next spec (same as toolbar Stop)
    connect(m_pageDisplay, &QtPageDisplay::nextSpecRequested,
            this,          &QtMainWindow::onStop);
    connect(m_engine, &QtIometerEngine::managerConnected,
            m_pageNetwork, &QtPageNetwork::onManagerConnected);
    connect(m_engine, &QtIometerEngine::managerDisconnected,
            m_pageNetwork, &QtPageNetwork::onManagerDisconnected);
}

void QtMainWindow::setupStatusBar()
{
    m_statusLeft  = new QLabel("Ready");
    m_statusRight = new QLabel("0 workers | 0 managers");
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusRight);
}

// -----------------------------------------------------------------------------
// Topology tree
// -----------------------------------------------------------------------------

void QtMainWindow::rebuildWorkerTree()
{
    // Save current selection so we can restore it after the rebuild.
    // m_workerTree->clear() fires currentItemChanged with no item, which would
    // call clearSelection() on all tab pages - we suppress that with blockSignals.
    const QString savedMgr    = m_selManagerName;
    const QString savedWorker = m_selWorkerId;

    m_workerTree->blockSignals(true);
    m_workerTree->clear();

    auto *style = QApplication::style();
    const QIcon computerIcon = style->standardIcon(QStyle::SP_ComputerIcon);
    const QIcon hdIcon       = style->standardIcon(QStyle::SP_DriveHDIcon);
    const QIcon netIcon      = style->standardIcon(QStyle::SP_DriveNetIcon);

    // "All Managers" root
    auto *root = new QTreeWidgetItem(m_workerTree);
    root->setText(0, "All Managers");
    root->setData(0, Qt::UserRole, "all");
    root->setIcon(0, computerIcon);
    QFont rf = root->font(0);
    rf.setBold(true);
    root->setFont(0, rf);

    int workerCount = 0;
    for (const auto &mgr : m_engine->managers()) {
        auto *mgrItem = new QTreeWidgetItem(root);
        mgrItem->setText(0, QString::fromStdString(mgr.name));
        mgrItem->setData(0, Qt::UserRole, QString::fromStdString("mgr:" + mgr.name));
        mgrItem->setIcon(0, computerIcon);
        mgrItem->setExpanded(true);

        for (const auto &w : mgr.workers) {
            auto *wItem = new QTreeWidgetItem(mgrItem);
            wItem->setText(0, QString::fromStdString(w.name));
            wItem->setData(0, Qt::UserRole, QString::fromStdString("worker:" + mgr.name + ":" + w.id));
            wItem->setIcon(0, w.type == "Network" ? netIcon : hdIcon);
            ++workerCount;
        }
    }
    root->setExpanded(true);

    m_statusRight->setText(QString("%1 worker(s) | %2 manager(s)")
        .arg(workerCount).arg(m_engine->managers().size()));

    m_workerTree->blockSignals(false);

    // Restore the previous selection (or notify pages if nothing to restore)
    QTreeWidgetItem *toSelect = nullptr;
    if (!savedMgr.isEmpty()) {
        if (QTreeWidgetItem *rootItem = m_workerTree->topLevelItem(0)) {
            for (int i = 0; i < rootItem->childCount() && !toSelect; ++i) {
                auto *mgrItem = rootItem->child(i);
                if (mgrItem->data(0, Qt::UserRole).toString() != "mgr:" + savedMgr)
                    continue;
                if (savedWorker.isEmpty()) {
                    toSelect = mgrItem;
                } else {
                    for (int j = 0; j < mgrItem->childCount(); ++j) {
                        auto *wItem = mgrItem->child(j);
                        if (wItem->data(0, Qt::UserRole).toString()
                                == "worker:" + savedMgr + ":" + savedWorker) {
                            toSelect = wItem;
                            break;
                        }
                    }
                    if (!toSelect) {
                        // Worker was deleted - land on the worker at the same
                        // position, or the last worker if it was the last one.
                        const int nw = mgrItem->childCount();
                        if (m_deletedWorkerPos >= 0 && nw > 0)
                            toSelect = mgrItem->child(
                                qMin(m_deletedWorkerPos, nw - 1));
                        else
                            toSelect = mgrItem;   // no workers left
                    }
                }
            }
        }
    }

    if (toSelect)
        m_workerTree->setCurrentItem(toSelect);   // fires onWorkerTreeSelectionChanged
    else
        onWorkerTreeSelectionChanged();            // nothing to restore - notify pages
}

void QtMainWindow::onWorkerTreeSelectionChanged()
{
    auto *item = m_workerTree->currentItem();
    if (!item) {
        m_selManagerName.clear();
        m_selWorkerId.clear();
        m_pageSetup->clearSelection();
        m_pageNetwork->clearSelection();
        updateTopologyButtons();
        return;
    }

    const QString key = item->data(0, Qt::UserRole).toString();

    if (key == "all") {
        m_selManagerName.clear();
        m_selWorkerId.clear();
        m_pageSetup->clearSelection();
        m_pageNetwork->clearSelection();
    } else if (key.startsWith("mgr:")) {
        m_selManagerName = key.mid(4);
        m_selWorkerId.clear();
        m_pageSetup->setSelectedManager(m_selManagerName);
        m_pageNetwork->setSelectedManager(m_selManagerName);
    } else if (key.startsWith("worker:")) {
        const QStringList parts = key.mid(7).split(':');
        if (parts.size() >= 2) {
            m_selManagerName = parts[0];
            m_selWorkerId    = parts[1];
            m_pageSetup->setSelectedWorker(m_selManagerName, m_selWorkerId);
            m_pageNetwork->setSelectedWorker(m_selManagerName, m_selWorkerId);
        }
    }
    updateTopologyButtons();
}

void QtMainWindow::updateTopologyButtons()
{
    if (m_running) return;   // all topology buttons disabled while running

    const bool hasMgr    = !m_selManagerName.isEmpty();
    const bool hasWorker = hasMgr && !m_selWorkerId.isEmpty();

    // NewDynamo always available when not running
    m_actNewDynamo->setEnabled(true);

    // Worker creation requires a manager to be selected
    m_actNewDiskWorker->setEnabled(hasMgr);
    m_actNewNetWorker->setEnabled(hasMgr);

    // Copy only when a worker is selected
    m_actCopyWorker->setEnabled(hasWorker);

    // Exit one: a worker or manager must be selected (not "All Managers")
    m_actExitOne->setEnabled(hasMgr);
}

// -----------------------------------------------------------------------------
// Toolbar / menu actions
// -----------------------------------------------------------------------------

void QtMainWindow::onNew()
{
    if (m_running) {
        QMessageBox::warning(this, "Test Running",
            "Please stop the test before creating a new configuration.");
        return;
    }
    m_engine->newConfig();
    setWindowTitle(QString("Iometer 1.1.0 [Built: %1 %2]").arg(__DATE__, __TIME__));
}

void QtMainWindow::onOpen()
{
    if (m_running) {
        QMessageBox::warning(this, "Test Running",
            "Please stop the test before loading a configuration.");
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, "Open ICF File", {}, "Iometer Config (*.icf);;All Files (*)");
    if (path.isEmpty()) return;
    if (!m_engine->loadConfig(path))
        QMessageBox::warning(this, "Load Failed", "Could not load: " + path);
    else
        setWindowTitle("Iometer -- " + path);
}

void QtMainWindow::onSave()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save ICF File", {}, "Iometer Config (*.icf);;All Files (*)");
    if (path.isEmpty()) return;
    if (!m_engine->saveConfig(path))
        QMessageBox::warning(this, "Save Failed", "Could not save: " + path);
    else
        setWindowTitle("Iometer -- " + path);
}

void QtMainWindow::onStart()
{
    if (m_running) return;

    // Build the run queue from the Assigned list; fall back to a single
    // empty spec so at least one pass runs when no specs are assigned.
    m_runQueue = m_pageAccess->currentAssignedSpecs();
    if (m_runQueue.isEmpty()) {
        const auto all = m_engine->accessSpecs();
        if (!all.isEmpty()) m_runQueue.append(all.first());
    }
    m_runQueueIdx   = 0;
    m_specAdvancing = false;

    // Prompt for results file
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Save Results To",
        QDir::currentPath() + "/results_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv",
        "CSV Files (*.csv);;All Files (*)");

    if (!path.isEmpty()) {
        delete m_resultsFile;
        m_resultsFile = new QFile(path, this);
        if (m_resultsFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(m_resultsFile);
            out << "Timestamp,Manager,Worker,"
                   "IOps,Read_IOps,Write_IOps,"
                   "MBps_Dec,Read_MBps,Write_MBps,MiBps,"
                   "Avg_Latency_ms,Max_Latency_ms,"
                   "CPU_Util_pct,CPU_User_pct,CPU_Kernel_pct,"
                   "Errors\n";
        } else {
            QMessageBox::warning(this, "Results File",
                "Could not open results file:\n" + path);
            delete m_resultsFile;
            m_resultsFile = nullptr;
        }
    }

    if (!m_runQueue.isEmpty())
        m_engine->setCurrentTestSpec(m_runQueue[0]);
    m_pageAccess->setActiveSpecIndex(0);
    m_engine->startTest();
}

void QtMainWindow::onStop()
{
    // Stop = end current spec and advance to next (like the original).
    m_specAdvancing = true;
    m_engine->stopTest();
}

void QtMainWindow::onStopAll()
{
    // Stop All = abort everything, do not advance.
    m_specAdvancing = false;
    m_engine->stopAll();
}

void QtMainWindow::onNewDynamo()
{
    // Launch Dynamo.exe from the same directory.
    // With no arguments Dynamo connects back to localhost:1066.
    const QString dynamo = QCoreApplication::applicationDirPath() + "/Dynamo.exe";
    if (!QProcess::startDetached(dynamo, {})) {
        QMessageBox::information(this, "Launch Dynamo",
            "Could not auto-launch Dynamo.exe.\n\n"
            "To connect a remote manager, run on the target machine:\n"
            "  Dynamo.exe -i <this_machine_ip> -m <hostname>");
    } else {
        m_statusLeft->setText("Launching local Dynamo - waiting for connection...");
    }
}

void QtMainWindow::onNewDiskWorker()
{
    if (m_selManagerName.isEmpty()) return;

    // Count existing workers to pick next number
    int workerNum = 1;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name == m_selManagerName.toStdString()) {
            workerNum = mgr.workers.size() + 1;
            break;
        }
    }
    WorkerInfo w;
    w.id          = QString("%1-disk-%2").arg(m_selManagerName).arg(workerNum).toStdString();
    w.name        = QString("Worker %1").arg(workerNum).toStdString();
    w.type        = "Disk";
    w.managerName = m_selManagerName.toStdString();
    w.queueDepth  = 1;
    m_engine->addWorker(m_selManagerName, w);
}

void QtMainWindow::onNewNetWorker()
{
    if (m_selManagerName.isEmpty()) return;

    int workerNum = 1;
    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name == m_selManagerName.toStdString()) {
            workerNum = mgr.workers.size() + 1;
            break;
        }
    }
    WorkerInfo w;
    w.id          = QString("%1-net-%2").arg(m_selManagerName).arg(workerNum).toStdString();
    w.name        = QString("Net Worker %1").arg(workerNum).toStdString();
    w.type        = "Network";
    w.managerName = m_selManagerName.toStdString();
    w.queueDepth  = 1;
    m_engine->addWorker(m_selManagerName, w);
}

void QtMainWindow::onCopyWorker()
{
    if (m_selManagerName.isEmpty() || m_selWorkerId.isEmpty()) return;

    for (const auto &mgr : m_engine->managers()) {
        if (mgr.name != m_selManagerName.toStdString()) continue;
        for (const auto &src : mgr.workers) {
            if (src.id != m_selWorkerId.toStdString()) continue;
            WorkerInfo w  = src;
            const int  n  = mgr.workers.size() + 1;
            w.id          = QString("%1-%2-copy").arg(m_selManagerName).arg(n).toStdString();
            w.name        = src.name + " (copy)";
            m_engine->addWorker(m_selManagerName, w);
            return;
        }
    }
}

void QtMainWindow::onExitOne()
{
    if (m_selManagerName.isEmpty()) return;

    if (!m_selWorkerId.isEmpty()) {
        // Record the worker's position so rebuildWorkerTree() can land on the
        // right sibling after the delete (same slot, or last if it was last).
        m_deletedWorkerPos = -1;
        for (const auto &mgr : m_engine->managers()) {
            if (mgr.name != m_selManagerName) continue;
            for (int i = 0; i < mgr.workers.size(); ++i) {
                if (mgr.workers[i].id == m_selWorkerId) {
                    m_deletedWorkerPos = i;
                    break;
                }
            }
            break;
        }
        m_engine->removeWorker(m_selManagerName, m_selWorkerId);
        // Do NOT clear m_selWorkerId here - removeWorker() fires configChanged
        // synchronously, which calls rebuildWorkerTree() → setCurrentItem() →
        // onWorkerTreeSelectionChanged(), which already sets m_selWorkerId to
        // the next worker. Clearing it here would wipe that correct value.
        m_deletedWorkerPos = -1;
    } else {
        // Disconnect the selected manager
        const auto btn = QMessageBox::question(this, "Disconnect Manager",
            QString("Disconnect manager \"%1\" and remove all its workers?")
                .arg(m_selManagerName));
        if (btn != QMessageBox::Yes) return;
        m_engine->disconnectManager(m_selManagerName);
        m_selManagerName.clear();
    }
}

void QtMainWindow::onAbout()
{
    QMessageBox::about(this, "About Iometer Qt GUI",
        "<h2>Iometer 1.1.0 Qt GUI</h2>"
        "<p>Cross-platform Qt port of the Iometer benchmark GUI.</p>"
        "<p>Original Iometer &copy; Intel Corporation. "
        "Qt GUI port by the Iometer Project.</p>"
        "<p>This GUI communicates with <b>Dynamo.exe</b> over TCP port 1066.</p>"
        "<p><a href='https://github.com/eilerjc/iometer'>github.com/eilerjc/iometer</a></p>");
}

// -----------------------------------------------------------------------------
// Engine signal handlers
// -----------------------------------------------------------------------------

void QtMainWindow::onTestStarted()
{
    setRunningState(true);
    m_tabs->setCurrentIndex(3);   // jump to Results Display
    m_statusLeft->setText("Test running...");
}

void QtMainWindow::onTestStopped()
{
    // Check if we should advance to the next spec in the run queue
    const int nextIdx = m_runQueueIdx + 1;
    if (m_specAdvancing && nextIdx < m_runQueue.size()) {
        m_specAdvancing = false;
        m_runQueueIdx   = nextIdx;
        m_engine->setCurrentTestSpec(m_runQueue[nextIdx]);
        m_pageAccess->setActiveSpecIndex(nextIdx);
        m_engine->startTest();
        m_statusLeft->setText(
            QString("Running spec %1/%2: %3")
                .arg(nextIdx + 1).arg(m_runQueue.size())
                .arg(QString::fromStdString(m_runQueue[nextIdx].name)));
        return;   // stay in running state - don't update toolbar
    }

    // Full stop (last spec finished, or Stop All was used)
    m_specAdvancing = false;
    m_runQueueIdx   = 0;
    m_pageAccess->setActiveSpecIndex(-1);   // clear the green arrow
    setRunningState(false);
    m_statusLeft->setText("Test stopped.");

    if (m_resultsFile) {
        m_resultsFile->close();
        delete m_resultsFile;
        m_resultsFile = nullptr;
    }
}

void QtMainWindow::onResultsUpdated(QVector<WorkerResult> results)
{
    for (const auto &r : results) {
        if (r.isAggregate) {
            m_statusLeft->setText(
                QString("%1 IOps  |  %2 MBps")
                    .arg(r.iops, 0, 'f', 0)
                    .arg(r.mbpsDec, 0, 'f', 1));
            break;
        }
    }

    // Append one row per worker (plus aggregate) to the open results file
    if (m_resultsFile && m_resultsFile->isOpen()) {
        QTextStream out(m_resultsFile);
        const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        for (const auto &r : results) {
            out << ts                    << ','
                << QString::fromStdString(r.managerName) << ','
                << QString::fromStdString(r.workerName)  << ','
                << r.iops                << ','
                << r.readIops            << ','
                << r.writeIops           << ','
                << r.mbpsDec             << ','
                << r.readMbpsDec         << ','
                << r.writeMbpsDec        << ','
                << r.mbpsBin             << ','
                << r.avgLatencyMs        << ','
                << r.maxLatencyMs        << ','
                << r.cpuUtil             << ','
                << r.cpuUser             << ','
                << r.cpuKernel           << ','
                << r.errors              << '\n';
        }
    }
}

void QtMainWindow::onStatusMessage(QString msg) { m_statusLeft->setText(msg); }

void QtMainWindow::onManagerConnected(ManagerInfo)
{
    rebuildWorkerTree();
    m_pageSetup->refreshForEngine();
    m_pageDisplay->refreshWorkerAssignments();
}

void QtMainWindow::onManagerDisconnected(QString) { rebuildWorkerTree(); }

void QtMainWindow::onConfigChanged()
{
    rebuildWorkerTree();
    m_pageAccess->loadSpecList();
    updateTopologyButtons();
}

void QtMainWindow::setRunningState(bool running)
{
    m_running = running;

    // File operations disabled while running
    m_actOpen->setEnabled(!running);
    m_actSave->setEnabled(true);      // can save config at any time

    // Topology operations disabled while running
    m_actNewDynamo->setEnabled(!running);
    m_actNewDiskWorker->setEnabled(false);   // re-evaluated by updateTopologyButtons
    m_actNewNetWorker->setEnabled(false);
    m_actCopyWorker->setEnabled(false);
    m_actExitOne->setEnabled(false);

    // Test control
    m_actStart->setEnabled(!running);
    m_actStop->setEnabled(running);
    // Stop All enabled when: running with multiple specs cycling, OR multiple managers
    m_actStopAll->setEnabled(running && (m_runQueue.size() > 1 || m_engine->managers().size() > 1));

    // Reset disabled while running
    m_actNew->setEnabled(!running);

    // Restore per-selection enable states when not running
    if (!running)
        updateTopologyButtons();
}

void QtMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_running) {
        const auto btn = QMessageBox::question(
            this, "Test Running",
            "A test is still running. Stop it and exit?",
            QMessageBox::Yes | QMessageBox::No);
        if (btn != QMessageBox::Yes) { event->ignore(); return; }
        m_engine->stopAll();
    }
    event->accept();
}
