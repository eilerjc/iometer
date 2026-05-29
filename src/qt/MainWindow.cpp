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
#include <QGroupBox>
#include <QVBoxLayout>
#include <QStyle>

// =============================================================================
// Icon helpers -- draw classic raised-3D bitmaps matching the original toolbar
// =============================================================================

static void paintRaisedBox(QPainter &p, int w, int h)
{
    // Windows classic raised-3D button background
    p.fillRect(0, 0, w, h, QColor(0xd4, 0xd0, 0xc8));
    p.setPen(QColor(0xff, 0xff, 0xff));
    p.drawLine(0, 0, w-2, 0);   // top highlight
    p.drawLine(0, 0, 0, h-2);   // left highlight
    p.setPen(QColor(0x80, 0x80, 0x80));
    p.drawLine(0, h-1, w-1, h-1); // bottom shadow
    p.drawLine(w-1, 0, w-1, h-1); // right shadow
    p.setPen(QColor(0x40, 0x40, 0x40));
    p.drawLine(1, h-2, w-2, h-2);
    p.drawLine(w-2, 1, w-2, h-2);
}

QIcon MainWindow::makeNewIcon()
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    // White page with fold
    p.setBrush(Qt::white);
    p.setPen(QColor(0x60, 0x60, 0x60));
    p.drawRect(5, 3, 11, 16);
    // Fold corner
    p.setBrush(QColor(0xd4, 0xd0, 0xc8));
    p.drawPolygon(QPolygon({ QPoint(12,3), QPoint(16,7), QPoint(16,3) }));
    p.setBrush(QColor(0xaa, 0xaa, 0xaa));
    p.drawPolygon(QPolygon({ QPoint(12,3), QPoint(16,7), QPoint(12,7) }));
    // Lines on page
    p.setPen(QColor(0xb0, 0xb0, 0xb0));
    for (int y : {9, 12, 15})
        p.drawLine(7, y, 14, y);
    return QIcon(pm);
}

QIcon MainWindow::makeOpenIcon()
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    // Folder body
    p.setBrush(QColor(0xff, 0xcc, 0x33));
    p.setPen(QColor(0x80, 0x60, 0x00));
    // Tab
    p.drawRect(4, 8, 5, 3);
    // Body
    p.drawRect(3, 10, 16, 9);
    // Open flap (slightly lighter)
    p.setBrush(QColor(0xff, 0xdd, 0x66));
    p.drawPolygon(QPolygon({ QPoint(3,10), QPoint(19,10), QPoint(17,7), QPoint(3,7) }));
    return QIcon(pm);
}

QIcon MainWindow::makeSaveIcon()
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    // Floppy body
    p.setBrush(QColor(0x44, 0x88, 0xcc));
    p.setPen(QColor(0x22, 0x44, 0x88));
    p.drawRect(4, 3, 16, 18);
    // White label area
    p.setBrush(Qt::white);
    p.setPen(QColor(0x88, 0x88, 0x88));
    p.drawRect(6, 4, 12, 7);
    // Shutter slot
    p.setBrush(QColor(0x22, 0x22, 0x22));
    p.setPen(Qt::NoPen);
    p.drawRect(10, 5, 4, 5);
    // Lines on label
    p.setPen(QColor(0xcc, 0xcc, 0xcc));
    p.drawLine(7, 7, 11, 7);
    p.drawLine(7, 9, 11, 9);
    return QIcon(pm);
}

QIcon MainWindow::makeStartIcon()
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    p.setRenderHint(QPainter::Antialiasing);
    // Green right-pointing triangle (play button)
    QPolygon tri;
    tri << QPoint(7, 5) << QPoint(7, 19) << QPoint(19, 12);
    p.setBrush(QColor(0x00, 0xcc, 0x44));
    p.setPen(QColor(0x00, 0x66, 0x22));
    p.drawPolygon(tri);
    return QIcon(pm);
}

