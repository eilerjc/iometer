// IometerEngine.h — Abstract interface between the Qt GUI and the I/O engine.
//
// The GUI binds to this interface exclusively; the concrete implementation
// can be DemoEngine (simulated data) or a future DynamoEngine (real Dynamo).
#pragma once

#include "IometerTypes.h"
#include <QObject>
#include <QVector>

class IometerEngine : public QObject
{
    Q_OBJECT

public:
    explicit IometerEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IometerEngine() = default;

    // ── Test control ─────────────────────────────────────────────────────────
    virtual void startTest()   = 0;
    virtual void stopTest()    = 0;
    virtual void stopAll()     = 0;
    virtual bool isRunning() const = 0;

    // ── Configuration ─────────────────────────────────────────────────────────
    virtual bool loadConfig(const QString &filepath) = 0;
    virtual bool saveConfig(const QString &filepath) = 0;
    virtual void newConfig() = 0;

    // ── Managers & workers ────────────────────────────────────────────────────
    virtual QList<ManagerInfo> managers() const = 0;
    virtual void connectManager(const QString &address, const QString &name = {}) = 0;
    virtual void disconnectManager(const QString &mgrName) = 0;
    virtual void addWorker(const QString &mgrName, const WorkerInfo &w) = 0;
    virtual void removeWorker(const QString &mgrName, const QString &workerId) = 0;
    virtual void updateWorker(const WorkerInfo &w) = 0;

    // ── Access specifications ─────────────────────────────────────────────────
    virtual QList<AccessSpec> accessSpecs() const = 0;
    virtual void setAccessSpecs(const QList<AccessSpec> &specs) = 0;

    // ── Results ───────────────────────────────────────────────────────────────
    // Most recent per-worker results (empty if not running)
    virtual QVector<WorkerResult> currentResults() const = 0;
    // Accumulated results list (one entry per completed test)
    virtual QVector<WorkerResult> savedResults() const = 0;
    virtual void clearSavedResults() = 0;

signals:
    void resultsUpdated(QVector<WorkerResult> results);
    void testStarted();
    void testStopped();
    void managerConnected(ManagerInfo mgr);
    void managerDisconnected(QString mgrName);
    void configChanged();
    void statusMessage(QString msg);
    void errorOccurred(QString msg);
};
