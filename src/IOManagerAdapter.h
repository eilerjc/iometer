// IOManagerAdapter.h - Bridge between MFC IOManager and pure C++ core libraries
#pragma once

#include "core/WorkerPool.h"
#include "core/TestRunner.h"
#include "core/IcfFile.h"
#include "core/ResultsWriter.h"
#include <string>
#include <vector>
#include <memory>

// Forward declarations
struct Target_Spec;
struct Test_Spec;

// Adapter class: converts between MFC types and std:: types for core libraries
class IOManagerAdapter {
public:
    IOManagerAdapter();
    ~IOManagerAdapter();

    // Configuration: MFC char[] → std::string
    bool loadConfig(const char *filepath);
    bool saveConfig(const char *filepath);

    // Workers: MFC arrays → std::vector
    bool addWorkerFromSpec(const char *managerName, const Target_Spec *spec);
    bool startTestFromSpec(const Test_Spec *spec);
    bool stopTest();

    // Results: std::vector → MFC output
    bool writeResults(const char *filepath);

    // Access to underlying libraries
    WorkerPool *getWorkerPool() { return m_workerPool.get(); }
    TestRunner *getTestRunner() { return m_testRunner.get(); }

private:
    std::unique_ptr<WorkerPool> m_workerPool;
    std::unique_ptr<TestRunner> m_testRunner;

    TestConfig m_config;
    std::vector<WorkerResult> m_results;

    // Conversion helpers
    static std::string mfcStringToStd(const char *mfcStr);
    static WorkerInfo specToWorkerInfo(const Target_Spec *spec, const char *managerName);
    static AccessSpec testSpecToAccessSpec(const Test_Spec *spec);
};