QIcon MainWindow::makeStopIcon(bool all)
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    p.setRenderHint(QPainter::Antialiasing);
    // Red octagon
    const int cx = 12, cy = all ? 10 : 12, r = all ? 7 : 9, cut = 2;
    QPolygon oct;
    oct << QPoint(cx-r+cut, cy-r) << QPoint(cx+r-cut, cy-r)
        << QPoint(cx+r, cy-r+cut) << QPoint(cx+r, cy+r-cut)
        << QPoint(cx+r-cut, cy+r) << QPoint(cx-r+cut, cy+r)
        << QPoint(cx-r, cy+r-cut) << QPoint(cx-r, cy-r+cut);
    p.setBrush(QColor(0xcc, 0x00, 0x00));
    p.setPen(QColor(0x66, 0x00, 0x00));
    p.drawPolygon(oct);
    // "STOP" text
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", all ? 4 : 5, QFont::Bold));
    p.drawText(QRect(cx-6, cy-5, 12, 10), Qt::AlignCenter, "STOP");
    if (all) {
        p.setFont(QFont("Arial", 4, QFont::Bold));
        p.setPen(QColor(0xcc, 0x00, 0x00));
        p.drawText(QRect(3, 18, 18, 5), Qt::AlignCenter, "ALL");
    }
    return QIcon(pm);
}

QIcon MainWindow::makeMeterIcon()
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintRaisedBox(p, 24, 24);
    p.setRenderHint(QPainter::Antialiasing);
    // Dark circular face
    p.setBrush(QColor(0x20, 0x20, 0x20));
    p.setPen(QColor(0x80, 0x80, 0x80));
    p.drawEllipse(3, 3, 18, 18);
    // Tick marks
    p.setPen(Qt::white);
    for (int i = 0; i <= 6; ++i) {
        const double a = (200.0 + i * (140.0/6.0)) * M_PI / 180.0;
        const int x1 = 12 + (int)(8.0 * std::cos(a));
        const int y1 = 12 + (int)(8.0 * std::sin(a));
        const int x2 = 12 + (int)(6.5 * std::cos(a));
        const int y2 = 12 + (int)(6.5 * std::sin(a));
        p.drawLine(x1, y1, x2, y2);
    }
    // Red needle
    p.setPen(QPen(Qt::red, 1));
    p.drawLine(12, 12, 12 + 5, 17);
    return QIcon(pm);
}

// =============================================================================
// MainWindow
// =============================================================================

MainWindow::MainWindow(IometerEngine *engine, QWidget *parent)
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

    connect(engine, &IometerEngine::testStarted,         this, &MainWindow::onTestStarted);
    connect(engine, &IometerEngine::testStopped,         this, &MainWindow::onTestStopped);
    connect(engine, &IometerEngine::resultsUpdated,      this, &MainWindow::onResultsUpdated);
    connect(engine, &IometerEngine::statusMessage,       this, &MainWindow::onStatusMessage);
    connect(engine, &IometerEngine::managerConnected,    this, &MainWindow::onManagerConnected);
    connect(engine, &IometerEngine::managerDisconnected, this, &MainWindow::onManagerDisconnected);
    connect(engine, &IometerEngine::configChanged,       this, &MainWindow::onConfigChanged);
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void MainWindow::setupToolBar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(24, 24));
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_actNew      = tb->addAction(makeNewIcon(),       "New");
    m_actOpen     = tb->addAction(makeOpenIcon(),      "Open");
    m_actSave     = tb->addAction(makeSaveIcon(),      "Save");
    tb->addSeparator();
    m_actStart    = tb->addAction(makeStartIcon(),     "Start");
    m_actStop     = tb->addAction(makeStopIcon(false), "Stop");
    m_actStopAll  = tb->addAction(makeStopIcon(true),  "Stop All");
    tb->addSeparator();
    m_actBigMeter = tb->addAction(makeMeterIcon(),     "Presentation Meter");

    m_actNew->setToolTip("New configuration (Ctrl+N)");
    m_actOpen->setToolTip("Open .icf file (Ctrl+O)");
    m_actSave->setToolTip("Save .icf file (Ctrl+S)");
    m_actStart->setToolTip("Start test (F5)");
    m_actStop->setToolTip("Stop current test (F6)");
    m_actStopAll->setToolTip("Stop all tests");
    m_actBigMeter->setToolTip("Open Presentation Meter");

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
        m_tabs->setCurrentIndex(3);   // Results Display tab
        m_pageDisplay->onBigMeterClicked();
    });

    // Menu bar
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QApplication::instance(), &QApplication::quit,
                        QKeySequence::Quit);

    auto *testMenu = menuBar()->addMenu("&Test");
    testMenu->addAction(m_actStart);
    testMenu->addAction(m_actStop);
    testMenu->addAction(m_actStopAll);

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_actBigMeter);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Iometer", this, &MainWindow::onAbout);
}

