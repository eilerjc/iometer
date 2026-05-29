// DynamoEngine.cpp
#include "DynamoEngine.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>
#include <cstring>
#include <cmath>

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Compute IOps / MBps from a single target's Raw_Result delta + elapsed seconds.
static void accumulateRaw(const DyRawResult &r, double elapsed,
                           WorkerResult &out)
{
    if (elapsed <= 0.0) return;
    out.iops       += (r.read_count + r.write_count) / elapsed;
    out.readIops   += r.read_count  / elapsed;
    out.writeIops  += r.write_count / elapsed;
    out.mbpsDec    += (r.bytes_read + r.bytes_written) / elapsed / 1.0e6;
    out.readMbpsDec  += r.bytes_read    / elapsed / 1.0e6;
    out.writeMbpsDec += r.bytes_written / elapsed / 1.0e6;
    out.mbpsBin    += (r.bytes_read + r.bytes_written) / elapsed / (1024.0*1024.0);
    out.errors     += static_cast<int>(r.read_errors + r.write_errors);

    // Latency in milliseconds
    const uint64_t totalIos = r.read_count + r.write_count;
    if (totalIos > 0) {
        // latency_sum is in ticks; caller should pass elapsed already in secs
        // so ticks = sum / timer_resolution is done outside
        // Here we just keep a running sum, finalised after accumulation
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DySession
// ─────────────────────────────────────────────────────────────────────────────

DySession::DySession(QTcpSocket *loginSocket, QObject *parent)
    : QObject(parent), m_loginSocket(loginSocket)
{
    // The login socket belongs to us now
    m_loginSocket->setParent(this);

    connect(m_loginSocket, &QTcpSocket::readyRead,
            this, &DySession::onLoginData);
    connect(m_loginSocket, &QTcpSocket::disconnected,
            this, [this]{ m_loginSocket->deleteLater(); m_loginSocket = nullptr; });

    // Prime the state machine — expect 8-byte login Message
    m_expecting   = DY_MSG_SIZE;
    m_afterExpect = State::WaitLoginMsg;

    // Update timer (starts only when test is running)
    m_updateTimer.setSingleShot(false);
    connect(&m_updateTimer, &QTimer::timeout, this, &DySession::onUpdateTimer);
}

DySession::~DySession()
{
    m_updateTimer.stop();
    if (m_loginSocket) m_loginSocket->abort();
    if (m_mainSocket)  m_mainSocket->abort();
}

// ── Low-level socket helpers ──────────────────────────────────────────────────

void DySession::sendMsg(int32_t purpose, int32_t data)
{
    if (!m_mainSocket || m_mainSocket->state() != QAbstractSocket::ConnectedState)
        return;
    DyMsg msg;
    msg.purpose = purpose;
    msg.data    = data;
    m_mainSocket->write(reinterpret_cast<const char*>(&msg), sizeof(msg));
}

void DySession::sendDataMsg(DyDataMessage *dm)
{
    if (!m_mainSocket || m_mainSocket->state() != QAbstractSocket::ConnectedState)
        return;
    dm->size  = DY_DATAMSG_SIZE;
    dm->count = dm->count;  // already set by caller
    m_mainSocket->write(reinterpret_cast<const char*>(dm), DY_DATAMSG_SIZE);
}

bool DySession::consume(int bytes)
{
    // Append any newly-arrived data into m_buf then check
    return m_buf.size() >= bytes;
}

QByteArray DySession::take(int bytes)
{
    QByteArray chunk = m_buf.left(bytes);
    m_buf.remove(0, bytes);
    return chunk;
}

// ── Login socket handler ──────────────────────────────────────────────────────

void DySession::onLoginData()
{
    // Append whatever arrived
    m_buf += m_loginSocket->readAll();

    if (m_state == State::WaitLoginMsg && m_buf.size() >= DY_MSG_SIZE) {
        processLoginMsg();
    }
    if (m_state == State::WaitLoginData && m_buf.size() >= DY_DATAMSG_SIZE) {
        processLoginData();
    }
}

void DySession::processLoginMsg()
{
    QByteArray chunk = take(DY_MSG_SIZE);
    const DyMsg *msg = reinterpret_cast<const DyMsg*>(chunk.constData());

    if (msg->purpose != DY_LOGIN) {
        qWarning() << "DySession: expected LOGIN, got" << msg->purpose;
        m_state = State::Error;
        emit errorOccurred(this, "Unexpected login message");
        return;
    }

    // Check version compatibility (ignore sub-minor byte)
    const uint32_t dynVer    = static_cast<uint32_t>(msg->data);
    const uint32_t ourVer    = static_cast<uint32_t>(DY_VERSION);
    if ((dynVer & DY_COMPAT_MASK) != (ourVer & DY_COMPAT_MASK)) {
        qWarning() << "DySession: version mismatch. Dynamo=" << dynVer
                   << "Ours=" << ourVer;
        // Send WRONG_VERSION
        DyMsg reply;
        reply.purpose = DY_LOGIN;
        reply.data    = DY_WRONG_VERSION;
        m_loginSocket->write(reinterpret_cast<const char*>(&reply), sizeof(reply));
        m_loginSocket->flush();
        m_state = State::Error;
        emit errorOccurred(this, QString("Version mismatch: Dynamo %1 vs Iometer %2")
                                 .arg(dynVer, 0, 16).arg(ourVer, 0, 16));
        return;
    }

    m_state = State::WaitLoginData;
    // Will be handled next time onLoginData fires with enough bytes
}

void DySession::processLoginData()
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());

    if (dm->size != DY_DATAMSG_SIZE) {
        qWarning() << "DySession: Data_Message size mismatch: got" << dm->size
                   << "expected" << DY_DATAMSG_SIZE;
        // Be lenient — continue anyway (might be a different build)
    }

    const DyManagerInfo &mi = dm->data.manager_info;
    m_managerName     = QString::fromLocal8Bit(mi.names[0]);
    m_address         = QString::fromLocal8Bit(mi.names[1]);
    m_mainPort        = mi.port_number;
    m_timerResolution = mi.timer_resolution > 0.0 ? mi.timer_resolution : 1.0;
    m_processorCount  = mi.processors > 0 ? mi.processors : 1;

    qDebug() << "DySession: login from" << m_managerName
             << "main port" << m_address << ":" << m_mainPort
             << "processors" << m_processorCount
             << "timer_res" << m_timerResolution;

    // Reply LOGIN_OK on the login socket
    DyMsg reply;
    reply.purpose = DY_LOGIN;
    reply.data    = DY_LOGIN_OK;
    m_loginSocket->write(reinterpret_cast<const char*>(&reply), sizeof(reply));
    m_loginSocket->flush();

    // Login socket is done — disconnect it
    disconnect(m_loginSocket, &QTcpSocket::readyRead, this, &DySession::onLoginData);
    m_loginSocket->disconnectFromHost();

    // Connect back to Dynamo's main port
    m_state      = State::Connecting;
    m_mainSocket = new QTcpSocket(this);
    connect(m_mainSocket, &QTcpSocket::connected,     this, &DySession::onMainConnected);
    connect(m_mainSocket, &QTcpSocket::readyRead,     this, &DySession::onMainData);
    connect(m_mainSocket, &QTcpSocket::disconnected,  this, &DySession::onMainDisconnected);

    QString host = m_address.isEmpty() ? "127.0.0.1" : m_address;
    m_mainSocket->connectToHost(host, m_mainPort);
    m_buf.clear();  // reset buffer for main-socket data
}

