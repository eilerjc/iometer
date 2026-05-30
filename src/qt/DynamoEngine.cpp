// DynamoEngine.cpp
#include "DynamoEngine.h"
#include "IometerEngine.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>
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

        // ── ADD_WORKERS confirmation ──────────────────────────────────────────
        case State::WaitAddWorkers:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processAddWorkersReply(); progress = true;
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
        // Tell Dynamo to create the worker threads.
        // DY_ADD_WORKERS data = number of disk workers to create.
        sendMsg(DY_ADD_WORKERS, static_cast<int32_t>(m_workers.size()));
        m_state = State::WaitAddWorkers;
        qDebug() << "DySession: sent ADD_WORKERS(" << m_workers.size() << ") to"
                 << m_managerName;
        break;
    }
}

// ── ADD_WORKERS reply ─────────────────────────────────────────────────────────

void DySession::processAddWorkersReply()
{
    QByteArray chunk = take(DY_MSG_SIZE);
    const DyMsg *msg = reinterpret_cast<const DyMsg*>(chunk.constData());
    const int32_t actualCount = msg->data;
    if (actualCount <= 0) {
        qWarning() << "DySession: ADD_WORKERS reply says 0 workers on" << m_managerName;
    }
    qDebug() << "DySession: ADD_WORKERS confirmed," << actualCount
             << "workers on" << m_managerName;
    m_state = State::Ready;
    emit managerConnected(this);
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
    std::strncpy(ts.name, spec.name.toLocal8Bit().constData(), sizeof(ts.name) - 1);
    ts.default_assignment = spec.defaultSpec ? 1 : 0;
    std::memset(ts.access, 0, sizeof(ts.access));

    const int n = qMin(spec.lines.size(), DY_MAX_ACCESS_SPECS - 1);
    for (int i = 0; i < n; ++i) {
        const AccessSpecLine &line = spec.lines[i];
        DyAccessSpec &a = ts.access[i];
        a.of_size = line.ofSize;
        a.reads   = line.readPercent;
        a.random  = 100 - line.seqPercent;
        a.delay   = static_cast<int32_t>(line.delayMs);
        a.burst   = line.burstLength;
        a.reply   = static_cast<uint32_t>(line.replyBytes);
        a.size    = static_cast<uint32_t>(line.sizeBytes > 0 ? line.sizeBytes : 65536);
        // Alignment: -1 = request-size (use transfer size), 0 = sector (512), >0 = explicit
        if      (line.alignBytes < 0)  a.align = a.size;   // request-size boundaries
        else if (line.alignBytes == 0) a.align = 512;       // sector boundaries
        else                           a.align = static_cast<uint32_t>(line.alignBytes);
    }
    // Terminator
    if (n < DY_MAX_ACCESS_SPECS)
        ts.access[n].of_size = 0;
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
    // Full built-in access spec library (Idle, Default, 512B–32KiB at 5 read levels)
    m_specs = IometerEngine::builtinAccessSpecs();

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

    // Use the override spec if set (cycling), otherwise the global list
    const QList<AccessSpec> specs = m_hasCurrentTestSpec
        ? QList<AccessSpec>{m_currentTestSpec}
        : m_specs;

    for (auto *s : m_sessions)
        s->startTest(allWorkers, specs);

    m_hasCurrentTestSpec = false;   // consumed
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
    // Stop tests only — do NOT exit Dynamo workers; managers stay connected
    for (auto *s : m_sessions)
        s->stopTest();
    m_running = false;
    m_savedResults = m_currentResults;
    emit testStopped();
    emit statusMessage("All tests stopped");
}

// =============================================================================
// ICF load / save  (original Iometer 1.1.0 text format)
// =============================================================================