void MainWindow::setupTopologyPanel()
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
            this, &MainWindow::onWorkerTreeSelectionChanged);
}

void MainWindow::setupTabs()
{
    m_pageSetup   = new PageSetup(m_engine);
    m_pageNetwork = new PageNetwork(m_engine);
    m_pageAccess  = new PageAccess(m_engine);
    m_pageDisplay = new PageDisplay(m_engine);
    m_pageResults = new PageResults(m_engine);

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
    connect(m_engine, &IometerEngine::resultsUpdated,
            m_pageDisplay, &PageDisplay::updateResults);
    connect(m_engine, &IometerEngine::testStarted,
            m_pageDisplay, &PageDisplay::onTestStarted);
    connect(m_engine, &IometerEngine::testStopped,
            m_pageDisplay, &PageDisplay::onTestStopped);
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

// -----------------------------------------------------------------------------
// Topology tree
// -----------------------------------------------------------------------------

void MainWindow::rebuildWorkerTree()
{
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
        mgrItem->setText(0, mgr.name);
        mgrItem->setData(0, Qt::UserRole, "mgr:" + mgr.name);
        mgrItem->setIcon(0, computerIcon);
        mgrItem->setExpanded(true);

        for (const auto &w : mgr.workers) {
            auto *wItem = new QTreeWidgetItem(mgrItem);
            wItem->setText(0, w.name);
            wItem->setData(0, Qt::UserRole, "worker:" + mgr.name + ":" + w.id);
            wItem->setIcon(0, w.type == "Network" ? netIcon : hdIcon);
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
    if (!item) {
        m_pageSetup->clearSelection();
        return;
    }

    const QString key = item->data(0, Qt::UserRole).toString();

    if (key == "all") {
        m_pageSetup->clearSelection();
        m_pageNetwork->clearSelection();
    } else if (key.startsWith("mgr:")) {
        const QString mgrName = key.mid(4);
        m_pageSetup->setSelectedManager(mgrName);
        m_pageNetwork->setSelectedManager(mgrName);
    } else if (key.startsWith("worker:")) {
        const QStringList parts = key.mid(7).split(':');
        if (parts.size() >= 2) {
            m_pageSetup->setSelectedWorker(parts[0], parts[1]);
            m_pageNetwork->setSelectedWorker(parts[0], parts[1]);
        }
    }
}

// -----------------------------------------------------------------------------
// Toolbar / menu actions
// -----------------------------------------------------------------------------

void MainWindow::onNew()
{
    if (m_running) {
        QMessageBox::warning(this, "Test Running",
            "Please stop the test before creating a new configuration.");
        return;
    }
    m_engine->newConfig();
    setWindowTitle(QString("Iometer 1.1.0 [Built: %1 %2]").arg(__DATE__, __TIME__));
}

void MainWindow::onOpen()
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

void MainWindow::onSave()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save ICF File", {}, "Iometer Config (*.icf);;All Files (*)");
    if (path.isEmpty()) return;
    if (!m_engine->saveConfig(path))
        QMessageBox::warning(this, "Save Failed", "Could not save: " + path);
    else
        setWindowTitle("Iometer -- " + path);
}

void MainWindow::onStart()    { if (!m_running) m_engine->startTest(); }
void MainWindow::onStop()     { m_engine->stopTest(); }
void MainWindow::onStopAll()  { m_engine->stopAll(); }

void MainWindow::onAbout()
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

void MainWindow::onTestStarted()
{
    setRunningState(true);
    m_tabs->setCurrentIndex(3);   // jump to Results Display
    m_statusLeft->setText("Test running...");
}

void MainWindow::onTestStopped()
{
    setRunningState(false);
    m_statusLeft->setText("Test stopped.");
}

void MainWindow::onResultsUpdated(QVector<WorkerResult> results)
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
}

void MainWindow::onStatusMessage(QString msg) { m_statusLeft->setText(msg); }

void MainWindow::onManagerConnected(ManagerInfo)
{
    rebuildWorkerTree();
    m_pageSetup->refreshForEngine();
    m_pageDisplay->refreshWorkerAssignments();
}

void MainWindow::onManagerDisconnected(QString) { rebuildWorkerTree(); }

void MainWindow::onConfigChanged()
{
    rebuildWorkerTree();
    m_pageAccess->loadSpecList();
}

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
