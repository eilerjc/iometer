// DemoEngine.cpp
#include "DemoEngine.h"
#include "IometerEngine.h"
#include <cmath>
#include <QRandomGenerator>

static constexpr double PI = 3.14159265358979323846;

DemoEngine::DemoEngine(QObject *parent) : IometerEngine(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &DemoEngine::tick);
    m_timer.setInterval(500);   // update twice per second
    buildDefaultConfig();
}

void DemoEngine::buildDefaultConfig()
{
    // ---- Local manager with 4 disk workers ----------------------------------
    ManagerInfo local;
    local.name           = "Local System";
    local.address        = "127.0.0.1";
    local.connected      = true;
    local.processorCount = 4;   // simulated quad-core

    // Available targets visible to the demo system (matches the Disk Targets tab)
    // C: has a filesystem but no iobw.tst → unprepared (red slash)
    // W: has a filesystem and iobw.tst   → prepared (normal icon)
    // PhysicalDriveN are raw devices     → always testable (normal icon)
    local.availableTargets = {
        TargetInfo{"C: \"Windows\"",         TargetKind::LogicalDisk,  false},
        TargetInfo{"W: \"nvme\"",            TargetKind::LogicalDisk,  true},
        TargetInfo{"\\\\.\\PhysicalDrive0",  TargetKind::PhysicalDisk, true},
        TargetInfo{"\\\\.\\PhysicalDrive1",  TargetKind::PhysicalDisk, true}
    };

    // Network interfaces available on this system
    local.availableNetInterfaces = {
        "192.168.1.100",
        "10.0.0.1"
    };

    const QStringList diskNames = {"W:", "X:", "Y:", "Z:"};
    for (int i = 0; i < 4; ++i) {
        WorkerInfo w;
        w.id          = QString("local-w%1").arg(i);
        w.name        = QString("Worker %1").arg(i + 1);
        w.type        = "Disk";
        w.managerName = local.name;
        w.targets     = QStringList{ diskNames[i] };
        w.queueDepth  = (i == 0) ? 32 : (i == 1) ? 16 : 8;
        local.workers.append(w);
    }
    m_managers.append(local);

    // ---- Access specs: use shared built-in library -------
    m_specs = IometerEngine::builtinAccessSpecs();
}

// ---- Test control -----------------------------------------------------------

void DemoEngine::startTest()
{
    if (m_running) return;
    m_running = true;
    m_t       = 0.0;
    m_timer.start();
    emit testStarted();
    emit statusMessage("Test running...");
}

void DemoEngine::stopTest()
{
    if (!m_running) return;
    m_running = false;
    m_timer.stop();

    if (!m_current.isEmpty())
        m_saved.append(m_current.last());

    emit testStopped();
    emit statusMessage("Test stopped.");
}

void DemoEngine::stopAll()
{
    stopTest();
}

// ---- Simulation tick --------------------------------------------------------

WorkerResult DemoEngine::makeResult(const WorkerInfo &w, const QString &mgrName, double t) const
{
    int idx = 0;
    for (int i = 0; i < m_managers.first().workers.size(); ++i)
        if (m_managers.first().workers[i].id == w.id) { idx = i; break; }
    const double phase = idx * 0.7;

    const double baseMbps = 500.0 + 100.0 * idx;
    const double waveMbps = 80.0  * std::sin(t * 0.4 + phase);
    const double spike    = (std::fmod(t + phase, 15.0) < 0.6) ? 250.0 : 0.0;
    const double mbps     = std::max(10.0, baseMbps + waveMbps + spike);

    const double iops       = (mbps * 1e6) / (64.0 * 1024.0);
    const double latBase    = 1000.0 / iops * w.queueDepth;
    const double latJitter  = latBase * 0.15 * std::sin(t * 1.3 + phase);

    WorkerResult r;
    r.managerName  = mgrName;
    r.workerName   = w.name;
    r.mbpsDec      = mbps;
    r.readMbpsDec  = mbps;
    r.mbpsBin      = mbps / 1.048576;
    r.iops         = iops;
    r.readIops     = iops;
    r.avgLatencyMs = std::max(0.01, latBase + latJitter);
    r.maxLatencyMs = r.avgLatencyMs * (1.5 + 0.5 * std::sin(t * 0.8 + phase));
    r.cpuUtil      = 5.0 + 3.0 * idx + 2.0 * std::sin(t * 0.6 + phase);
    r.cpuUser      = r.cpuUtil * 0.4;
    r.cpuKernel    = r.cpuUtil * 0.6;
    r.errors       = 0;
    return r;
}