// ── Main socket handlers ──────────────────────────────────────────────────────

void DySession::onMainConnected()
{
    qDebug() << "DySession: main socket connected to" << m_address << ":" << m_mainPort;

    // Request disk target list
    m_state = State::WaitTargetsDisk;
    sendMsg(DY_REPORT_TARGETS, DY_MANAGER);
    // Dynamo will send 3 Data_Messages: disks, TCP, VI
}

void DySession::onMainDisconnected()
{
    m_updateTimer.stop();
    m_state = State::Disconnected;
    emit managerDisconnected(this);
}

void DySession::onMainData()
{
    m_buf += m_mainSocket->readAll();
    dispatchMainData();
}

void DySession::dispatchMainData()
{
    // Keep processing as long as we have enough data for the current state
    bool progress = true;
    while (progress) {
        progress = false;

        switch (m_state) {

        // ── Target discovery ──────────────────────────────────────────────────
        case State::WaitTargetsDisk:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processTargetList(0); progress = true;
            }
            break;
        case State::WaitTargetsTcp:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processTargetList(1); progress = true;
            }
            break;
        case State::WaitTargetsVi:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processTargetList(2); progress = true;
            }
            break;

        // ── Worker setup ──────────────────────────────────────────────────────
        case State::SetupAccess:
            // Waiting for 8-byte reply to SET_ACCESS
            if (m_buf.size() >= DY_MSG_SIZE) {
                processSetAccessReply(); progress = true;
            }
            break;
        case State::SetupTargetsReply:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processSetTargetsReply(); progress = true;
            }
            break;
        case State::SetupTargetsData:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processSetTargetsData(); progress = true;
            }
            break;

        // ── Start sequence ────────────────────────────────────────────────────
        case State::WaitStartReply:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processStartReply(); progress = true;
            }
            break;
        case State::WaitBeginIoReply:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processBeginIoReply(); progress = true;
            }
            break;

        // ── Results during test ───────────────────────────────────────────────
        case State::WaitUpdateMgr:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processUpdateMgr(); progress = true;
            }
            break;
        case State::WaitUpdateWorker:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processUpdateWorker(); progress = true;
            }
            break;

        // ── Stop sequence ─────────────────────────────────────────────────────
        case State::WaitRecordOff:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processRecordOffReply(); progress = true;
            }
            break;
        case State::WaitStop:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processStopReply(); progress = true;
            }
            break;
        case State::WaitFinalMgr:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processFinalMgr(); progress = true;
            }
            break;
        case State::WaitFinalWorker:
            if (m_buf.size() >= DY_DATAMSG_SIZE) {
                processFinalWorker(); progress = true;
            }
            break;

        default:
            break;
        }
    }
}

