#include "TestRunner.h"
#include <chrono>

TestRunner::TestRunner()
    : m_state(State::Idle), m_startTime(0), m_elapsed(0)
{
}

TestRunner::~TestRunner()
{
    if (isRunning()) {
        stopAll();
    }
}

bool TestRunner::startTest(const TestConfig &cfg,
                           const std::vector<WorkerInfo> &workers,
                           const std::vector<AccessSpec> &specs)
{
    if (!isIdle()) {
        recordError("Cannot start test - runner is not idle");
        return false;
    }

    if (workers.empty()) {
        recordError("Cannot start test - no workers configured");
        return false;
    }

    if (specs.empty()) {
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
    if (onStatusMessage) onStatusMessage("Starting test with " + std::to_string(workers.size()) + " workers");

    // Transition to running
    auto now = std::chrono::high_resolution_clock::now();
    m_startTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    setState(State::Running);

    return true;
}

bool TestRunner::stopTest()
{
    if (!isRunning()) {
        recordError("Cannot stop test - test is not running");
        return false;
    }

    setState(State::Stopping);
    if (onStatusMessage) onStatusMessage("Stopping test gracefully");

    // Transition to stopped
    setState(State::Stopped);

    return true;
}

bool TestRunner::stopAll()
{
    if (m_state == State::Idle || m_state == State::Error) {
        return true;  // Already stopped
    }

    setState(State::Stopping);
    if (onStatusMessage) onStatusMessage("Emergency shutdown");

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

    auto now = std::chrono::high_resolution_clock::now();
    auto currentMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return (currentMs - m_startTime) / 1000.0;
}

int TestRunner::activeWorkers() const
{
    return isRunning() ? m_workers.size() : 0;
}

void TestRunner::setState(State newState)
{
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    if (onStateChanged) onStateChanged(m_state);
}

void TestRunner::recordError(const std::string &message)
{
    m_lastError = message;
    setState(State::Error);
    if (onErrorOccurred) onErrorOccurred(message);
}
