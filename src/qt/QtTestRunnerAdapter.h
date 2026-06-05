// QtTestRunnerAdapter.h - Qt signal wrapper around pure C++ TestRunner
#pragma once

#include <QObject>
#include "../core/TestRunner.h"

// Qt adapter: converts pure C++ TestRunner callbacks to Qt signals
class QtTestRunnerAdapter : public QObject {
    Q_OBJECT

public:
    explicit QtTestRunnerAdapter(QObject *parent = nullptr);
    ~QtTestRunnerAdapter();

    // Delegate to underlying TestRunner
    bool startTest(const TestConfig &cfg,
                   const std::vector<WorkerInfo> &workers,
                   const std::vector<AccessSpec> &specs);
    bool stopTest();
    bool stopAll();

    bool isRunning() const;
    bool isIdle() const;
    int elapsedSeconds() const;
    std::vector<std::string> activeWorkers() const;
    TestRunner::State state() const;

    // Access underlying TestRunner
    TestRunner *getTestRunner() { return m_testRunner.get(); }

signals:
    // Emitted when test state changes (Idle→Starting→Running→etc)
    void stateChanged(int newState);

    // Emitted for status messages during test
    void statusMessage(const QString &message);

    // Emitted on errors
    void errorOccurred(const QString &errorMsg);

private:
    std::unique_ptr<TestRunner> m_testRunner;

    // Callback handlers (called by TestRunner, emit signals)
    void onStateChangedCallback(TestRunner::State newState);
    void onStatusMessageCallback(const std::string &msg);
    void onErrorOccurredCallback(const std::string &errMsg);
};