// ── Target discovery handlers ─────────────────────────────────────────────────

void DySession::processTargetList(int phase)
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());
    const int count = std::max(0, std::min(dm->count, DY_MAX_TARGETS));

    switch (phase) {
    case 0: // disk
        m_diskTargets.clear();
        for (int i = 0; i < count; ++i)
            m_diskTargets.append(dm->data.targets[i]);
        m_state = State::WaitTargetsTcp;
        break;
    case 1: // tcp
        m_tcpTargets.clear();
        for (int i = 0; i < count; ++i)
            m_tcpTargets.append(dm->data.targets[i]);
        m_state = State::WaitTargetsVi;
        break;
    case 2: // vi
        m_viTargets.clear();
        for (int i = 0; i < count; ++i)
            m_viTargets.append(dm->data.targets[i]);

        // All targets discovered — create one disk worker per CPU core, matching
        // the original Iometer AddDefaultWorkers() behaviour (disk_worker_count=-1
        // → use manager->processors).  Workers are named "Worker 1", "Worker 2", …
        // None have targets pre-assigned; the user assigns them via Disk Targets tab.
        m_workers.clear();
        for (int i = 0; i < m_processorCount; ++i) {
            WorkerInfo w;
            w.id          = QString("%1-disk-%2").arg(m_managerName).arg(i);
            w.name        = QString("Worker %1").arg(i + 1);
            w.type        = "Disk";
            w.managerName = m_managerName;
            w.queueDepth  = 1;
            w.targets     = {};   // no targets pre-assigned
            m_workers.append(w);
        }
        m_state = State::Ready;
        qDebug() << "DySession: ready —" << m_diskTargets.size() << "disks,"
                 << m_tcpTargets.size() << "TCP targets";
        emit managerConnected(this);
        break;
    }
}

// ── Worker setup handlers ─────────────────────────────────────────────────────

void DySession::beginSetupWorker(int idx)
{
    if (idx >= m_workers.size()) {
        // All workers configured — start the test
        beginStartSequence();
        return;
    }

    const WorkerInfo &w = m_workers[idx];
    const int wIdx      = idx;  // worker index passed to Dynamo

    // ── SET_ACCESS ──
    DyDataMessage *dm = new DyDataMessage;
    std::memset(dm, 0, sizeof(*dm));
    buildTestSpec(dm, m_accessSpecs.isEmpty()
                      ? AccessSpec{} : m_accessSpecs.first());
    dm->count = 1;

    sendMsg(DY_SET_ACCESS, wIdx);
    sendDataMsg(dm);
    delete dm;

    m_setupWorkerIdx = idx;
    m_state = State::SetupAccess;
}

