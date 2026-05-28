// MainWindow.cpp
#include "MainWindow.h"
#include "IometerEngine.h"
#include "PageDisplay.h"
#include "PageSetup.h"
#include "PageAccess.h"
#include "PageResults.h"
#include "PageNetwork.h"

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
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QHeaderView>

// ─────────────────────────────────────────────────────────────────────────────

static const QString APP_STYLE = R"(
QMainWindow { background: #0d1117; }
QMenuBar  { background:#0d1117; color:#aabbcc; }
QMenuBar::item:selected { background:#1a2a3a; }
QMenu     { background:#0d1520; color:#ccddff; border:1px solid #2a3a4a; }
QMenu::item:selected { background:#1a3a6a; }
QToolBar  { background:#111820; border-bottom:1px solid #2a3a4a; spacing:4px; padding:2px; }
QTabWidget::pane { border:1px solid #2a3a4a; background:#111820; }
QTabBar::tab {
    background:#0d1520; color:#8899aa; border:1px solid #2a3a4a;
    padding:5px 16px; margin-right:2px; border-bottom:none;
}
QTabBar::tab:selected { background:#111820; color:#aaddff; border-bottom:1px solid #111820; }
QTabBar::tab:hover    { background:#1a2530; color:#ccddff; }
QSplitter::handle { background:#1a2a3a; width:5px; }
QTreeWidget {
    background:#0d1520; color:#ccddff;
    border:1px solid #2a3a4a; border-right:none;
}
QTreeWidget::item:selected { background:#1a3a6a; }
QTreeWidget::branch { background:#0d1520; }
QHeaderView::section { background:#0d1520; color:#556677; border:none; padding:2px; }
QStatusBar { background:#080d14; color:#5577aa; border-top:1px solid #1a2a3a; font-size:11px; }
)";

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(IometerEngine *engine, QWidget *parent)
    : QMainWindow(parent), m_engine(engine)
{
    setWindowTitle("Iometer — Qt GUI");
    resize(1100, 720);
    setStyleSheet(APP_STYLE);

    setupToolBar();
    setupWorkerTree();
    setupTabs();
    setupStatusBar();
    rebuildWorkerTree();
    setRunningState(false);

    // ── Engine signals ────────────────────────────────────────────────────
    connect(engine, &IometerEngine::testStarted,
            this, &MainWindow::onTestStarted);
    connect(engine, &IometerEngine::testStopped,
            this, &MainWindow::onTestStopped);
    connect(engine, &IometerEngine::resultsUpdated,
            this, &MainWindow::onResultsUpdated);
    connect(engine, &IometerEngine::statusMessage,
            this, &MainWindow::onStatusMessage);
    connect(engine, &IometerEngine::managerConnected,
            this, &MainWindow::onManagerConnected);
    connect(engine, &IometerEngine::managerDisconnected,
            this, &MainWindow::onManagerDisconnected);
    connect(engine, &IometerEngine::configChanged,
            this, &MainWindow::onConfigChanged);
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // Helper to build a plain-text "icon" as a coloured square
    auto makeColorIcon = [](const QColor &c, const QString &ch) -> QIcon {
        QPixmap pm(20, 20);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.fillRect(pm.rect(), c.darker(140));
        p.setPen(c.lighter(150));
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(pm.rect(), Qt::AlignCenter, ch);
        return QIcon(pm);
    };

    m_actNew     = tb->addAction(makeColorIcon(QColor(0x4499ff), "N"), "New");
    m_actOpen    = tb->addAction(makeColorIcon(QColor(0x4499ff), "O"), "Open");
    m_actSave    = tb->addAction(makeColorIcon(QColor(0x4499ff), "S"), "Save");
    tb->addSeparator();
    m_actStart   = tb->addAction(makeColorIcon(QColor(0x44ee88), "▶"), "Start");
    m_actStop    = tb->addAction(makeColorIcon(QColor(0xffaa44), "■"), "Stop");
    m_actStopAll = tb->addAction(makeColorIcon(QColor(0xff4444), "⏹"), "Stop All");
    tb->addSeparator();
    m_actBigMeter = tb->addAction(makeColorIcon(QColor(0xaa66ff), "⚡"), "BigMeter");

    // Tooltips
    m_actNew->setToolTip("New configuration (Ctrl+N)");
    m_actOpen->setToolTip("Open .icf file (Ctrl+O)");
    m_actSave->setToolTip("Save .icf file (Ctrl+S)");
    m_actStart->setToolTip("Start test (F5)");
    m_actStop->setToolTip("Stop current test (F6)");
    m_actStopAll->setToolTip("Stop all tests");
    m_actBigMeter->setToolTip("Open Presentation Meter");

    // Keyboard shortcuts
    m_actNew->setShortcut(QKeySequence::New);
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actSave->setShortcut(QKeySequence::Save);
    m_actStart->setShortcut(Qt::Key_F5);
    m_actStop->setShortcut(Qt::Key_F6);

    connect(m_actNew,      &QAction::triggered, this, &MainWindow::onNew);
    connect(m_actOpen,     &QAction::triggered, this, &MainWindow::onOpen);
    connect(m_actSave,     &QAction::triggered, this, &MainWindow::onSave);
    connect(m_actStart,    &QAction::triggered, this, &MainWindow::onStart);
    connect(m_actStop,     &QAction::triggered, this, &MainWindow::onStop);
    connect(m_actStopAll,  &QAction::triggered, this, &MainWindow::onStopAll);
    connect(m_actBigMeter, &QAction::triggered, this, [this]{
        m_tabs->setCurrentIndex(2);   // switch to Display tab
        m_pageDisplay->onBigMeterClicked();
    });

    // ── Menu bar ──────────────────────────────────────────────────────────
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QApplication::instance(), &QApplication::quit, QKeySequence::Quit);

    auto *testMenu = menuBar()->addMenu("&Test");
    testMenu->addAction(m_actStart);
    testMenu->addAction(m_actStop);
    testMenu->addAction(m_actStopAll);

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_actBigMeter);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Iometer", this, &MainWindow::onAbout);
}

void MainWindow::setupWorkerTree()
{
    m_workerTree = new QTreeWidget;
    m_workerTree->setHeaderHidden(true);
    m_workerTree->setIndentation(14);
    m_workerTree->setMinimumWidth(160);
    m_workerTree->setMaximumWidth(280);

    QFont tf("Consolas", 10);
    m_workerTree->setFont(tf);

    connect(m_workerTree, &QTreeWidget::currentItemChanged,
            this, &MainWindow::onWorkerTreeSelectionChanged);
}

void MainWindow::setupTabs()
{
    m_pageSetup   = new PageSetup(m_engine);
    m_pageAccess  = new PageAccess(m_engine);
    m_pageDisplay = new PageDisplay(m_engine);
    m_pageResults = new PageResults(m_engine);
    m_pageNetwork = new PageNetwork(m_engine);

    m_tabs = new QTabWidget;
    m_tabs->addTab(m_pageSetup,   "⚙  Setup");
    m_tabs->addTab(m_pageAccess,  "📋  Access Specs");
    m_tabs->addTab(m_pageDisplay, "📊  Display");
    m_tabs->addTab(m_pageResults, "📈  Results");
    m_tabs->addTab(m_pageNetwork, "🌐  Network");
    m_tabs->setCurrentIndex(0);

    // ── Assemble the splitter ─────────────────────────────────────────────
    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(m_workerTree);
    m_splitter->addWidget(m_tabs);
    m_splitter->setSizes({200, 900});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    setCentralWidget(m_splitter);

    // ── Forward engine signals to tab pages ───────────────────────────────
    connect(m_engine, &IometerEngine::resultsUpdated,
            m_pageDisplay, &PageDisplay::updateResults);
    connect(m_engine, &IometerEngine::resultsUpdated,
            m_pageResults, &PageResults::updateResults);
    connect(m_engine, &IometerEngine::testStarted,
            m_pageDisplay, &PageDisplay::onTestStarted);
    connect(m_engine, &IometerEngine::testStopped,
            m_pageDisplay, &PageDisplay::onTestStopped);
    connect(m_engine, &IometerEngine::testStopped,
            m_pageResults, &PageResults::onTestStopped);
    connect(m_engine, &IometerEngine::managerConnected,
            m_pageNetwork, &PageNetwork::onManagerConnected);
    connect(m_engine, &IometerEngine::managerDisconnected,
            m_pageNetwork, &PageNetwork::onManagerDisconnected);
}

void MainWindow::setupStatusBar()
{
    m_statusLeft  = new QLabel("Ready");
    m_statusRight = new QLabel("0 workers | 0 managers");
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusRight);
}

// ─────────────────────────────────────────────────────────────────────────────
// Worker tree
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::rebuildWorkerTree()
{
    m_workerTree->clear();

    // "All Managers" root
    auto *root = new QTreeWidgetItem(m_workerTree);
    root->setText(0, "All Managers");
    root->setData(0, Qt::UserRole, "all");
    QFont rf = root->font(0);
    rf.setBold(true);
    root->setFont(0, rf);
    root->setForeground(0, QColor(0x55, 0x99, 0xff));

    int workerCount = 0;
    for (const auto &mgr : m_engine->managers()) {
        auto *mgrItem = new QTreeWidgetItem(root);
        mgrItem->setText(0, mgr.name);
        mgrItem->setData(0, Qt::UserRole, "mgr:" + mgr.name);
        mgrItem->setForeground(0, mgr.connected ? QColor(0x88, 0xcc, 0x88) : QColor(0xaa, 0x55, 0x55));
        mgrItem->setExpanded(true);

        for (const auto &w : mgr.workers) {
            auto *wItem = new QTreeWidgetItem(mgrItem);
            wItem->setText(0, QString("  %1 [%2]").arg(w.name, w.type));
            wItem->setData(0, Qt::UserRole, "worker:" + mgr.name + ":" + w.id);
            wItem->setForeground(0, QColor(0xaa, 0xbb, 0xcc));
            ++workerCount;
        }
    }
    root->setExpanded(true);

    m_statusRight->setText(QString("%1 worker(s) | %2 manager(s)")
        .arg(workerCount).arg(m_engine->managers().size()));
}

void MainWindow::onWorkerTreeSelectionChanged()
{
    auto *item = m_workerTree->currentItem();
    if (!item) return;

    const QString key = item->data(0, Qt::UserRole).toString();
    if (key.startsWith("worker:")) {
        m_tabs->setCurrentIndex(0);   // switch to Setup tab
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolbar / menu actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onNew()
{
    if (m_running) {
        QMessageBox::warning(this, "Test Running", "Please stop the test before creating a new configuration.");
        return;
    }
    m_engine->newConfig();
    setWindowTitle("Iometer — Qt GUI");
}

void MainWindow::onOpen()
{
    if (m_running) {
        QMessageBox::warning(this, "Test Running", "Please stop the test before loading a configuration.");
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, "Open ICF File", {}, "Iometer Config (*.icf);;All Files (*)");
    if (path.isEmpty()) return;
    if (!m_engine->loadConfig(path))
        QMessageBox::warning(this, "Load Failed", "Could not load: " + path);
    else
        setWindowTitle("Iometer — " + path);
}

void MainWindow::onSave()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save ICF File", {}, "Iometer Config (*.icf);;All Files (*)");
    if (path.isEmpty()) return;
    if (!m_engine->saveConfig(path))
        QMessageBox::warning(this, "Save Failed", "Could not save: " + path);
    else
        setWindowTitle("Iometer — " + path);
}

void MainWindow::onStart()
{
    if (m_running) return;
    m_engine->startTest();
}

void MainWindow::onStop()
{
    m_engine->stopTest();
}

void MainWindow::onStopAll()
{
    m_engine->stopAll();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About Iometer Qt GUI",
        "<h2>Iometer Qt GUI</h2>"
        "<p>Cross-platform Qt port of the Iometer benchmark GUI.</p>"
        "<p>Original Iometer by Intel Corporation. Qt GUI port by the Iometer Project.</p>"
        "<p>This GUI communicates with <b>Dynamo.exe</b> over TCP port 1066.<br>"
        "Run <code>Dynamo.exe -i &lt;this-host&gt; -m &lt;hostname&gt;</code> to connect a worker.</p>"
        "<p><a href='https://github.com/eilerjc/iometer'>github.com/eilerjc/iometer</a></p>");
}

// ─────────────────────────────────────────────────────────────────────────────
// Engine signal handlers
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onTestStarted()
{
    setRunningState(true);
    m_tabs->setCurrentIndex(2);   // switch to Display tab when test starts
    m_statusLeft->setText("● Test running...");
    m_statusLeft->setStyleSheet("color:#44ff88; font-weight:bold;");
}

void MainWindow::onTestStopped()
{
    setRunningState(false);
    m_statusLeft->setText("■ Test stopped");
    m_statusLeft->setStyleSheet("color:#ffaa44;");
}

void MainWindow::onResultsUpdated(QVector<WorkerResult> results)
{
    // Show aggregate IOps in status bar
    for (const auto &r : results) {
        if (r.isAggregate) {
            m_statusLeft->setText(QString("● %.0f IOps  |  %.1f MBps")
                .arg(r.iops).arg(r.mbpsDec));
            break;
        }
    }
}

void MainWindow::onStatusMessage(QString msg)
{
    m_statusLeft->setText(msg);
    m_statusLeft->setStyleSheet("color:#ccddff;");
}

void MainWindow::onManagerConnected(ManagerInfo)   { rebuildWorkerTree(); m_pageSetup->refreshWorkers(); m_pageDisplay->refreshWorkerAssignments(); }
void MainWindow::onManagerDisconnected(QString)    { rebuildWorkerTree(); }
void MainWindow::onConfigChanged()                 { rebuildWorkerTree(); }

void MainWindow::setRunningState(bool running)
{
    m_running = running;
    m_actStart->setEnabled(!running);
    m_actStop->setEnabled(running);
    m_actStopAll->setEnabled(running);
    m_actNew->setEnabled(!running);
    m_actOpen->setEnabled(!running);
}

void MainWindow::closeEvent(QCloseEvent *event)
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
