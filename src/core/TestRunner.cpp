#include "TestRunner.h"
#include "../IometerTypes.h"
#include <QDateTime>

TestRunner::TestRunner(QObject *parent)
    : QObject(parent),
      m_state(State::Idle),
      m_startTime(0),
      m_elapsed(0)
{
}

TestRunner::~TestRunner()
{
    if (isRunning()) {
        stopAll();
    }
}

bool TestRunner::startTest(const TestConfig &cfg,
                           const QList<WorkerInfo> &workers,
                           const QList<AccessSpec> &specs)
{
    if (!isIdle()) {
        recordError("Cannot start test - runner is not idle");
        return false;
    }

    if (workers.isEmpty()) {
        recordError("Cannot start test - no workers configured");
        return false;
    }

    if (specs.isEmpty()) {
        recordError("Cannot start test - no access specs configured");
        return false;
    }

    // Store configuration
    m_config = cfg;
    m_workers = workers;
    m_specs = specs;
    m_results.clear();

    // Start test
    setState(State::Starting);
    emit statusMessage(QString("Starting test with %1 workers").arg(workers.size()));

    // In real implementation, this would send setup messages to Dynamo
    // For now, just transition to running
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    setState(State::Running);
    emit testStarted();

    return true;
}

bool TestRunner::stopTest()
{
    if (!isRunning()) {
        recordError("Cannot stop test - test is not running");
        return false;
    }

    setState(State::Stopping);
    emit statusMessage("Stopping test gracefully");

    // In real implementation, this would send STOP to Dynamo
    // For now, just transition to stopped
    setState(State::Stopped);
    emit testStopped();

    return true;
}

bool TestRunner::stopAll()
{
    if (m_state == State::Idle || m_state == State::Error) {
        return true;  // Already stopped
    }

    setState(State::Stopping);
    emit statusMessage("Emergency shutdown");

    // In real implementation, this would send STOP_ALL to Dynamo
    setState(State::Stopped);

    return true;
}

bool TestRunner::isRunning() const
{
    return m_state == State::Starting || m_state == State::Running;
}

bool TestRunner::isIdle() const
{
    return m_state == State::Idle;
}

double TestRunner::elapsedSeconds() const
{
    if (!isRunning()) {
        return m_elapsed / 1000.0;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return (now - m_startTime) / 1000.0;
}

int TestRunner::activeWorkers() const
{
    return isRunning() ? m_workers.size() : 0;
}

const QList<WorkerResult>& TestRunner::results() const
{
    return m_results;
}

QString TestRunner::lastError() const
{
    return m_lastError;
}

const TestConfig& TestRunner::config() const
{
    return m_config;
}

const QList<WorkerInfo>& TestRunner::workers() const
{
    return m_workers;
}

const QList<AccessSpec>& TestRunner::specs() const
{
    return m_specs;
}

void TestRunner::onStartupComplete()
{
    if (m_state == State::Starting) {
        setState(State::Running);
        emit testStarted();
    }
}

void TestRunner::onShutdownComplete()
{
    if (m_state == State::Stopping) {
        m_elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        setState(State::Stopped);
        emit testStopped();
    }
}

void TestRunner::setState(State newState)
{
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    emit stateChanged(m_state);
}

void TestRunner::recordError(const QString &message)
{
    m_lastError = message;
    setState(State::Error);
    emit errorOccurred(message);
}