void DySession::processSetAccessReply()
{
    QByteArray chunk = take(DY_MSG_SIZE);
    const DyMsg *msg = reinterpret_cast<const DyMsg*>(chunk.constData());
    if (!msg->data) {
        qWarning() << "DySession: SET_ACCESS failed for worker" << m_setupWorkerIdx;
    }

    // ── SET_TARGETS ──
    const WorkerInfo &w = m_workers[m_setupWorkerIdx];
    DyDataMessage *dm = new DyDataMessage;
    std::memset(dm, 0, sizeof(*dm));
    buildTargetMsg(dm, w);

    sendMsg(DY_SET_TARGETS, m_setupWorkerIdx);
    sendDataMsg(dm);
    delete dm;

    m_state = State::SetupTargetsReply;
}

void DySession::processSetTargetsReply()
{
    QByteArray chunk = take(DY_MSG_SIZE);
    const DyMsg *msg = reinterpret_cast<const DyMsg*>(chunk.constData());
    if (!msg->data) {
        qWarning() << "DySession: SET_TARGETS failed for worker" << m_setupWorkerIdx;
    }
    // Dynamo also sends a Data_Message back with updated target info
    m_state = State::SetupTargetsData;
}

void DySession::processSetTargetsData()
{
    // Consume the reply Data_Message (we use it only for error info)
    take(DY_DATAMSG_SIZE);
    // Move to next worker
    beginSetupWorker(m_setupWorkerIdx + 1);
}

void DySession::advanceSetupWorkers()
{
    beginSetupWorker(0);
}

// ── Start sequence ────────────────────────────────────────────────────────────

void DySession::beginStartSequence()
{
    m_activeWorkerCount = m_workers.size();
    sendMsg(DY_START, DY_ALL_WORKERS);
    m_state = State::WaitStartReply;
}

void DySession::processStartReply()
{
    take(DY_MSG_SIZE);  // consume reply
    sendMsg(DY_BEGIN_IO, DY_ALL_WORKERS);
    m_state = State::WaitBeginIoReply;
}

void DySession::processBeginIoReply()
{
    take(DY_MSG_SIZE);  // consume reply
    sendMsg(DY_RECORD_ON, DY_ALL_WORKERS);
    // RECORD_ON has no reply
    m_state = State::Running;
    m_updateTimer.start(k_updateIntervalMs);
    qDebug() << "DySession: test started on" << m_managerName;
}

// ── Update timer — request results while running ──────────────────────────────

void DySession::onUpdateTimer()
{
    if (m_state != State::Running) return;

    sendMsg(DY_REPORT_UPDATE, DY_MANAGER);
    m_state             = State::WaitUpdateMgr;
    m_workerResultIdx   = 0;
    m_pendingResults.clear();

    // Trigger immediate dispatch in case data already arrived
    dispatchMainData();
}

void DySession::processUpdateMgr()
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());
    std::memcpy(&m_pendingMgrResults, &dm->data.manager_results, sizeof(m_pendingMgrResults));

    m_state           = State::WaitUpdateWorker;
    m_workerResultIdx = 0;
    dispatchMainData();
}

void DySession::processUpdateWorker()
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());
    const DyWorkerResults &wr = dm->data.worker_results;

    const QString wName = (m_workerResultIdx < m_workers.size())
                          ? m_workers[m_workerResultIdx].name
                          : QString("Worker%1").arg(m_workerResultIdx);

    WorkerResult result = decodeWorkerResults(wr, m_managerName, wName);
    m_pendingResults.append(result);

    ++m_workerResultIdx;
    if (m_workerResultIdx >= m_activeWorkerCount) {
        // All worker results received — back to Running state
        m_state = State::Running;
        if (m_stopRequested) {
            m_stopRequested = false;
            stopTest();
        } else {
            emit resultsReady(this);
        }
    } else {
        dispatchMainData();
    }
}

// ── Stop sequence ─────────────────────────────────────────────────────────────

void DySession::stopTest()
{
    if (m_state == State::WaitUpdateMgr || m_state == State::WaitUpdateWorker) {
        // Wait until current update completes
        m_stopRequested = true;
        return;
    }
    if (m_state != State::Running) return;

    m_updateTimer.stop();
    sendMsg(DY_RECORD_OFF, DY_ALL_WORKERS);
    m_state = State::WaitRecordOff;
    dispatchMainData();
}

void DySession::stopAll()
{
    stopTest();
    if (m_mainSocket && m_mainSocket->state() == QAbstractSocket::ConnectedState) {
        sendMsg(DY_EXIT, DY_ALL_WORKERS);
        m_mainSocket->flush();
    }
}

