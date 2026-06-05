// DynamoEngine.cpp
#include "DynamoEngine.h"
#include "IometerEngine.h"
#include "../core/ResultsWriter.h"
#include "../core/IcfFile.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>
#include <QSet>
#include <cstring>
#include <cmath>

// --- Helpers ------------------------------------------------------------------

// Container conversions between Qt and std containers.  The *element* type is
// the same shared core domain type in both cases - only the container differs.
template <class T>
static std::vector<T> toVec(const QList<T> &l) {
    // Note: in Qt6 QVector is an alias for QList, so this covers both.
    return std::vector<T>(l.begin(), l.end());
}
template <class T>
static QList<T> toQList(const std::vector<T> &v) {
    QList<T> l;
    l.reserve(static_cast<int>(v.size()));
    for (const auto &x : v) l.append(x);
    return l;
}
static QStringList toQStringList(const std::vector<std::string> &v) {
    QStringList l;
    for (const auto &s : v) l << QString::fromStdString(s);
    return l;
}
static std::vector<std::string> toStdStrings(const QStringList &l) {
    std::vector<std::string> v;
    for (const auto &s : l) v.push_back(s.toStdString());
    return v;
}

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

// -----------------------------------------------------------------------------
// DySession
// -----------------------------------------------------------------------------

