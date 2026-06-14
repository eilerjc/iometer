// QtIometerEngine.h -- Abstract interface between the Qt GUI and the I/O engine.
//
// The GUI binds to this interface exclusively; concrete implementations are
// QtDemoEngine (simulated data) and QtDynamoEngine (real Dynamo.exe).
#pragma once

#include "QtIometerTypes.h"
#include <QObject>
#include <QVector>
#include <QList>

class QtIometerEngine : public QObject
{
    Q_OBJECT

public:
    explicit QtIometerEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~QtIometerEngine() = default;

    // ---- Test control -------------------------------------------------------
    virtual void startTest()          = 0;
    virtual void stopTest()           = 0;
    virtual void stopAll()            = 0;
    virtual bool isRunning()    const = 0;

    // ---- Configuration ------------------------------------------------------
    virtual bool loadConfig(const QString &filepath) = 0;
    virtual bool saveConfig(const QString &filepath) = 0;
    virtual void newConfig()          = 0;

    // ---- Managers & workers -------------------------------------------------
    virtual QList<ManagerInfo> managers()                                        const = 0;
    virtual void connectManager(const QString &address, const QString &name = {})     = 0;
    virtual void disconnectManager(const QString &mgrName)                            = 0;
    virtual void addWorker(const QString &mgrName, const WorkerInfo &w)               = 0;
    virtual void removeWorker(const QString &mgrName, const QString &workerId)        = 0;
    virtual void updateWorker(const WorkerInfo &w)                                    = 0;

    // -- Manager restore (from a loaded ICF) -------------------------------------
    // The managers a loaded config expects, and how many have connected. Engines
    // that don't do ICF-based restore (e.g. the demo engine) use the defaults.
    virtual QStringList expectedManagers()      const { return {}; }
    virtual bool        isWaitingForManagers()  const { return false; }
    virtual int         connectedManagerCount() const { return 0; }

    // ---- Access specifications ----------------------------------------------
    virtual QList<AccessSpec> accessSpecs()                          const = 0;
    virtual void setAccessSpecs(const QList<AccessSpec> &specs)           = 0;

    // Override which spec is used for the next startTest() without modifying
    // the global library.  Pass a null/default spec to clear the override.
    virtual void setCurrentTestSpec(const AccessSpec &spec)               = 0;

    // ---- Test Setup configuration -------------------------------------------
    virtual TestConfig testConfig()                     const = 0;
    virtual void setTestConfig(const TestConfig &cfg)         = 0;

    // ---- Results ------------------------------------------------------------
    virtual QVector<WorkerResult> currentResults()      const = 0;
    virtual QVector<WorkerResult> savedResults()        const = 0;
    virtual void clearSavedResults()                          = 0;

    // Write results to a CSV in the original Iometer format (batch mode output).
    virtual bool saveBatchResults(const QString &filepath)    = 0;

    // ---- Built-in spec library (shared by all engines) ----------------------
    // Mirrors the original AccessSpecList::InsertIdleSpec() + InsertDefaultSpecs().
    // Mirrors InsertIdleSpec() + InsertDefaultSpecs() exactly, including the
    // IOMTR_SETTING_USE_NEW_ACCESS_SPEC and NO_OLD_ACCESS_SPEC groups plus
    // the multi-line "All in one" combined spec.
    static QList<AccessSpec> builtinAccessSpecs()
    {
        // Delegates to the shared core catalog (iocore::defaultAccessSpecs) so
        // both GUIs use one definition of the built-in specs.
        QList<AccessSpec> out;
        for (const auto &s : iocore::defaultAccessSpecs())
            out.append(s);
        return out;
    }

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