void DySession::processRecordOffReply()
{
    take(DY_MSG_SIZE);
    sendMsg(DY_STOP, DY_ALL_WORKERS);
    m_state = State::WaitStop;
}

void DySession::processStopReply()
{
    take(DY_MSG_SIZE);
    sendMsg(DY_REPORT_RESULTS, DY_MANAGER);
    m_state           = State::WaitFinalMgr;
    m_workerResultIdx = 0;
    m_pendingResults.clear();
}

void DySession::processFinalMgr()
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());
    std::memcpy(&m_pendingMgrResults, &dm->data.manager_results, sizeof(m_pendingMgrResults));
    m_state = State::WaitFinalWorker;
    dispatchMainData();
}

void DySession::processFinalWorker()
{
    QByteArray chunk = take(DY_DATAMSG_SIZE);
    const DyDataMessage *dm = reinterpret_cast<const DyDataMessage*>(chunk.constData());
    const DyWorkerResults &wr = dm->data.worker_results;

    const QString wName = (m_workerResultIdx < m_workers.size())
                          ? m_workers[m_workerResultIdx].name
                          : QString("Worker%1").arg(m_workerResultIdx);

    m_pendingResults.append(decodeWorkerResults(wr, m_managerName, wName));
    ++m_workerResultIdx;

    if (m_workerResultIdx >= m_activeWorkerCount) {
        m_state = State::Ready;
        emit resultsReady(this);
        qDebug() << "DySession: final results received on" << m_managerName;
    } else {
        dispatchMainData();
    }
}

// ── Test control (called by DynamoEngine) ─────────────────────────────────────

void DySession::startTest(const QList<WorkerInfo> &workers, const QList<AccessSpec> &specs)
{
    if (m_state != State::Ready) return;

    m_accessSpecs     = specs.isEmpty() ? QList<AccessSpec>{AccessSpec{}} : specs;
    m_stopRequested   = false;
    m_pendingResults.clear();

    // Use the provided workers (or fall back to the auto-discovered ones)
    if (!workers.isEmpty()) {
        // Filter to only workers belonging to this session's manager
        m_workers.clear();
        for (const auto &w : workers)
            if (w.managerName == m_managerName)
                m_workers.append(w);
    }

    if (m_workers.isEmpty()) {
        qWarning() << "DySession: no workers for" << m_managerName
                   << "— creating" << m_processorCount << "default workers";
        for (int i = 0; i < m_processorCount; ++i) {
            WorkerInfo w;
            w.id          = QString("%1-disk-%2").arg(m_managerName).arg(i);
            w.name        = QString("Worker %1").arg(i + 1);
            w.type        = "Disk";
            w.managerName = m_managerName;
            w.queueDepth  = 1;
            w.targets     = {};
            m_workers.append(w);
        }
    }

    if (m_workers.isEmpty()) {
        qWarning() << "DySession: no targets discovered for" << m_managerName;
        return;
    }

    advanceSetupWorkers();
}

// ── Result decoding ───────────────────────────────────────────────────────────