void DemoEngine::tick()
{
    m_t += 0.5;
    m_current.clear();

    WorkerResult aggregate;
    aggregate.managerName = "ALL";
    aggregate.workerName  = "ALL";
    aggregate.isAggregate = true;

    for (const auto &mgr : std::as_const(m_managers)) {
        if (!mgr.connected) continue;
        for (const auto &w : mgr.workers) {
            WorkerResult r = makeResult(w, mgr.name, m_t);
            m_current.append(r);

            aggregate.iops         += r.iops;
            aggregate.readIops     += r.readIops;
            aggregate.writeIops    += r.writeIops;
            aggregate.mbpsDec      += r.mbpsDec;
            aggregate.readMbpsDec  += r.readMbpsDec;
            aggregate.writeMbpsDec += r.writeMbpsDec;
            aggregate.mbpsBin      += r.mbpsBin;
            aggregate.avgLatencyMs  = std::max(aggregate.avgLatencyMs, r.avgLatencyMs);
            aggregate.maxLatencyMs  = std::max(aggregate.maxLatencyMs, r.maxLatencyMs);
            aggregate.cpuUtil       = std::max(aggregate.cpuUtil, r.cpuUtil);
            aggregate.cpuUser       = std::max(aggregate.cpuUser, r.cpuUser);
            aggregate.cpuKernel     = std::max(aggregate.cpuKernel, r.cpuKernel);
            aggregate.errors       += r.errors;
        }
    }

    if (!m_current.isEmpty())
        m_current.prepend(aggregate);

    emit resultsUpdated(m_current);
}

// ---- Config -----------------------------------------------------------------

void DemoEngine::newConfig()
{
    m_managers.clear();
    m_specs.clear();
    m_current.clear();
    buildDefaultConfig();
    emit configChanged();
}

bool DemoEngine::loadConfig(const QString &)
{
    emit statusMessage("Config load not yet implemented in demo mode.");
    return false;
}

bool DemoEngine::saveConfig(const QString &)
{
    emit statusMessage("Config save not yet implemented in demo mode.");
    return false;
}

// ---- Manager / worker management --------------------------------------------

void DemoEngine::connectManager(const QString &address, const QString &name)
{
    for (auto &m : m_managers) {
        if (m.address == address) { m.connected = true; emit managerConnected(m); return; }
    }
    ManagerInfo m;
    m.name      = name.isEmpty() ? address : name;
    m.address   = address;
    m.connected = true;
    m_managers.append(m);
    emit managerConnected(m);
}

void DemoEngine::disconnectManager(const QString &mgrName)
{
    for (int i = 0; i < m_managers.size(); ++i) {
        if (m_managers[i].name == mgrName) {
            m_managers[i].connected = false;
            emit managerDisconnected(mgrName);
            return;
        }
    }
}

void DemoEngine::addWorker(const QString &mgrName, const WorkerInfo &w)
{
    for (auto &m : m_managers)
        if (m.name == mgrName) { m.workers.append(w); emit configChanged(); return; }
}

void DemoEngine::removeWorker(const QString &mgrName, const QString &workerId)
{
    for (auto &m : m_managers)
        if (m.name == mgrName)
            for (int i = 0; i < m.workers.size(); ++i)
                if (m.workers[i].id == workerId) { m.workers.removeAt(i); emit configChanged(); return; }
}

void DemoEngine::updateWorker(const WorkerInfo &w)
{
    for (auto &m : m_managers)
        for (auto &worker : m.workers)
            if (worker.id == w.id) { worker = w; emit configChanged(); return; }
}
