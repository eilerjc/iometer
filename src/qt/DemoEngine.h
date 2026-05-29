// DemoEngine.h -- Simulated Iometer engine for UI testing.
//
// Generates realistic-looking I/O statistics for multiple fake workers
// without requiring a real Dynamo instance.
#pragma once

#include "IometerEngine.h"
#include <QTimer>

class DemoEngine : public IometerEngine
{
    Q_OBJECT

public:
    explicit DemoEngine(QObject *parent = nullptr);

    void startTest()            override;
    void stopTest()             override;
    void stopAll()              override;
    bool isRunning()    const   override { return m_running; }

    bool loadConfig(const QString &filepath) override;
    bool saveConfig(const QString &filepath) override;
    void newConfig()            override;

    QList<ManagerInfo>  managers()      const override { return m_managers; }
    void connectManager(const QString &address, const QString &name) override;
    void disconnectManager(const QString &mgrName) override;
    void addWorker(const QString &mgrName, const WorkerInfo &w) override;
    void removeWorker(const QString &mgrName, const QString &workerId) override;
    void updateWorker(const WorkerInfo &w) override;

    QList<AccessSpec>   accessSpecs()   const override { return m_specs; }
    void setAccessSpecs(const QList<AccessSpec> &specs) override { m_specs = specs; }
    void setCurrentTestSpec(const AccessSpec &spec) override { m_currentTestSpec = spec; }

    TestConfig testConfig()             const override { return m_testConfig; }
    void setTestConfig(const TestConfig &cfg) override { m_testConfig = cfg; }

    QVector<WorkerResult> currentResults() const override { return m_current; }
    QVector<WorkerResult> savedResults()   const override { return m_saved;   }
    void clearSavedResults() override { m_saved.clear(); }

private slots:
    void tick();

private:
    void buildDefaultConfig();
    WorkerResult makeResult(const WorkerInfo &w, const QString &mgrName, double t) const;

    QList<ManagerInfo>    m_managers;
    QList<AccessSpec>     m_specs;
    AccessSpec            m_currentTestSpec;
    QVector<WorkerResult> m_current;
    QVector<WorkerResult> m_saved;
    TestConfig            m_testConfig;

    QTimer  m_timer;
    bool    m_running    = false;
    double  m_t          = 0.0;
};
