// IometerEngine.h -- Abstract interface between the Qt GUI and the I/O engine.
//
// The GUI binds to this interface exclusively; concrete implementations are
// DemoEngine (simulated data) and DynamoEngine (real Dynamo.exe).
#pragma once

#include "IometerTypes.h"
#include <QObject>
#include <QVector>
#include <QList>

class IometerEngine : public QObject
{
    Q_OBJECT

public:
    explicit IometerEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IometerEngine() = default;

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
        QList<AccessSpec> specs;

        // Helper: build an AccessSpecLine
        auto line = [](int sz, int readPct, int seqPct, int align=0,
                       int ofSz=100) -> AccessSpecLine {
            AccessSpecLine l;
            l.sizeBytes   = sz;
            l.readPercent = readPct;
            l.seqPercent  = seqPct;
            l.alignBytes  = align;
            l.ofSize      = ofSz;
            return l;
        };

        // Helper: make a single-line spec
        auto addOne = [&](const QString &name, AccessSpecLine l) {
            AccessSpec s;
            s.name = name;
            s.lines.append(l);
            specs.append(s);
        };

        // ── Idle (size = 0) ───────────────────────────────────────────────
        {
            AccessSpec s;
            s.name = "Idle";
            AccessSpecLine l;
            l.sizeBytes = 0; l.ofSize = 100;
            s.lines.append(l);
            specs.append(s);
        }

        // ── Default (64 KiB, 100% read, sequential) ───────────────────────
        {
            AccessSpec s;
            s.name = "Default"; s.defaultSpec = true;
            s.lines.append(line(65536, 100, 100));
            specs.append(s);
        }

        // ── Collect all patterns in order — used for both individual specs
        //    and for building "All in one" ──────────────────────────────────
        struct Pat { const char *name; AccessSpecLine l; };
        QList<Pat> patterns = {
            // 512 B sequential
            { "512 B; 100% Read; 0% random",  line(512, 100,100) },
            { "512 B; 75% Read; 0% random",   line(512,  75,100) },
            { "512 B; 50% Read; 0% random",   line(512,  50,100) },
            { "512 B; 25% Read; 0% random",   line(512,  25,100) },
            { "512 B; 0% Read; 0% random",    line(512,   0,100) },
            // 4 KiB sequential
            { "4 KiB; 100% Read; 0% random",  line(4096,100,100) },
            { "4 KiB; 75% Read; 0% random",   line(4096, 75,100) },
            { "4 KiB; 50% Read; 0% random",   line(4096, 50,100) },
            { "4 KiB; 25% Read; 0% random",   line(4096, 25,100) },
            { "4 KiB; 0% Read; 0% random",    line(4096,  0,100) },
            // 4 KiB aligned random (IOMTR_SETTING_USE_NEW_ACCESS_SPEC)
            { "4 KiB aligned; 100% Read; 100% random", line(4096,100,0,4096) },
            { "4 KiB aligned; 50% Read; 100% random",  line(4096, 50,0,4096) },
            { "4 KiB aligned; 0% Read; 100% random",   line(4096,  0,0,4096) },
            // 16 KiB sequential
            { "16 KiB; 100% Read; 0% random", line(16384,100,100) },
            { "16 KiB; 75% Read; 0% random",  line(16384, 75,100) },
            { "16 KiB; 50% Read; 0% random",  line(16384, 50,100) },
            { "16 KiB; 25% Read; 0% random",  line(16384, 25,100) },
            { "16 KiB; 0% Read; 0% random",   line(16384,  0,100) },
            // 32 KiB sequential
            { "32 KiB; 100% Read; 0% random", line(32768,100,100) },
            { "32 KiB; 75% Read; 0% random",  line(32768, 75,100) },
            { "32 KiB; 50% Read; 0% random",  line(32768, 50,100) },
            { "32 KiB; 25% Read; 0% random",  line(32768, 25,100) },
            { "32 KiB; 0% Read; 0% random",   line(32768,  0,100) },
            // 64 KiB sequential (IOMTR_SETTING_USE_NEW_ACCESS_SPEC)
            { "64 KiB; 100% Read; 0% random", line(65536,100,100) },
            { "64 KiB; 50% Read; 0% random",  line(65536, 50,100) },
            { "64 KiB; 0% Read; 0% random",   line(65536,  0,100) },
            // 256 KiB sequential (IOMTR_SETTING_USE_NEW_ACCESS_SPEC)
            { "256 KiB; 100% Read; 0% random", line(262144,100,100) },
            { "256 KiB; 50% Read; 0% random",  line(262144, 50,100) },
            { "256 KiB; 0% Read; 0% random",   line(262144,  0,100) },
        };

        // Add individual single-line specs
        for (const auto &p : patterns)
            addOne(QString::fromLatin1(p.name), p.l);

        // ── "All in one" — all patterns with equal % access distribution ──
        {
            AccessSpec s;
            s.name = "All in one";
            const int n = patterns.size();
            for (int i = 0; i < n; ++i) {
                AccessSpecLine l = patterns[i].l;
                // Distribute 100% as evenly as possible (matches original rounding)
                l.ofSize = (i < 100 % n) ? 100 / n + 1 : 100 / n;
                s.lines.append(l);
            }
            specs.append(s);
        }

        return specs;
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
