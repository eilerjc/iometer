// DynamoEngine.h — Real Iometer engine: TCP server on port 1066 + Dynamo protocol
#pragma once

#include "IometerEngine.h"
#include "DyProto.h"
#include <QObject>
#include <QList>
#include <QVector>
#include <QByteArray>
#include <QTimer>

class QTcpServer;
class QTcpSocket;

// ─────────────────────────────────────────────────────────────────────────────
// DySession — manages the protocol exchange with one connected Dynamo instance
// ─────────────────────────────────────────────────────────────────────────────
class DySession : public QObject
{
    Q_OBJECT

public:
    enum class State {
        WaitLoginMsg,       // expecting 8-byte Message (LOGIN + version)
        WaitLoginData,      // expecting full DyDataMessage (Manager_Info)
        Connecting,         // connecting main socket to Dynamo
        WaitTargetsDisk,    // sent REPORT_TARGETS, awaiting disk Data_Message
        WaitTargetsTcp,     // awaiting TCP targets Data_Message
        WaitTargetsVi,      // awaiting VI targets Data_Message
        WaitAddWorkers,     // sent ADD_WORKERS, awaiting 8-byte confirmation
        Ready,              // connected, idle — test not started
        SetupAccess,        // sending SET_ACCESS for workers, awaiting reply
        SetupTargetsReply,  // sent SET_TARGETS, awaiting 8-byte reply
        SetupTargetsData,   // awaiting SET_TARGETS reply Data_Message
        WaitStartReply,     // sent START, awaiting 8-byte reply
        WaitBeginIoReply,   // sent BEGIN_IO, awaiting 8-byte reply
        Running,            // RECORD_ON sent, update timer ticking
        WaitUpdateMgr,      // sent REPORT_UPDATE, awaiting Manager_Results
        WaitUpdateWorker,   // awaiting Worker_Results [n]
        WaitRecordOff,      // sent RECORD_OFF, awaiting reply
        WaitStop,           // sent STOP, awaiting reply
        WaitFinalMgr,       // sent REPORT_RESULTS, awaiting Manager_Results
        WaitFinalWorker,    // awaiting final Worker_Results [n]
        Error,
        Disconnected
    };

    explicit DySession(QTcpSocket *loginSocket, QObject *parent = nullptr);
    ~DySession() override;

    State    state()       const { return m_state; }
    QString  managerName() const { return m_managerName; }
    QString  address()     const { return m_address; }
    bool     isReady()     const { return m_state == State::Ready; }
    bool     isRunning()   const { return m_state == State::Running
                                        || m_state == State::WaitUpdateMgr
                                        || m_state == State::WaitUpdateWorker; }
    double   timerRes()    const { return m_timerResolution; }

    int      processorCount() const { return m_processorCount; }
    const QList<WorkerInfo>& workers() const { return m_workers; }
    const QList<DyTargetSpec>& discoveredTargets() const { return m_diskTargets; }

    // Returns typed descriptors for all discovered disk targets
    QList<TargetInfo> diskTargetInfos() const {
        QList<TargetInfo> infos;
        for (const auto &t : m_diskTargets) {
            TargetInfo ti;
            ti.name = QString::fromLocal8Bit(t.name);
            if (t.type == DY_PHYSICAL_DISK) {
                ti.kind  = TargetKind::PhysicalDisk;
                ti.ready = true;   // physical disks are always directly testable
            } else {
                ti.kind  = TargetKind::LogicalDisk;
                ti.ready = (t.disk_info.ready != 0);
            }
            infos.append(ti);
        }
        return infos;
    }

    // ── Test control (called by DynamoEngine after state==Ready) ─────────────
    void startTest(const QList<WorkerInfo> &workers, const QList<AccessSpec> &specs);
    void stopTest();
    void stopAll();

    // ── Incremental result vectors (updated after each REPORT_UPDATE) ────────
    const QVector<WorkerResult>& pendingResults() const { return m_pendingResults; }

signals:
    void managerConnected(DySession *self);
    void managerDisconnected(DySession *self);
    void resultsReady(DySession *self);
    void errorOccurred(DySession *self, const QString &msg);

private slots:
    void onLoginData();
    void onMainConnected();
    void onMainData();
    void onMainDisconnected();
    void onUpdateTimer();

private:
    // Socket helpers
    void         expect(int bytes, State nextState);
    bool         consume(int bytes);   // returns true when m_buf.size() >= bytes
    QByteArray   take(int bytes);      // take exactly N bytes from front of m_buf
    void         sendMsg(int32_t purpose, int32_t data);
    void         sendDataMsg(DyDataMessage *dm);