DySession::DySession(QTcpSocket *loginSocket, QObject *parent)
    : QObject(parent), m_loginSocket(loginSocket)
{
    // The login socket belongs to us now
    m_loginSocket->setParent(this);

    connect(m_loginSocket, &QTcpSocket::readyRead,
            this, &DySession::onLoginData);
    connect(m_loginSocket, &QTcpSocket::disconnected,
            this, [this]{ m_loginSocket->deleteLater(); m_loginSocket = nullptr; });

    // Prime the state machine - expect 8-byte login Message
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

// -- Low-level socket helpers --------------------------------------------------

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

// -- Login socket handler ------------------------------------------------------

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
        // Be lenient - continue anyway (might be a different build)
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

    // Login socket is done - disconnect it
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

// -- Main socket handlers ------------------------------------------------------

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

        // -- Target discovery --------------------------------------------------
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

        // -- ADD_WORKERS confirmation ------------------------------------------
        case State::WaitAddWorkers:
            if (m_buf.size() >= DY_MSG_SIZE) {
                processAddWorkersReply(); progress = true;
            }
            break;

        // -- Worker setup ------------------------------------------------------
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

        // -- Start sequence ----------------------------------------------------
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

        // -- Results during test -----------------------------------------------
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

        // -- Stop sequence -----------------------------------------------------
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

// -- Target discovery handlers -------------------------------------------------

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

        // All targets discovered - create one disk worker per CPU core, matching
        // the original Iometer AddDefaultWorkers() behaviour (disk_worker_count=-1
        // → use manager->processors).  Workers are named "Worker 1", "Worker 2", …
        // None have targets pre-assigned; the user assigns them via Disk Targets tab.
        m_workers.clear();
        for (int i = 0; i < m_processorCount; ++i) {
            WorkerInfo w;
            w.id          = QString("%1-disk-%2").arg(m_managerName).arg(i).toStdString();
            w.name        = QString("Worker %1").arg(i + 1).toStdString();
            w.type        = "Disk";
            w.managerName = m_managerName.toStdString();
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

// -- ADD_WORKERS reply ---------------------------------------------------------

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

// -- Worker setup handlers -----------------------------------------------------

void DySession::beginSetupWorker(int idx)
{
    if (idx >= m_workers.size()) {
        // All workers configured - start the test
        beginStartSequence();
        return;
    }

    const WorkerInfo &w = m_workers[idx];
    const int wIdx      = idx;  // worker index passed to Dynamo

    // -- SET_ACCESS --
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

    // -- SET_TARGETS --
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

// -- Start sequence ------------------------------------------------------------

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

// -- Update timer - request results while running ------------------------------

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
                          ? QString::fromStdString(m_workers[m_workerResultIdx].name)
                          : QString("Worker%1").arg(m_workerResultIdx);

    WorkerResult result = decodeWorkerResults(wr, m_managerName, wName);
    m_pendingResults.append(result);

    ++m_workerResultIdx;
    if (m_workerResultIdx >= m_activeWorkerCount) {
        // All worker results received - back to Running state
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

// -- Stop sequence -------------------------------------------------------------

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
                          ? QString::fromStdString(m_workers[m_workerResultIdx].name)
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

// -- Test control (called by DynamoEngine) -------------------------------------

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
            if (w.managerName == m_managerName.toStdString())
                m_workers.append(w);
    }

    if (m_workers.isEmpty()) {
        qWarning() << "DySession: no workers for" << m_managerName
                   << "- creating" << m_processorCount << "default workers";
        for (int i = 0; i < m_processorCount; ++i) {
            WorkerInfo w;
            w.id          = QString("%1-disk-%2").arg(m_managerName).arg(i).toStdString();
            w.name        = QString("Worker %1").arg(i + 1).toStdString();
            w.type        = "Disk";
            w.managerName = m_managerName.toStdString();
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

// -- Result decoding -----------------------------------------------------------

WorkerResult DySession::decodeWorkerResults(const DyWorkerResults &wr,
                                             const QString &mgrName,
                                             const QString &workerName)
{
    WorkerResult r;
    r.managerName = mgrName.toStdString();
    r.workerName  = workerName.toStdString();
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

// -- Build protocol messages ---------------------------------------------------

void DySession::buildTestSpec(DyDataMessage *dm, const AccessSpec &spec)
{
    dm->count = 1;
    DyTestSpec &ts = dm->data.spec;
    std::strncpy(ts.name, spec.name.c_str(), sizeof(ts.name) - 1);
    ts.default_assignment = spec.defaultSpec ? 1 : 0;
    std::memset(ts.access, 0, sizeof(ts.access));

    const int n = qMin(static_cast<int>(spec.lines.size()), DY_MAX_ACCESS_SPECS - 1);
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
    for (const std::string &targetName : w.targets) {
        for (const DyTargetSpec &t : m_diskTargets) {
            if (QString::fromLocal8Bit(t.name) == QString::fromStdString(targetName)) {
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

// -----------------------------------------------------------------------------
// DynamoEngine
// -----------------------------------------------------------------------------

DynamoEngine::DynamoEngine(bool startListening, QObject *parent)
    : IometerEngine(parent)
{
    m_specs = IometerEngine::builtinAccessSpecs();

    // Bridge the core TestRunner's std::function callbacks to Qt signals.
    m_testRunner.onStatusMessage = [this](const std::string &msg) {
        emit statusMessage(QString::fromStdString(msg));
    };
    m_testRunner.onErrorOccurred = [this](const std::string &msg) {
        emit errorOccurred(QString::fromStdString(msg));
    };

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &DynamoEngine::onNewConnection);

    if (startListening) {
        if (m_server->listen(QHostAddress::Any, DY_PORT)) {
            emit statusMessage(QString("Listening for Dynamo on port %1 - start Dynamo.exe to connect")
                               .arg(DY_PORT));
        } else {
            emit errorOccurred(QString("Cannot listen on port %1: %2")
                               .arg(DY_PORT).arg(m_server->errorString()));
        }
    }
}

DynamoEngine::~DynamoEngine()
{
    for (auto *s : m_sessions) s->deleteLater();
    m_server->close();
}

// -- New Dynamo connection -----------------------------------------------------

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

// -- Session event handlers ----------------------------------------------------

void DynamoEngine::onSessionConnected(DySession *s)
{
    rebuildManagers();

    ManagerInfo mgr;
    mgr.name             = s->managerName().toStdString();
    mgr.address          = s->address().toStdString();
    mgr.connected        = true;
    mgr.processorCount   = s->processorCount();
    mgr.workers          = toVec(s->workers());
    mgr.availableTargets = toVec(s->diskTargetInfos());

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

// -- IometerEngine interface ---------------------------------------------------

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
    auto managers = m_workerPool.managerInfos();
    for (const auto &mgr : managers)
        for (const auto &w : m_workerPool.workers(mgr.name))
            allWorkers.append(w);

    // In batch mode the ICF specifies an exact worker count.  Trim the list so
    // that extra Dynamo workers (beyond what the ICF defines) never receive
    // DY_SET_ACCESS / DY_SET_TARGETS and stay idle - matching original behaviour.
    if (!m_batchWorkers.isEmpty() && allWorkers.size() > m_batchWorkers.size())
        allWorkers = allWorkers.mid(0, m_batchWorkers.size());

    // Use the override spec if set (cycling), otherwise the global list
    const QList<AccessSpec> specs = m_hasCurrentTestSpec
        ? QList<AccessSpec>{m_currentTestSpec}
        : m_specs;

    // Delegate to the core TestRunner (Qt container -> std::vector; same elements)
    if (!m_testRunner.startTest(m_testConfig, toVec(allWorkers), toVec(specs))) {
        m_running = false;
        emit errorOccurred(QString::fromStdString(m_testRunner.lastError()));
        return;
    }

    // Start test on all sessions (protocol-level)
    for (auto *s : m_sessions)
        s->startTest(allWorkers, specs);

    m_hasCurrentTestSpec = false;   // consumed
    emit testStarted();
    emit statusMessage("Test started");
}

void DynamoEngine::stopTest()
{
    if (!m_running) return;

    // Stop test in TestRunner (state machine)
    m_testRunner.stopTest();

    // Stop protocol-level test on all sessions
    for (auto *s : m_sessions)
        s->stopTest();

    m_running = false;
    m_savedResults = m_currentResults;
    emit testStopped();
    emit statusMessage("Test stopped");
}

void DynamoEngine::stopAll()
{
    // Stop tests only - do NOT exit Dynamo workers; managers stay connected
    m_testRunner.stopAll();

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
    TestConfig cfg;
    std::vector<AccessSpec> specs;
    std::vector<IcfFile::BatchWorker> batchWorkers;

    // Use IcfFile to parse entire ICF format
    if (!IcfFile::load(filepath.toStdString(), cfg, specs, batchWorkers)) {
        return false;
    }

    // Convert IcfFile BatchWorker to DynamoEngine BatchWorkerConfig
    m_batchWorkers.clear();
    for (const auto &bw : batchWorkers) {
        BatchWorkerConfig wc;
        wc.name = QString::fromStdString(bw.name);
        wc.assignedSpecs = toQStringList(bw.assignedSpecs);
        wc.targets = toQStringList(bw.targets);
        m_batchWorkers.append(wc);
    }

    setAccessSpecs(specs.empty() ? builtinAccessSpecs() : toQList(specs));
    m_testConfig = cfg;
    emit configChanged();
    return true;
}

bool DynamoEngine::saveConfig(const QString &filepath)
{
    // Convert DynamoEngine BatchWorkerConfig to IcfFile BatchWorker
    std::vector<IcfFile::BatchWorker> batchWorkers;

    auto managers = m_workerPool.managerInfos();
    if (!managers.empty()) {
        // Live Dynamo connection - write actual connected state
        for (const auto &mgr : managers) {
            for (const auto &w : m_workerPool.workers(mgr.name)) {
                IcfFile::BatchWorker bw;
                bw.name = w.name;                 // std::string -> std::string
                bw.assignedSpecs = w.assignedSpecs;
                bw.targets = w.targets;
                batchWorkers.push_back(bw);
            }
        }
    } else {
        // No live connection - use batch config from loaded ICF
        for (const auto &bw : m_batchWorkers) {
            IcfFile::BatchWorker ibw;
            ibw.name = bw.name.toStdString();
            ibw.assignedSpecs = toStdStrings(bw.assignedSpecs);
            ibw.targets = toStdStrings(bw.targets);
            batchWorkers.push_back(ibw);
        }
    }

    return IcfFile::save(filepath.toStdString(), m_testConfig, toVec(m_specs), batchWorkers);
}
bool DynamoEngine::saveBatchResults(const QString &filepath)
{
    const auto &r = m_savedResults.isEmpty() ? m_currentResults : m_savedResults;
    return writeBatchResultsCsv(filepath, r, m_testConfig);
}

bool DynamoEngine::writeBatchResultsCsv(const QString &filepath,
                                        const QVector<WorkerResult> &results,
                                        const TestConfig &cfg)
{
    // Delegate to ResultsWriter for CSV generation
    // This keeps CSV format logic in one place for both MFC and Qt
    return ResultsWriter::writeBatchResultsCsv(filepath.toStdString(), toVec(results), cfg);
}

void DynamoEngine::newConfig()
{
    for (auto *s : m_sessions) s->deleteLater();
    m_sessions.clear();
    m_workerPool.clear();
    m_testRunner.stopAll();  // Reset test state machine
    m_currentResults.clear();
    m_running = false;
    emit configChanged();
}

QList<ManagerInfo> DynamoEngine::managers() const
{
    return toQList(m_workerPool.managerInfos());
}

void DynamoEngine::connectManager(const QString &, const QString &)
{
    // Managers connect by launching Dynamo.exe - this is a no-op here
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
    m_workerPool.addWorker(mgrName.toStdString(), w);
    emit configChanged();
}

void DynamoEngine::removeWorker(const QString &mgrName, const QString &workerId)
{
    m_workerPool.removeWorker(mgrName.toStdString(), workerId.toStdString());
    emit configChanged();
}

void DynamoEngine::updateWorker(const WorkerInfo &w)
{
    m_workerPool.updateWorker(w);
    emit configChanged();
}

void DynamoEngine::setAccessSpecs(const QList<AccessSpec> &specs)
{
    m_specs = specs;
}

// -- Helpers -------------------------------------------------------------------

void DynamoEngine::rebuildManagers()
{
    m_workerPool.clear();
    for (const auto *s : m_sessions) {
        if (s->state() == DySession::State::Ready ||
            s->state() == DySession::State::Running ||
            s->isRunning()) {
            ManagerInfo mgr;
            mgr.name             = s->managerName().toStdString();
            mgr.address          = s->address().toStdString();
            mgr.connected        = true;
            mgr.processorCount   = s->processorCount();
            // Convert QList to std::vector
            for (const auto &w : s->workers())
                mgr.workers.push_back(w);
            // Convert QList to std::vector
            for (const auto &t : s->diskTargetInfos())
                mgr.availableTargets.push_back(t);

            // Add manager to WorkerPool
            m_workerPool.addManager(mgr);

            // Add workers from this session
            for (const auto &w : s->workers()) {
                m_workerPool.addWorker(mgr.name, w);
            }
        }
    }
}