WorkerResult DySession::decodeWorkerResults(const DyWorkerResults &wr,
                                             const QString &mgrName,
                                             const QString &workerName)
{
    WorkerResult r;
    r.managerName = mgrName;
    r.workerName  = workerName;
    r.isAggregate = false;

    // Elapsed time in seconds
    const uint64_t t0 = wr.time[DY_FIRST_SNAP];
    const uint64_t t1 = wr.time[DY_LAST_SNAP];
    const double elapsed = (t1 > t0 && m_timerResolution > 0.0)
                           ? static_cast<double>(t1 - t0) / m_timerResolution
                           : 1.0;

    // Sum across all targets
    const int tcount = std::max(0, std::min(wr.target_results.count, DY_MAX_TARGETS));
    uint64_t totalBytes  = 0, readBytes = 0, writeBytes = 0;
    uint64_t readIos = 0, writeIos = 0;
    uint64_t readLatSum = 0, writeLatSum = 0;
    uint64_t maxReadLat = 0, maxWriteLat = 0;
    uint32_t errs = 0;

    for (int i = 0; i < tcount; ++i) {
        const DyRawResult &raw = wr.target_results.result[i];
        totalBytes   += raw.bytes_read + raw.bytes_written;
        readBytes    += raw.bytes_read;
        writeBytes   += raw.bytes_written;
        readIos      += raw.read_count;
        writeIos     += raw.write_count;
        readLatSum   += raw.read_latency_sum;
        writeLatSum  += raw.write_latency_sum;
        maxReadLat    = std::max(maxReadLat,  raw.max_raw_read_latency);
        maxWriteLat   = std::max(maxWriteLat, raw.max_raw_write_latency);
        errs         += raw.read_errors + raw.write_errors;
    }

    const uint64_t totalIos = readIos + writeIos;

    r.iops         = elapsed > 0 ? totalIos / elapsed : 0.0;
    r.readIops     = elapsed > 0 ? readIos  / elapsed : 0.0;
    r.writeIops    = elapsed > 0 ? writeIos / elapsed : 0.0;
    r.mbpsDec      = elapsed > 0 ? totalBytes / elapsed / 1.0e6  : 0.0;
    r.readMbpsDec  = elapsed > 0 ? readBytes  / elapsed / 1.0e6  : 0.0;
    r.writeMbpsDec = elapsed > 0 ? writeBytes / elapsed / 1.0e6  : 0.0;
    r.mbpsBin      = elapsed > 0 ? totalBytes / elapsed / (1024.0*1024.0) : 0.0;
    r.errors       = static_cast<int>(errs);

    // Average latency in milliseconds
    if (totalIos > 0 && m_timerResolution > 0.0) {
        const double avgTicksPerIo = static_cast<double>(readLatSum + writeLatSum)
                                     / static_cast<double>(totalIos);
        r.avgLatencyMs = avgTicksPerIo / m_timerResolution * 1000.0;
    } else {
        r.avgLatencyMs = 0.0;
    }

    // Max latency in milliseconds
    const uint64_t maxLatTicks = std::max(maxReadLat, maxWriteLat);
    r.maxLatencyMs = m_timerResolution > 0.0
                     ? static_cast<double>(maxLatTicks) / m_timerResolution * 1000.0
                     : 0.0;

    // CPU from manager results (average across CPUs)
    r.cpuUtil   = 0.0;
    r.cpuUser   = 0.0;
    r.cpuKernel = 0.0;
    if (m_pendingMgrResults.cpu_results.count > 0) {
        const int ncpu = std::min(m_pendingMgrResults.cpu_results.count,
                                  DY_MAX_CPUS);
        for (int i = 0; i < ncpu; ++i) {
            r.cpuUtil   += m_pendingMgrResults.cpu_results.CPU_utilization[i][0];
            r.cpuUser   += m_pendingMgrResults.cpu_results.CPU_utilization[i][1];
            r.cpuKernel += m_pendingMgrResults.cpu_results.CPU_utilization[i][2];
        }
        r.cpuUtil   /= ncpu;
        r.cpuUser   /= ncpu;
        r.cpuKernel /= ncpu;
    }

    return r;
}

// ── Build protocol messages ───────────────────────────────────────────────────

void DySession::buildTestSpec(DyDataMessage *dm, const AccessSpec &spec)
{
    dm->count = 1;
    DyTestSpec &ts = dm->data.spec;
    std::strncpy(ts.name, "IometerQt", sizeof(ts.name) - 1);
    ts.default_assignment = 0;

    // Build a simple access spec array:
    // 100% of the entries are the same spec; mark end with of_size=0
    std::memset(ts.access, 0, sizeof(ts.access));

    // Convert AccessSpec (bytes) to wire format
    DyAccessSpec &a = ts.access[0];
    a.of_size = 100;
    a.reads   = spec.readPercent;
    a.random  = 100 - spec.seqPercent;  // seqPercent → random = (100 - seqPercent)
    a.delay   = static_cast<int32_t>(spec.delayMs);
    a.burst   = spec.burstLength;
    a.align   = static_cast<uint32_t>(spec.alignBytes > 0 ? spec.alignBytes : 512);
    a.reply   = 0;
    a.size    = static_cast<uint32_t>(spec.xferSizeBytes > 0 ? spec.xferSizeBytes : 65536);

    // Terminator
    ts.access[1].of_size = 0;
}