    // State handlers
    void processLoginMsg();
    void processLoginData();
    void processTargetList(int phase);   // disk=0, tcp=1, vi=2
    void processAddWorkersReply();
    void processSetAccessReply();
    void processSetTargetsReply();
    void processSetTargetsData();
    void processStartReply();
    void processBeginIoReply();
    void processUpdateMgr();
    void processUpdateWorker();
    void processRecordOffReply();
    void processStopReply();
    void processFinalMgr();
    void processFinalWorker();
    void dispatchMainData();

    // Setup helpers
    void beginSetupWorker(int idx);
    void advanceSetupWorkers();
    void beginStartSequence();
    void buildTestSpec(DyDataMessage *dm, const AccessSpec &spec);
    void buildTargetMsg(DyDataMessage *dm, const WorkerInfo &w);

    // Result helpers
    WorkerResult decodeWorkerResults(const DyWorkerResults &wr,
                                     const QString &mgrName,
                                     const QString &workerName);

    // State
    State          m_state         = State::WaitLoginMsg;
    QTcpSocket    *m_loginSocket   = nullptr;   // only during login phase
    QTcpSocket    *m_mainSocket    = nullptr;   // main command channel
    QByteArray     m_buf;                        // receive accumulator
    int            m_expecting     = 0;          // bytes needed for current operation
    State          m_afterExpect   = State::WaitLoginMsg;

    // Manager info from login
    QString  m_managerName;
    QString  m_address;           // names[1] = main-port host
    uint16_t m_mainPort      = 0;
    double   m_timerResolution = 1.0;
    int      m_processorCount = 1;   // CPU count sent by Dynamo at login

    // Discovered targets
    QList<DyTargetSpec> m_diskTargets;
    QList<DyTargetSpec> m_tcpTargets;
    QList<DyTargetSpec> m_viTargets;

    // Workers configured for current test
    QList<WorkerInfo>  m_workers;
    QList<AccessSpec>  m_accessSpecs;
    int                m_setupWorkerIdx   = 0;  // current worker in setup loop
    int                m_activeWorkerCount = 0;
    int                m_workerResultIdx  = 0;

    // Update timer (REPORT_UPDATE polling)
    QTimer             m_updateTimer;
    static constexpr int k_updateIntervalMs = 1000;

    // Pending results assembled during one update cycle
    QVector<WorkerResult> m_pendingResults;
    DyManagerResults      m_pendingMgrResults{};

    // Stopping flag
    bool m_stopRequested = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// DynamoEngine — IometerEngine implementation backed by real Dynamo processes
// ─────────────────────────────────────────────────────────────────────────────
class DynamoEngine : public IometerEngine
{
    Q_OBJECT

public:
    explicit DynamoEngine(QObject *parent = nullptr);
    ~DynamoEngine() override;

    // ── IometerEngine interface ───────────────────────────────────────────────
    void startTest() override;
    void stopTest()  override;
    void stopAll()   override;
    bool isRunning() const override { return m_running; }

    bool loadConfig(const QString &filepath) override;
    bool saveConfig(const QString &filepath) override;
    void newConfig()  override;

    QList<ManagerInfo>   managers()     const override { return m_managers; }
    void connectManager(const QString &address, const QString &name = {}) override;
    void disconnectManager(const QString &mgrName)                        override;
    void addWorker(const QString &mgrName, const WorkerInfo &w)           override;
    void removeWorker(const QString &mgrName, const QString &workerId)    override;
    void updateWorker(const WorkerInfo &w)                                override;

    QList<AccessSpec>   accessSpecs()  const override { return m_specs; }
    void setAccessSpecs(const QList<AccessSpec> &specs) override;
    void setCurrentTestSpec(const AccessSpec &spec) override {
        m_currentTestSpec = spec; m_hasCurrentTestSpec = true;
    }

    TestConfig testConfig()                     const override { return m_testConfig; }
    void setTestConfig(const TestConfig &cfg)         override { m_testConfig = cfg; }

    QVector<WorkerResult> currentResults() const override { return m_currentResults; }
    QVector<WorkerResult> savedResults()   const override { return m_savedResults;   }
    void clearSavedResults() override { m_savedResults.clear(); }

    int listenPort() const { return DY_PORT; }

private slots:
    void onNewConnection();
    void onSessionConnected(DySession *s);
    void onSessionDisconnected(DySession *s);
    void onSessionResults(DySession *s);
    void onSessionError(DySession *s, const QString &msg);

private:
    void rebuildManagers();
    void computeAggregate();

    QTcpServer           *m_server   = nullptr;
    QList<DySession*>     m_sessions;
    QList<ManagerInfo>    m_managers;
    QList<AccessSpec>     m_specs;
    AccessSpec            m_currentTestSpec;
    bool                  m_hasCurrentTestSpec = false;
    TestConfig            m_testConfig;
    QVector<WorkerResult> m_currentResults;
    QVector<WorkerResult> m_savedResults;
    bool                  m_running  = false;
};
