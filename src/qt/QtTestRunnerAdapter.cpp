// QtTestRunnerAdapter.cpp
#include "QtTestRunnerAdapter.h"

QtTestRunnerAdapter::QtTestRunnerAdapter(QObject *parent)
    : QObject(parent), m_testRunner(std::make_unique<TestRunner>())
{
    // Wire up callbacks to signal emission
    m_testRunner->onStateChanged = [this](TestRunner::State newState) {
        onStateChangedCallback(newState);
    };

    m_testRunner->onStatusMessage = [this](const std::string &msg) {
        onStatusMessageCallback(msg);
    };

    m_testRunner->onErrorOccurred = [this](const std::string &errMsg) {
        onErrorOccurredCallback(errMsg);
    };
}

QtTestRunnerAdapter::~QtTestRunnerAdapter()
{
}

bool QtTestRunnerAdapter::startTest(const TestConfig &cfg,
                                     const std::vector<WorkerInfo> &workers,
                                     const std::vector<AccessSpec> &specs)
{
    return m_testRunner->startTest(cfg, workers, specs);
}

bool QtTestRunnerAdapter::stopTest()
{
    return m_testRunner->stopTest();
}

bool QtTestRunnerAdapter::stopAll()
{
    return m_testRunner->stopAll();
}

bool QtTestRunnerAdapter::isRunning() const
{
    return m_testRunner->isRunning();
}

bool QtTestRunnerAdapter::isIdle() const
{
    return m_testRunner->isIdle();
}

int QtTestRunnerAdapter::elapsedSeconds() const
{
    return m_testRunner->elapsedSeconds();
}

std::vector<std::string> QtTestRunnerAdapter::activeWorkers() const
{
    return m_testRunner->activeWorkers();
}

TestRunner::State QtTestRunnerAdapter::state() const
{
    return m_testRunner->state();
}

void QtTestRunnerAdapter::onStateChangedCallback(TestRunner::State newState)
{
    emit stateChanged(static_cast<int>(newState));
}

void QtTestRunnerAdapter::onStatusMessageCallback(const std::string &msg)
{
    emit statusMessage(QString::fromStdString(msg));
}

void QtTestRunnerAdapter::onErrorOccurredCallback(const std::string &errMsg)
{
    emit errorOccurred(QString::fromStdString(errMsg));
}