void DySession::buildTargetMsg(DyDataMessage *dm, const WorkerInfo &w)
{
    dm->count = 0;

    // Find each target in the discovered disk list
    for (const QString &targetName : w.targets) {
        for (const DyTargetSpec &t : m_diskTargets) {
            if (QString::fromLocal8Bit(t.name) == targetName) {
                if (dm->count < DY_MAX_TARGETS) {
                    DyTargetSpec &dst = dm->data.targets[dm->count];
                    std::memcpy(&dst, &t, sizeof(dst));
                    dst.queue_depth = w.queueDepth;
                    ++dm->count;
                }
                break;
            }
        }
    }

    // If no targets matched, fall back to first disk
    if (dm->count == 0 && !m_diskTargets.isEmpty()) {
        std::memcpy(&dm->data.targets[0], &m_diskTargets.first(), sizeof(DyTargetSpec));
        dm->data.targets[0].queue_depth = w.queueDepth;
        dm->count = 1;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DynamoEngine
// ─────────────────────────────────────────────────────────────────────────────

DynamoEngine::DynamoEngine(QObject *parent)
    : IometerEngine(parent)
{
    // Default access spec
    AccessSpec def;
    def.name         = "Default (64KB Sequential Reads)";
    def.xferSizeBytes = 65536;
    def.alignBytes    = 65536;
    def.readPercent   = 100;
    def.seqPercent    = 100;
    def.burstLength   = 1;
    def.defaultSpec   = true;
    m_specs.append(def);

    // Start the TCP server
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &DynamoEngine::onNewConnection);

    if (m_server->listen(QHostAddress::Any, DY_PORT)) {
        emit statusMessage(QString("Listening for Dynamo on port %1 — start Dynamo.exe to connect")
                           .arg(DY_PORT));
    } else {
        emit errorOccurred(QString("Cannot listen on port %1: %2")
                           .arg(DY_PORT).arg(m_server->errorString()));
    }
}

DynamoEngine::~DynamoEngine()
{
    for (auto *s : m_sessions) s->deleteLater();
    m_server->close();
}

// ── New Dynamo connection ─────────────────────────────────────────────────────

void DynamoEngine::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *sock = m_server->nextPendingConnection();
        auto *session = new DySession(sock, this);

        connect(session, &DySession::managerConnected,
                this, &DynamoEngine::onSessionConnected);
        connect(session, &DySession::managerDisconnected,
                this, &DynamoEngine::onSessionDisconnected);
        connect(session, &DySession::resultsReady,
                this, &DynamoEngine::onSessionResults);
        connect(session, &DySession::errorOccurred,
                this, &DynamoEngine::onSessionError);

        m_sessions.append(session);
        emit statusMessage(QString("Dynamo connecting from %1…")
                           .arg(sock->peerAddress().toString()));
    }
}

// ── Session event handlers ────────────────────────────────────────────────────

void DynamoEngine::onSessionConnected(DySession *s)
{
    rebuildManagers();

    ManagerInfo mgr;
    mgr.name             = s->managerName();
    mgr.address          = s->address();
    mgr.connected        = true;
    mgr.processorCount   = s->processorCount();
    mgr.workers          = s->workers();
    mgr.availableTargets = s->diskTargetInfos();

    emit managerConnected(mgr);
    emit statusMessage(QString("Dynamo manager '%1' connected (%2 targets)")
                       .arg(s->managerName())
                       .arg(s->discoveredTargets().size()));
    emit configChanged();
}

void DynamoEngine::onSessionDisconnected(DySession *s)
{
    const QString name = s->managerName();
    m_sessions.removeAll(s);
    s->deleteLater();

    rebuildManagers();
    emit managerDisconnected(name);
    emit configChanged();
    emit statusMessage(QString("Dynamo '%1' disconnected").arg(name));
}

