#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "IometerTypes.h"

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

class TestRunner {

public:
    enum class State {
        Idle,           // Ready to start test
        Starting,       // Setting up workers
        Running,        // Test executing
        Stopping,       // Shutting down gracefully
        Stopped,        // Test complete, results ready
        Error           // Error occurred
    };

    TestRunner();
    ~TestRunner();

    // Test control
    bool startTest(const TestConfig &cfg,
                   const std::vector<WorkerInfo> &workers,
                   const std::vector<AccessSpec> &specs);
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
    const std::vector<WorkerResult>& results() const { return m_results; }
    std::string lastError() const { return m_lastError; }

    // Configuration
    const TestConfig& config() const { return m_config; }
    const std::vector<WorkerInfo>& workers() const { return m_workers; }
    const std::vector<AccessSpec>& specs() const { return m_specs; }

    // Optional callbacks (for Qt/MFC signal integration)
    std::function<void(State)> onStateChanged;
    std::function<void(const std::string&)> onStatusMessage;
    std::function<void(const std::string&)> onErrorOccurred;

private:
    void setState(State newState);
    void recordError(const std::string &message);

    State m_state;
    TestConfig m_config;
    std::vector<WorkerInfo> m_workers;
    std::vector<AccessSpec> m_specs;
    std::vector<WorkerResult> m_results;
    std::string m_lastError;

    // Timing (milliseconds since epoch)
    int64_t m_startTime;
    int64_t m_elapsed;
};
