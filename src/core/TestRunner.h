#pragma once

#include <QString>
#include <QObject>
#include <functional>
#include "../qt/IometerTypes.h"

// Forward declarations (included via IometerTypes.h above, repeated here for clarity)
// struct TestConfig;
// struct AccessSpec;
// struct WorkerInfo;
// struct WorkerResult;

// Platform-agnostic test runner managing test lifecycle
// Handles:
//   - Test start/stop/stopAll state machine
//   - Worker setup and configuration
//   - Result collection
//   - Progress tracking
// Used by both MFC and Qt implementations

class TestRunner : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,           // Ready to start test
        Starting,       // Setting up workers
        Running,        // Test executing
        Stopping,       // Shutting down gracefully
        Stopped,        // Test complete, results ready
        Error           // Error occurred
    };

    explicit TestRunner(QObject *parent = nullptr);
    ~TestRunner() override;

    // Test control
    bool startTest(const TestConfig &cfg,
                   const QList<WorkerInfo> &workers,
                   const QList<AccessSpec> &specs);
    bool stopTest();
    bool stopAll();

    // State queries
    State state() const { return m_state; }
    bool isRunning() const;
    bool isIdle() const;

    // Progress
    double elapsedSeconds() const;
    int activeWorkers() const;

    // Results (only valid when stopped or error)
    const QList<WorkerResult>& results() const;
    QString lastError() const;

    // Configuration
    const TestConfig& config() const { return m_config; }
    const QList<WorkerInfo>& workers() const { return m_workers; }
    const QList<AccessSpec>& specs() const { return m_specs; }

signals:
    void stateChanged(State newState);
    void statusMessage(const QString &message);
    void resultsUpdated(const QList<WorkerResult> &results);
    void errorOccurred(const QString &message);
    void testStarted();
    void testStopped();
    void progressUpdated(double elapsedSeconds, int activeWorkers);

private slots:
    void onStartupComplete();
    void onShutdownComplete();

private:
    void setState(State newState);
    void recordError(const QString &message);

    State m_state;
    TestConfig m_config;
    QList<WorkerInfo> m_workers;
    QList<AccessSpec> m_specs;
    QList<WorkerResult> m_results;
    QString m_lastError;

    // Timing
    qint64 m_startTime;
    qint64 m_elapsed;
};