void DynamoEngine::onSessionResults(DySession *s)
{
    // Merge this session's results into m_currentResults
    QVector<WorkerResult> all;
    for (auto *sess : m_sessions)
        all += sess->pendingResults();

    // Append aggregate
    if (!all.isEmpty()) {
        WorkerResult agg;
        agg.managerName = "ALL";
        agg.workerName  = "ALL";
        agg.isAggregate = true;
        for (const auto &r : all) {
            agg.iops         += r.iops;
            agg.readIops     += r.readIops;
            agg.writeIops    += r.writeIops;
            agg.mbpsDec      += r.mbpsDec;
            agg.readMbpsDec  += r.readMbpsDec;
            agg.writeMbpsDec += r.writeMbpsDec;
            agg.mbpsBin      += r.mbpsBin;
            agg.errors       += r.errors;
        }
        agg.avgLatencyMs = 0.0;
        for (const auto &r : all) agg.avgLatencyMs += r.avgLatencyMs;
        if (!all.isEmpty()) agg.avgLatencyMs /= all.size();
        agg.maxLatencyMs = 0.0;
        for (const auto &r : all)
            agg.maxLatencyMs = std::max(agg.maxLatencyMs, r.maxLatencyMs);
        agg.cpuUtil = 0.0;
        for (const auto &r : all) agg.cpuUtil += r.cpuUtil;
        if (!all.isEmpty()) agg.cpuUtil /= all.size();
        all.prepend(agg);
    }

    m_currentResults = all;
    emit resultsUpdated(m_currentResults);
}

void DynamoEngine::onSessionError(DySession *s, const QString &msg)
{
    emit errorOccurred(QString("[%1] %2").arg(s->managerName(), msg));
}

// ── IometerEngine interface ───────────────────────────────────────────────────

void DynamoEngine::startTest()
{
    if (m_running) return;
    if (m_sessions.isEmpty()) {
        emit errorOccurred("No Dynamo managers connected");
        return;
    }

    m_running = true;
    m_savedResults.clear();

    QList<WorkerInfo> allWorkers;
    for (const auto &mgr : m_managers)
        allWorkers += mgr.workers;

    for (auto *s : m_sessions)
        s->startTest(allWorkers, m_specs);

    emit testStarted();
    emit statusMessage("Test started");
}

void DynamoEngine::stopTest()
{
    if (!m_running) return;
    for (auto *s : m_sessions)
        s->stopTest();
    m_running = false;
    m_savedResults = m_currentResults;
    emit testStopped();
    emit statusMessage("Test stopped");
}

void DynamoEngine::stopAll()
{
    for (auto *s : m_sessions)
        s->stopAll();
    m_running = false;
    emit testStopped();
}

bool DynamoEngine::loadConfig(const QString &) { return false; }
bool DynamoEngine::saveConfig(const QString &) { return false; }
void DynamoEngine::newConfig()
{
    for (auto *s : m_sessions) s->deleteLater();
    m_sessions.clear();
    m_managers.clear();
    m_currentResults.clear();
    m_running = false;
    emit configChanged();
}

void DynamoEngine::connectManager(const QString &, const QString &)
{
    // Managers connect by launching Dynamo.exe — this is a no-op here
    emit statusMessage(QString("To connect a manager: run Dynamo.exe -i <this-host> on the target machine"));
}

void DynamoEngine::disconnectManager(const QString &mgrName)
{
    for (auto *s : m_sessions) {
        if (s->managerName() == mgrName) {
            s->stopAll();
            break;
        }
    }
}

void DynamoEngine::addWorker(const QString &mgrName, const WorkerInfo &w)
{
    for (auto &mgr : m_managers) {
        if (mgr.name == mgrName) {
            mgr.workers.append(w);
            emit configChanged();
            return;
        }
    }
}

void DynamoEngine::removeWorker(const QString &mgrName, const QString &workerId)
{
    for (auto &mgr : m_managers) {
        if (mgr.name == mgrName) {
            mgr.workers.removeIf([&](const WorkerInfo &w){ return w.id == workerId; });
            emit configChanged();
            return;
        }
    }
}

void DynamoEngine::updateWorker(const WorkerInfo &w)
{
    for (auto &mgr : m_managers) {
        if (mgr.name == w.managerName) {
            for (auto &existing : mgr.workers) {
                if (existing.id == w.id) {
                    existing = w;
                    emit configChanged();
                    return;
                }
            }
        }
    }
}

void DynamoEngine::setAccessSpecs(const QList<AccessSpec> &specs)
{
    m_specs = specs;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void DynamoEngine::rebuildManagers()
{
    m_managers.clear();
    for (const auto *s : m_sessions) {
        if (s->state() == DySession::State::Ready ||
            s->state() == DySession::State::Running ||
            s->isRunning()) {
            ManagerInfo mgr;
            mgr.name             = s->managerName();
            mgr.address          = s->address();
            mgr.connected        = true;
            mgr.processorCount   = s->processorCount();
            mgr.workers          = s->workers();
            mgr.availableTargets = s->diskTargetInfos();
            m_managers.append(mgr);
        }
    }
}