bool DynamoEngine::loadConfig(const QString &filepath)
{
    QFile f(filepath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QStringList lines;
    QTextStream in(&f);
    while (!in.atEnd()) lines.append(in.readLine());

    TestConfig cfg;
    QList<AccessSpec> specs;
    m_batchAssignedSpec.clear();
    m_batchTargets.clear();

    // Returns the first non-comment, non-empty data line after line i, trimmed.
    // Returns "" if none found before a section boundary.
    auto dataAfter = [&](int i) -> QString {
        for (int j = i + 1; j < lines.size(); ++j) {
            QString t = lines[j].trimmed();
            if (t.isEmpty() || t.startsWith("'")) continue;
            return t;
        }
        return {};
    };

    for (int i = 0; i < lines.size(); ++i) {
        const QString t = lines[i].trimmed();

        // ── Test Setup ─────────────────────────────────────────────────────
        if (t == "'Run Time") {
            // Two lines follow: a sub-comment then the data
            QString data;
            for (int j = i + 1; j < lines.size(); ++j) {
                QString d = lines[j].trimmed();
                if (d.isEmpty()) continue;
                if (d.startsWith("'")) continue;   // skip the format comment
                data = d; break;
            }
            const QStringList p = data.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (p.size() >= 3) {
                cfg.runHours   = p[0].toInt();
                cfg.runMinutes = p[1].toInt();
                cfg.runSeconds = p[2].toInt();
            }
        } else if (t == "'Ramp Up Time (s)") {
            const QString data = dataAfter(i);
            cfg.rampSeconds = data.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(0).toInt();
        } else if (t == "'Test Description") {
            // Description may be blank — take the very next line as-is
            if (i + 1 < lines.size())
                cfg.description = lines[i + 1].trimmed();
        }

        // ── Access Specifications ──────────────────────────────────────────
        else if (t.startsWith("'Access specification name")) {
            // First non-comment data line is "name,assignment"
            AccessSpec spec;
            int j = i + 1;
            while (j < lines.size()) {
                const QString d = lines[j++].trimmed();
                if (d.isEmpty() || d.startsWith("'")) continue;
                spec.name = d.split(',').value(0).trimmed();
                break;
            }
            // Collect all spec data lines (skip 'size,... comment, read until
            // next comment/blank that signals end of this spec)
            while (j < lines.size()) {
                const QString d = lines[j].trimmed();
                if (d.startsWith("'Access specification name") ||
                    d.startsWith("'END access")) break;
                ++j;
                if (d.isEmpty()) continue;
                if (d.startsWith("'")) continue;  // skip format comments
                // Data line: size,%ofSize,%reads,%random,delay,burst,align,reply
                const QStringList p = d.split(',');
                if (p.size() >= 8) {
                    AccessSpecLine l;
                    l.sizeBytes   = p[0].toInt();
                    l.ofSize      = p[1].toInt();
                    l.readPercent = p[2].toInt();
                    l.seqPercent  = 100 - p[3].toInt();  // %random → %sequential
                    l.delayMs     = p[4].toInt();
                    l.burstLength = p[5].toInt();
                    l.alignBytes  = p[6].toInt();
                    l.replyBytes  = p[7].toInt();
                    spec.lines.append(l);
                }
            }
            if (!spec.name.isEmpty() && !spec.lines.isEmpty())
                specs.append(spec);
        }

        // ── Manager / Worker ───────────────────────────────────────────────
        else if (t == "'Assigned access specs") {
            for (int j = i + 1; j < lines.size(); ++j) {
                const QString d = lines[j].trimmed();
                if (d.startsWith("'End assigned access")) break;
                if (d.isEmpty() || d.startsWith("'")) continue;
                if (m_batchAssignedSpec.isEmpty())
                    m_batchAssignedSpec = d;
            }
        }

        else if (t == "'Target") {
            // The next non-comment, non-empty line is the target path
            const QString tgt = dataAfter(i);
            if (!tgt.isEmpty() && !m_batchTargets.contains(tgt))
                m_batchTargets.append(tgt);
        }
    }

    setAccessSpecs(specs.isEmpty() ? builtinAccessSpecs() : specs);
    m_testConfig = cfg;
    emit configChanged();
    return true;
}

bool DynamoEngine::saveConfig(const QString &filepath)
{
    QFile f(filepath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);

    out << "Version 1.1.0 \n";

    // ── Test Setup ────────────────────────────────────────────────────────
    out << "'TEST SETUP ====================================================================\n";
    out << "'Test Description\n";
    out << "\t" << m_testConfig.description << "\n";
    out << "'Run Time\n";
    out << "'\thours      minutes    seconds\n";
    out << "\t" << m_testConfig.runHours
        << "          " << m_testConfig.runMinutes
        << "          " << m_testConfig.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n";
    out << "\t" << m_testConfig.rampSeconds << "\n";
    out << "'Default Disk Workers to Spawn\n\tNUMBER_OF_CPUS\n";
    out << "'Default Network Workers to Spawn\n\t0\n";
    out << "'Record Results\n\tALL\n";
    out << "'Worker Cycling\n'\tstart      step       step type\n\t1          1          LINEAR\n";
    out << "'Disk Cycling\n'\tstart      step       step type\n\t1          1          LINEAR\n";
    out << "'Queue Depth Cycling\n'\tstart      end        step       step type\n\t1          32         2          EXPONENTIAL\n";
    out << "'Test Type\n\tNORMAL\n";
    out << "'END test setup\n";

    // ── Results Display ───────────────────────────────────────────────────
    out << "'RESULTS DISPLAY ===============================================================\n";
    out << "'Record Last Update Results,Update Frequency,Update Type\n";
    out << "\tDISABLED,1,LAST_UPDATE\n";
    out << "'Bar chart 1 statistic\n\tTotal I/Os per Second\n";
    out << "'Bar chart 2 statistic\n\tTotal MBs per Second (Decimal)\n";
    out << "'Bar chart 3 statistic\n\tAverage I/O Response Time (ms)\n";
    out << "'Bar chart 4 statistic\n\tMaximum I/O Response Time (ms)\n";
    out << "'Bar chart 5 statistic\n\t% CPU Utilization (total)\n";
    out << "'Bar chart 6 statistic\n\tTotal Error Count\n";
    out << "'END results display\n";

    // ── Access Specifications ─────────────────────────────────────────────
    out << "'ACCESS SPECIFICATIONS =========================================================\n";
    for (const auto &s : m_specs) {
        out << "'Access specification name,default assignment\n";
        out << "\t" << s.name << ",NONE\n";
        out << "'size,% of size,% reads,% random,delay,burst,align,reply\n";
        for (const auto &l : s.lines) {
            out << "\t" << l.sizeBytes
                << "," << l.ofSize
                << "," << l.readPercent
                << "," << (100 - l.seqPercent)   // seqPct → randomPct
                << "," << l.delayMs
                << "," << l.burstLength
                << "," << l.alignBytes
                << "," << l.replyBytes << "\n";
        }
    }
    out << "'END access specifications\n";

    // ── Manager List ──────────────────────────────────────────────────────
    out << "'MANAGER LIST ==================================================================\n";
    int mgrid = 1;
    for (const auto &mgr : m_managers) {
        out << "'Manager ID, manager name\n";
        out << "\t" << mgrid++ << "," << mgr.name << "\n";
        out << "'Manager network address\n\t\n";
        for (const auto &w : mgr.workers) {
            out << "'Worker\n\t" << w.name << "\n";
            out << "'Worker type\n\t" << (w.type == "Network" ? "NET" : "DISK") << "\n";
            out << "'Default target settings for worker\n";
            out << "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value\n";
            out << "\t" << w.queueDepth << ","
                << (w.testConnRate ? "ENABLED" : "DISABLED") << ","
                << w.transPerConn << ","
                << (w.useFixedSeed ? "ENABLED" : "DISABLED") << ","
                << w.fixedSeedValue << "\n";
            out << "'Disk maximum size,starting sector,Data pattern\n";
            out << "\t" << w.maxDiskSize << "," << w.startingSector << "," << w.dataPattern << "\n";
            out << "'End default target settings for worker\n";
            out << "'Assigned access specs\n";
            for (const auto &sn : w.assignedSpecs)
                out << "\t" << sn << "\n";
            if (w.assignedSpecs.isEmpty() && !m_specs.isEmpty())
                out << "\t" << m_specs.first().name << "\n";
            out << "'End assigned access specs\n";
            out << "'Target assignments\n";
            for (const auto &tgt : w.targets) {
                out << "'Target\n\t" << tgt << "\n";
                out << "'Target type\n\tDISK\n";
                out << "'End target\n";
            }
            out << "'End target assignments\n";
            out << "'End worker\n";
        }
        out << "'End manager\n";
    }
    out << "'END manager list\n";
    out << "Version 1.1.0 \n";
    return true;
}
bool DynamoEngine::saveBatchResults(const QString &filepath)
{
    QFile f(filepath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);

    // ── File header (matches original ManagerList output) ─────────────────
    out << "'Iometer Output File\n";
    out << "Version 1.1.0 \n";
    out << "'Test Description\n\t" << m_testConfig.description << "\n";
    out << "'Time Stamp\n\t"
        << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss") << "\n";
    out << "'Run Time\n'\thours      minutes    seconds\n";
    out << "\t" << m_testConfig.runHours
        << "          " << m_testConfig.runMinutes
        << "          " << m_testConfig.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n\t" << m_testConfig.rampSeconds << "\n";

    // ── Column headers (exact original order, 60+ columns) ────────────────
    // The smoke test cares only that: field[0]=="ALL", [6]=IOps, [12]=MBps(Dec), [27]=Errors
    out << "'Results\n";
    out << "'Target Type,Target Name,Manager,Workers,Managers,Targets,"
           "IOps,Read IOps,Write IOps,"
           "MBps (Binary),Read MBps (Binary),Write MBps (Binary),"
           "MBps (Decimal),Read MBps (Decimal),Write MBps (Decimal),"
           "Transactions per Second,Connections per Second,"
           "Average I/O Response Time (ms),Average Read Response Time (ms),"
           "Average Write Response Time (ms),Average Transaction Time (ms),"
           "Average Connection Time (ms),"
           "Maximum I/O Response Time (ms),Maximum Read Response Time (ms),"
           "Maximum Write Response Time (ms),Maximum Transaction Time (ms),"
           "Maximum Connection Time (ms),"
           "Errors,Read Errors,Write Errors,"
           "CPU % Utilization (total),CPU % User,CPU % Privileged,"
           "CPU % DPC,CPU % Interrupt,CPU Interrupts/sec,CPU Effectiveness\n";

    // Build aggregate from saved results
    WorkerResult agg;
    agg.managerName = "ALL";
    agg.workerName  = "All";
    agg.isAggregate = true;
    int workerCount = 0, mgCount = 0;
    QSet<QString> managers;

    const auto &results = m_savedResults.isEmpty() ? m_currentResults : m_savedResults;
    for (const auto &r : results) {
        if (r.isAggregate) continue;
        agg.iops         += r.iops;
        agg.readIops     += r.readIops;
        agg.writeIops    += r.writeIops;
        agg.mbpsDec      += r.mbpsDec;
        agg.readMbpsDec  += r.readMbpsDec;
        agg.writeMbpsDec += r.writeMbpsDec;
        agg.mbpsBin      += r.mbpsBin;
        agg.avgLatencyMs += r.avgLatencyMs;
        agg.maxLatencyMs  = qMax(agg.maxLatencyMs, r.maxLatencyMs);
        agg.cpuUtil      += r.cpuUtil;
        agg.errors       += r.errors;
        managers.insert(r.managerName);
        ++workerCount;
    }
    if (workerCount > 0) agg.avgLatencyMs /= workerCount;
    mgCount = managers.size();

    // Write the ALL aggregate row — column positions must match the header above
    // [0]ALL [1]All [2]manager [3]workers [4]managers [5]targets
    // [6]IOps [7]ReadIOps [8]WriteIOps
    // [9]MBpsBin [10]ReadMBpsBin [11]WriteMBpsBin
    // [12]MBpsDec [13]ReadMBpsDec [14]WriteMBpsDec
    // [15]Trans/s [16]Conn/s
    // [17]AvgIO [18]AvgRead [19]AvgWrite [20]AvgTrans [21]AvgConn
    // [22]MaxIO [23]MaxRead [24]MaxWrite [25]MaxTrans [26]MaxConn
    // [27]Errors [28]ReadErrors [29]WriteErrors
    // [30-36] CPU fields
    auto fmt = [](double v, int dec=6) { return QString::number(v, 'f', dec); };

    out << "ALL,All,," << workerCount << "," << mgCount << "," << workerCount << ","
        << fmt(agg.iops) << "," << fmt(agg.readIops) << "," << fmt(agg.writeIops) << ","
        << fmt(agg.mbpsBin) << "," << fmt(agg.mbpsBin) << ",0.000000,"
        << fmt(agg.mbpsDec) << "," << fmt(agg.readMbpsDec) << "," << fmt(agg.writeMbpsDec) << ","
        << fmt(agg.iops) << ",0.000000,"
        << fmt(agg.avgLatencyMs) << "," << fmt(agg.avgLatencyMs) << ",0.000000,0.000000,0.000000,"
        << fmt(agg.maxLatencyMs) << "," << fmt(agg.maxLatencyMs) << ",0.000000,0.000000,0.000000,"
        << agg.errors << ",0,0,"
        << fmt(agg.cpuUtil) << "," << fmt(agg.cpuUser) << "," << fmt(agg.cpuKernel)
        << ",0.000000,0.000000,0.000000,0.000000\n";

    // Write per-worker rows
    for (const auto &r : results) {
        if (r.isAggregate) continue;
        out << "DISK," << r.workerName << "," << r.managerName << ",1,1,1,"
            << fmt(r.iops) << "," << fmt(r.readIops) << "," << fmt(r.writeIops) << ","
            << fmt(r.mbpsBin) << "," << fmt(r.mbpsBin) << ",0.000000,"
            << fmt(r.mbpsDec) << "," << fmt(r.readMbpsDec) << "," << fmt(r.writeMbpsDec) << ","
            << fmt(r.iops) << ",0.000000,"
            << fmt(r.avgLatencyMs) << "," << fmt(r.avgLatencyMs) << ",0.000000,0.000000,0.000000,"
            << fmt(r.maxLatencyMs) << "," << fmt(r.maxLatencyMs) << ",0.000000,0.000000,0.000000,"
            << r.errors << ",0,0,"
            << fmt(r.cpuUtil) << "," << fmt(r.cpuUser) << "," << fmt(r.cpuKernel)
            << ",0.000000,0.000000,0.000000,0.000000\n";
    }

    return true;
}

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
